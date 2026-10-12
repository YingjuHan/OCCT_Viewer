#include <ViewCaeGeom/MeshCutUnion.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <ViewCae/InteractiveCaeCutView.h>
#include <RWCae/MeshScalarProperty.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#ifdef near
#undef near
#endif
using namespace ViewCaeGeom;
static void require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}
static void near(double actual, double expected, const char* message)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > 2e-5) {
        std::cerr << message << ": " << actual << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
static Handle(RWCae::MeshData) cube()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(1,1,0),Graphic3d_Vec3(0,1,0),
        Graphic3d_Vec3(0,0,1),Graphic3d_Vec3(1,0,1),Graphic3d_Vec3(1,1,1),Graphic3d_Vec3(0,1,1)}) { mesh->AddNodePosition(p); }
    const RWCae::MeshEntityId nodes[] = {0,1,2,3,4,5,6,7}; mesh->AddElement(RWCae::MeshElementHexahedron(nodes)); return mesh;
}
static void basic()
{
    auto mesh = cube(); MeshCutViewManager manager(mesh);
    require(manager.AddPlane("a",Graphic3d_Vec4d(1,0,0,-0.4)),"Add a");
    manager.SetVisibility("a",1);
    auto single = manager.EvaluatePresentation(); near(single->Volume,0.4,"Single volume"); near(single->Area,3.6,"Single area");
    require(manager.Duplicate("a","b"),"Duplicate");
    auto duplicate = manager.EvaluatePresentation(); near(duplicate->Volume,0.4,"Duplicate volume"); near(duplicate->Area,3.6,"Duplicate area");
    require(duplicate->Unified && !duplicate->Provenance.empty(),"Union provenance missing");
    for (const auto& p : duplicate->Provenance) { require(p.CutId == "a" && p.SourceElement == 0,"Unstable duplicate owner"); }
    manager.SetPlane("b",Graphic3d_Vec4d(0,1,0,-0.3));
    auto crossing = manager.EvaluatePresentation(); near(crossing->Volume,0.58,"OR volume"); near(crossing->Area,5.16,"OR boundary area");
    manager.SetVisibility("a",7); manager.SetVisibility("b",7);
    auto all = manager.EvaluatePresentation(); near(all->Volume,1,"Full volume"); near(all->Area,6,"Internal caps must disappear");
    auto independent = manager.Evaluate("a"); require(!independent.PresentationOnly && !independent.Portions[1].IsNull(),"Independent section changed");
    bool rejected = false;
    try { MeshCutIntegration::HeatFlux(all->Branches[0],{{0,1,2}}); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected,"Union accepted for physical integration");
    manager.SetVisibility("a",2); manager.SetVisibility("b",2);
    auto sheets = manager.EvaluatePresentation(); near(sheets->Volume,0,"Sheets volume"); near(sheets->Area,2,"Crossing sheets area");
    manager.SetPlane("b",Graphic3d_Vec4d(1,0,0,-0.4));
    auto same = manager.EvaluatePresentation(); near(same->Area,1,"Duplicate sheet area");
}
// Independent rectangular occupancy oracle: no OCCT or cut backend operations.
static void masks()
{
    MeshCutViewManager manager(cube()); manager.AddPlane("x",Graphic3d_Vec4d(1,0,0,-0.4)); manager.AddPlane("y",Graphic3d_Vec4d(0,1,0,-0.3));
    const double dx[] = {0.4,0.6}, dy[] = {0.3,0.7};
    for (unsigned x = 0; x < 8; ++x) { for (unsigned y = 0; y < 8; ++y) {
        manager.SetVisibility("x",x); manager.SetVisibility("y",y);
        bool kept[2][2]; double volume = 0, perimeter = 0, sheets = 0;
        for (int i = 0; i < 2; ++i) { for (int j = 0; j < 2; ++j) {
            kept[i][j] = (x&(i ? 4 : 1)) || (y&(j ? 4 : 1)); if (kept[i][j]) { volume += dx[i]*dy[j]; }
        } }
        for (int i = 0; i < 2; ++i) { for (int j = 0; j < 2; ++j) { if (kept[i][j]) {
            perimeter += dy[j]*(i == 0 || !kept[i-1][j]); perimeter += dy[j]*(i == 1 || !kept[i+1][j]);
            perimeter += dx[i]*(j == 0 || !kept[i][j-1]); perimeter += dx[i]*(j == 1 || !kept[i][j+1]);
        } } }
        if (x&2) { for (int j = 0; j < 2; ++j) { if (!kept[0][j] && !kept[1][j]) { sheets += dy[j]; } } }
        if (y&2) { for (int i = 0; i < 2; ++i) { if (!kept[i][0] && !kept[i][1]) { sheets += dx[i]; } } }
        const auto result = manager.EvaluatePresentation();
        near(result->Volume,volume,"Mask volume"); near(result->Area,2*volume+perimeter+sheets,"Mask exposed area");
    } }
}
static void adjacent(bool connected)
{
    auto mesh = cube();
    const RWCae::MeshEntityId interfaceNodes[] = {1,2,6,5};
    const Graphic3d_Vec3 positions[] = {Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(2,0,0),Graphic3d_Vec3(2,1,0),Graphic3d_Vec3(1,1,0),
        Graphic3d_Vec3(1,0,1),Graphic3d_Vec3(2,0,1),Graphic3d_Vec3(2,1,1),Graphic3d_Vec3(1,1,1)};
    RWCae::MeshEntityId nodes[8];
    for (int n = 0; n < 8; ++n) {
        const int face = n == 0 ? 0 : n == 3 ? 1 : n == 7 ? 2 : n == 4 ? 3 : -1;
        nodes[n] = connected && face >= 0 ? interfaceNodes[face] : mesh->AddNodePosition(positions[n]);
    }
    mesh->AddElement(RWCae::MeshElementHexahedron(nodes));
    MeshCutViewManager manager(mesh); manager.AddPlane("a",Graphic3d_Vec4d(0,1,0,-0.4)); manager.SetVisibility("a",1); manager.Duplicate("a","b");
    const auto result = manager.EvaluatePresentation(); near(result->Volume,0.8,"Adjacent volume");
    near(result->Area,connected ? 6.4 : 7.2,"Trusted shared boundary or disconnected separation");
    manager.SetPlane("a",Graphic3d_Vec4d(1,0,0,-1)); manager.SetPlane("b",Graphic3d_Vec4d(1,0,0,-1));
    manager.SetVisibility("a",2); manager.SetVisibility("b",2);
    near(manager.EvaluatePresentation()->Area,connected ? 1 : 2,"Shared On ownership");
}
int main()
{
    try { basic(); masks(); adjacent(true); adjacent(false); std::cout << "CUT_UNION_PASS\n"; return 0; }
    catch (const std::exception& e) { std::cerr << "CUT_UNION_FAIL: " << e.what() << '\n'; return 1; }
}
