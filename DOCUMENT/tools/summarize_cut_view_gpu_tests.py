"""Validate and archive cut-view GPU measurements; Python standard library only."""
import argparse
import csv
import hashlib
import io
import json
import math
from pathlib import Path
import re
import statistics
import xml.etree.ElementTree as ET


SCENES = ("crossing", "rotating", "deformed")
CARDS = {"MX450": "NVIDIA GeForce MX450", "IrisXe": "Intel(R) Iris(R) Xe Graphics"}
CASES = ("placement", "material", "free_body", "iso_binding", "slices",
         "plot_options", "automatic", "scope")


def read(path):
    data = path.read_bytes()
    return data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def metrics(rows):
    values = sorted(float(row["total_ms"]) for row in rows)
    n = len(values)
    fps = 1000 * n / sum(values)
    p95 = values[math.ceil(.95 * n) - 1]
    return {"frames": n, "p50_ms": values[n // 2], "p95_ms": p95, "fps": fps,
            "target": fps >= 15 and p95 <= 67,
            "stage_means": {key: statistics.mean(float(row[key]) for row in rows)
                            for key in rows[0] if key.endswith("_ms")},
            "submitted_slice_bytes_mean": statistics.mean(float(row["submitted_slice_bytes"]) for row in rows),
            "body_builds": sorted({int(row["body_builds"]) for row in rows}),
            "submitted_body_bytes": sorted({int(row["submitted_body_bytes"]) for row in rows})}


def performance(gpu, root, frames, repetitions, archive, label, historical=False):
    manifests = None if historical else json.loads(read(root / "manifest.json"))
    if manifests is not None:
        require(manifests["succeeded"] and manifests["preferenceRestored"], f"Incomplete suite: {root}")
        require(manifests["gpu"] == gpu and manifests["frames"] == frames
                and manifests["repetitions"] == repetitions, f"Protocol mismatch: {root}")
    result = {"directory": str(root), "manifest": manifests, "rounds": [], "pooled": {}}
    if not historical:
        result["hashes"] = json.loads(read(root / "hashes.json"))
        result["hardware"] = {"cpu": json.loads(read(root / "cpu.json")),
                              "adapters": json.loads(read(root / "adapters.json")),
                              "power_plan": read(root / "power-plan.txt").strip()}
        result["probes"] = {}
        for phase in ("before", "after"):
            probe = read(root / f"renderer-{phase}.log")
            require(CARDS[gpu] in probe and "CUT_GPU_PROBE_PASS" in probe, f"Invalid GPU probe: {root}/{phase}")
            result["probes"][phase] = probe
    pooled = {scene: [] for scene in SCENES}
    for index in range(1, repetitions + 1):
        folder = root if historical else root / "performance" / f"round-{index}"
        log = read(folder / "run.log")
        require("CUT_BENCHMARK_PASS" in log and "CUT_BENCHMARK_FAIL" not in log, f"Failed run: {folder}")
        records = {}
        for line in log.splitlines():
            if "CUT_BENCHMARK_RESULT " in line:
                tokens = line.split("CUT_BENCHMARK_RESULT ", 1)[1].split()
                record = dict(zip(tokens[::2], tokens[1::2]))
                records[record["scenario"]] = record
        require(set(records) == set(SCENES), f"Missing scenes: {folder}")
        for scene in SCENES:
            path = folder / f"{scene}.csv"
            rows = list(csv.DictReader(io.StringIO(read(path))))
            require(len(rows) == frames and [int(row["frame"]) for row in rows] == list(range(frames)), f"Invalid frames: {path}")
            require(all(int(row["body_builds"]) == 1 and int(row["submitted_body_bytes"]) == 0 for row in rows), f"Body upload/rebuild: {path}")
            renderer = read(folder / f"{scene}-renderer.txt").strip()
            require(CARDS[gpu] in renderer, f"Wrong GPU: {path}: {renderer}")
            record = records[scene]
            require(record["cells"] == "3375000" and record["mask"] == "3" and record["backend"] == "auto"
                    and record["actual_redraw"] == "1" and record["original_FE"] == "1"
                    and int(record["frames"]) == frames, f"Wrong workload: {folder}/{scene}")
            require(abs(float(record["deformation_scale"]) - (.1 if scene == "deformed" else 0)) < 1e-12, f"Wrong scale: {folder}/{scene}")
            summary = metrics(rows)
            for key in ("p50_ms", "p95_ms", "fps"):
                require(abs(summary[key] - float(record[key])) < .001, f"CSV/log mismatch: {path}/{key}")
            require(int(record["target"]) == int(summary["target"]), f"Target mismatch: {path}")
            summary.update({"round": index, "scenario": scene, "renderer": renderer,
                            "import_ms": float(record["import_ms"]), "setup_ms": float(record["setup_ms"]),
                            "csv_sha256": sha(path), "run_log_sha256": sha(folder / "run.log")})
            result["rounds"].append(summary)
            pooled[scene].extend(rows)
            archive.extend(dict(dataset=label, gpu=gpu, round=index, scenario=scene, **row) for row in rows)
    for scene, rows in pooled.items():
        summary = metrics(rows)
        rounds = [row for row in result["rounds"] if row["scenario"] == scene]
        summary["round_fps_range"] = [min(row["fps"] for row in rounds), max(row["fps"] for row in rounds)]
        summary["round_p95_range_ms"] = [min(row["p95_ms"] for row in rounds), max(row["p95_ms"] for row in rounds)]
        summary["import_mean_ms"] = statistics.mean(row["import_ms"] for row in rounds)
        summary["setup_mean_ms"] = statistics.mean(row["setup_ms"] for row in rounds)
        result["pooled"][scene] = summary
    return result


def functional(root):
    tree = ET.fromstring(read(root / "functional.xml"))
    tests = [{"name": test.attrib["name"], "seconds": float(test.get("time", 0)),
              "passed": not any(test.find(tag) is not None for tag in ("failure", "error", "skipped"))}
             for test in tree.iter("testcase")]
    require(len(tests) == 84 and all(test["passed"] for test in tests), "Functional suite did not pass 84/84")
    large = {}
    for case in CASES:
        path = root / "large" / case / "run.log"
        log = read(path)
        marker = f"CUT_{case.upper()}_LARGE_PASS"
        require(marker in log, f"Large gate failed: {path}")
        large[case] = {"pass": marker, "log_sha256": sha(path),
                       "states": [line for line in log.splitlines()
                                  if "sourceReserved" in line or "_LARGE_BATCH_MS" in line]}
    return {"passed": len(tests), "total": len(tests), "cases": tests,
            "elapsed_seconds": float(re.search(r"Total Test time \(real\) = ([\d.]+)", read(root / "functional.log"))[1]),
            "junit_sha256": sha(root / "functional.xml"), "large": large}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("mx450", "irisxe", "mx450-baseline", "irisxe-baseline", "output"):
        parser.add_argument(f"--{name}", required=True, type=Path)
    args = parser.parse_args()
    frames = []
    report = {"schema": 1, "protocol": {"cells": 3375000, "nodes": 3442951, "mask": 3,
               "frames_per_round": 60, "rounds": 3, "deformation_scale": .1,
               "p50": "sorted[n//2] (upper median)", "p95": "nearest rank ceil(0.95*n)",
               "fps": "1000*n/sum(total_ms)", "target": "fps>=15 and p95_ms<=67"},
              "long_sample": {}, "baseline20": {}}
    for gpu, root, baseline in (("MX450", args.mx450, args.mx450_baseline),
                                ("IrisXe", args.irisxe, args.irisxe_baseline)):
        report["long_sample"][gpu] = performance(gpu, root, 60, 3, frames, "long_sample")
        report["baseline20"][gpu] = performance(gpu, baseline, 20, 1, frames, "baseline20", historical=gpu == "IrisXe")
    report["mx450_functional"] = functional(args.mx450)
    # Compare exact input and native renderer binaries across the two long runs.
    hashes = lambda gpu: {Path(row["Path"]).name: row["Hash"] for row in report["long_sample"][gpu]["hashes"]}
    require(hashes("MX450") == hashes("IrisXe"), "Input/executable hashes differ between GPU runs")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "summary.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    with (args.output / "frames.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(frames[0]))
        writer.writeheader()
        writer.writerows(frames)
    for gpu in CARDS:
        for scene, row in report["long_sample"][gpu]["pooled"].items():
            print(f'{gpu:7} {scene:8} p50={row["p50_ms"]:.3f} p95={row["p95_ms"]:.3f} FPS={row["fps"]:.3f} target={int(row["target"])}')
    print(f"Archived {len(frames)} frames; functional 84/84; large gates 8/8")


if __name__ == "__main__":
    main()
