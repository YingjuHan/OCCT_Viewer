"""GDB source script for the configured OCCT Debug SDK (not frame timing).

Set OCCTV_OCCT_SOURCE and CUTVIEW_UPLOAD_REPORT, then use GDB's `source`.
Breakpoints observe initNormalVbo and the two glBufferSubData call sites.
"""
import gdb
import json
import os

phase = 0
roles = {}
events = []
source = os.environ.get("OCCTV_OCCT_SOURCE", "D:/workspace/OpenCASCADE/OCCT_private/src")
report = os.environ["CUTVIEW_UPLOAD_REPORT"]
gdb.execute("set breakpoint pending on")
gdb.execute("set pagination off")
gdb.execute("set print thread-events off")


class Marker(gdb.Breakpoint):
    def stop(self):
        global phase
        phase += 1
        gdb.write(f"UPLOAD_MARKER {phase}\n")
        return False


class Upload(gdb.Breakpoint):
    def __init__(self, location, kind):
        super().__init__(location)
        self.kind = kind

    def stop(self):
        pointer = gdb.parse_and_eval("(Graphic3d_Buffer*)myAttribs.entity")
        address = int(pointer)
        if not address:
            return False
        count = int(pointer.dereference()["NbElements"])
        if self.kind == "init":
            dynamic = str(pointer.dynamic_type)
            if "Graphic3d_AttribBuffer" not in dynamic:
                return False
            attributes = gdb.parse_and_eval("(Graphic3d_AttribBuffer*)myAttribs.entity").dereference()
            if not bool(attributes["myIsMutable"]):
                return False
            roles[address] = "slice" if bool(attributes["myIsInterleaved"]) else "body"
        role = roles.get(address)
        if role is None:
            return False
        event = dict(phase=phase, kind=self.kind, role=role, attributes=hex(address), vertices=count)
        if self.kind not in ("init", "render"):
            event.update(start=int(gdb.parse_and_eval("aRange.Start")), bytes=int(gdb.parse_and_eval("aRange.Length")))
        events.append(event)
        gdb.write("UPLOAD_EVENT " + json.dumps(event) + "\n")
        return False


# The SDK source call sites are verified against OpenGl_PrimitiveArray.cxx.
Upload("OpenGl_PrimitiveArray::initNormalVbo", "init")
Upload("OpenGl_PrimitiveArray::Render", "render")
Upload(source + "/OpenGl/OpenGl_PrimitiveArray.cxx:374", "attributes")
Upload(source + "/OpenGl/OpenGl_PrimitiveArray.cxx:394", "indices")
Marker("CutBenchmarkCommands.cpp:memory")


def finished(event):
    failures = []
    if getattr(event, "exit_code", -1) != 0:
        failures.append("DRAW did not exit successfully")
    if phase != 5:
        failures.append("Missing capture phase markers")
    if not any(e["kind"] == "init" and e["role"] == "body" and e["phase"] == 0 for e in events):
        failures.append("No cold body VBO initialization observed")
    if any(e["role"] == "body" and (e["phase"] == 3 or (e["phase"] == 1 and e["kind"] != "render")) for e in events):
        failures.append("Warm motion or slice-only frame uploaded body data")
    if not any(e["role"] == "slice" and e["kind"] == "attributes" and e["phase"] == 1 for e in events):
        failures.append("No warm slice attribute uploads observed")
    if any(e["role"] == "slice" and e["kind"] == "indices" and e["phase"] == 1 for e in events):
        failures.append("Stable slice topology uploaded identity indices")
    count_updates = [e for e in events if e["role"] == "slice" and e["kind"] == "indices" and e["phase"] == 4]
    if not count_updates or any(e["start"] != 0 or e["bytes"] not in (2, 4) for e in count_updates):
        failures.append("Changed slice count did not upload a single reserved index")
    if not any(e["vertices"] == 0 for e in count_updates):
        failures.append("No empty slice draw-count update observed")
    if any(e["role"] == "slice" and e["kind"] == "init" and e["phase"] in (1, 2, 3, 4) for e in events):
        failures.append("Section VBO was recreated within its reserved capacity")
    field = [e for e in events if e["role"] == "body" and e["phase"] == 2 and e["kind"] != "render"]
    if not field or any(e["kind"] != "attributes" or e["start"] != e["vertices"]*24
                        or e["bytes"] != e["vertices"]*8 for e in field):
        failures.append("Field change did not upload only the contiguous UV range")
    with open(report, "w", encoding="utf-8") as stream:
        json.dump(dict(passed=not failures, failures=failures, events=events), stream, indent=2)
    gdb.write("UPLOAD_CAPTURE_" + ("FAIL " + str(failures) if failures else "PASS") + "\n")
    if failures:
        gdb.execute("quit 1")


gdb.events.exited.connect(finished)
