#!/usr/bin/env python3
"""Write a single-layer unstructured hex plate, without the external VTK SDK."""
import argparse
import math
from pathlib import Path


def generate(output, stem, cycles=1.0, phase=0.0):
    n = 6
    points = []
    for layer in range(2):
        for j in range(n + 1):
            for i in range(n + 1):
                x, y = i / n, j / n
                top = 0.25 + 0.15 * math.sin(2 * math.pi * cycles * x + phase)
                points.append((x, y, top - (1 - layer) * 0.05))

    def node(i, j, k):
        return i + (n + 1) * (j + (n + 1) * k)

    # Permute point IDs and cell order: consumers must follow connectivity.
    order = list(range(0, len(points), 2)) + list(range(1, len(points), 2))
    remap = {old: new for new, old in enumerate(order)}
    points = [points[i] for i in order]
    cells = []
    for j in reversed(range(n)):
        for i in reversed(range(n)):
            cells.append([remap[node(a, b, k)] for a, b, k in (
                (i, j, 0), (i+1, j, 0), (i+1, j+1, 0), (i, j+1, 0),
                (i, j, 1), (i+1, j, 1), (i+1, j+1, 1), (i, j+1, 1))])
    xyz = [" ".join(format(v, ".17g") for v in p) for p in points]
    temperature = [format(p[2], ".17g") for p in points]
    output.mkdir(parents=True, exist_ok=True)
    legacy = ("# vtk DataFile Version 3.0\n6x6 sine plate; vertical thickness 0.05\n"
              "ASCII\nDATASET UNSTRUCTURED_GRID\nPOINTS 98 double\n"
              + "\n".join(xyz) + "\nCELLS 36 324\n"
              + "\n".join("8 " + " ".join(map(str, c)) for c in cells)
              + "\nCELL_TYPES 36\n" + "12\n" * 36
              + "POINT_DATA 98\nSCALARS temperature double 1\nLOOKUP_TABLE default\n"
              + "\n".join(temperature) + "\n")
    xml = ('<?xml version="1.0"?>\n<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian">\n'
           '  <UnstructuredGrid><Piece NumberOfPoints="98" NumberOfCells="36">\n'
           '    <Points><DataArray type="Float64" NumberOfComponents="3" format="ascii">\n'
           + "\n".join(xyz) + '\n    </DataArray></Points>\n    <Cells>\n'
           '      <DataArray type="Int32" Name="connectivity" format="ascii">\n'
           + "\n".join(" ".join(map(str, c)) for c in cells) + '\n      </DataArray>\n'
           '      <DataArray type="Int32" Name="offsets" format="ascii">'
           + " ".join(str(8 * (i+1)) for i in range(36)) + '</DataArray>\n'
           '      <DataArray type="UInt8" Name="types" format="ascii">'
           + " ".join(["12"] * 36) + '</DataArray>\n    </Cells>\n'
           '    <PointData Scalars="temperature"><DataArray type="Float64" Name="temperature" format="ascii">\n'
           + "\n".join(temperature) + '\n    </DataArray></PointData>\n'
           '  </Piece></UnstructuredGrid>\n</VTKFile>\n')
    for suffix, content in (("vtk", legacy), ("vtu", xml)):
        path = output / (stem + "." + suffix)
        with path.open("w", encoding="utf-8", newline="\n") as stream:
            stream.write(content)
        print(path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("build/sine-plate"))
    parser.add_argument("--stem", default="sine_plate_6x6")
    parser.add_argument("--cycles", type=float, default=1.0)
    parser.add_argument("--phase", type=float, default=0.0, help="Phase in radians")
    args = parser.parse_args()
    if not math.isfinite(args.cycles) or args.cycles <= 0 or not math.isfinite(args.phase):
        parser.error("cycles must be finite and positive; phase must be finite")
    if Path(args.stem).name != args.stem or args.stem in ("", ".", ".."):
        parser.error("stem must be a filename without a directory")
    generate(args.output, args.stem, args.cycles, args.phase)
