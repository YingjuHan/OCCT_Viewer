#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCae/InteractiveCaeCutView.h>
#include <ViewCae/CaeMeshTools.h>
#include <RWCae/MeshScalarProperty.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

// Windows headers define the historical near qualifier as a macro.
#ifdef near
#undef near
#endif

using namespace ViewCaeGeom;

namespace {
void require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}
void near(double actual, double expected, const char* message, double tolerance = 1e-6)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > tolerance) {
        std::cerr << message << ": " << actual << " != " << expected << '\n';
        throw std::runtime_error(message);
    }
}
Handle(RWCae::MeshData) mesh(bool surface = false)
{
    Handle(RWCae::MeshData) result = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),
                         Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) { result->AddNodePosition(p); }
    const RWCae::MeshEntityId nodes[] = {0,1,2,3};
    if (surface) { result->AddElement(RWCae::MeshElementTriangle(nodes)); }
    else { result->AddElement(RWCae::MeshElementTetrahedron(nodes)); }
    return result;
}
Handle(RWCae::MeshNVecProperty) field(bool nodal)
{
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty("value",1,nodal
        ? RWCae::MeshPropertyBase::MappingType::Nodal : RWCae::MeshPropertyBase::MappingType::Elemental);
    result->Init<RWCae::MeshScalarProperty>();
    auto component = Handle(RWCae::MeshScalarProperty)::DownCast(result->GetComponent(0));
    component->Init(nodal ? 4 : 1);
    for (int n = 0; n < (nodal ? 4 : 1); ++n) {
        component->SetValue(RWCae::MeshPropertyEntityInfo(n),nodal ? n : 7);
    }
    result->UpdateComponents();
    return result;
}
void interpolation(const MeshCutResult& result)
{
    for (int part = 0; part < 3; ++part) {
        if (result.Portions[part].IsNull()) { continue; }
        const auto cut = result.Portions[part];
        const auto data = cut->GetMeshData();
        for (RWCae::MeshEntityId n = 0; n < data->NbNodes(); ++n) {
            const auto& p = data->Nodes()[n];
            near(result.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),
                p.x()+2*p.y()+3*p.z(),"Curved-cut linear nodal interpolation");
            MeshInterpolatedNode origin;
            require(cut->OriginNode(n,origin),"Curved-cut node provenance missing");
            double sum = 0;
            for (const auto& base : origin.BaseNodes()) { sum += base.second; }
            near(sum,1,"Curved-cut partition of unity");
        }
        for (RWCae::MeshEntityId e = 0; e < data->NbIndexedElems(); ++e) {
            RWCae::MeshEntityId origin = 1;
            const auto& element = data->IndexedElements()[e];
            require(cut->OriginElement(MeshGeomElem(element.Index,element.Type,false),origin) && origin == 0,
                "Curved-cut element provenance missing");
        }
    }
}
void sphere()
{
    MeshCutViewManager manager(mesh());
    const Graphic3d_Vec3d center(0.2,0.2,0.2);
    const double radius = 0.05, pi = std::acos(-1.0);
    require(manager.AddSphere("sphere",center,radius),"Add sphere failed");
    manager.SetVisibility("sphere",7);
    auto result = manager.Evaluate("sphere",field(true));
    near(result.Volumes[0],4*pi*std::pow(radius,3)/3,"Enclosed sphere volume");
    near(result.Volumes[0]+result.Volumes[2],1.0/6,"Sphere volume conservation");
    near(result.Areas[1],4*pi*radius*radius,"Enclosed sphere area");
    require(result.Portions[1]->GetMeshData()->NbIndexedElems() > 10,"Enclosed sphere disappeared");
    interpolation(result);
    for (const auto& p : result.Portions[1]->GetMeshData()->Nodes()) {
        near((Graphic3d_Vec3d(p)-center).Modulus(),radius,"Sphere node off analytic surface");
    }
    auto constant = manager.Evaluate("sphere",field(false));
    for (int part = 0; part < 3; ++part) {
        for (int e = 0; e < constant.Fields[part]->GetComponent(0)->NbValues(); ++e) {
            near(constant.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e)),7,"Curved CellData");
        }
    }
    manager.SetCellMode("sphere",MeshCutCellMode::Projected);
    auto projected = manager.Evaluate("sphere",field(false));
    require(projected.Fields[1]->IsNodal(),"Curved projected CellData is not nodal");
    near(projected.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)),7,"Curved projected CellData");
    for (unsigned mask = 0; mask < 8; ++mask) {
        manager.SetVisibility("sphere",mask);
        auto visible = manager.Evaluate("sphere");
        for (unsigned part = 0; part < 3; ++part) {
            require(visible.Portions[part].IsNull() == ((mask & (1u << part)) == 0),"Curved visibility combination");
        }
    }
    manager.SetVisibility("sphere",7);
    const auto json = manager.DumpDefinitionsJson();
    require(manager.RestoreDefinitionsJson(json) && manager.DumpDefinitionsJson() == json,"Curved JSON round trip");
    require(!manager.SetSphere("sphere",center,0) && !manager.SetSphere("sphere",center,-1)
        && !manager.SetSphere("sphere",center,INFINITY),"Invalid radius accepted");
    require(manager.DumpDefinitionsJson() == json,"Invalid edit mutated curved cut");
    require(manager.Duplicate("sphere","copy"),"Duplicate curved cut failed");
    manager.Remove("copy");
    require(manager.SetSphere("sphere",Graphic3d_Vec3d(0,0,0),0.3),"Sphere move failed");
    auto octant = manager.Evaluate("sphere",field(true));
    near(octant.Volumes[0],pi*std::pow(0.3,3)/6,"Sphere octant volume");
    near(octant.Areas[1],pi*0.3*0.3/2,"Sphere octant area");
    interpolation(octant);
    require(manager.SetSphere("sphere",Graphic3d_Vec3d(2,2,2),0.1),"Outside sphere move");
    auto outside = manager.Evaluate("sphere");
    near(outside.Volumes[0],0,"Outside sphere inside volume");
    near(outside.Volumes[2],1.0/6,"Outside sphere outside volume");
    require(outside.Portions[1]->GetMeshData()->NbNodes() == 0,"Outside sphere has section");
    manager.SetSphere("sphere",Graphic3d_Vec3d(-0.1,0,0),0.1);
    auto tangent = manager.Evaluate("sphere");
    near(tangent.Volumes[0],0,"Tangent sphere volume");
    near(tangent.Areas[1],0,"Tangent sphere must not create a face");
    manager.SetSphere("sphere",Graphic3d_Vec3d(0),2);
    auto containing = manager.Evaluate("sphere");
    near(containing.Volumes[0],1.0/6,"Containing sphere inside volume");
    near(containing.Volumes[2],0,"Containing sphere outside volume");
    near(containing.Areas[1],0,"Containing sphere fabricated a section");
}
void cylinder()
{
    MeshCutViewManager manager(mesh());
    const double pi = std::acos(-1.0), radius = 0.05;
    require(manager.AddCylinder("cylinder",Graphic3d_Vec3d(0.2,0.2,0),Graphic3d_Vec3d(0,0,2),radius),
        "Add cylinder failed");
    manager.SetVisibility("cylinder",7);
    auto result = manager.Evaluate("cylinder",field(true));
    near(result.Volumes[0],pi*radius*radius*0.6,"Enclosed cylinder volume");
    near(result.Volumes[0]+result.Volumes[2],1.0/6,"Cylinder volume conservation");
    near(result.Areas[1],2*pi*radius*0.6,"Enclosed cylinder area",1e-5);
    interpolation(result);
    for (const auto& p : result.Portions[1]->GetMeshData()->Nodes()) {
        near(std::hypot(p.x()-0.2,p.y()-0.2),radius,"Cylinder includes cap nodes");
    }
    const auto before = manager.DumpDefinitionsJson();
    require(!manager.SetCylinder("cylinder",Graphic3d_Vec3d(0),Graphic3d_Vec3d(0),radius),"Zero cylinder axis accepted");
    require(manager.DumpDefinitionsJson() == before,"Invalid cylinder edit mutated state");
    require(manager.SetCylinder("cylinder",Graphic3d_Vec3d(0),Graphic3d_Vec3d(0,0,1),0.25),"Cylinder move failed");
    auto quadrant = manager.Evaluate("cylinder",field(true));
    near(quadrant.Volumes[0],pi*0.25*0.25/4-2*std::pow(0.25,3)/3,"Cylinder quadrant volume");
    near(quadrant.Areas[1],pi*0.25/2-2*0.25*0.25,"Cylinder quadrant area",1e-5);
    interpolation(quadrant);
    const auto oblique = Graphic3d_Vec3d(0.34506636574889995,0.7399972093835984,0.5773502691896258);
    require(manager.SetCylinder("cylinder",Graphic3d_Vec3d(0.2),oblique,0.08),"Oblique cylinder failed");
    const auto json = manager.DumpDefinitionsJson();
    for (int n = 0; n < 10; ++n) {
        require(manager.RestoreDefinitionsJson(json) && manager.DumpDefinitionsJson() == json,"Oblique cylinder JSON drift");
    }
    interpolation(manager.Evaluate("cylinder",field(true)));
}
void surface()
{
    MeshCutViewManager manager(mesh(true));
    const double pi = std::acos(-1.0), radius = 0.05;
    require(manager.AddSphere("circle",Graphic3d_Vec3d(0.2,0.2,0),radius),"Surface sphere add");
    manager.SetVisibility("circle",7);
    auto result = manager.Evaluate("circle",field(true));
    near(result.Areas[0],pi*radius*radius,"Surface circle enclosed area");
    near(result.Areas[0]+result.Areas[2],0.5,"Surface area conservation");
    auto slice = Handle(MeshGeometrySlice)::DownCast(result.Portions[1]);
    require(slice->Lines().size() > 10 && !result.LineField.IsNull(),"Enclosed surface loop missing");
    double length = 0;
    for (const auto& line : slice->Lines()) {
        const auto& nodes = slice->GetMeshData()->Nodes();
        length += (nodes[line.Nodes[0]]-nodes[line.Nodes[1]]).Modulus();
    }
    near(length,2*pi*radius,"Enclosed circle line length",pi*radius*0.02);
    interpolation(result);
    auto constant = manager.Evaluate("circle",field(false));
    require(!constant.LineField.IsNull() && constant.LineField->IsElemental(),"Surface CellData missing");
    for (int n = 0; n < constant.LineField->GetComponent(0)->NbValues(); ++n) {
        near(constant.LineField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),7,"Surface line CellData");
    }
}
void tolerances()
{
    MeshCutViewManager manager(mesh(true));
    manager.AddSphere("circle",Graphic3d_Vec3d(0.2,0.2,0),0.05);
    const auto coarse = manager.Evaluate("circle");
    const auto count = Handle(MeshGeometrySlice)::DownCast(coarse.Portions[1])->Lines().size();
    require(count > 0,"Missing coarse curved-line reference");
    const auto epoch = manager.Statistics().SourceEpoch;
    MeshCutGeometryOptions options; options.AbsoluteTolerance = 1e-5; options.RelativeTolerance = 0;
    require(manager.SetGeometryOptions(options),"Curved geometry options rejected");
    const auto fine = manager.Evaluate("circle");
    require(Handle(MeshGeometrySlice)::DownCast(fine.Portions[1])->Lines().size() > count,"Curved line ignores tighter tolerance");
    require(manager.Statistics().SourceEpoch > epoch,"Curved tolerance change retained source epoch");
    options.RelativeTolerance = -1;
    const auto saved = manager.Statistics().SourceEpoch;
    require(!manager.SetGeometryOptions(options) && manager.Statistics().SourceEpoch == saved,"Invalid curved options changed state");
}
void display()
{
    Handle(ViewCae::InteractiveCaeCutView) view = new ViewCae::InteractiveCaeCutView(mesh());
    view->Cuts().AddSphere("sphere",Graphic3d_Vec3d(0.2),0.05);
    view->Cuts().SetVisibility("sphere",7);
    view->SetField(field(true)); view->Synchronize();
    require(view->Presentations().size() == 3,"Curved portion presentations missing");
    for (const auto& presentation : view->Presentations()) {
        presentation->GetParts();
        auto triangles = ViewCae::CaeMeshTools::CreateShadedMesh(presentation->GetMeshGeometry(),
            presentation->GetGeomScalarProps(),false,ViewCae::MeshShadedType::FACES,1.0f);
        require(!triangles.IsNull() && triangles->VertexNumber() > 0 && triangles->HasVertexTexels(),
            "Curved boundary contour primitives missing");
    }
}
void deformationAndDiagnostics()
{
    MeshCutViewManager manager(mesh());
    Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("displacement",3,
        RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
    displacement->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 3; ++c) {
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c));
        component->Init(4);
        for (int n = 0; n < 4; ++n) { component->SetValue(RWCae::MeshPropertyEntityInfo(n),c == 0 ? 1 : 0); }
    }
    displacement->UpdateComponents();
    require(manager.SetDeformation(displacement,1.0),"Curved deformation rejected");
    const double shift = manager.EffectiveDeformationScale();
    const Graphic3d_Vec3d center(0.2+shift,0.2,0.2);
    require(manager.AddSphere("sphere",center,0.05),"Deformed sphere failed");
    auto result = manager.Evaluate("sphere",field(true));
    near(result.Areas[1],4*std::acos(-1.0)*0.05*0.05,"Curved cut ignored deformation");
    for (RWCae::MeshEntityId n = 0; n < result.Portions[1]->GetMeshData()->NbNodes(); ++n) {
        const auto& p = result.Portions[1]->GetMeshData()->Nodes()[n];
        near(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),
            p.x()-shift+2*p.y()+3*p.z(),"Deformed curved provenance");
    }
    manager.InvalidateGeometry();
    near(manager.Evaluate("sphere").Areas[1],result.Areas[1],"Curved geometry invalidation stale");
    Handle(RWCae::MeshData) unsupported = new RWCae::MeshData();
    const auto original = mesh();
    for (const auto& p : original->Nodes()) { unsupported->AddNodePosition(p); }
    const RWCae::MeshEntityId nodes[] = {0,1,2,3};
