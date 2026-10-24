"""Validate and summarize full-frame cut benchmark CSVs (standard library only)."""
import argparse
import csv
import json
import math
from pathlib import Path
from statistics import mean


def summarize(path):
    with path.open(newline="", encoding="utf-8-sig") as stream:
        rows = [{key: float(value) for key, value in row.items()} for row in csv.DictReader(stream)]
    if not rows:
        raise ValueError(f"Empty benchmark: {path}")
    reasons = ["fallback_unsupported", "fallback_near_plane", "fallback_warped",
               "fallback_degenerate", "fallback_near_endpoint"]
    for index, row in enumerate(rows):
        if any(not math.isfinite(value) or value < 0 for value in row.values()):
            raise ValueError(f"Invalid counter: {path}:{index + 2}")
        if row["fallbacks"] != sum(row[key] for key in reasons):
            raise ValueError(f"Fallback reasons do not partition work: {path}:{index + 2}")
        if sum(row[key] for key in ["shared_point_ms", "append_ms", "fallback_ms"]) > row["merge_ms"] + 0.001:
            raise ValueError(f"Merge subphases overlap: {path}:{index + 2}")
        if row["submitted_body_bytes"] != 0 or row["body_builds"] != 1:
            raise ValueError(f"Warm frame rebuilt/uploaded body: {path}:{index + 2}")
    total = sorted(row["total_ms"] for row in rows)
    timings = {key: mean(row[key] for row in rows) for key in rows[0] if key.endswith("_ms")}
    counts = {key: sum(row[key] for row in rows) for key in reasons + ["candidates", "fallbacks", "topology_hits"]}
    fps = 1000 * len(rows) / sum(total)
    p95 = total[math.ceil(0.95 * len(total)) - 1]
    return dict(file=str(path), frames=len(rows), fps=fps, p50_ms=total[len(total) // 2],
                p95_ms=p95, target=fps >= 15 and p95 <= 67, mean_ms=timings, counts=counts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    files = sorted(path for path in args.directory.rglob("*.csv")
                   if path.stem in {"crossing", "rotating", "deformed"})
    if not files:
        parser.error("No crossing/rotating/deformed CSV found")
    results = [summarize(path) for path in files]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(results, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    for item in results:
        print(f"{item['file']}: FPS={item['fps']:.2f}, p95={item['p95_ms']:.2f} ms, target={item['target']}")


if __name__ == "__main__":
    main()
