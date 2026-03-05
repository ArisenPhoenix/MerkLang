#!/usr/bin/env python3
import argparse
import json
import os
import re
import statistics
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
from pathlib import Path
import time

MACRO_KEYS = ["tokenize", "parse", "eval", "total"]
MICRO_KEYS = ["isNull", "isCallable", "toString", "copyAssign", "arithmetic", "hash", "total"]
SCHEMA_VERSION = 1

ROOT = Path(__file__).parent


def get_executable_path() -> Path:
    return (ROOT / "build" / "merk").resolve()

def get_code_dir() -> Path:
    return (ROOT / "code").resolve()

def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def default_cpu_spec() -> str:
    cpu_count = os.cpu_count() or 1
    return f"0-{cpu_count - 1}"


def parse_cpu_list(cpu_list_raw: str) -> list[str]:
    if not cpu_list_raw:
        return []
    cpus: list[str] = []
    seen: set[int] = set()
    parts = [p.strip() for p in cpu_list_raw.split(",") if p.strip()]
    if not parts:
        raise ValueError("cpu list is empty")
    for part in parts:
        if "-" in part:
            bounds = [b.strip() for b in part.split("-", maxsplit=1)]
            if len(bounds) != 2 or not bounds[0] or not bounds[1]:
                raise ValueError(f"invalid cpu range '{part}'")
            start = int(bounds[0])
            end = int(bounds[1])
            if start < 0 or end < 0 or end < start:
                raise ValueError(f"invalid cpu range '{part}'")
            for cpu in range(start, end + 1):
                if cpu not in seen:
                    cpus.append(str(cpu))
                    seen.add(cpu)
        else:
            cpu = int(part)
            if cpu < 0:
                raise ValueError(f"invalid cpu id '{part}'")
            if cpu not in seen:
                cpus.append(str(cpu))
                seen.add(cpu)
    if not cpus:
        raise ValueError("cpu list is empty")
    return cpus


def run_command_and_parse(cmd: list[str], patterns: dict[str, str], run_cwd: str | None = None) -> dict[str, float]:
    proc = subprocess.run(cmd, capture_output=True, text=True, cwd=run_cwd)
    if proc.returncode != 0:
        raise RuntimeError(
            f"command failed: {' '.join(cmd)}\nstdout:\n{proc.stdout}\nstderr:\n{proc.stderr}"
        )
    out = proc.stdout
    parsed: dict[str, float] = {}
    for key, pat in patterns.items():
        m = re.search(pat, out)
        if not m:
            raise RuntimeError(f"missing metric '{key}' in output\n{out}")
        parsed[key] = float(m.group(1))
    return parsed


def run_many(
    base_cmd: list[str],
    patterns: dict[str, str],
    runs: int,
    workers: int,
    cpus: list[str],
    run_cwd: str | None = None,
) -> dict[str, list[float]]:
    vals = {k: [] for k in patterns.keys()}

    def one_run(i: int) -> dict[str, float]:
        cmd = list(base_cmd)
        if cpus:
            cpu = cpus[i % len(cpus)]
            cmd = ["taskset", "-c", cpu] + cmd
        return run_command_and_parse(cmd, patterns, run_cwd=run_cwd)

    if workers <= 1:
        for i in range(runs):
            out = one_run(i)
            for k, v in out.items():
                vals[k].append(v)
        return vals

    with ThreadPoolExecutor(max_workers=workers) as ex:
        futures = [ex.submit(one_run, i) for i in range(runs)]
        for fut in as_completed(futures):
            out = fut.result()
            for k, v in out.items():
                vals[k].append(v)

    return vals


def compute_summary(vals: dict[str, list[float]]) -> dict[str, dict[str, float]]:
    summary: dict[str, dict[str, float]] = {}
    for key, arr in vals.items():
        summary[key] = {
            "median": statistics.median(arr),
            "mean": statistics.mean(arr),
            "std": statistics.pstdev(arr),
            "min": min(arr),
            "max": max(arr),
        }
    return summary


def print_summary(title: str, vals: dict[str, list[float]]) -> None:
    print(title)
    for k, arr in vals.items():
        print(
            f"{k:10s} median={statistics.median(arr):.3f} "
            f"mean={statistics.mean(arr):.3f} std={statistics.pstdev(arr):.3f} "
            f"min={min(arr):.3f} max={max(arr):.3f}"
        )


def run_macro(args: argparse.Namespace) -> dict[str, list[float]]:
    patterns = {
        "tokenize": r"Avg tokenize:\s+([0-9.]+) ms",
        "parse": r"Avg parse:\s+([0-9.]+) ms",
        "eval": r"Avg eval:\s+([0-9.]+) ms",
        "total": r"Avg total:\s+([0-9.]+) ms",
    }
    base_cmd = [
        args.bin,
        "--bench",
        str(args.file),
        "--bench-iters",
        str(args.iters),
        "--bench-warmup",
        str(args.warmup),
    ]
    vals = run_many(base_cmd, patterns, args.runs, args.workers, args.cpus, run_cwd=args.run_cwd)
    print_summary(
        f"MACRO runs={args.runs} workers={args.workers} iters={args.iters} warmup={args.warmup} cpus={','.join(args.cpus) if args.cpus else 'none'}",
        vals,
    )
    return vals


