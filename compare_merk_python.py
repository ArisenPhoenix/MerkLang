#!/usr/bin/env python3
import argparse
import contextlib
import io
import json
import re
import statistics
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parent
DEFAULT_MERK_BIN = ROOT / "build-release" / "merk"
DEFAULT_OUT_JSON = ROOT / "results_compare.json"


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def summarize(samples_ms: list[float]) -> dict[str, float]:
    return {
        "median": statistics.median(samples_ms),
        "mean": statistics.mean(samples_ms),
        "std": statistics.pstdev(samples_ms) if len(samples_ms) > 1 else 0.0,
        "min": min(samples_ms),
        "max": max(samples_ms),
    }


def run_once(cmd: list[str], cwd: Path) -> float:
    start = time.perf_counter()
    proc = subprocess.run(cmd, cwd=str(cwd), stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    elapsed_ms = (time.perf_counter() - start) * 1000.0
    if proc.returncode != 0:
        raise RuntimeError(f"command failed: {' '.join(cmd)}\nstderr:\n{proc.stderr}")
    return elapsed_ms


def benchmark_case(cmd: list[str], cwd: Path, runs: int, warmup: int) -> list[float]:
    for _ in range(warmup):
        _ = run_once(cmd, cwd)
    out: list[float] = []
    for _ in range(runs):
        out.append(run_once(cmd, cwd))
    return out


def parse_merk_bench_total_ms(stdout: str) -> float:
    m = re.search(r"Avg total:\s+([0-9.]+)\s+ms", stdout)
    if not m:
        raise RuntimeError(f"failed to parse Merk benchmark output:\n{stdout}")
    return float(m.group(1))


def run_merk_throughput_once(
    merk_bin: Path,
    merk_file_name: str,
    bench_iters: int,
    bench_warmup: int,
) -> float:
    cmd = [
        str(merk_bin),
        "--bench",
        merk_file_name,
        "--bench-iters",
        str(bench_iters),
        "--bench-warmup",
        str(bench_warmup),
    ]
    proc = subprocess.run(cmd, cwd=str(merk_bin.parent), capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"command failed: {' '.join(cmd)}\nstdout:\n{proc.stdout}\nstderr:\n{proc.stderr}")
    return parse_merk_bench_total_ms(proc.stdout)


def run_python_throughput_once(
    python_bin: str,
    py_file: Path,
    bench_iters: int,
    bench_warmup: int,
    cwd: Path,
) -> float:
    driver = (
        "import contextlib, io, pathlib, sys, time\n"
        "p = pathlib.Path(sys.argv[1]).resolve()\n"
        "iters = int(sys.argv[2])\n"
        "warmup = int(sys.argv[3])\n"
        "src = p.read_text(encoding='utf-8')\n"
        "code = compile(src, str(p), 'exec')\n"
        "def run_once():\n"
        "    g = {'__name__': '__main__', '__file__': str(p)}\n"
        "    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):\n"
        "        exec(code, g, g)\n"
        "for _ in range(warmup):\n"
        "    run_once()\n"
        "t0 = time.perf_counter()\n"
        "for _ in range(iters):\n"
        "    run_once()\n"
        "t1 = time.perf_counter()\n"
        "print((t1 - t0) * 1000.0 / iters)\n"
    )
    cmd = [python_bin, "-c", driver, str(py_file), str(bench_iters), str(bench_warmup)]
    proc = subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"command failed: {' '.join(cmd)}\nstdout:\n{proc.stdout}\nstderr:\n{proc.stderr}")
    try:
        return float(proc.stdout.strip())
    except ValueError as exc:
        raise RuntimeError(f"failed to parse python throughput output:\n{proc.stdout}") from exc


