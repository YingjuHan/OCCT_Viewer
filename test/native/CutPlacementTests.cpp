#include <ViewCaeGeom/MeshCutFrame.h>
#include <ViewCaeGeom/MeshCutSlicing.h>
#include <iostream>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace ViewCaeGeom;
namespace {
void require(bool value, const char* message) { if (!value) { throw std::runtime_error(message); } }
void check(double a, double b) { require(std::isfinite(a) && std::abs(a-b) < 1e-10,"Placement numerical mismatch"); }
void tests()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (auto p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) { mesh->AddNodePosition(p); }
    const RWCae::MeshEntityId nodes[] = {0,1,2,3}; mesh->AddElement(RWCae::MeshElementTetrahedron(nodes));
    MeshCutViewManager cuts(mesh); require(cuts.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.25)),"Add plane");
    MeshCutPlanePlacement p;
    require(p.Define(Graphic3d_Vec3d(.25,.1,.2),Graphic3d_Vec3d(2,0,0),Graphic3d_Vec3d(0,0,3)),"Custom axis2");
    require(cuts.SetPlanePlacement("p",p),"Set placement");
    cuts.Evaluate("p"); const auto source = cuts.Statistics();
    require(cuts.TranslatePlane("p",.2) && cuts.RotatePlane("p",2,90),"Absolute motion");
    auto d = cuts.Definitions()[0]; check(d.Plane.x(),0); check(d.Plane.y(),1); check(d.Plane.w(),-.1); check(d.Placement.Offset,0);
    const auto json = cuts.DumpDefinitionsJson();
    for (int i = 0; i < 100; ++i) { require(cuts.RotatePlane("p",2,90),"Repeat angle"); }
    require(cuts.DumpDefinitionsJson() == json,"Absolute rotation accumulated");
    require(cuts.RotatePlane("p",3,90),"Other original tangent");
    d = cuts.Definitions()[0]; check(d.Plane.z(),1); check(d.Plane.w(),-.2);
    require(cuts.TranslatePlane("p",.1),"Translate rotated plane");
    check(cuts.Definitions()[0].Plane.w(),-.3);
    require(cuts.RotatePlane("p",3,0),"Return original pose");
    cuts.Evaluate("p"); require(cuts.Statistics().SourcePreparations == source.SourcePreparations,"Motion rebuilt source");
    const auto baseline = cuts.DumpDefinitionsJson();
    p.Axis2 = p.Normal; require(!cuts.SetPlanePlacement("p",p),"Parallel axes accepted");
    require(!cuts.RotatePlane("p",1,20) && !cuts.RotatePlane("p",2,std::numeric_limits<double>::infinity()),"Invalid rotation accepted");
    require(!cuts.TranslatePlane("p",std::numeric_limits<double>::quiet_NaN()),"NaN offset accepted");
    require(cuts.DumpDefinitionsJson() == baseline,"Invalid operation changed live state");
    require(cuts.RestoreDefinitionsJson(json) && cuts.DumpDefinitionsJson() == json,"Placement roundtrip");
    auto invalid = json; const auto where = invalid.find("\"angle\":90"); require(where != std::string::npos,"Test angle missing");
    invalid.replace(where,10,"\"angle\":60");
    require(!cuts.RestoreDefinitionsJson(invalid) && cuts.DumpDefinitionsJson() == json,"Mismatched equation accepted");
    require(cuts.Duplicate("p","copy"),"Copy placement");
    const auto copy = cuts.Definitions()[0].Placement; check(copy.Origin.y(),.1); check(copy.Angle,90);
    const std::string legacy = "{\"version\":1,\"cuts\":[{\"id\":\"p\",\"name\":\"P\",\"plane\":[0,2,0,-1],\"visibility\":2,\"enabled\":true,\"cellMode\":\"constant\"}]}";
    require(cuts.RestoreDefinitionsJson(legacy),"Old equation migration");
    d = cuts.Definitions()[0]; check(d.Placement.Origin.y(),.5); check(d.Placement.Angle,0);
    require(d.Placement.IsValid(),"Migrated frame invalid");
    auto version7 = json;
    version7.replace(version7.find("\"version\":9"),11,"\"version\":7");
    require(cuts.RestoreDefinitionsJson(version7),"Version 7 migration");
    check(cuts.Definitions()[0].Placement.Angle,0);
    check(cuts.Definitions()[0].Plane.y(),1);
    MeshCutFrame frame; frame.Origin = Graphic3d_Vec3d(4,5,6); frame.X = Graphic3d_Vec3d(0,1,0); frame.Y = Graphic3d_Vec3d(-1,0,0);
    const auto global = frame.PlacementToGlobal(d.Placement), local = frame.PlacementToLocal(global);
    for (int i = 0; i < 3; ++i) { check(local.Origin[i],d.Placement.Origin[i]); check(local.Normal[i],d.Placement.Normal[i]); }
    for (const auto& sample : MeshCutSlicing::Range(d,-.1,.1,3)) {
        const auto equation = sample.Definition.Placement.Equation();
        for (int i = 0; i < 4; ++i) { check(equation[i],sample.Definition.Plane[i]); }
    }
}
}
int main()
{
    try { tests(); std::cout << "CUT_PLACEMENT_PASS\n"; return 0; }
    catch (const std::exception& error) { std::cerr << "CUT_PLACEMENT_FAIL " << error.what() << '\n'; return 1; }
}
