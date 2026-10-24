#!/usr/bin/env python3
"""Stream million-cell tetra/prism/pyramid/mixed/warped grids without VTK."""
import argparse
from array import array
import json
import math
from pathlib import Path
import struct
import sys
import time


class Grid:
    def __init__(self, n, topology, warp):
        self.n, self.topology, self.warp = n, topology, warp
        self.kinds = [topology if topology != "mixed" else ("hex", "prism", "pyramid")[x % 3] for x in range(n)]
        self.center_rank = {}
        for x, kind in enumerate(self.kinds):
            if kind == "pyramid":
                self.center_rank[x] = len(self.center_rank)
        self.grid_points = (n + 1) ** 3
        self.points = self.grid_points + n * n * len(self.center_rank)
        sizes = {"hex": (1, 8), "tetra": (6, 24), "prism": (2, 12), "pyramid": (6, 30)}
        self.cells = n * n * sum(sizes[k][0] for k in self.kinds)
        self.connectivity = n * n * sum(sizes[k][1] for k in self.kinds)

    def point(self, x, y, z):
        if not self.warp or x in (0, self.n) or y in (0, self.n) or z in (0, self.n):
            return x, y, z
        a = self.warp * math.sin(math.pi*x/self.n) * math.sin(math.pi*y/self.n) * math.sin(math.pi*z/self.n)
        point = (x+a*math.sin(.31*y), y+a*math.sin(.27*z), z+a*math.sin(.19*x))
        # Scalar references must use exactly the float coordinates stored in VTK.
        return struct.unpack("=3f", struct.pack("=3f", *point))

    def point_layers(self):
        for z in range(self.n + 1):
            yield (self.point(x, y, z) for y in range(self.n + 1) for x in range(self.n + 1))
        for z in range(self.n):
            yield (self.point(x+.5, y+.5, z+.5) for y in range(self.n) for x in self.center_rank)

    def cell_layer(self, z):
        n, side = self.n, self.n + 1
        tetra = ((0,1,2,6),(0,2,3,6),(0,3,7,6),(0,7,4,6),(0,4,5,6),(0,5,1,6))
        faces = ((0,1,2,3),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7))
        for y in range(n):
            for x, kind in enumerate(self.kinds):
                p = x + side * (y + side*z)
                h = (p,p+1,p+side+1,p+side,p+side*side,p+side*side+1,p+side*side+side+1,p+side*side+side)
                if kind == "hex":
                    yield 12, h
                elif kind == "tetra":
                    for ids in tetra:
                        yield 10, tuple(h[i] for i in ids)
                elif kind == "prism":
                    for ids in ((0,1,2,4,5,6),(0,2,3,4,6,7)):
                        yield 13, tuple(h[i] for i in ids)
                else:
                    center = self.grid_points + (z*n+y)*len(self.center_rank) + self.center_rank[x]
                    for ids in faces:
                        yield 14, tuple(h[i] for i in ids) + (center,)

    def blocks(self, name):
        if name in ("points", "temperature", "displacement"):
            for layer in self.point_layers():
                values = array("f" if name == "points" else "d")
                for x, y, z in layer:
                    if name == "points":
                        values.extend((x, y, z))
                    elif name == "temperature":
                        values.append(x + 2*y + 3*z)
                    else:
                        values.extend((.02*y, .01*x, .005*z))
                yield values
        else:
            offset, element = 0, 0
            for z in range(self.n):
                values = array("B" if name == "types" else "d" if name == "pressure" else "i")
                for kind, nodes in self.cell_layer(z):
                    if name in ("connectivity", "legacy_cells"):
                        if name == "legacy_cells":
                            values.append(len(nodes))
                        values.extend(nodes)
                    elif name == "offsets":
                        offset += len(nodes)
                        values.append(offset)
                    elif name in ("types", "legacy_types"):
                        values.append(kind)
                    elif name == "pressure":
                        values.append(7 + .25*element)
                    else:
                        raise ValueError(name)
                    element += 1
                yield values

    def payload(self, stream, name, endian):
        for values in self.blocks(name):
            if values.itemsize > 1 and sys.byteorder != endian:
                values.byteswap()
            values.tofile(stream)

    def write(self, path, legacy=False):
        with path.open("wb") as stream:
            if legacy:
                def line(text):
                    stream.write((text + "\n").encode("ascii"))
                line("# vtk DataFile Version 3.0\nNative cut mixed grid\nBINARY\nDATASET UNSTRUCTURED_GRID")
                line(f"POINTS {self.points} float")
                self.payload(stream, "points", "big")
                line(f"\nCELLS {self.cells} {self.connectivity+self.cells}")
                self.payload(stream, "legacy_cells", "big")
                line(f"\nCELL_TYPES {self.cells}")
                self.payload(stream, "legacy_types", "big")
                line(f"\nPOINT_DATA {self.points}\nSCALARS temperature double 1\nLOOKUP_TABLE default")
                self.payload(stream, "temperature", "big")
                line("\nVECTORS displacement double")
                self.payload(stream, "displacement", "big")
                line(f"\nCELL_DATA {self.cells}\nSCALARS pressure double 1\nLOOKUP_TABLE default")
                self.payload(stream, "pressure", "big")
                line("")
                return
            arrays = [("points","Float32",3,12*self.points), ("connectivity","Int32",1,4*self.connectivity),
                      ("offsets","Int32",1,4*self.cells), ("types","UInt8",1,self.cells),
                      ("temperature","Float64",1,8*self.points), ("displacement","Float64",3,24*self.points),
                      ("pressure","Float64",1,8*self.cells)]
            tags, offset = {}, 0
            for name, kind, components, size in arrays:
                tags[name] = f'<DataArray type="{kind}" Name="{name}" NumberOfComponents="{components}" format="appended" offset="{offset}"/>'
                offset += 8 + size
            header = ('<?xml version="1.0"?>\n<VTKFile type="UnstructuredGrid" version="1.0" byte_order="LittleEndian" header_type="UInt64">\n'
                      f'<UnstructuredGrid><Piece NumberOfPoints="{self.points}" NumberOfCells="{self.cells}">\n'
                      f'<Points>{tags["points"]}</Points><Cells>' + ''.join(tags[k] for k in ("connectivity","offsets","types")) + '</Cells>\n'
                      f'<PointData Scalars="temperature" Vectors="displacement">{tags["temperature"]}{tags["displacement"]}</PointData>\n'
                      f'<CellData Scalars="pressure">{tags["pressure"]}</CellData></Piece></UnstructuredGrid>\n<AppendedData encoding="raw">_')
            stream.write(header.encode("ascii"))
            for name, _, _, size in arrays:
                stream.write(struct.pack("<Q", size)); before = stream.tell()
                self.payload(stream, name, "little")
                if stream.tell() - before != size:
                    raise ValueError(f"Incorrect {name} payload size")
            stream.write(b"\n</AppendedData>\n</VTKFile>\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--grid", type=int, required=True)
    parser.add_argument("--topology", choices=["hex","tetra","prism","pyramid","mixed"], required=True)
    parser.add_argument("--warp", type=float, default=0, help="Interior perturbation in cell units; boundary remains a cube")
    parser.add_argument("--formats", nargs="+", choices=["vtk","vtu"], default=["vtu"])
    parser.add_argument("--output", type=Path, default=Path("build/cut-nonregular"))
    args = parser.parse_args()
    if not 2 <= args.grid <= 200 or not 0 <= args.warp <= .2:
        parser.error("grid must be 2..200 and warp 0..0.2")
    grid = Grid(args.grid, args.topology, args.warp)
    if grid.cells > 10000000:
        parser.error("Model exceeds the reader's default ten-million-cell limit")
    args.output.mkdir(parents=True, exist_ok=True)
    stem = f"{args.topology}_{args.grid}" + ("_warped" if args.warp else "")
    records = []
    for extension in args.formats:
        path = args.output / f"{stem}.{extension}"
        temporary = path.with_suffix(path.suffix + ".tmp"); start = time.monotonic()
        grid.write(temporary, extension == "vtk"); temporary.replace(path)
        record = dict(file=path.name, topology=args.topology, grid=args.grid, warp=args.warp, points=grid.points,
                      cells=grid.cells, bytes=path.stat().st_size, seconds=time.monotonic()-start)
        records.append(record); print(json.dumps(record), flush=True)
    (args.output / f"{stem}.json").write_text(json.dumps(records, indent=2)+"\n", encoding="utf-8")


if __name__ == "__main__":
    main()
