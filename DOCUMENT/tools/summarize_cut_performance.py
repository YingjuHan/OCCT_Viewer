"""Verify and archive T10: two GPUs, 85 regressions, large gates and 1080 frames."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import shutil

from summarize_cut_view_gpu_tests import CARDS, SCENES, functional, performance, read, require


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mx450", type=Path, required=True)
    parser.add_argument("--irisxe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    spec = importlib.util.spec_from_file_location("cut_profile", repo / "test/analyze-cut-profile.py")
    profile = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(profile)
    report = {"schema": 1, "protocol": {"cells": 3375000, "nodes": 3442951,
              "frames_per_scene_per_round": 60, "rounds": 3, "mask": 3,
              "backend": "auto", "display_mode": "cloud", "deformation_scale": .1,
              "p50": "sorted[n//2] (upper median)", "p95": "nearest rank ceil(0.95*n)",
              "fps": "1000*n/sum(total_ms)", "target": "fps>=15 and p95_ms<=67",
              "memory": "Windows process counters sampled after each frame timer; not GPU memory",
              "peak_working_set": "process lifetime high-water mark, including preceding scenes"},
              "gpus": {}}
    archived_frames = []
    for gpu, root in (("MX450", args.mx450), ("IrisXe", args.irisxe)):
        result = performance(gpu, root, 60, 3, archived_frames, "T10")
        manifest = result["manifest"]
        require(manifest["mode"] == "All" and manifest["displayMode"] == "cloud"
                and manifest["parallelPlaneQuery"] and not manifest["cellTopologyReuse"]
                and not manifest["fullValidation"], f"Unexpected default protocol: {root}")
        result["functional"] = functional(root, expected_tests=85)
        for case, evidence in result["functional"]["large"].items():
            states = [state for state in evidence["states"] if "sourceReserved" in state]
            require(bool(states), f"Missing budget evidence: {root}/{case}")
            for state in states:
                values = {key: int(value) for key, value in re.findall(
                    r"(sourceLimit|resultLimit|sourceReserved|resultReserved) (\d+)", state)}
                require(values.get("sourceLimit") == 4 * 2**30 and values.get("resultLimit") == 16 * 2**30
                        and values.get("sourceReserved", 2**64) <= values["sourceLimit"]
                        and values.get("resultReserved", 2**64) <= values["resultLimit"],
                        f"Budget limit/reservation mismatch: {root}/{case}: {state}")
        for scene in SCENES:
            rounds = [item for item in result["rounds"] if item["scenario"] == scene]
            for item in rounds:
                path = root / "performance" / f"round-{item['round']}" / f"{scene}.csv"
                verified = profile.summarize(path)
                require("memory_bytes" in verified, f"Missing process memory: {path}")
                require(verified["memory_bytes"]["process_private_bytes"]["maximum"] > 0,
                        f"Unsupported/empty memory counters: {path}")
                item.update(counts=verified["counts"], memory_bytes=verified["memory_bytes"])
            result["pooled"][scene]["counts"] = {
                key: sum(item["counts"][key] for item in rounds) for key in rounds[0]["counts"]}
            result["pooled"][scene]["memory_bytes"] = {
                key: dict(maximum=max(item["memory_bytes"][key]["maximum"] for item in rounds),
                          rounds=[dict(round=item["round"], **item["memory_bytes"][key]) for item in rounds])
                for key in rounds[0]["memory_bytes"]}
        # Preserve raw evidence; PNGs remain in build and have a separate hash /
        # pixel comparison record. No large model is copied into Git.
        for path in root.rglob("*"):
            if path.is_file() and path.suffix.lower() in {".csv", ".json", ".txt", ".log", ".xml"}:
                target = args.output / gpu / path.relative_to(root)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, target)
        report["gpus"][gpu] = result
    hashes = lambda gpu: {Path(item["Path"]).name: item["Hash"] for item in report["gpus"][gpu]["hashes"]}
    require(hashes("MX450") == hashes("IrisXe"), "Models/executables differ across GPU suites")
    require(len(archived_frames) == 1080, "Expected 1080 measured frames")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "summary.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for gpu in CARDS:
        for scene, item in report["gpus"][gpu]["pooled"].items():
            print(f"{gpu} {scene}: {item['fps']:.3f} FPS / p95 {item['p95_ms']:.3f} ms / target {int(item['target'])}")
    print("Archived 1080 frames; each GPU: 85/85 regressions and 8/8 large gates")


if __name__ == "__main__":
    main()
