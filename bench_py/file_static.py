from pathlib import Path
import shutil

tmp_dir = Path("tmp_merk_file_static_py")
base = tmp_dir / "sample.txt"
copy_path = tmp_dir / "sample_copy.txt"
renamed_path = tmp_dir / "sample_renamed.txt"

tmp_dir.mkdir(parents=True, exist_ok=True)

base.write_text("line1", encoding="utf-8")
with base.open("a", encoding="utf-8") as f:
    f.write("\nline2")

exists_before = base.exists()
size_before = base.stat().st_size if base.exists() else 0
content_before = base.read_text(encoding="utf-8")

shutil.copyfile(base, copy_path)
copy_path.rename(renamed_path)

copied_exists = renamed_path.exists()
copied_size = renamed_path.stat().st_size if renamed_path.exists() else 0
copied_content = renamed_path.read_text(encoding="utf-8")

if base.exists():
    base.unlink()
if renamed_path.exists():
    renamed_path.unlink()

exists_after_base = base.exists()
exists_after_renamed = renamed_path.exists()

print(exists_before)
print(size_before)
print(content_before)
print(copied_exists)
print(copied_size)
print(copied_content)
print(exists_after_base)
print(exists_after_renamed)
