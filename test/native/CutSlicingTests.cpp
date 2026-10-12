#include <ViewCaeGeom/MeshCutSlicing.h>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace ViewCaeGeom;
static void require(bool value, const char* message) { if (!value) { throw std::runtime_error(message); } }
static void closeTo(double value, double reference, double tolerance = 1e-9) {
    if (!std::isfinite(value) || std::abs(value-reference) >= tolerance) {
        std::cerr << "actual=" << value << " reference=" << reference << " tolerance=" << tolerance << '\n';
        throw std::runtime_error("Slice reference mismatch");
    }
}
template<class F> static void rejects(F action) { bool rejected = false; try { action(); } catch (const std::invalid_argument&) { rejected = true; } require(rejected,"Invalid slicing input accepted"); }
int main()
{
    try {
        MeshCutDefinition plane; plane.Id = "slice"; plane.Plane = Graphic3d_Vec4d(2,0,0,-1);
        const auto regular = MeshCutSlicing::Regular(plane,0,0.25,5);
        require(regular.size() == 5,"Regular count");
        for (size_t i = 0; i < regular.size(); ++i) {
            closeTo(regular[i].Definition.Plane.x(),1); closeTo(regular[i].Definition.Plane.w(),-0.5-0.25*i);
            require(regular[i].Frame.IsValid(),"Regular frame"); closeTo(regular[i].Frame.Origin.x(),0.5+0.25*i);
        }
        closeTo(regular[0].IntervalBegin,0); closeTo(regular[0].IntervalEnd,0.125); closeTo(regular[4].IntervalEnd,1);
        const auto range = MeshCutSlicing::Range(plane,-1,1,3); closeTo(range[1].Definition.Plane.w(),-0.5);
        const double pi = std::acos(-1.0);
        const auto rotated = MeshCutSlicing::Rotational(plane,Graphic3d_Vec3d(0,0,2),Graphic3d_Vec3d(0,0,0),0,pi/2,3);
        closeTo(rotated.back().Definition.Plane.x(),0); closeTo(rotated.back().Definition.Plane.y(),1); closeTo(rotated.back().Definition.Plane.w(),-0.5);
        const auto pivot = MeshCutSlicing::Rotational(plane,Graphic3d_Vec3d(0,0,1),Graphic3d_Vec3d(1,0,0),pi/2,pi/2,1);
        closeTo(pivot[0].Frame.Origin.x(),1); closeTo(pivot[0].Frame.Origin.y(),-0.5); closeTo(pivot[0].Definition.Plane.w(),0.5);
        const auto path = MeshCutSlicing::Path("path",{Graphic3d_Vec3d(0,0,0),Graphic3d_Vec3d(1,0,0),Graphic3d_Vec3d(1,1,0),Graphic3d_Vec3d(1,1,2)},Graphic3d_Vec3d(0,0,1));
        for (size_t i = 0; i < path.size(); ++i) {
            require(path[i].Frame.IsValid(),"Transport frame");
            const auto p = path[i].Frame.Origin;
            closeTo(path[i].Definition.Plane.x()*p.x()+path[i].Definition.Plane.y()*p.y()+path[i].Definition.Plane.z()*p.z()+path[i].Definition.Plane.w(),0);
        }
        closeTo(path.back().Coordinate,4); closeTo(path[1].IntervalBegin,0.5); closeTo(path[2].IntervalEnd,3);
        Handle(RWCae::MeshData) tetra = new RWCae::MeshData();
        for (auto p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) { tetra->AddNodePosition(p); }
        RWCae::MeshEntityId ids[] = {0,1,2,3}; tetra->AddElement(RWCae::MeshElementTetrahedron(ids));
        MeshCutViewManager manager(tetra); plane.Plane = Graphic3d_Vec4d(1,0,0,0);
        for (const auto& sample : MeshCutSlicing::Range(plane,0.1,0.9,5)) {
            require(manager.AddPlane(sample.Definition.Id,sample.Definition.Plane),"Slice branch creation");
            const auto result = manager.Evaluate(sample.Definition.Id);
            const auto mesh = result.Portions[1]->GetMeshData();
            require(mesh->NbIndexedElems() == 1 && mesh->NbElems(RWCae::MeshElemType::Triangle) == 1,"Slice section topology");
            std::vector<RWCae::MeshEntityId> triangle; mesh->ElementNodes(0,triangle);
            const Graphic3d_Vec3d a(mesh->Nodes()[triangle[0]]), b(mesh->Nodes()[triangle[1]]), c(mesh->Nodes()[triangle[2]]);
            // Generated MeshData positions use float storage; the exact frame checks
            // above retain their double tolerance.
            closeTo(Graphic3d_Vec3d::Cross(b-a,c-a).Modulus()/2,(1-sample.Coordinate)*(1-sample.Coordinate)/2,2e-7);
        }
        // Transport preserves the initial X through a reversal; it does not invent an arbitrary roll.
        const auto reversal = MeshCutSlicing::Path("reverse",{Graphic3d_Vec3d(0,0,0),Graphic3d_Vec3d(1,0,0),Graphic3d_Vec3d(0,0,0)},Graphic3d_Vec3d(0,0,1));
        closeTo(reversal[0].Frame.X.Dot(reversal[1].Frame.X),1);
        rejects([&] { MeshCutSlicing::Regular(plane,0,0,10); });
        rejects([&] { MeshCutSlicing::Range(plane,1,0,10); });
        rejects([&] { MeshCutSlicing::Range(plane,0,1,1); });
        rejects([&] { MeshCutSlicing::Range(plane,0,1,10001); });
        rejects([&] { MeshCutSlicing::Rotational(plane,Graphic3d_Vec3d(0),Graphic3d_Vec3d(0),0,1,3); });
        rejects([&] { MeshCutSlicing::Path("bad",{Graphic3d_Vec3d(0),Graphic3d_Vec3d(0)},Graphic3d_Vec3d(0,0,1)); });
        rejects([&] { MeshCutSlicing::Path("bad",{Graphic3d_Vec3d(0),Graphic3d_Vec3d(1,0,0)},Graphic3d_Vec3d(1,0,0)); });
        std::cout << "CUT_SLICING_PASS\n"; return 0;
    } catch (const std::exception& error) { std::cerr << "CUT_SLICING_FAIL " << error.what() << '\n'; return 1; }
}