#ifdef OCCTV_USE_VTK
    RWCae::MeshData::GenericCell unavailable;
    unavailable.Type = 255; unavailable.Nodes.assign(nodes,nodes+4);
    unsupported->AddGenericCell(unavailable);
#else
    unsupported->AddElement(RWCae::MeshElementQuad(nodes));
#endif
    MeshCutViewManager other(unsupported);
    require(!other.AddSphere("bad",Graphic3d_Vec3d(0),1) && !other.AddCylinder("bad",Graphic3d_Vec3d(0),
        Graphic3d_Vec3d(0,0,1),1) && other.Definitions().empty(),"Unsupported cell silently linearized");
    auto degenerate = mesh();
    degenerate->SetNodePosition(3,Graphic3d_Vec3(0.2f,0.2f,0));
    MeshCutViewManager invalid(degenerate);
    invalid.AddSphere("sphere",Graphic3d_Vec3d(0),0.5);
    bool rejected = false;
    try { invalid.Evaluate("sphere"); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected,"Degenerate tetrahedron not diagnosed");
    degenerate->SetNodePosition(3,Graphic3d_Vec3(0,0,1)); invalid.InvalidateGeometry();
    require(invalid.Evaluate("sphere").Areas[1] > 0,"Failed intersection could not recover after invalidation");
}
void adjacentCells()
{
    auto data = mesh(); data->AddNodePosition(Graphic3d_Vec3(0,0,-1));
    const RWCae::MeshEntityId nodes[] = {0,2,1,4};
    data->AddElement(RWCae::MeshElementTetrahedron(nodes));
    MeshCutViewManager manager(data);
    const double radius = 0.05, area = 4*std::acos(-1.0)*radius*radius;
    manager.AddSphere("sphere",Graphic3d_Vec3d(0.2,0.2,0),radius);
    manager.SetVisibility("sphere",7);
    const auto result = manager.Evaluate("sphere");
    near(result.Areas[0],area,"Adjacent-cell inside has internal caps");
    near(result.Areas[1],area,"Adjacent-cell section area");
    near(result.Areas[2],2+std::sqrt(3.0)+area,"Adjacent-cell outside has internal walls");
    near(result.Volumes[0]+result.Volumes[2],1.0/3,"Adjacent-cell volume conservation");
}
}

int main()
{
    try {
        sphere(); cylinder(); surface(); tolerances(); display(); deformationAndDiagnostics(); adjacentCells();
        std::cout << "Curved cut numerical tests passed\n";
        return 0;
    } catch (const Standard_Failure& error) {
        std::cerr << error.GetMessageString() << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; }
    return 1;
}
