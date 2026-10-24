"""Archive the million-cell grid/display matrix and its numerical evidence."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import shutil

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("directory", type=Path)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("cut_profile", root / "test/analyze-cut-profile.py")
profile = importlib.util.module_from_spec(spec)
spec.loader.exec_module(profile)
records = json.loads((args.directory / "matrix.json").read_text(encoding="utf-8-sig"))
summary = []
for record in records:
    case = Path(record["directory"])
    manifest = json.loads((case / "manifest.json").read_text(encoding="utf-8-sig"))
    if not manifest["succeeded"] or not manifest["fullValidation"]:
        raise ValueError(f"Incomplete matrix case: {case}")
    target = args.output / "evidence" / record["gpu"] / record["model"] / record["displayMode"]
    target.mkdir(parents=True, exist_ok=True)
    for name in ["manifest.json", "hashes.json"]:
        shutil.copyfile(case / name, target / name)
    for scene in ["crossing", "rotating", "deformed"]:
        row = profile.summarize(case / "performance/round-1" / f"{scene}.csv")
        line = next(line for line in record["results"] if f"scenario {scene} " in line)
        checks = float(re.search(r"full_checks (\S+)", line).group(1))
        if checks != 2:
            raise ValueError(f"Missing full numerical checks: {case}/{scene}")
        row.update(gpu=record["gpu"], model=record["model"], display_mode=record["displayMode"],
                   scene=scene, full_checks=int(checks), maximum_area_error=float(re.search(r"maximum_area_error (\S+)", line).group(1)))
        summary.append(row)
        shutil.copyfile(row["file"], target / f"{scene}.csv")
        shutil.copyfile(case / "performance/round-1" / f"{scene}-renderer.txt", target / f"{scene}-renderer.txt")
args.output.mkdir(parents=True, exist_ok=True)
(args.output / "summary.json").write_text(json.dumps(summary, indent=2)+"\n", encoding="utf-8")
(args.output / "matrix.json").write_text(json.dumps(records, indent=2)+"\n", encoding="utf-8")
print(f"Archived {len(records)} cases / {len(summary)} scenes / {sum(row['frames'] for row in summary)} frames")
