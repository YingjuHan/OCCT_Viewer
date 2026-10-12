#include <ViewCaeGeom/MeshCutTrace.h>
#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#ifdef near
#undef near
#endif
using namespace ViewCaeGeom;
namespace {
void require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
void near(double actual, double expected, const char* message)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > 2e-5) {
        std::cerr << message << ": " << actual << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
Handle(RWCae::MeshData) mesh(bool surface, bool connected)
{
    Handle(RWCae::MeshData) result = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1),
        Graphic3d_Vec3(-1,0,0),Graphic3d_Vec3(1,0,0)}) { result->AddNodePosition(p); }
    RWCae::MeshEntityId left[] = {0,1,2,3}, right[] = {0,2,1,4};
    if (!connected) { for (int n = 0; n < 3; ++n) { right[n] = result->AddNodePosition(result->Nodes()[right[n]]); } }
    if (surface) {
        const RWCae::MeshEntityId a[] = {0,1,3}, b[] = {right[0],right[2],4};
        result->AddElement(RWCae::MeshElementTriangle(a)); result->AddElement(RWCae::MeshElementTriangle(b));
    } else { result->AddElement(RWCae::MeshElementTetrahedron(left)); result->AddElement(RWCae::MeshElementTetrahedron(right)); }
    return result;
}
Handle(RWCae::MeshNVecProperty) field(const Handle(RWCae::MeshData)& source)
{
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty("jump",1,RWCae::MeshPropertyBase::MappingType::Elemental);
    result->Init<RWCae::MeshNodeVecProperty>();
    auto values = Handle(RWCae::MeshNodeVecProperty)::DownCast(result->GetComponent(0));
    values->Init(source->NbIndexedElems(),0,false,false);
    for (RWCae::MeshEntityId e = 0; e < source->NbIndexedElems(); ++e) {
        std::vector<RWCae::MeshEntityId> nodes; source->ElementNodes(e,nodes);
        for (auto n : nodes) { const auto p = source->Nodes()[n]; values->SetValue(RWCae::MeshPropertyEntityInfo(e,n),100*e+p.y()+2*p.z(),false); }
    }
    result->UpdateComponents(); return result;
}
void verify(bool surface, bool connected, bool deformed)
{
    const auto source = mesh(surface,connected); MeshCutViewManager manager(source); double shift = 0;
    if (deformed) {
        Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("displacement",3,
            RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
        displacement->Init<RWCae::MeshScalarProperty>();
        for (int c = 0; c < 3; ++c) {
            auto values = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c)); values->Init(source->NbNodes());
            for (RWCae::MeshEntityId n = 0; n < source->NbNodes(); ++n) { values->SetValue(RWCae::MeshPropertyEntityInfo(n),c == 0 ? 1 : 0); }
        }
        displacement->UpdateComponents(); require(manager.SetDeformation(displacement,0.1),"Deformation rejected"); shift = manager.EffectiveDeformationScale();
    }
    require(manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-shift)),"Plane rejected"); const auto values = field(source);
    for (unsigned mask = 0; mask < 8; ++mask) {
        manager.SetVisibility("cut",mask); const auto cut = manager.Evaluate("cut",values);
        if (!(mask&2)) { require(cut.Portions[1].IsNull(),"Hidden section populated"); continue; }
        auto traced = Handle(MeshGeometryTraceSection)::DownCast(cut.Portions[1]); require(!traced.IsNull(),"Missing trace section");
        const auto& records = surface ? traced->LineTraces() : traced->ElementTraces();
        require(records.size() == (connected ? 1u : 2u),"Shared primitive ownership or disconnected topology lost");
        const auto generated = traced->GetMeshData(); const auto mapped = surface ? cut.LineField : cut.PhysicalFields[1];
        require(!mapped.IsNull() && mapped->HasNodeVecComponents(),"Original element-node mapping lost");
        for (size_t e = 0; e < records.size(); ++e) {
            require(records[e].size() == (connected ? 2u : 1u),"Wrong source count");
            for (const auto& trace : records[e]) {
                require(!trace.Ambiguous,"Conforming opposite support is ambiguous");
                for (const auto& sample : trace.Weights) {
                    double sum = 0, value = 0; Graphic3d_Vec3d point(0);
                    for (const auto& weight : sample.second.BaseNodes()) {
                        if (weight.second == 0) { continue; }
                        sum += weight.second; auto p = Graphic3d_Vec3d(source->Nodes()[weight.first]); p.x() += shift;
                        point += p*weight.second;
                        value += weight.second*values->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(trace.Element,weight.first));
                    }
                    const auto p = generated->Nodes()[sample.first]; near(sum,1,"FE partition unity"); near((point-Graphic3d_Vec3d(p)).Modulus(),0,"FE geometry mapping");
                    near(value,100*trace.Element+p.y()+2*p.z(),"Double-sided jump lost");
                    if (trace.Element == records[e].front().Element) { near(mapped->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e,sample.first)),value,"Default lowest source mapping"); }
                }
            }
        }
        if (!surface) {
            const auto indexed = generated->IndexedElements()[0]; const MeshGeomElem first(indexed.Index,indexed.Type,false);
            const auto subset = traced->CalculateProperty(values,&first);
            require(subset->NbValues() == 1,"Element subset count changed");
            for (const auto& sample : records[0][0].Weights) {
                near(subset->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,sample.first)),
                    mapped->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,sample.first)),"Element subset binding changed");
            }
            const auto integral = MeshCutIntegration::HeatFlux(cut,{0,0,0}); near(integral.Area,connected ? 0.5 : 1,"Shared integral area");
            near(integral.HeatFlow,connected ? 0.5 : 51,"Shared integral source");
        }
    }
    manager.SetVisibility("cut",2); manager.SetPlane("cut",Graphic3d_Vec4d(1,0,0,-0.25-shift));
    const auto moved = manager.Evaluate("cut",values); auto traced = Handle(MeshGeometryTraceSection)::DownCast(moved.Portions[1]);
    for (const auto& records : surface ? traced->LineTraces() : traced->ElementTraces()) { require(records.size() == 1,"Moved plane retained interface traces"); }
    manager.Clear(); // Existing numerical result must retain source/root lifetime.
    require(!(surface ? moved.LineField : moved.PhysicalFields[1]).IsNull(),"Clear lost mapped field");
}
#ifdef OCCTV_USE_VTK
void lines()
{
    Handle(RWCae::MeshData) source = new RWCae::MeshData();
    for (int n = -1; n <= 1; ++n) { source->AddNodePosition(Graphic3d_Vec3(n,0,0)); }
    for (RWCae::MeshEntityId e = 0; e < 2; ++e) { RWCae::MeshData::GenericCell cell; cell.Type = 3; cell.Nodes = {e,e+1}; source->AddGenericCell(cell); }
    MeshCutViewManager manager(source); manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,0));
    auto cut = manager.Evaluate("cut",field(source)); auto traced = Handle(MeshGeometryTraceSection)::DownCast(cut.Portions[1]);
    require(traced->ElementTraces().size() == 1 && traced->ElementTraces()[0].size() == 2,"Shared line endpoint lacks two traces");
}
#endif
}
int main()
{
    try {
        for (bool surface : {false,true}) { for (bool connected : {false,true}) { for (bool deformed : {false,true}) { verify(surface,connected,deformed); } } }
#ifdef OCCTV_USE_VTK
        lines();
#endif
        std::cout << "CUT_TRACE_BINDING_PASS\n"; return 0;
    } catch (const std::exception& e) { std::cerr << "CUT_TRACE_BINDING_FAIL: " << e.what() << '\n'; return 1; }
}
