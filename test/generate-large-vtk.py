#!/usr/bin/env python3
"""Stream regular hex grids to binary legacy VTK and raw-appended VTU.

Uses only the Python standard library. Each block contains one grid layer;
memory usage does not grow with the number of layers. Generated files belong
in build/, never in the source fixtures directory.
"""

import argparse
from array import array
import json
from pathlib import Path
import struct
import sys
import time


def blocks(n, name):
    side = n + 1
    if name in ("points", "temperature", "displacement"):
        for z in range(side):
            values = array("f" if name == "points" else "d")
            for y in range(side):
                for x in range(side):
                    if name == "points":
                        values.extend((x, y, z))
                    elif name == "temperature":
                        values.append(x + 2 * y + 3 * z)
                    else:
                        # Affine deformation allows independent analytic checks.
                        values.extend((0.02 * y, 0.01 * x, 0.005 * z))
            yield values
    elif name in ("connectivity", "legacy_cells"):
        for z in range(n):
            values = array("i")
            for y in range(n):
                for x in range(n):
                    p = x + side * (y + side * z)
                    if name == "legacy_cells":
                        values.append(8)
                    values.extend((p, p+1, p+side+1, p+side,
                                   p+side*side, p+side*side+1,
                                   p+side*side+side+1, p+side*side+side))
            yield values
    else:
        for z in range(n):
            if name == "offsets":
                yield array("i", (8 * (i + 1) for i in range(z*n*n, (z+1)*n*n)))
            elif name == "types":
                yield array("B", [12]) * (n*n)
            elif name == "legacy_types":
                yield array("i", [12]) * (n*n)
            elif name == "pressure":
                yield array("d", (7 + 0.25 * (x + n*(y+n*z))
                                  for y in range(n) for x in range(n)))
            else:
                raise ValueError(name)


def payload(stream, n, name, endian):
    for values in blocks(n, name):
        if values.itemsize > 1 and sys.byteorder != endian:
            values.byteswap()
        values.tofile(stream)


def legacy(path, n, deformation):
    points, cells = (n+1)**3, n**3
    with path.open("wb") as stream:
        def line(text):
            stream.write((text + "\n").encode("ascii"))
        line("# vtk DataFile Version 3.0\nLarge regular hexahedra\nBINARY\nDATASET UNSTRUCTURED_GRID")
        line(f"POINTS {points} float")
        payload(stream, n, "points", "big")
        line(f"\nCELLS {cells} {9*cells}")
        payload(stream, n, "legacy_cells", "big")
        line(f"\nCELL_TYPES {cells}")
        payload(stream, n, "legacy_types", "big")
        line(f"\nPOINT_DATA {points}\nSCALARS temperature double 1\nLOOKUP_TABLE default")
        payload(stream, n, "temperature", "big")
        if deformation:
            line("\nVECTORS displacement double")
            payload(stream, n, "displacement", "big")
        line(f"\nCELL_DATA {cells}\nSCALARS pressure double 1\nLOOKUP_TABLE default")
        payload(stream, n, "pressure", "big")
        line("")


def xml(path, n, deformation):
    points, cells = (n+1)**3, n**3
    arrays = [("points", "Float32", 3, 12*points),
              ("connectivity", "Int32", 1, 32*cells),
              ("offsets", "Int32", 1, 4*cells),
              ("types", "UInt8", 1, cells),
              ("temperature", "Float64", 1, 8*points),
              ("pressure", "Float64", 1, 8*cells)]
    if deformation:
        arrays.append(("displacement", "Float64", 3, 24*points))
    tags, offset = {}, 0
    for name, kind, components, size in arrays:
        tags[name] = (f'<DataArray type="{kind}" Name="{name}" '
                      f'NumberOfComponents="{components}" format="appended" offset="{offset}"/>')
        offset += 8 + size
    header = ('<?xml version="1.0"?>\n'
              '<VTKFile type="UnstructuredGrid" version="1.0" '
              'byte_order="LittleEndian" header_type="UInt64">\n'
              f'<UnstructuredGrid><Piece NumberOfPoints="{points}" NumberOfCells="{cells}">\n'
              f'<Points>{tags["points"]}</Points>\n<Cells>'
              + ''.join(tags[k] for k in ("connectivity", "offsets", "types"))
              + '</Cells>\n<PointData Scalars="temperature"'
              + (' Vectors="displacement"' if deformation else '') + '>' + tags["temperature"]
              + (tags["displacement"] if deformation else '')
              + '</PointData>\n<CellData Scalars="pressure">' + tags["pressure"]
              + '</CellData>\n</Piece></UnstructuredGrid>\n<AppendedData encoding="raw">_')
    with path.open("wb") as stream:
        stream.write(header.encode("ascii"))
        for name, _, _, size in arrays:
            stream.write(struct.pack("<Q", size))
            before = stream.tell()
            payload(stream, n, name, "little")
            if stream.tell() - before != size:
                raise RuntimeError(f"Incorrect payload size: {name}")
        stream.write(b"\n</AppendedData>\n</VTKFile>\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("build/large-vtk"))
    parser.add_argument("--grids", type=int, nargs="+", default=[128, 150])
    parser.add_argument("--formats", choices=["vtk", "vtu"], nargs="+", default=["vtk", "vtu"])
    parser.add_argument("--deformation", action="store_true")
    args = parser.parse_args()
    # Int32 legacy counts/connectivity and current reader's 10M-cell default.
    if any(n < 2 or n > 200 for n in args.grids):
        parser.error("Grid side must be between 2 and 200")
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = []
    for n in args.grids:
        for extension in args.formats:
            path = args.output / f"hex_{n}.{extension}"
            temporary = path.with_suffix(path.suffix + ".tmp")
            start = time.monotonic()
            (legacy if extension == "vtk" else xml)(temporary, n, args.deformation)
            temporary.replace(path)
            record = dict(file=path.name, grid=n, points=(n+1)**3, cells=n**3,
                          bytes=path.stat().st_size, deformation=args.deformation,
                          seconds=round(time.monotonic()-start, 3))
            manifest.append(record)
            print(json.dumps(record), flush=True)
    (args.output / "manifest.json").write_text(json.dumps(manifest, indent=2)+"\n", encoding="utf-8")


if __name__ == "__main__":
    main()