def benchmark_throughput_case(
    merk_bin: Path,
    merk_file: Path,
    python_bin: str,
    py_file: Path,
    runs: int,
    bench_iters: int,
    bench_warmup: int,
) -> tuple[list[float], list[float]]:
    merk_samples: list[float] = []
    py_samples: list[float] = []
    for _ in range(runs):
        merk_samples.append(
            run_merk_throughput_once(merk_bin, merk_file.name, bench_iters, bench_warmup)
        )
        py_samples.append(
            run_python_throughput_once(python_bin, py_file, bench_iters, bench_warmup, ROOT)
        )
    return merk_samples, py_samples


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare Merk and Python script runtimes.")
    parser.add_argument("--merk-bin", default=str(DEFAULT_MERK_BIN))
    parser.add_argument("--python-bin", default="python3")
    parser.add_argument("--runs", type=int, default=20, help="outer repeated samples")
    parser.add_argument("--warmup", type=int, default=3, help="warmup runs (process mode)")
    parser.add_argument(
        "--mode",
        choices=["process", "throughput", "both"],
        default="both",
        help="process: full process timing; throughput: startup-reduced per-iter timing",
    )
    parser.add_argument("--bench-iters", type=int, default=200, help="inner iters for throughput mode")
    parser.add_argument("--bench-warmup", type=int, default=20, help="inner warmup for throughput mode")
    parser.add_argument("--out", default=str(DEFAULT_OUT_JSON))
    args = parser.parse_args()

    merk_bin = Path(args.merk_bin).resolve()
    if not merk_bin.exists():
        raise RuntimeError(f"merk binary not found: {merk_bin}")

    cases = [
        ("test1", ROOT / "code" / "test1.merk", ROOT / "bench_py" / "test1.py"),
        ("class", ROOT / "code" / "class.merk", ROOT / "bench_py" / "class_case.py"),
        ("fib", ROOT / "code" / "fib.merk", ROOT / "bench_py" / "fib.py"),
        ("file_static", ROOT / "code" / "file_static.merk", ROOT / "bench_py" / "file_static.py"),
        ("compute_loop", ROOT / "code" / "compute_loop.merk", ROOT / "bench_py" / "compute_loop.py"),
    ]

    mode_entries = []
    print(
        f"outer_runs={args.runs} process_warmup={args.warmup} "
        f"bench_iters={args.bench_iters} bench_warmup={args.bench_warmup}"
    )

    def run_mode_process() -> dict:
        case_results = []
        print("\n[mode=process]")
        for case_name, merk_file, py_file in cases:
            if not merk_file.exists():
                raise RuntimeError(f"missing merk file: {merk_file}")
            if not py_file.exists():
                raise RuntimeError(f"missing python file: {py_file}")

            merk_cmd = [str(merk_bin), merk_file.name]
            py_cmd = [args.python_bin, str(py_file)]

            merk_samples = benchmark_case(merk_cmd, merk_bin.parent, args.runs, args.warmup)
            py_samples = benchmark_case(py_cmd, ROOT, args.runs, args.warmup)

            merk_summary = summarize(merk_samples)
            py_summary = summarize(py_samples)
            speed_ratio = py_summary["median"] / merk_summary["median"] if merk_summary["median"] > 0 else 0.0

            print(
                f"{case_name:12s} "
                f"merk_med={merk_summary['median']:.3f}ms "
                f"py_med={py_summary['median']:.3f}ms "
                f"py/merk={speed_ratio:.2f}x"
            )

            case_results.append(
                {
                    "case": case_name,
                    "merk": {"command": merk_cmd, "samples_ms": merk_samples, "summary_ms": merk_summary},
                    "python": {"command": py_cmd, "samples_ms": py_samples, "summary_ms": py_summary},
                    "python_over_merk_median_ratio": speed_ratio,
                }
            )
        return {"mode": "process", "cases": case_results}

    def run_mode_throughput() -> dict:
        case_results = []
        print("\n[mode=throughput]")
        for case_name, merk_file, py_file in cases:
            if not merk_file.exists():
                raise RuntimeError(f"missing merk file: {merk_file}")
            if not py_file.exists():
                raise RuntimeError(f"missing python file: {py_file}")

            merk_samples, py_samples = benchmark_throughput_case(
                merk_bin,
                merk_file,
                args.python_bin,
                py_file,
                args.runs,
                args.bench_iters,
                args.bench_warmup,
            )

            merk_summary = summarize(merk_samples)
            py_summary = summarize(py_samples)
            speed_ratio = py_summary["median"] / merk_summary["median"] if merk_summary["median"] > 0 else 0.0

            print(
                f"{case_name:12s} "
                f"merk_med={merk_summary['median']:.3f}ms "
                f"py_med={py_summary['median']:.3f}ms "
                f"py/merk={speed_ratio:.2f}x"
            )

            case_results.append(
                {
                    "case": case_name,
                    "merk": {
                        "command": [
                            str(merk_bin),
                            "--bench",
                            merk_file.name,
                            "--bench-iters",
                            args.bench_iters,
                            "--bench-warmup",
                            args.bench_warmup,
                        ],
                        "samples_ms": merk_samples,
                        "summary_ms": merk_summary,
                    },
                    "python": {
                        "command": [args.python_bin, "-c", "<in-process loop>", str(py_file)],
                        "samples_ms": py_samples,
                        "summary_ms": py_summary,
                    },
                    "python_over_merk_median_ratio": speed_ratio,
                }
            )
        return {"mode": "throughput", "bench_iters": args.bench_iters, "bench_warmup": args.bench_warmup, "cases": case_results}

    if args.mode in ("process", "both"):
        mode_entries.append(run_mode_process())
    if args.mode in ("throughput", "both"):
        mode_entries.append(run_mode_throughput())

    output = {
        "timestamp": utc_now_iso(),
        "source": "compare_merk_python.py",
        "config": {
            "outer_runs": args.runs,
            "process_warmup": args.warmup,
            "mode": args.mode,
            "bench_iters": args.bench_iters,
            "bench_warmup": args.bench_warmup,
            "merk_bin": str(merk_bin),
            "python_bin": args.python_bin,
        },
        "entries": mode_entries,
    }

    out_path = Path(args.out).resolve()
    with out_path.open("w", encoding="utf-8") as f:
        json.dump(output, f, indent=2)
        f.write("\n")

    print(f"wrote {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
