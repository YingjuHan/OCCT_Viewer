#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <ViewCae/InteractiveCaeCutView.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ViewCaeGeom;
namespace {
void require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
void closeValue(double actual, double expected, const char* message)
{
    if (!std::isfinite(actual) || (expected == 0 ? std::abs(actual) > 2e-12 : std::abs((actual-expected)/expected) > 2e-12)) {
        std::cerr << message << ": " << actual << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
template<class Function> void rejects(Function function, const char* message)
{
    bool rejected = false; try { function(); } catch (const std::exception&) { rejected = true; }
    require(rejected,message);
}
double value(const std::vector<double>& values, MeshCutTensorMeasure measure,
    MeshCutTensorLayout layout = MeshCutTensorLayout::XXYYZZXYXZYZ, bool engineering = false)
{
    MeshCutTensorOptions options; options.Measure = measure; options.Layout = layout; options.EngineeringShear = engineering;
    return CutTensorValue(values,options);
}
void references()
{
    using M = MeshCutTensorMeasure; using L = MeshCutTensorLayout;
    closeValue(value({10,0,0,0,0,0},M::Mises),10,"Uniaxial Mises");
    closeValue(value({10,0,0,0,0,0},M::Pressure),-10.0/3,"Pressure sign");
    for (auto measure : {M::PrincipalMax,M::PrincipalMid,M::PrincipalMin}) {
        closeValue(value({-7,-7,-7,0,0,0},measure),-7,"Hydrostatic repeated principal root");
    }
    closeValue(value({-7,-7,-7,0,0,0},M::Mises),0,"Hydrostatic Mises");
    closeValue(value({0,0,0,4,0,0},M::Mises),4*std::sqrt(3.0),"Pure shear Mises");
    closeValue(value({0,0,0,4,0,0},M::Tresca),8,"Pure shear Tresca");
    closeValue(value({0,0,0,4,0,0},M::PrincipalMax),4,"Shear maximum principal");
    closeValue(value({0,0,0,4,0,0},M::PrincipalMin),-4,"Shear minimum principal");
    closeValue(value({0,0,0,8,0,0},M::Tresca,L::XXYYZZXYXZYZ,true),8,"Engineering shear factor");
    // Rotate known eigenvalues (11,3,-2) about two independent axes. Expected
    // measures follow those eigenvalues, without invoking another eigensolver.
    for (int angle = 1; angle <= 30; ++angle) {
        const double c = std::cos(angle*0.071), s = std::sin(angle*0.071);
        const double d = std::cos(angle*0.053), t = std::sin(angle*0.053);
        const double q[3][3] = {{c,-s*d,s*t},{s,c*d,-c*t},{0,t,d}};
        std::vector<double> matrix(9); const double eigen[] = {11,3,-2};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) { for (int k = 0; k < 3; ++k) { matrix[3*i+j] += q[i][k]*eigen[k]*q[j][k]; } }
        }
        const std::vector<double> six = {matrix[0],matrix[4],matrix[8],matrix[1],matrix[2],matrix[5]};
        const std::vector<double> alternate = {matrix[0],matrix[4],matrix[8],matrix[1],matrix[5],matrix[2]};
        for (const auto& pair : {std::make_pair(six,L::XXYYZZXYXZYZ),std::make_pair(alternate,L::XXYYZZXYYZXZ),
            std::make_pair(matrix,L::RowMajor),std::make_pair(matrix,L::ColumnMajor)}) {
            closeValue(value(pair.first,M::PrincipalMax,pair.second),11,"Rotated maximum principal");
            closeValue(value(pair.first,M::PrincipalMid,pair.second),3,"Rotated middle principal");
            closeValue(value(pair.first,M::PrincipalMin,pair.second),-2,"Rotated minimum principal");
            closeValue(value(pair.first,M::Mises,pair.second),std::sqrt(129.0),"Rotated Mises");
            closeValue(value(pair.first,M::Pressure,pair.second),-4,"Rotated pressure");
            closeValue(value(pair.first,M::Tresca,pair.second),13,"Rotated Tresca");
        }
    }
    closeValue(value({1e308,1e308,1e308,0,0,0},M::Pressure),-1e308,"Large hydrostatic trace overflow");
    closeValue(value({1e308,1e308,1e308,0,0,0},M::PrincipalMid),1e308,"Large principal overflow");
    closeValue(value({1e308,1e308,1e308,0,0,0},M::Mises),0,"Large hydrostatic deviator");
    closeValue(value({0,0,0,1e-300,0,0},M::Mises),std::sqrt(3.0)*1e-300,"Tiny measure underflow");
    rejects([&] { value({0,0,0,1e308,0,0},M::Tresca); },"Unrepresentable Tresca accepted");
    rejects([&] { value({1,2,3,4,5,6,7,8,9},M::Mises,L::RowMajor); },"Nonsymmetric matrix silently symmetrized");
    rejects([&] { value({1,2,3},M::Mises); },"Wrong component layout accepted");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    require(std::isnan(value({1,2,3,4,nan,6},M::Mises)),"Partial tensor fabricated a measure");
    rejects([&] { value({1,2,3,INFINITY,nan,6},M::Mises); },"NaN concealed infinite tensor input");
}
Handle(RWCae::MeshData) mesh(bool surface = false)
{
    Handle(RWCae::MeshData) result = new RWCae::MeshData();
    for (int x = 0; x < 3; ++x) {
        for (const auto yz : {Graphic3d_Vec2(0,0),Graphic3d_Vec2(1,0),Graphic3d_Vec2(1,1),Graphic3d_Vec2(0,1)}) {
            result->AddNodePosition(Graphic3d_Vec3(x,yz.x(),yz.y()));
        }
    }
    for (RWCae::MeshEntityId e = 0; e < 2; ++e) {
        const RWCae::MeshEntityId nodes[] = {4*e,4*e+4,4*e+5,4*e+1,4*e+3,4*e+7,4*e+6,4*e+2};
        if (surface) { result->AddElement(RWCae::MeshElementQuad(nodes)); }
        else { result->AddElement(RWCae::MeshElementHexahedron(nodes)); }
    }
    return result;
}
Handle(RWCae::MeshNVecProperty) field()
{
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty("stress",6,
        RWCae::MeshPropertyBase::MappingType::Elemental,RWCae::MeshNVecPropertyType::Tensor);
    result->Init<RWCae::MeshScalarProperty>();
    const double values[2][6] = {{7,0,0,2,0,0},{9,0,0,-2,0,0}};
    for (int c = 0; c < 6; ++c) {
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(result->GetComponent(c)); component->Init(2);
        component->SetDisplayUnits("MPa"); component->SetValue(RWCae::MeshPropertyEntityInfo(0),values[0][c]);
        component->SetValue(RWCae::MeshPropertyEntityInfo(1),values[1][c]);
    }
    result->SetTime(2.5,3); result->SetDescription("Original stress"); result->UpdateComponents(); return result;
}
void recoveryAndPhysical()
{
    auto source = mesh(); auto stress = field(); MeshCutViewManager cuts(source);
    cuts.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.25)); cuts.SetCellMode("cut",MeshCutCellMode::Projected);
    const double left = std::sqrt(61.0), right = std::sqrt(93.0);
    for (int before = 0; before < 2; ++before) {
        MeshCutRecoveryOptions options; options.Tensor.Measure = MeshCutTensorMeasure::Mises; options.InvariantsBeforeAveraging = before;
        cuts.SetRecoveryOptions(options); cuts.SetVisibility("cut",2);
        const auto result = cuts.Evaluate("cut",stress);
        require(result.TensorDerived && result.DisplayRecovered && result.SourceField->NbComponents() == 1,"Tensor display binding missing");
        const double interface = before ? (left+right)/2 : 8;
        // Measures are evaluated on recovered source tuples, then their scalar
        // values are interpolated onto the cut. They are not recomputed there.
        auto generated = result.Portions[1]->GetMeshData();
        for (int e = 0; e < (int)generated->NbIndexedElems(); ++e) {
            std::vector<RWCae::MeshEntityId> nodes; generated->ElementNodes(e,nodes);
            for (auto n : nodes) {
                closeValue(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(
                    result.Fields[1]->IsNodal() ? (int)n : e,result.Fields[1]->HasNodeVecComponents() ? (int)n : -1)),
                    0.75*left+0.25*interface,"Recovery invariant ordering/reference");
            }
        }
        const auto force = MeshCutIntegration::Stress(result,{0,3,4,3,1,5,4,5,2},Graphic3d_Vec3d(0));
        closeValue(force.Force.x(),7,"Tensor display changed physical stress X");
        closeValue(force.Force.y(),2,"Engineering/measure display changed original shear");
        options.Tensor.EngineeringShear = true; cuts.SetRecoveryOptions(options);
        closeValue(MeshCutIntegration::Stress(cuts.Evaluate("cut",stress),{0,3,4,3,1,5,4,5,2},Graphic3d_Vec3d(0)).Force.y(),2,"Engineering display altered physical integration");
        options.Tensor.EngineeringShear = false; options.Regions = {10,20}; cuts.SetRecoveryOptions(options);
        auto derived = cuts.Evaluate("cut",stress); require(derived.SourceField->HasNodeVecComponents(),"Tensor region trace merged");
        closeValue(derived.SourceField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0,4)),left,"Tensor material jump smoothed");
        closeValue(derived.SourceField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(1,4)),right,"Tensor second trace lost");
        for (unsigned mask = 0; mask < 8; ++mask) { cuts.SetVisibility("cut",mask); cuts.Evaluate("cut",stress); }
        cuts.SetVisibility("cut",2);
    }
    closeValue(stress->GetComponent(3)->Value(RWCae::MeshPropertyEntityInfo(1)),-2,"Original tensor mutated");
    MeshCutRecoveryOptions incompatible; incompatible.Tensor.Measure = MeshCutTensorMeasure::Mises;
    incompatible.Tensor.Layout = MeshCutTensorLayout::RowMajor; cuts.SetRecoveryOptions(incompatible);
    rejects([&] { cuts.Evaluate("cut",stress); },"Mismatched display layout accepted");
    closeValue(MeshCutIntegration::Stress(cuts.EvaluatePhysical("cut",stress),{0,3,4,3,1,5,4,5,2},Graphic3d_Vec3d(0)).Force.x(),7,"Invalid display layout blocked original physical integration");
    MeshCutRecoveryOptions options; options.Tensor.Measure = MeshCutTensorMeasure::PrincipalMax;
    options.Tensor.Layout = MeshCutTensorLayout::XXYYZZXYYZXZ; options.Tensor.EngineeringShear = true;
    cuts.SetRecoveryOptions(options); const auto saved = cuts.DumpDefinitionsJson();
    cuts.SetRecoveryOptions(MeshCutRecoveryOptions()); require(cuts.RestoreDefinitionsJson(saved) && cuts.DumpDefinitionsJson() == saved,"Tensor definition round trip");
    auto invalid = saved; const auto index = invalid.find("\"measure\":3"); require(index != std::string::npos,"Missing tensor definition");
    invalid.replace(index,11,"\"measure\":99");
    require(!cuts.RestoreDefinitionsJson(invalid) && cuts.DumpDefinitionsJson() == saved,"Invalid tensor restore changed live state");
    auto legacy = saved; const auto version = legacy.find("\"version\":");
    require(version != std::string::npos,"Missing definition version"); legacy.replace(version,legacy.find(',',version)-version,"\"version\":5");
    require(cuts.RestoreDefinitionsJson(legacy) && cuts.RecoveryOptions().Tensor.Measure == MeshCutTensorMeasure::Components,"v5 migration did not use tensor defaults");
    auto scalar = DeriveCutTensorField(stress,options.Tensor);
    require(scalar->GetComponent(0)->DisplayUnits() == "MPa" && scalar->TimeStep() == 3 && scalar->Description() == "Original stress","Tensor metadata lost");
    stress->GetComponent(5)->SetValue(RWCae::MeshPropertyEntityInfo(1),std::numeric_limits<double>::quiet_NaN()); stress->UpdateComponents();
    for (bool before : {false,true}) {
        options = MeshCutRecoveryOptions(); options.Tensor.Measure = MeshCutTensorMeasure::Mises; options.InvariantsBeforeAveraging = before;
        cuts.SetRecoveryOptions(options); cuts.InvalidateFields();
        const auto missing = cuts.Evaluate("cut",stress).SourceField;
        require(std::isnan(missing->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(1,4))),"Missing tensor trace filled by neighbor");
    }
}
void surfaceAndValidation()
{
    auto source = mesh(true); auto stress = field(); MeshCutViewManager cuts(source);
    cuts.AddPlane("line",Graphic3d_Vec4d(0,1,0,-0.5)); cuts.SetVisibility("line",2); cuts.SetCellMode("line",MeshCutCellMode::Projected);
    MeshCutRecoveryOptions options; options.Tensor.Measure = MeshCutTensorMeasure::Mises; options.Regions = {10,20}; cuts.SetRecoveryOptions(options);
    const auto result = cuts.Evaluate("line",stress); const auto slice = Handle(MeshGeometrySlice)::DownCast(result.Portions[1]);
    require(!result.LineField.IsNull() && slice->Lines().size() == 2,"Derived tensor surface lines absent");
    for (int l = 0; l < (int)slice->Lines().size(); ++l) {
        for (auto n : slice->Lines()[l].Nodes) {
            closeValue(result.LineField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(l,n)),
                slice->Lines()[l].SourceElement == 0 ? std::sqrt(61.0) : std::sqrt(93.0),"Tensor line material provenance lost");
        }
    }
    Handle(ViewCae::InteractiveCaeCutView) view = new ViewCae::InteractiveCaeCutView(source);
    view->Cuts().AddPlane("line",Graphic3d_Vec4d(0,1,0,-0.5)); view->Cuts().SetVisibility("line",2);
    view->Cuts().SetRecoveryOptions(options); view->SetField(stress,5); view->Synchronize();
    require(!view->LineArrays().empty(),"Derived tensor line presentation absent");
    stress->GetComponent(5)->SetDisplayUnits("kPa");
    rejects([&] { DeriveCutTensorField(stress,options.Tensor); },"Mixed tensor units accepted");
    Handle(RWCae::MeshNVecProperty) traces = new RWCae::MeshNVecProperty("trace",6,
        RWCae::MeshPropertyBase::MappingType::Elemental,RWCae::MeshNVecPropertyType::Tensor);
    traces->Init<RWCae::MeshNodeVecProperty>();
    for (int c = 0; c < 6; ++c) {
        auto component = Handle(RWCae::MeshNodeVecProperty)::DownCast(traces->GetComponent(c)); component->Init(2,0,false,false);
        component->SetValue(RWCae::MeshPropertyEntityInfo(0,0),c == 0 ? 7 : 0);
        component->SetValue(RWCae::MeshPropertyEntityInfo(1,4),c == 0 ? 9 : 0);
    }
    traces->UpdateComponents(); auto derived = DeriveCutTensorField(traces,options.Tensor);
    closeValue(derived->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(1,4)),9,"Element-node tensor tuple mapping");
    traces->GetComponent(5)->SetValue(RWCae::MeshPropertyEntityInfo(1,5),1); traces->UpdateComponents();
    rejects([&] { DeriveCutTensorField(traces,options.Tensor); },"Mismatched tensor trace keys dropped");
}
void presentation()
{
    auto stress = field(); Handle(ViewCae::InteractiveCaeCutView) view = new ViewCae::InteractiveCaeCutView(mesh());
    view->Cuts().AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.25)); view->Cuts().SetVisibility("cut",2);
    MeshCutRecoveryOptions options; options.Tensor.Measure = MeshCutTensorMeasure::Mises; view->Cuts().SetRecoveryOptions(options);
    view->SetField(stress,5); view->Synchronize(); require(!view->Presentations().empty(),"Derived tensor presentation absent");
    const auto count = view->SynchronizationCount(); const auto presentations = view->Presentations();
    stress->GetComponent(5)->SetValue(RWCae::MeshPropertyEntityInfo(1),INFINITY); stress->UpdateComponents(); view->Cuts().InvalidateFields();
    rejects([&] { view->Synchronize(); },"Infinite tensor display accepted");
    require(view->SynchronizationCount() == count && view->Presentations() == presentations,"Failed tensor display replaced valid presentation");
    stress->GetComponent(5)->SetValue(RWCae::MeshPropertyEntityInfo(1),0); stress->UpdateComponents(); view->Cuts().InvalidateFields();
    view->Cuts().SetEnabled("cut",false); view->Synchronize(); require(!view->Presentations().empty(),"Uncut derived tensor display missing");
}
}
int main()
{
    try { references(); recoveryAndPhysical(); surfaceAndValidation(); presentation(); std::cout << "CUT_TENSOR_PASS\n"; }
    catch (const std::exception& error) { std::cerr << "CUT_TENSOR_FAIL: " << error.what() << '\n'; return 1; }
    return 0;
}
