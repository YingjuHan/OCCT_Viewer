#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCae/InteractiveCaeCutView.h>
#include <RWCae/MeshScalarProperty.h>
#include <vtkGenericCell.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#ifdef near
#undef near
#endif
using namespace ViewCaeGeom;
namespace {
void require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
void near(double value, double expected, double tolerance, const char* message)
{
    if (!std::isfinite(value) || std::abs(value-expected) > tolerance) {
        std::cerr << message << ": " << value << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
Handle(RWCae::MeshData) fixture(int type, bool rational = false)
{
    const std::map<int,int> sizes = {{3,2},{5,3},{8,4},{9,4},{10,4},{11,8},{12,8},{13,6},{14,5},
        {21,3},{22,6},{23,8},{24,10},{25,20},{26,15},{27,13},{28,9},{29,27},{30,6},
        {31,12},{32,18},{33,24},{34,7},{35,4},{37,19},{68,3},{69,6},{70,9},{71,10},
        {72,27},{73,18},{75,3},{76,6},{77,9},{78,10},{79,27},{80,18}};
    const int count = sizes.at(type); vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(count); grid->SetPoints(points); std::vector<vtkIdType> ids(count);
    for (int n = 0; n < count; ++n) { ids[n] = n; }
    grid->InsertNextCell(type,count,ids.data());
    const int order[] = {2,2,2};
    if (type >= 68) { vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3); degrees->InsertNextTypedTuple(order); grid->GetCellData()->SetHigherOrderDegrees(degrees); }
    vtkNew<vtkGenericCell> cell; grid->GetCell(0,cell); const auto q = cell->GetParametricCoords(); require(q,"Missing fixture chart");
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData(); RWCae::MeshData::GenericCell topology; topology.Type = type;
    if (type >= 68) { topology.Degrees = {2,2,2}; }
    for (int n = 0; n < count; ++n) {
        topology.Nodes.push_back(mesh->AddNodePosition(Graphic3d_Vec3((float)q[3*n],(float)q[3*n+1],(float)q[3*n+2])));
        if (rational) { topology.RationalWeights.push_back(0.7+0.4*q[3*n]); }
    }
    mesh->AddGenericCell(topology); return mesh;
}
Handle(RWCae::MeshNVecProperty) field(const Handle(RWCae::MeshData)& mesh, const char* name,
    const std::function<double(const Graphic3d_Vec3&)>& value, bool nodal = true)
{
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty(name,1,nodal
        ? RWCae::MeshPropertyBase::MappingType::Nodal : RWCae::MeshPropertyBase::MappingType::Elemental);
    result->Init<RWCae::MeshScalarProperty>(); auto component = Handle(RWCae::MeshScalarProperty)::DownCast(result->GetComponent(0));
    component->Init(nodal ? mesh->NbNodes() : mesh->NbIndexedElems());
    for (int n = 0; n < component->NbValues(); ++n) { component->SetValue(RWCae::MeshPropertyEntityInfo(n),value(nodal ? mesh->Nodes()[n] : Graphic3d_Vec3(0))); }
    result->UpdateComponents(); return result;
}
MeshCutIsoOptions options(double tolerance = 0.004)
{
    MeshCutIsoOptions o; o.AbsoluteTolerance = tolerance; o.RelativeTolerance = 0; return o;
}
void write(const Handle(RWCae::MeshData)& mesh, const Handle(RWCae::MeshNVecProperty)& cut)
{
    const auto& cell = mesh->GenericCells()[0]; vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points;
    std::vector<vtkIdType> ids; for (const auto& p : mesh->Nodes()) { ids.push_back(points->InsertNextPoint(p.x(),p.y(),p.z())); }
    grid->SetPoints(points); grid->InsertNextCell(cell.Type,ids.size(),ids.data());
    if (!cell.Degrees.empty()) { vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3); degrees->InsertNextTypedTuple(cell.Degrees.data()); grid->GetCellData()->SetHigherOrderDegrees(degrees); }
    for (const char* name : {"cut","temperature"}) {
        vtkNew<vtkDoubleArray> array; array->SetName(name);
        for (int n = 0; n < (int)mesh->NbNodes(); ++n) { const auto p = mesh->Nodes()[n]; array->InsertNextValue(std::string(name) == "cut" ? cut->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)) : p.x()+2*p.y()+3*p.z()); }
        grid->GetPointData()->AddArray(array);
    }
    vtkNew<vtkDoubleArray> pressure; pressure->SetName("pressure"); pressure->InsertNextValue(7); grid->GetCellData()->AddArray(pressure);
    std::filesystem::create_directories(CUT_ISO_FIXTURE_DIR); const auto path = std::string(CUT_ISO_FIXTURE_DIR)+"/nonmonotone.vtu";
    vtkNew<vtkXMLUnstructuredGridWriter> writer; writer->SetFileName(path.c_str()); writer->SetInputData(grid); writer->SetDataModeToAscii(); require(writer->Write(),"Cannot write iso fixture");
}
void checkFields(const MeshCutResult& cut, double threshold)
{
    require(cut.SourceApproximate && cut.IsoFieldResidual <= cut.IsoFieldTolerance,"Iso residual contract missing");
    for (int part = 0; part < 3; ++part) {
        if (cut.Portions[part].IsNull()) { continue; } const auto mesh = cut.Portions[part]->GetMeshData();
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
            MeshInterpolatedNode origin; require(cut.Portions[part]->OriginNode(n,origin),"Iso FE provenance missing");
            double sum = 0; for (const auto& base : origin.BaseNodes()) { sum += base.second; } near(sum,1,1e-9,"Iso partition of unity");
            if (part == 1 && !cut.Fields[part].IsNull()) { near(cut.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),threshold,cut.IsoFieldTolerance*0.01,"Iso original FE root"); }
        }
        for (const auto& element : mesh->IndexedElements()) {
            RWCae::MeshEntityId source; require(cut.Portions[part]->OriginElement(MeshGeomElem(element.Index,element.Type,false),source) && source == 0,"Iso source element lost");
        }
    }
}
void familyCharts()
{
    for (int type : {3,5,8,9,10,11,12,13,14,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,37,68,69,70,71,72,73,75,76,77,78,79,80}) {
        std::cout << "ISO_CHART " << type << std::endl;
        const auto mesh = fixture(type); auto x = field(mesh,"x",[](const auto& p) { return p.x(); });
        MeshCutViewManager manager(mesh); manager.SetIsoOptions(options(0.05));
        auto geometry = manager.GeometryOptions(); geometry.AbsoluteTolerance = 0.05; geometry.RelativeTolerance = 0; manager.SetGeometryOptions(geometry);
        require(manager.AddIsosurface("iso",x,0,0.31),"Chart iso rejected");
        const auto cut = manager.Evaluate("iso",x); checkFields(cut,0.31);
        require(cut.Portions[1]->GetMeshData()->NbNodes() > 0,"Chart iso empty");
        for (const auto& p : cut.Portions[1]->GetMeshData()->Nodes()) { near(p.x(),0.31,2e-6,"Affine iso reference"); }
    }
}
void nonmonotone()
{
    for (int type : {21,22,24,68,69,71,72}) {
        const auto mesh = fixture(type); auto f = field(mesh,"cut",[](const auto& p) { return p.x()*(1-p.x()); });
        MeshCutViewManager manager(mesh); manager.SetIsoOptions(options()); require(manager.AddIsosurface("iso",f,0,0.16),"Nonmonotone iso rejected");
        manager.SetVisibility("iso",7); const auto cut = manager.Evaluate("iso",f); checkFields(cut,0.16);
        for (const auto& p : cut.Portions[1]->GetMeshData()->Nodes()) { near(std::min(std::abs(p.x()-0.2),std::abs(p.x()-0.8)),0,2e-6,"Two-root independent reference"); }
        if (type == 24 || type == 71) {
            near(cut.Volumes[2],(std::pow(0.8,3)-std::pow(0.2,3))/6,2e-6,"Nonmonotone tetra middle volume");
            near(cut.Volumes[0]+cut.Volumes[2],1.0/6,2e-6,"Nonmonotone volume partition"); near(cut.Areas[1],0.34,2e-6,"Two tetra sections");
        }
        if (type == 72) { near(cut.Volumes[2],0.6,3e-6,"Nonmonotone hex middle volume"); near(cut.Areas[1],2,2e-6,"Two hex sections"); }
        if (type == 69) {
            write(mesh,f);
            for (unsigned mask = 0; mask < 8; ++mask) { manager.SetVisibility("iso",mask); const auto visible = manager.Evaluate("iso",f); for (int p = 0; p < 3; ++p) { require(visible.Portions[p].IsNull() == ((mask&(1u<<p)) == 0),"Iso masks"); } }
        }
    }
}
void nativeSecondOrders()
{
    for (int type : {22,23,24,25,26,27}) {
        const auto generic = fixture(type); Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
        for (const auto& p : generic->Nodes()) { mesh->AddNodePosition(p); }
        const auto& ids = generic->GenericCells()[0].Nodes;
        switch (type) {
            case 22: mesh->AddElement(RWCae::MeshElementTriangle2nd(ids.data())); break;
            case 23: mesh->AddElement(RWCae::MeshElementQuad2nd(ids.data())); break;
            case 24: mesh->AddElement(RWCae::MeshElementTetrahedron2nd(ids.data())); break;
            case 25: mesh->AddElement(RWCae::MeshElementHexahedron2nd(ids.data())); break;
            case 26: mesh->AddElement(RWCae::MeshElementPrism2nd(ids.data())); break;
            case 27: mesh->AddElement(RWCae::MeshElementPyramid2nd(ids.data())); break;
        }
        auto x = field(mesh,"x",[](const auto& p) { return p.x(); }); MeshCutViewManager manager(mesh);
        auto geometry = manager.GeometryOptions(); geometry.AbsoluteTolerance = 0.05; geometry.RelativeTolerance = 0; manager.SetGeometryOptions(geometry); manager.SetIsoOptions(options(0.05));
        require(manager.AddIsosurface("iso",x,0,0.31),"Native second-order iso rejected"); checkFields(manager.Evaluate("iso",x),0.31);
    }
}
void rational()
{
    const double disc = std::sqrt(0.418*0.418-4*0.45*0.056);
    const double q[] = {(0.418-disc)/0.9,(0.418+disc)/0.9};
    const auto x = [](double u) { return (0.9*u+0.2*u*u)/(0.7+0.4*u); };
    for (int type : {75,77,79}) {
        const auto mesh = fixture(type,true); auto f = field(mesh,"cut",[](const auto& p) { return p.x()*(1-p.x()); });
        MeshCutViewManager manager(mesh); auto geometry = manager.GeometryOptions(); geometry.AbsoluteTolerance = 0.004; geometry.RelativeTolerance = 0; manager.SetGeometryOptions(geometry); manager.SetIsoOptions(options());
        require(manager.AddIsosurface("iso",f,0,0.08),"Rational iso rejected"); const auto cut = manager.Evaluate("iso",f); checkFields(cut,0.08);
        require(cut.Portions[1]->GetMeshData()->NbNodes() >= 2,"Rational double crossing missing");
        for (const auto& p : cut.Portions[1]->GetMeshData()->Nodes()) { near(std::min(std::abs(p.x()-x(q[0])),std::abs(p.x()-x(q[1]))),0,2e-6,"Independent rational iso root"); }
        if (type == 79) { near(cut.Areas[1],2,2e-5,"Rational tensor two sections"); }
    }
}
void enclosedAndConvergence()
{
    double previous = INFINITY; int previousDepth = -1; bool refined = false;
    for (double tolerance : {0.03,0.008,0.002}) {
        const auto mesh = fixture(70); auto f = field(mesh,"radius",[](const auto& p) { return (p.x()-0.5)*(p.x()-0.5)+(p.y()-0.5)*(p.y()-0.5); });
        MeshCutViewManager manager(mesh); manager.SetIsoOptions(options(tolerance)); manager.AddIsosurface("circle",f,0,0.04); manager.SetVisibility("circle",3);
        const auto cut = manager.Evaluate("circle",f); checkFields(cut,0.04); near(cut.Areas[0],std::acos(-1)*0.04,0.003,"Enclosed circle area");
        const double error = std::abs(cut.Areas[0]-std::acos(-1)*0.04);
        if (cut.IsoSubdivisionDepth > previousDepth) { require(error < previous,"Iso refinement did not improve independent reference"); refined |= previousDepth >= 0; }
        else { require(error <= previous+1e-12,"Iso tolerance plateau worsened reference"); }
        previous = error; previousDepth = cut.IsoSubdivisionDepth;
    }
    require(refined,"Convergence did not exercise finer resolution");
    const auto mesh = fixture(72); auto f = field(mesh,"radius",[](const auto& p) { return (p.x()-0.5)*(p.x()-0.5)+(p.y()-0.5)*(p.y()-0.5)+(p.z()-0.5)*(p.z()-0.5); });
    MeshCutViewManager manager(mesh); manager.SetIsoOptions(options(0.015)); manager.AddIsosurface("sphere",f,0,0.04); manager.SetVisibility("sphere",3);
    const auto cut = manager.Evaluate("sphere",f); checkFields(cut,0.04);
    near(cut.Volumes[0],4*std::acos(-1)*0.008/3,0.003,"Enclosed sphere volume"); near(cut.Areas[1],4*std::acos(-1)*0.04,0.03,"Enclosed sphere section area");
}
void lifecycle()
{
    const auto mesh = fixture(69); auto f = field(mesh,"cut",[](const auto& p) { return p.x()*(1-p.x()); }); f->SetTime(0.5,2);
    Handle(ViewCae::InteractiveCaeCutView) view = new ViewCae::InteractiveCaeCutView(mesh); auto& manager = view->Cuts(); manager.SetIsoOptions(options()); manager.AddIsosurface("iso",f,0,0.16);
    view->SetField(f); view->Synchronize(); const auto old = view->Presentations(); const auto revisions = view->SynchronizedRevisions(); const auto count = view->SynchronizationCount();
    const auto snapshot = manager.DumpDefinitionsJson(); auto invalid = manager.IsoOptions(); invalid.MinimumDepth = 0;
    require(!manager.SetIsoOptions(invalid) && manager.DumpDefinitionsJson() == snapshot,"Invalid iso options changed state");
    auto tiny = manager.IsoOptions(); tiny.MaxPrimitives = 2; manager.SetIsoOptions(tiny); bool rejected = false;
    try { view->Synchronize(); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected && view->Presentations() == old && view->SynchronizedRevisions() == revisions && view->SynchronizationCount() == count,"Failed preparation cleared valid presentation");
    require(manager.RestoreDefinitionsJson(snapshot) && manager.DumpDefinitionsJson() == snapshot,"Iso settings persistence"); view->Synchronize();
    const auto oldSection = manager.Evaluate("iso").Portions[1];
    auto values = Handle(RWCae::MeshScalarProperty)::DownCast(f->GetComponent(0)); for (int n = 0; n < values->NbValues(); ++n) { values->SetValue(RWCae::MeshPropertyEntityInfo(n),mesh->Nodes()[n].x()); } f->UpdateComponents(); manager.InvalidateFields();
    auto moved = manager.Evaluate("iso",f); require(oldSection != moved.Portions[1],"Iso field mutation kept stale geometry");
    for (const auto& p : moved.Portions[1]->GetMeshData()->Nodes()) { near(p.x(),0.16,2e-6,"Mutated FE iso"); }
    auto cells = field(mesh,"pressure",[](const auto&) { return 7.0; },false);
    auto constant = manager.Evaluate("iso",cells); require(!constant.Fields[1].IsNull() || !Handle(MeshGeometrySlice)::DownCast(constant.Portions[1])->CalculateLineProperty(cells).IsNull(),"Iso CellData missing");
    manager.SetCellMode("iso",MeshCutCellMode::Projected); auto projected = manager.Evaluate("iso",cells); require(projected.DisplayRecovered,"Iso CellData projection missing");
    Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("disp",3,RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector); displacement->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 3; ++c) { auto v = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c)); v->Init(mesh->NbNodes()); for (int n = 0; n < v->NbValues(); ++n) { v->SetValue(RWCae::MeshPropertyEntityInfo(n),c == 0 ? 1 : 0); } } displacement->UpdateComponents();
    manager.SetDeformation(displacement,1); const double shift = manager.EffectiveDeformationScale();
    for (const auto& p : manager.Evaluate("iso",f).Portions[1]->GetMeshData()->Nodes()) { near(p.x(),0.16+shift,2e-6,"High-order iso deformation"); }
    manager.SetDeformation(nullptr,0);
}
void curvedDeformation()
{
    const auto mesh = fixture(69);
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
        auto p = mesh->Nodes()[n]; p.z() = 0.4f*p.x()*(1-p.x()); mesh->SetNodePosition(n,p);
    }
    auto f = field(mesh,"cut",[](const auto& p) { return p.x()*(1-p.x()); });
    MeshCutViewManager manager(mesh); manager.SetIsoOptions(options()); manager.AddIsosurface("iso",f,0,0.16);
    Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("disp",3,RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector); displacement->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 3; ++c) {
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c)); component->Init(mesh->NbNodes());
        for (int n = 0; n < component->NbValues(); ++n) { const double u = mesh->Nodes()[n].x(); component->SetValue(RWCae::MeshPropertyEntityInfo(n),c == 0 ? 0.2*u*u : c == 2 ? 0.1*u*u : 0); }
    }
    displacement->UpdateComponents(); require(manager.SetDeformation(displacement,0.25),"Nonuniform deformation rejected"); const double scale = manager.EffectiveDeformationScale();
    auto check = [&](double factor) {
        const auto cut = manager.Evaluate("iso",f); checkFields(cut,0.16);
        for (const auto& p : cut.Portions[1]->GetMeshData()->Nodes()) {
            const double u = p.x() < 0.5 ? 0.2 : 0.8;
            near(p.x(),u+factor*0.2*u*u,3e-6,"Nonuniform deformed iso map X");
            near(p.z(),0.4*u*(1-u)+factor*0.1*u*u,3e-6,"Curved deformed iso map Z");
        }
    };
    check(scale); manager.SetDeformation(nullptr,0); check(0);
}
}
int main()
{
    try { familyCharts(); nativeSecondOrders(); nonmonotone(); rational(); enclosedAndConvergence(); lifecycle(); curvedDeformation(); std::cout << "CUT_ISO_PASS\n"; return 0; }
    catch (const std::exception& e) { std::cerr << "CUT_ISO_FAIL: " << e.what() << '\n'; return 1; }
}
