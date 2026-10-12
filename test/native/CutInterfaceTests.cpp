#include <ViewCaeGeom/MeshVtkCutSource.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <ViewCaeGeom/MeshCutTrace.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <vtkCellData.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>

#ifdef near
#undef near
#endif
using namespace ViewCaeGeom;
namespace {
void require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}
void near(double actual, double expected, const char* message, double tolerance = 2e-5)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > tolerance) {
        std::cerr << message << ": " << actual << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
template<class F> void rejects(F operation, const char* message)
{
    try { operation(); } catch (const std::exception&) { return; }
    throw std::runtime_error(message);
}
Handle(RWCae::MeshData) grid(int type, bool connected = true, double bend = 0)
{
    const int count = type == 25 ? 20 : type == 72 ? 27 : 8;
    vtkNew<vtkGenericCell> cell; cell->SetCellType(type == 42 ? 12 : type);
    cell->GetPoints()->SetNumberOfPoints(count); cell->GetPointIds()->SetNumberOfIds(count);
    for (int n = 0; n < count; ++n) { cell->GetPointIds()->SetId(n,n); }
    if (type == 72) {
        vtkNew<vtkUnstructuredGrid> source; vtkNew<vtkPoints> points; points->SetNumberOfPoints(count);
        source->SetPoints(points); std::vector<vtkIdType> nodes(count);
        for (int n = 0; n < count; ++n) { nodes[n] = n; }
        source->InsertNextCell(type,count,nodes.data());
        vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3); const int order[] = {2,2,2};
        degrees->InsertNextTypedTuple(order); source->GetCellData()->SetHigherOrderDegrees(degrees); source->GetCell(0,cell);
    }
    const auto parameters = cell->GetParametricCoords();
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    std::map<std::tuple<float,float,float>,RWCae::MeshEntityId> shared;
    for (int e = 0; e < 2; ++e) {
        RWCae::MeshData::GenericCell topology; topology.Type = type;
        if (type == 72) { topology.Degrees = {2,2,2}; }
        for (int n = 0; n < count; ++n) {
            const double y = parameters[3*n+1], z = parameters[3*n+2];
            const Graphic3d_Vec3 p((float)(parameters[3*n]+e+bend*(y*(1-y)+z*(1-z))),(float)y,(float)z);
            const auto key = std::make_tuple(p.x(),p.y(),p.z());
            if (!connected || !shared.count(key)) { shared[key] = mesh->AddNodePosition(p); }
            topology.Nodes.push_back(shared[key]);
        }
        if (type == 42) {
            for (const auto& face : std::vector<std::vector<int>>{{0,3,2,1},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}}) {
                std::vector<RWCae::MeshEntityId> nodes;
                for (int n : face) { nodes.push_back(topology.Nodes[n]); } topology.Faces.push_back(std::move(nodes));
            }
        }
        mesh->AddGenericCell(topology);
    }
    return mesh;
}
Handle(RWCae::MeshNVecProperty) traceField(const Handle(RWCae::MeshData)& mesh, bool quadratic)
{
    Handle(RWCae::MeshNVecProperty) field = new RWCae::MeshNVecProperty("trace",1,
        RWCae::MeshPropertyBase::MappingType::Elemental);
    field->Init<RWCae::MeshNodeVecProperty>();
    auto component = Handle(RWCae::MeshNodeVecProperty)::DownCast(field->GetComponent(0));
    component->Init(mesh->NbIndexedElems(),0,false,false);
    for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
        std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(e,nodes);
        for (auto n : nodes) {
            const auto p = mesh->Nodes()[n];
            const double y = quadratic ? p.y()*p.y() : p.y(), z = quadratic ? p.z()*p.z() : p.z();
            component->SetValue(RWCae::MeshPropertyEntityInfo(e,n),e == 0 ? y+2*z : 100+3*y-z,false);
        }
    }
    field->UpdateComponents(); return field;
}
void checkTraces(const Handle(RWCae::MeshData)& mesh, MeshCutViewManager& manager,
    const Graphic3d_Vec3d& position, bool quadratic)
{
    const auto root = manager.SourceGeometry();
    MeshVtkCutSource source(17,manager.GeometryOptions());
    rejects([&] { source.InterfaceTraces(0,0,position); },"Unprepared interface was accepted");
    source.Prepare(mesh,root.get()); source.Prepare(mesh,root.get());
    require(source.Work.SourcePreparations == 1,"Trace lookup rebuilt the prepared source");
    const auto field = traceField(mesh,quadratic);
    size_t references = 0;
    for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
        const auto& cell = source.GetCell(e);
        for (size_t f = 0; f < cell.Faces.size(); ++f) {
            const auto& face = cell.Faces[f];
            if (!face.Internal) {
                rejects([&] { source.InterfaceTraces(e,f,position); },"Exterior face was accepted as an interface");
                continue;
            }
            ++references;
            const auto traces = source.InterfaceTraces(e,f,position);
            require(traces[0].Element == 0 && traces[1].Element == 1,"Source ordering depends on lookup side");
            require(face.Owner == 0 && face.NeighborElement == 1-e,"Lowest-ID ownership or reciprocal neighbor changed");
            require(traces[e].FaceIndex == f,"Source face reference was lost");
            require(traces[0].Interior.x() < position.x() && traces[1].Interior.x() > position.x(),
                "Deformed interior points do not identify the two sides");
            for (const auto& trace : traces) {
                double sum = 0, value = 0; Graphic3d_Vec3d mapped(0);
                for (const auto& weight : trace.Weights.BaseNodes()) {
                    std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(trace.Element,nodes);
                    require(std::find(nodes.begin(),nodes.end(),weight.first) != nodes.end(),"Trace used a node outside its source element");
                    auto p = mesh->Nodes()[weight.first]; root->ModifyNode(weight.first,p);
                    mapped += Graphic3d_Vec3d(p)*weight.second; sum += weight.second;
                    value += field->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(trace.Element,weight.first))*weight.second;
                }
                near(sum,1,"Trace weights do not form a partition of unity");
                for (int a = 0; a < 3; ++a) { near(mapped[a],position[a],"Trace does not map to the deformed interface point"); }
                const double y = quadratic ? position.y()*position.y() : position.y();
                const double z = quadratic ? position.z()*position.z() : position.z();
                near(value,trace.Element == 0 ? y+2*z : 100+3*y-z,"Original element-node jump was lost");
            }
            rejects([&] { source.InterfaceTraces(e,f,position+Graphic3d_Vec3d(0,2,0)); },"Point outside face was accepted");
            rejects([&] { source.InterfaceTraces(e,f,Graphic3d_Vec3d(NAN,0,0)); },"NaN interface point was accepted");
        }
    }
    require(references == 2,"Prepared interface must have exactly two reciprocal references");
}
void adjacent(int type, double bend = 0, bool deformed = false, bool numerical = false)
{
    const auto mesh = grid(type,true,bend); MeshCutViewManager manager(mesh);
    if (numerical) {
        auto options = manager.GeometryOptions(); options.Mode = MeshCutGeometryMode::Numerical;
        require(manager.SetGeometryOptions(options),"Numerical geometry options rejected");
    }
    double shift = 0;
    if (deformed) {
        Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("displacement",3,
            RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
        displacement->Init<RWCae::MeshScalarProperty>();
        for (int c = 0; c < 3; ++c) {
            auto component = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c)); component->Init(mesh->NbNodes());
            for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
                component->SetValue(RWCae::MeshPropertyEntityInfo(n),c == 0 ? 1 : 0);
            }
        }
        displacement->UpdateComponents(); require(manager.SetDeformation(displacement,0.1),"Deformation rejected");
        shift = manager.EffectiveDeformationScale();
    }
    checkTraces(mesh,manager,Graphic3d_Vec3d(1+shift+bend*0.375,0.25,0.75),type == 25 || type == 72);
    if (bend == 0) {
        const auto field = traceField(mesh,type == 25 || type == 72);
        require(manager.AddPlane("interface",Graphic3d_Vec4d(1,0,0,-1-shift)),"Plane rejected");
        for (unsigned mask = 0; mask < 8; ++mask) {
            manager.SetVisibility("interface",mask); const auto result = manager.EvaluatePhysical("interface",field);
            for (unsigned part = 0; part < 3; ++part) {
                require(result.Portions[part].IsNull() == ((mask & (1u << part)) == 0),"Interface metadata changed visibility masks");
            }
            if (mask & 2) {
                near(result.Areas[1],1,"Shared section is no longer counted once");
                auto traced = Handle(MeshGeometryTraceSection)::DownCast(result.Portions[1]);
                require(!traced.IsNull() && !traced->ElementTraces().empty(),"General section has no bound traces");
                for (const auto& primitive : traced->ElementTraces()) {
                    require(primitive.size() == 2 && primitive[0].Element == 0 && primitive[1].Element == 1,
                        "General shared section lost a source");
                    for (const auto& trace : primitive) {
                        for (const auto& sample : trace.Weights) {
                            double value = 0;
                            for (const auto& weight : sample.second.BaseNodes()) {
                                if (weight.second != 0) { value += weight.second*field->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(trace.Element,weight.first)); }
                            }
                            const auto p = traced->GetMeshData()->Nodes()[sample.first];
                            const double y = type == 25 || type == 72 ? p.y()*p.y() : p.y();
                            const double z = type == 25 || type == 72 ? p.z()*p.z() : p.z();
                            near(value,trace.Element == 0 ? y+2*z : 100+3*y-z,"Bound original FE trace changed");
                        }
                    }
                }
                const auto section = result.Portions[1]->GetMeshData();
                for (const auto& indexed : section->IndexedElements()) {
                    RWCae::MeshEntityId origin = 99;
                    require(result.Portions[1]->OriginElement(MeshGeomElem(indexed.Index,indexed.Type,false),origin) && origin == 0,
                        "Adjacency preparation changed default physical ownership");
                }
            }
        }
        Handle(RWCae::MeshNVecProperty) flux = new RWCae::MeshNVecProperty("flux",1,
            RWCae::MeshPropertyBase::MappingType::Elemental);
        flux->Init<RWCae::MeshScalarProperty>();
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(flux->GetComponent(0)); component->Init(2);
        component->SetValue(RWCae::MeshPropertyEntityInfo(0),7); component->SetValue(RWCae::MeshPropertyEntityInfo(1),9);
        flux->UpdateComponents();
        near(MeshCutIntegration::HeatFlux(manager.EvaluatePhysical("interface",flux),{{0,0,0}}).HeatFlow,7,
            "Dual-source preparation changed original lowest-ID physical integration");
    }
    std::cout << "CUT_INTERFACE_SOURCE_PASS " << type << " bend " << bend << " deformed " << deformed
              << " numerical " << numerical << '\n';
}
void untrusted()
{
    for (int kind = 0; kind < 3; ++kind) {
        auto mesh = grid(12,kind != 0);
        if (kind == 1) { mesh->AddGenericCell(mesh->GenericCells()[0]); }
        if (kind == 2) {
            Handle(RWCae::MeshData) duplicate = new RWCae::MeshData();
            for (const auto& p : mesh->Nodes()) { duplicate->AddNodePosition(p); }
            duplicate->AddGenericCell(mesh->GenericCells()[0]); duplicate->AddGenericCell(mesh->GenericCells()[0]); mesh = duplicate;
        }
        MeshCutViewManager manager(mesh); const auto root = manager.SourceGeometry();
        MeshVtkCutSource source(1,manager.GeometryOptions()); source.Prepare(mesh,root.get());
        for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
            for (size_t f = 0; f < source.GetCell(e).Faces.size(); ++f) {
                require(!source.GetCell(e).Faces[f].Internal,"Disconnected/ambiguous/same-support faces were paired");
                rejects([&] { source.InterfaceTraces(e,f,Graphic3d_Vec3d(1,0.5,0.5)); },"Untrusted interface was accepted");
            }
        }
        if (kind) { require(source.Work.UnmergedFaceGroups > 0,"Ambiguous topology was not reported"); }
    }
}
void nativeTetra()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(1,1,0),Graphic3d_Vec3(1,0,1),
        Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(2,0,0)}) { mesh->AddNodePosition(p); }
    const RWCae::MeshEntityId left[] = {0,1,2,3}, right[] = {0,2,1,4};
    mesh->AddElement(RWCae::MeshElementTetrahedron(left)); mesh->AddElement(RWCae::MeshElementTetrahedron(right));
    MeshCutViewManager manager(mesh); checkTraces(mesh,manager,Graphic3d_Vec3d(1,0.25,0.25),false);
}
void lowerDimensions()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(1,1,0)}) {
        mesh->AddNodePosition(p);
    }
    const RWCae::MeshEntityId a[] = {0,1,2}, b[] = {1,3,2}, pointNode[] = {0};
    mesh->AddElement(RWCae::MeshElementTriangle(a)); mesh->AddElement(RWCae::MeshElementTriangle(b));
    RWCae::MeshData::GenericCell line; line.Type = 3; line.Nodes = {0,1}; mesh->AddGenericCell(line);
    mesh->AddElement(RWCae::MeshElement0d(pointNode));
    MeshCutViewManager manager(mesh); const auto root = manager.SourceGeometry();
    MeshVtkCutSource source(1,manager.GeometryOptions()); source.Prepare(mesh,root.get());
    for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
        require(source.GetCell(e).Faces.empty(),"A surface/line/point was reported as a volume interface");
        rejects([&] { source.InterfaceTraces(e,0,Graphic3d_Vec3d(0.5,0,0)); },"Lower-dimensional cell acquired volume face traces");
    }
}
}
int main()
{
    try {
        for (int type : {12,11,25,72,42}) { adjacent(type); }
        adjacent(12,0,true); adjacent(25,0.08); adjacent(72,0.08,true); adjacent(25,0.08,false,true);
        nativeTetra(); lowerDimensions(); untrusted(); std::cout << "CUT_INTERFACE_NATIVE_PASS\n"; return 0;
    } catch (const Standard_Failure& error) { std::cerr << "CUT_INTERFACE_FAIL " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "CUT_INTERFACE_FAIL " << error.what() << '\n'; }
    return 1;
}