def run_micro(args: argparse.Namespace) -> dict[str, list[float]]:
    patterns = {
        "isNull": r"Avg isNull:\s+([0-9.]+) ms",
        "isCallable": r"Avg isCallable:\s+([0-9.]+) ms",
        "toString": r"Avg toString:\s+([0-9.]+) ms",
        "copyAssign": r"Avg copyAssign:\s+([0-9.]+) ms",
        "arithmetic": r"Avg arithmetic:\s+([0-9.]+) ms",
        "hash": r"Avg hash:\s+([0-9.]+) ms",
        "total": r"Avg total:\s+([0-9.]+) ms",
    }
    base_cmd = [
        args.bin,
        "--bench-node",
        "--bench-iters",
        str(args.iters),
        "--bench-warmup",
        str(args.warmup),
    ]
    vals = run_many(base_cmd, patterns, args.runs, args.workers, args.cpus, run_cwd=args.run_cwd)
    print_summary(
        f"MICRO runs={args.runs} workers={args.workers} iters={args.iters} warmup={args.warmup} cpus={','.join(args.cpus) if args.cpus else 'none'}",
        vals,
    )
    return vals


def load_results_store(results_json: Path) -> dict:
    if not results_json.exists():
        return {"schema_version": SCHEMA_VERSION, "runs": []}
    with results_json.open("r", encoding="utf-8") as f:
        data = json.load(f)
    if not isinstance(data, dict):
        raise RuntimeError(f"invalid JSON root in {results_json}")
    if "runs" not in data or not isinstance(data["runs"], list):
        raise RuntimeError(f"missing/invalid 'runs' list in {results_json}")
    data.setdefault("schema_version", SCHEMA_VERSION)
    return data


def save_results_store(results_json: Path, data: dict) -> None:
    tmp = results_json.with_suffix(results_json.suffix + ".tmp")
    with tmp.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        f.write("\n")
    tmp.replace(results_json)



def append_run_result(store: dict, args: argparse.Namespace, mode: str, vals: dict[str, list[float]]) -> None:
    entry = {
        "timestamp": utc_now_iso(),
        "source": "runs.py",
        "mode": mode,
        "args": {
            "bin": args.bin,
            "file": str(args.file) if mode == "macro" else None,
            "runs": args.runs,
            "iters": args.iters,
            "warmup": args.warmup,
            "workers": args.workers,
            "cpus": args.cpus,
            "cpus_spec": args.cpus_spec,
        },
        "samples_ms": vals,
        "summary_ms": compute_summary(vals),
    }
    store["runs"].append(entry)


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Merk benchmark runner")
    p.add_argument("mode", nargs="?", default="macro", choices=["macro", "micro"])
    p.add_argument("--runs", type=int, default=12, help="number of repeated outer runs")
    p.add_argument("--iters", type=int, default=200, help="inner benchmark iterations passed to merk")
    p.add_argument("--warmup", type=int, default=20, help="inner warmup iterations passed to merk")
    p.add_argument("--workers", type=int, default=1, help="parallel outer runs")
    p.add_argument("--file", default="test1.merk", help="bench file for macro mode")
    p.add_argument("--bin", default=str(get_executable_path()), help="path to merk executable")
    p.add_argument("--cpus", default=default_cpu_spec(), help="cpu ids/ranges for taskset, e.g. '0-11' or '0,2,4', empty to disable")
    p.add_argument("--results-json", default="results.json", help="path to JSON results file")
    return p


def main() -> int:
    curr = time.time()
    parser = build_parser()
    args = parser.parse_args()

    if args.runs < 1 or args.iters < 1 or args.warmup < 1 or args.workers < 1:
        print("runs/iters/warmup/workers must be >= 1")
        return 1

    args.cpus_spec = args.cpus
    try:
        args.cpus = parse_cpu_list(args.cpus)
    except ValueError as e:
        print(f"invalid --cpus: {e}")
        return 1

    args.run_cwd = str(Path(args.bin).resolve().parent)

    results_json = Path(args.results_json)
    if args.workers > 1:
        print("warning: workers>1 speeds up wall clock but increases run interference/noise")

    try:
        store = load_results_store(results_json)
        if args.mode == "macro":
            vals = run_macro(args)
        else:
            vals = run_micro(args)

        append_run_result(store, args, args.mode, vals)
        save_results_store(results_json, store)
        print(f"appended run to {results_json}")
    except RuntimeError as e:
        print(str(e))
        print(f"time taken: {time.time() - curr:.3f}s")
        return 1
    
    print(f"time taken: {time.time() - curr:.3f}s")

    return 0


if __name__ == "__main__":
    sys.exit(main())