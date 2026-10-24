"""Rebuild/check the bounded direct OCCT dependency appendix (stdlib only)."""
from pathlib import Path
import argparse
import re

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "DOCUMENT/cut-view-manager-occt-api-inventory.md"
MARKER = "<!-- generated-occt-inventory:start -->"
END = "<!-- generated-occt-inventory:end -->"


def source_files():
    files = set()
    for folder in ("source/ViewCaeGeom", "source/RWCae", "source/RWVtk"):
        files.update(p for p in (ROOT / folder).iterdir() if p.suffix in (".h", ".cpp"))
    for prefix in ("NativeCut",):
        files.update((ROOT / "source/Viewer").glob(prefix + "*.h"))
        files.update((ROOT / "source/Viewer").glob(prefix + "*.cpp"))
    for prefix in ("InteractiveCaeCut", "MeshCutGpuPresentation", "MeshPresentation",
                   "MeshPart", "MeshColor", "MeshPropertyData", "CaeMeshTools", "ColorScaleTools"):
        for suffix in (".h", ".cpp"):
            files.update((ROOT / "source/ViewCae").glob(prefix + "*" + suffix))
    for name in ("source/RWBase/TaskManager.h", "source/RWBase/TaskManager.cpp",
                 "source/RWMeshConv/MeshLoader.h", "source/RWMeshConv/MeshLoader.cpp",
                 "source-draw/ViewCaeTest/CutViewCommands.h",
                 "source-draw/ViewCaeTest/CutViewCommands.cpp",
                 "source-draw/ViewCaeTest/CutBenchmarkCommands.cpp"):
        path = ROOT / name
        if not path.is_file():
            raise RuntimeError("Missing evidence source: " + name)
        files.add(path)
    return sorted(files)


def group(name):
    groups = (("BRep", "BRep / 布尔 / 离散化"), ("Geom", "几何曲线曲面与投影"),
              ("GCPnts", "几何曲线曲面与投影"), ("GProp", "几何测量"),
              ("BVH", "BVH 空间与值域加速"), ("Bnd", "包围盒与值域"),
              ("gp_", "基础几何数学"), ("math_", "基础几何数学"),
              ("Poly", "网格与离散数据"), ("NCollection", "容器与内存"),
              ("TCol", "容器与内存"), ("Standard", "基础运行时"),
              ("OSD", "线程 / 文件系统"), ("Graphic3d", "GPU / 图形结构"),
              ("Prs3d", "显示属性与图元"), ("AIS", "交互与对象生命周期"),
              ("Select", "拾取"), ("V3d", "视口与视图"), ("Aspect", "窗口与属性"),
              ("Image", "纹理与截图"), ("Quantity", "颜色"), ("Top", "拓扑数据与遍历"),
              ("LDOM", "XML"), ("RWGltf", "输入辅助"), ("Message", "日志"),
              ("Draw", "DRAW 测试宿主"), ("DBRep", "DRAW 测试宿主"),
              ("ViewerTest", "DRAW 测试宿主"))
    return next((label for prefix, label in groups if name.startswith(prefix)), "其他 OCCT 辅助")


def inventory():
    files = source_files()
    includes = {}
    texts = {}
    for path in files:
        content = path.read_text(encoding="utf-8-sig")
        texts[path] = content
        for line, text in enumerate(content.splitlines(), 1):
            match = re.match(r"\s*#\s*include\s*[<\"]([^>\"]+\.hxx)[>\"]", text)
            if match:
                includes.setdefault(match[1], []).append((path, line))
    lines = [MARKER, "", "## 扫描范围与完整直接头文件清单", "",
             f"扫描 {len(files)} 个项目源文件，发现 {len(includes)} 种直接 `.hxx` 依赖。",
             "这是指定范围的词法依赖清单，不是完整运行时调用图。头文件可能只提供类型，",
             "不能据此断言该算法每帧执行。OCCT 内部传递调用、虚函数调度和模板展开未展开。",
             "核心功能与成员方法的人工核对见前文；引用位置由下面的列表覆盖。", "",
             "| 功能组 | OCCT 头文件 | 明确限定类名的静态调用 | 直接引用文件 |",
             "| --- | --- | --- | --- |"]
    for header, refs in sorted(includes.items(), key=lambda item: (group(item[0]), item[0])):
        stem = Path(header).stem
        calls = set()
        for path, content in texts.items():
            # This column intentionally excludes inferred member receivers.
            calls.update(re.findall(r"\b" + re.escape(stem) + r"::([A-Za-z_]\w*)\s*\(", content))
        links = []
        for path in sorted(set(path for path, _ in refs)):
            rel = path.relative_to(ROOT).as_posix()
            links.append(f"[{path.stem}]({ '../' + rel })")
        static = ", ".join("`" + name + "`" for name in sorted(calls)) or "—"
        lines.append(f"| {group(header)} | `{header}` | {static} | {'、'.join(links)} |")
    lines += ["", "### 纳入扫描的源文件", "", "<details>", "<summary>展开全部证据文件</summary>", ""]
    for path in files:
        rel = path.relative_to(ROOT).as_posix()
        lines.append(f"- [{rel}](../{rel})")
    lines += ["", "</details>", "", END]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    content = OUTPUT.read_text(encoding="utf-8")
    start, end = content.index(MARKER), content.index(END) + len(END)
    generated = inventory()
    if args.check:
        if content[start:end] != generated:
            raise SystemExit("OCCT inventory is stale; rebuild it without --check")
        print("OCCT inventory matches current source")
        return
    OUTPUT.write_text(content[:start] + generated + content[end:], encoding="utf-8", newline="\n")
    print("Updated " + str(OUTPUT))


if __name__ == "__main__":
    main()
