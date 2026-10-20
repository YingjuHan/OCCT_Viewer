#include <ViewCaeGeom/MeshCutTrace.h>
#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <cmath>
#include <iostream>
#include <limits>
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
template<class F> void rejects(F operation, const char* message)
{
    try { operation(); } catch (const std::invalid_argument&) { return; } throw std::runtime_error(message);
}
void selections(bool surface, bool iso, bool reversed, bool deformed)
{
    const auto source = mesh(surface,true); const auto values = field(source); MeshCutViewManager manager(source);
    double shift = 0;
    if (deformed) {
        Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("displacement",3,
            RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
        displacement->Init<RWCae::MeshScalarProperty>();
        for (int c = 0; c < 3; ++c) {
            auto scalar = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c)); scalar->Init(source->NbNodes());
            for (RWCae::MeshEntityId n = 0; n < source->NbNodes(); ++n) { scalar->SetValue(RWCae::MeshPropertyEntityInfo(n),c == 0 ? 1 : 0); }
        }
        displacement->UpdateComponents(); require(manager.SetDeformation(displacement,0.2),"Selection deformation rejected"); shift = manager.EffectiveDeformationScale();
    }
    if (iso) {
        Handle(RWCae::MeshNVecProperty) cutting = new RWCae::MeshNVecProperty("cutting",1,RWCae::MeshPropertyBase::MappingType::Nodal);
        cutting->Init<RWCae::MeshScalarProperty>(); auto scalar = Handle(RWCae::MeshScalarProperty)::DownCast(cutting->GetComponent(0)); scalar->Init(source->NbNodes());
        for (RWCae::MeshEntityId n = 0; n < source->NbNodes(); ++n) { scalar->SetValue(RWCae::MeshPropertyEntityInfo(n),(reversed ? -1 : 1)*source->Nodes()[n].x()); }
        cutting->UpdateComponents(); require(manager.AddIsosurface("cut",cutting,0,0),"Iso interface rejected");
    } else { manager.AddPlane("cut",Graphic3d_Vec4d(reversed ? -1 : 1,0,0,(reversed ? 1 : -1)*shift)); }
    MeshCutTraceOptions display, physical;
    for (auto policy : {MeshCutTracePolicy::Lowest,MeshCutTracePolicy::Negative,MeshCutTracePolicy::Positive,MeshCutTracePolicy::Region}) {
        physical.Policy = policy; physical.Regions = {std::numeric_limits<int64_t>::min(),std::numeric_limits<int64_t>::max()}; physical.Region = physical.Regions[1];
        require(manager.SetTraceOptions("cut",true,physical),"Physical policy rejected");
        display.Policy = policy == MeshCutTracePolicy::Positive ? MeshCutTracePolicy::Negative : MeshCutTracePolicy::Positive;
        require(manager.SetTraceOptions("cut",false,display),"Display policy rejected");
        const int physicalOwner = policy == MeshCutTracePolicy::Lowest ? 0 : policy == MeshCutTracePolicy::Region ? 1
            : policy == MeshCutTracePolicy::Positive ? (reversed ? 0 : 1) : (reversed ? 1 : 0);
        const int displayOwner = display.Policy == MeshCutTracePolicy::Positive ? (reversed ? 0 : 1) : (reversed ? 1 : 0);
        for (unsigned mask = 0; mask < 8; ++mask) {
            manager.SetVisibility("cut",mask); const auto cut = manager.Evaluate("cut",values); if (!(mask&2)) { continue; }
            auto traced = Handle(MeshGeometryTraceSection)::DownCast(cut.Portions[1]);
            const auto shown = surface ? cut.LineField : cut.Fields[1], raw = surface ? cut.PhysicalLineField : cut.PhysicalFields[1];
            const auto& traces = surface ? traced->LineTraces() : traced->ElementTraces();
            for (size_t e = 0; e < traces.size(); ++e) {
                for (const auto& sample : traces[e][0].Weights) {
                    const auto p = traced->GetMeshData()->Nodes()[sample.first];
                    near(shown->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e,sample.first)),100*displayOwner+p.y()+2*p.z(),"Display policy mapped wrong source");
                    near(raw->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e,sample.first)),100*physicalOwner+p.y()+2*p.z(),"Physical policy mapped wrong source");
                }
            }
            const auto original = manager.EvaluatePhysical("cut",values);
            require(!original.DisplayRecovered,"Physical evaluation acquired recovery");
            const auto after = surface ? traced->CalculateLineProperty(values) : traced->CalculateProperty(values);
            const auto n = traces[0][0].Weights.begin()->first;
            near(after->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,n)),shown->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,n)),"Physical evaluation mutated display selection");
            if (!surface && !iso) {
                const auto integral = MeshCutIntegration::HeatFlux(cut,{0,0,0});
                near(integral.HeatFlow,(reversed ? -1 : 1)*(50*physicalOwner+0.5),"Physical integral policy or orientation changed");
            }
        }
    }
    const auto before = manager.Definitions()[0].PhysicalTrace;
    const auto saved = manager.DumpDefinitionsJson();
    require(manager.RestoreDefinitionsJson(saved) && manager.DumpDefinitionsJson() == saved,"Trace JSON v7 exact round trip");
    for (const auto& replacements : std::vector<std::pair<std::string,std::string>>{
        {"\"policy\":3","\"policy\":8"}, {"\"region\":9223372036854775807","\"region\":1.5"},
        {"\"region\":9223372036854775807","\"region\":9223372036854775808"},
        {"\"region\":9223372036854775807","\"region\":0"},
        {"[-9223372036854775808,9223372036854775807]","[-9223372036854775808]"}}) {
        auto invalid = saved; const auto at = invalid.find(replacements.first); require(at != std::string::npos,"Trace JSON mutation fixture missing");
        invalid.replace(at,replacements.first.size(),replacements.second);
        require(!manager.RestoreDefinitionsJson(invalid) && manager.DumpDefinitionsJson() == saved,"Invalid trace JSON was not atomic");
    }
    auto legacy = saved; const auto versionEnd = legacy.find(',',legacy.find("\"version\":"));
    legacy.replace(legacy.find("\"version\":"),versionEnd-legacy.find("\"version\":"),"\"version\":6");
    require(manager.RestoreDefinitionsJson(legacy),"JSON v6 migration failed");
    require(manager.Definitions()[0].DisplayTrace.Policy == MeshCutTracePolicy::Lowest
        && manager.Definitions()[0].PhysicalTrace.Policy == MeshCutTracePolicy::Lowest,"JSON v6 trace defaults changed");
    require(manager.RestoreDefinitionsJson(saved),"Trace JSON restore after migration failed");
    physical.Regions = {1}; require(!manager.SetTraceOptions("cut",true,physical),"Wrong region count accepted");
    require(manager.Definitions()[0].PhysicalTrace.Regions == before.Regions,"Invalid policy changed definition");
    physical.Policy = (MeshCutTracePolicy)9; require(!manager.SetTraceOptions("cut",true,physical),"Invalid policy enum accepted");
    physical = before; physical.Regions = {5,5}; physical.Region = 5;
    require(!manager.SetTraceOptions("cut",true,physical),"Ambiguous region source accepted");
    physical.Region = 6;
    require(!manager.SetTraceOptions("cut",true,physical),"Unavailable region source accepted");
    require(manager.Definitions()[0].PhysicalTrace.Regions == before.Regions,"Rejected region changed binding");
}
void ambiguous()
{
    auto source = mesh(false,true); const RWCae::MeshEntityId same[] = {0,1,2,3}; source->AddElement(RWCae::MeshElementTetrahedron(same));
    MeshCutViewManager manager(source); manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,0)); manager.Evaluate("cut",field(source));
    MeshCutTraceOptions options; options.Policy = MeshCutTracePolicy::Negative;
    require(!manager.SetTraceOptions("cut",false,options),"Non-manifold trace selection accepted");
}
void lines()
{
    Handle(RWCae::MeshData) source = new RWCae::MeshData();
    for (int n = -1; n <= 1; ++n) { source->AddNodePosition(Graphic3d_Vec3(n,0,0)); }
    for (RWCae::MeshEntityId e = 0; e < 2; ++e) { RWCae::MeshData::GenericCell cell; cell.Type = 3; cell.Nodes = {e,e+1}; source->AddGenericCell(cell); }
    MeshCutViewManager manager(source); manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,0));
    auto cut = manager.Evaluate("cut",field(source)); auto traced = Handle(MeshGeometryTraceSection)::DownCast(cut.Portions[1]);
    require(traced->ElementTraces().size() == 1 && traced->ElementTraces()[0].size() == 2,"Shared line endpoint lacks two traces");
    MeshCutTraceOptions options; options.Policy = MeshCutTracePolicy::Positive;
    manager.SetTraceOptions("cut",true,options);
    auto positive = manager.EvaluatePhysical("cut",field(source));
    const auto node = traced->ElementTraces()[0][0].Weights.begin()->first;
    near(positive.PhysicalFields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,node)),100,"Line positive source");
    manager.AddSphere("sphere",Graphic3d_Vec3d(-1,0,0),1); manager.SetTraceOptions("sphere",true,options);
    auto sphere = manager.EvaluatePhysical("sphere",field(source));
    near(sphere.PhysicalFields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,0)),100,"Sphere signed side");
    for (unsigned mask = 0; mask < 8; ++mask) {
        manager.SetVisibility("cut",mask);
        for (auto policy : {MeshCutTracePolicy::Lowest,MeshCutTracePolicy::Negative,MeshCutTracePolicy::Positive,MeshCutTracePolicy::Region}) {
            options.Policy = policy; options.Region = -1; options.Regions = {1,-1};
            require(manager.SetTraceOptions("cut",true,options),"Line trace selection rejected");
            auto result = manager.EvaluatePhysical("cut",field(source));
            if (!(mask&2)) { require(result.Portions[1].IsNull(),"Line mask ignored"); continue; }
            auto section = Handle(MeshGeometryTraceSection)::DownCast(result.Portions[1]);
            const auto nodeId = section->ElementTraces()[0][0].Weights.begin()->first;
            near(result.PhysicalFields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,nodeId)),
                policy == MeshCutTracePolicy::Lowest || policy == MeshCutTracePolicy::Negative ? 0 : 100,"Line selected jump");
        }
    }
}
}
int main()
{
    try {
        for (bool surface : {false,true}) { for (bool connected : {false,true}) { for (bool deformed : {false,true}) { verify(surface,connected,deformed); } } }
        for (bool surface : {false,true}) { for (bool iso : {false,true}) { for (bool reversed : {false,true}) {
            for (bool deformed : {false,true}) { selections(surface,iso,reversed,deformed); }
        } } }
        ambiguous();
        lines();
        std::cout << "CUT_TRACE_BINDING_PASS\n"; return 0;
    } catch (const std::exception& e) { std::cerr << "CUT_TRACE_BINDING_FAIL: " << e.what() << '\n'; return 1; }
}
