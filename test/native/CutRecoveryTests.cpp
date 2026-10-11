#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <RWCae/MeshScalarProperty.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ViewCaeGeom;
namespace {
void require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
void near(double actual, double expected, const char* message)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > 2e-6) {
        std::cerr << message << ": " << actual << " != " << expected << '\n';
        throw std::runtime_error(message);
    }
}
Handle(RWCae::MeshData) joined(bool surface = false)
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (int x = 0; x < 3; ++x) {
        for (const auto yz : {Graphic3d_Vec2(0,0),Graphic3d_Vec2(1,0),Graphic3d_Vec2(1,1),Graphic3d_Vec2(0,1)}) {
            mesh->AddNodePosition(Graphic3d_Vec3(x,yz.x(),yz.y()));
        }
    }
    for (RWCae::MeshEntityId e = 0; e < 2; ++e) {
        const RWCae::MeshEntityId nodes[] = {4*e,4*e+4,4*e+5,4*e+1,4*e+3,4*e+7,4*e+6,4*e+2};
        if (surface) { mesh->AddElement(RWCae::MeshElementQuad(nodes)); }
        else { mesh->AddElement(RWCae::MeshElementHexahedron(nodes)); }
    }
    return mesh;
}
Handle(RWCae::MeshNVecProperty) field(const std::vector<std::vector<double>>& values,
    RWCae::MeshNVecPropertyType type = RWCae::MeshNVecPropertyType::ScalarSet)
{
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty("physical",values.size(),
        RWCae::MeshPropertyBase::MappingType::Elemental,type);
    result->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < (int)values.size(); ++c) {
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(result->GetComponent(c));
        component->Init(values[c].size());
        for (int e = 0; e < (int)values[c].size(); ++e) { component->SetValue(RWCae::MeshPropertyEntityInfo(e),values[c][e]); }
    }
    result->UpdateComponents(); return result;
}
void recovery()
{
    auto mesh = joined(); auto values = field({{7,9}});
    MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(0,1,0,-0.5));
    manager.SetVisibility("cut",2); manager.SetCellMode("cut",MeshCutCellMode::Projected);
    auto projected = manager.Evaluate("cut",values);
    require(projected.Fields[1]->IsNodal(),"Default projection changed");
    const auto geometry = projected.Portions[1]; const auto revision = projected.Revision;
    for (int n = 0; n < (int)geometry->GetMeshData()->NbNodes(); ++n) {
        near(projected.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),
            7+geometry->GetMeshData()->Nodes()[n].x(),"Legacy recovered contour");
    }
    MeshCutRecoveryOptions options; options.Regions = {10,20};
    require(manager.SetRecoveryOptions(options),"Regions rejected");
    auto recovered = manager.Evaluate("cut",values);
    require(recovered.Portions[1] == geometry && recovered.Revision > revision,"Recovery rebuilt geometry or retained field revision");
    auto check = [&](const MeshCutResult& cut) {
        require(cut.Fields[1]->HasNodeVecComponents(),"Discontinuous recovery lost elemental-node storage");
        auto output = cut.Portions[1]->GetMeshData();
        for (int e = 0; e < (int)output->NbIndexedElems(); ++e) {
            const auto cell = output->IndexedElements()[e]; RWCae::MeshEntityId source = 0;
            require(cut.Portions[1]->OriginElement(MeshGeomElem(cell.Index,cell.Type,false),source),"Lost recovery provenance");
            std::vector<RWCae::MeshEntityId> nodes; output->ElementNodes(e,nodes);
            for (auto n : nodes) { near(cut.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e,n)),7+2*source,"Material jump smoothed"); }
        }
    };
    check(recovered);
    auto stableRevision = recovered.Revision; require(manager.SetRecoveryOptions(options),"Identical options rejected");
    require(manager.Evaluate("cut",values).Revision == stableRevision,"Identical recovery invalidated fields");
    options.Regions.clear(); options.ThresholdPercent = 0; manager.SetRecoveryOptions(options);
    check(manager.Evaluate("cut",values));
    options.ThresholdPercent = 99; manager.SetRecoveryOptions(options); check(manager.Evaluate("cut",values));
    options.ThresholdPercent = 100; manager.SetRecoveryOptions(options);
    require(manager.Evaluate("cut",values).Fields[1]->IsNodal(),"100 percent threshold did not restore averaging");
    auto valid = manager.RecoveryOptions();
    for (double invalid : {-1.0,101.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
        options.ThresholdPercent = invalid; require(!manager.SetRecoveryOptions(options),"Invalid threshold accepted");
        near(manager.RecoveryOptions().ThresholdPercent,valid.ThresholdPercent,"Invalid settings changed live state");
    }
    options = valid; options.Regions = {1}; require(!manager.SetRecoveryOptions(options),"Wrong region count accepted");
    auto missing = RecoverCutField(mesh,field({{7,std::numeric_limits<double>::quiet_NaN()}}),valid);
    near(missing->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,4)),7,"Finite trace lost");
    require(std::isnan(missing->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(1,4))),"Missing trace fabricated");
    bool rejected = false; try { RecoverCutField(mesh,field({{7,INFINITY}}),valid); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected,"Infinite recovery input accepted");
}
void invariantsAndPhysical()
{
    auto mesh = joined(); auto vector = field({{1,-1},{0,0},{0,0}},RWCae::MeshNVecPropertyType::Vector);
    MeshCutRecoveryOptions options;
    auto after = RecoverCutField(mesh,vector,options);
    near(after->GetRawComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,4)),0,"Magnitude not computed after averaging");
    options.InvariantsBeforeAveraging = true;
    auto before = RecoverCutField(mesh,vector,options);
    near(before->GetRawComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,4)),1,"Before-average magnitude lost");
    near(before->GetRawComponent(1)->Value(RWCae::MeshPropertyEntityInfo(0,4)),0,"Before-average raw component binding changed");
    near(vector->GetRawComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)),1,"Original vector modified");
    MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.25)); manager.SetVisibility("cut",2);
    auto flux = field({{7,9},{0,0},{0,0}},RWCae::MeshNVecPropertyType::Vector);
    auto stress = field({{7,9},{0,0},{0,0},{0,0},{0,0},{0,0}},RWCae::MeshNVecPropertyType::Tensor);
    for (int policy = 0; policy < 4; ++policy) {
        manager.SetCellMode("cut",policy == 0 ? MeshCutCellMode::Constant : MeshCutCellMode::Projected);
        options = MeshCutRecoveryOptions();
        if (policy == 2) { options.Regions = {10,20}; }
        if (policy == 3) { options.InvariantsBeforeAveraging = true; }
        manager.SetRecoveryOptions(options);
        const auto cut = manager.Evaluate("cut",flux);
        const auto heat = MeshCutIntegration::HeatFlux(cut,{0,1,2});
        near(heat.Area,1,"Physical section area"); near(heat.HeatFlow,7,"Display recovery changed heat flow");
        const auto force = MeshCutIntegration::Stress(manager.Evaluate("cut",stress),{0,3,4,3,1,5,4,5,2},Graphic3d_Vec3d(0));
        near(force.Force.x(),7,"Display recovery changed force"); near(force.Moment.y(),3.5,"Display recovery changed moment Y");
        near(force.Moment.z(),-3.5,"Display recovery changed moment Z");
        if (policy == 1) {
            near(cut.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)),7.25,"Expected projected display value");
            auto missing = cut; missing.PhysicalFields[1].Nullify(); bool rejected = false;
            try { MeshCutIntegration::HeatFlux(missing,{0,1,2}); } catch (const std::invalid_argument&) { rejected = true; }
            require(rejected,"Recovered display field used as physical fallback");
        }
    }
}
void linesAndOriginalNodeVec()
{
    auto mesh = joined(true); auto values = field({{7,9}});
    MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(0,1,0,-0.5)); manager.SetVisibility("cut",2);
    manager.SetCellMode("cut",MeshCutCellMode::Projected); MeshCutRecoveryOptions options; options.Regions = {10,20}; manager.SetRecoveryOptions(options);
    auto cut = manager.Evaluate("cut",values); auto slice = Handle(MeshGeometrySlice)::DownCast(cut.Portions[1]);
    require(slice->Lines().size() == 2 && !cut.LineField.IsNull() && cut.LineField->HasNodeVecComponents(),"Recovered line field missing");
    for (int l = 0; l < (int)slice->Lines().size(); ++l) {
        for (auto n : slice->Lines()[l].Nodes) { near(cut.LineField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(l,n)),
            7+2*slice->Lines()[l].SourceElement,"Recovered line lost material trace"); }
    }
    auto nodeVec = RecoverCutField(mesh,values,options);
    manager.SetCellMode("cut",MeshCutCellMode::Constant); auto original = manager.Evaluate("cut",nodeVec);
    require(!original.DisplayRecovered && !original.LineField.IsNull(),"Original element-node line binding failed");
    manager.SetRecoveryOptions(MeshCutRecoveryOptions()); manager.SetCellMode("cut",MeshCutCellMode::Projected);
    auto projected = manager.Evaluate("cut",nodeVec); require(projected.DisplayRecovered,"Original element-node input not recovered");
    near(projected.SourceField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,4)),8,"Original element-node recovery did not average shared nodes");
}
// Synthetic FE weights isolate signed interpolation from the geometry backend.
class SignedSlice : public MeshGeometrySlice {
public:
    explicit SignedSlice(const Handle(MeshBvhData)& bvh) : MeshGeometrySlice(bvh) {}
    void Prepare(MeshGeometry* parent)
    {
        Handle(RWCae::MeshData) output = new RWCae::MeshData();
        for (int n = 0; n < 3; ++n) {
            output->AddNodePosition(Graphic3d_Vec3(0,n,0)); MeshInterpolatedNode origin;
            origin.AddBaseNode(0,1.25); origin.AddBaseNode(1,-0.25); origin.AddBaseNode(2,0);
            myNodesMap.Bind(n,std::make_pair(parent,origin));
        }
        const RWCae::MeshEntityId nodes[] = {0,1,2}; output->AddElement(RWCae::MeshElementTriangle(nodes));
        myElementsMap.Bind(MeshGeomElem(0,RWCae::MeshElemType::Triangle,false),std::make_pair(parent,MeshGeomElem(0,RWCae::MeshElemType::Quad,false)));
        SetMeshData(output);
    }
};
void signedWeights()
{
    auto mesh = joined(true); Handle(MeshBvhData) bvh = new MeshBvhData();
    Handle(MeshGeometry) root = new MeshGeometryBase(bvh,mesh); root->GetMeshData();
    Handle(SignedSlice) slice = new SignedSlice(bvh); root->Append(slice); slice->Prepare(root.get());
    Handle(RWCae::MeshNVecProperty) source = new RWCae::MeshNVecProperty("signed",1,RWCae::MeshPropertyBase::MappingType::Elemental);
    source->Init<RWCae::MeshNodeVecProperty>(); auto component = Handle(RWCae::MeshNodeVecProperty)::DownCast(source->GetComponent(0));
    component->Init(2,0,false,false); component->SetValue(RWCae::MeshPropertyEntityInfo(0,0),2);
    component->SetValue(RWCae::MeshPropertyEntityInfo(0,1),10);
    component->SetValue(RWCae::MeshPropertyEntityInfo(0,2),std::numeric_limits<double>::quiet_NaN()); source->UpdateComponents();
    auto mapped = slice->CalculateProperty(source);
    require(!mapped.IsNull(),"Signed-weight fixture did not map fields");
    for (int n = 0; n < 3; ++n) { near(mapped->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,n)),0,"Signed FE weights clamped or zero weight propagated NaN"); }
}
}
int main()
{
    try { recovery(); invariantsAndPhysical(); linesAndOriginalNodeVec(); signedWeights(); std::cout << "CUT_RECOVERY_PASS\n"; }
    catch (const std::exception& error) { std::cerr << "CUT_RECOVERY_FAIL: " << error.what() << '\n'; return 1; }
    return 0;
}
