#include <ViewCaeGeom/MeshCutViewManager.h>
#include <RWCae/MeshScalarProperty.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkGenericCell.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>

#ifdef near
#undef near
#endif

using namespace ViewCaeGeom;
namespace {
void require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}
void near(double actual, double expected, double tolerance, const char* message)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > tolerance) {
        std::cerr << std::setprecision(16) << message << ": " << actual << " != " << expected << '\n';
        throw std::runtime_error(message);
    }
}
double integrate(const std::function<double(double)>& function, double end)
{
    constexpr int count = 2048;
    double sum = function(0)+function(end);
    for (int n = 1; n < count; ++n) { sum += (n%2 ? 4 : 2)*function(end*n/count); }
    return sum*end/(3*count);
}
bool bezier(int type) { return type >= 75 && type <= 80; }
bool triangle(int type) { return type == 22 || type == 34 || type == 69 || type == 76; }
bool tetrahedron(int type) { return type == 71 || type == 78; }
bool curve(int type) { return type == 21 || type == 35 || type == 68 || type == 75; }
struct Reference {
    bool Bezier;
    bool Rational;
    int Degree;
    double Height;
    double denominator(double u) const { return Rational ? 0.7+0.4*u : 1; }
    double x(double u) const
    {
        return Rational ? ((0.7+0.4/Degree)*u+0.4*(1-1.0/Degree)*u*u)/denominator(u) : u;
    }
    double dx(double u) const
    {
        if (!Rational) { return 1; }
        const double a = 0.7+0.4/Degree, b = 0.4*(1-1.0/Degree), d = denominator(u);
        return ((a+2*b*u)*d-0.4*(a*u+b*u*u))/(d*d);
    }
    double bow(double u) const
    {
        const double factor = Bezier ? 1-1.0/Degree : 1;
        const double weight = Rational ? 0.7+0.4/Degree+0.4*(1-2.0/Degree)*u : 1;
        return Height*factor*u*(1-u)*weight/denominator(u);
    }
    double dbow(double u) const
    {
        const double factor = Bezier ? 1-1.0/Degree : 1;
        const double a = Rational ? 0.7+0.4/Degree : 1;
        const double b = Rational ? 0.4*(1-2.0/Degree) : 0;
        const double d = denominator(u), numerator = u*(1-u)*(a+b*u);
        return Height*factor*(((1-2*u)*(a+b*u)+b*u*(1-u))*d
            -(Rational ? 0.4 : 0)*numerator)/(d*d);
    }
    double transverse(double u) const { return Rational ? (0.7+0.2*u)/denominator(u) : 1; }
    double squared(double u) const
    {
        if (!Bezier) { return u*u; }
        // Independent binomial moments of the original Bernstein coefficients i^2/p^2.
        const double second = u*u+u*(1-u)/Degree;
        const double third = u*u*u+3*u*u*(1-u)/Degree+u*(1-u)*(1-2*u)/(Degree*Degree);
        return Rational ? (0.7*second+0.4*third)/denominator(u) : second;
    }
    double parameter(double position) const
    {
        double low = 0, high = 1;
        for (int n = 0; n < 60; ++n) {
            const double middle = (low+high)/2;
            if (x(middle) < position) { low = middle; } else { high = middle; }
        }
        return (low+high)/2;
    }
};
Handle(RWCae::MeshData) fixture(int type, bool rational = false, int count = 0,
    const std::vector<int>& orders = {2,2,2}, double weightScale = 1)
{
    const std::map<int,int> sizes = {{21,3},{35,4},{68,3},{75,3},{22,6},{23,8},{28,9},
        {30,6},{34,7},{69,6},{70,9},{76,6},{77,9},{71,10},{78,10}};
    const int size = count ? count : sizes.at(type);
    vtkNew<vtkGenericCell> cell; cell->SetCellType(type);
    cell->GetPoints()->SetNumberOfPoints(size); cell->GetPointIds()->SetNumberOfIds(size);
    for (int n = 0; n < size; ++n) { cell->GetPointIds()->SetId(n,n); }
    if (type >= 68) {
        vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points;
        points->SetNumberOfPoints(size); grid->SetPoints(points);
        std::vector<vtkIdType> ids(size); for (int n = 0; n < size; ++n) { ids[n] = n; }
        grid->InsertNextCell(type,size,ids.data());
        vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3);
        degrees->InsertNextTypedTuple(orders.data()); grid->GetCellData()->SetHigherOrderDegrees(degrees);
        grid->GetCell(0,cell);
    }
    const auto q = cell->GetParametricCoords(); require(q != nullptr,"Reference cell has no parameter chart");
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData(); RWCae::MeshData::GenericCell topology;
    topology.Type = type;
    if (type >= 68) { topology.Degrees = orders; }
    for (int n = 0; n < size; ++n) {
        const double u = q[3*n], v = q[3*n+1], w = q[3*n+2];
        Graphic3d_Vec3 p((float)u,(float)v,(float)(0.4*u*(1-u)));
        if (curve(type)) { p.y() = (float)(u*(1-u)); p.z() = 0; }
        if (tetrahedron(type)) { p.z() = (float)(w+0.4*u*(1-u-v-w)); }
        topology.Nodes.push_back(mesh->AddNodePosition(p));
        if (rational) { topology.RationalWeights.push_back(weightScale*(0.7+0.4*u)); }
    }
    mesh->AddGenericCell(topology); return mesh;
}
Handle(RWCae::MeshNVecProperty) field(const Handle(RWCae::MeshData)& mesh, bool squared)
{
    Handle(RWCae::MeshNVecProperty) value = new RWCae::MeshNVecProperty(squared ? "u_squared" : "temperature",1,
        RWCae::MeshPropertyBase::MappingType::Nodal);
    value->Init<RWCae::MeshScalarProperty>();
    auto component = Handle(RWCae::MeshScalarProperty)::DownCast(value->GetComponent(0)); component->Init(mesh->NbNodes());
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
        const auto p = mesh->Nodes()[n];
        component->SetValue(RWCae::MeshPropertyEntityInfo(n),squared ? (double)p.x()*p.x() : p.x()+2*p.y()+3*p.z());
    }
    value->UpdateComponents(); return value;
}
void writeFixture(const Handle(RWCae::MeshData)& mesh, const std::string& name)
{
    vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points; points->SetDataTypeToDouble();
    for (const auto& p : mesh->Nodes()) { points->InsertNextPoint(p.x(),p.y(),p.z()); }
    grid->SetPoints(points); const auto& topology = mesh->GenericCells()[0];
    std::vector<vtkIdType> ids(topology.Nodes.begin(),topology.Nodes.end());
    grid->InsertNextCell(topology.Type,ids.size(),ids.data());
    if (!topology.Degrees.empty()) {
        vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3); degrees->InsertNextTypedTuple(topology.Degrees.data());
        grid->GetCellData()->SetHigherOrderDegrees(degrees);
    }
    if (!topology.RationalWeights.empty()) {
        vtkNew<vtkDoubleArray> weights; weights->SetName("RationalWeights");
        for (double weight : topology.RationalWeights) { weights->InsertNextValue(weight); }
        grid->GetPointData()->SetRationalWeights(weights);
    }
    for (bool squared : {false,true}) {
        const auto values = field(mesh,squared); vtkNew<vtkDoubleArray> array; array->SetName(values->Name().ToCString());
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
            array->InsertNextValue(values->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)));
        }
        grid->GetPointData()->AddArray(array);
    }
    vtkNew<vtkDoubleArray> pressure; pressure->SetName("pressure"); pressure->InsertNextValue(7); grid->GetCellData()->AddArray(pressure);
    std::filesystem::create_directories(CUT_REFERENCE_FIXTURE_DIR);
    const std::string path = std::string(CUT_REFERENCE_FIXTURE_DIR)+"/"+name+".vtu";
    vtkNew<vtkXMLUnstructuredGridWriter> writer; writer->SetFileName(path.c_str()); writer->SetInputData(grid); writer->SetDataModeToAscii();
    require(writer->Write(),"Cannot write curved-reference fixture");
}
MeshCutGeometryOptions options()
{
    MeshCutGeometryOptions value; value.Mode = MeshCutGeometryMode::AnalyticRequired;
    value.AbsoluteTolerance = 0.01; value.RelativeTolerance = 0; return value;
}
void checkValues(const MeshCutResult& cut, const Reference& reference, bool squared)
{
    require(!cut.SourceApproximate,"Curved reference unexpectedly fell back to numerical geometry");
    require(!cut.Fields[1].IsNull(),"Missing original FE section field");
    for (int part = 0; part < 3; ++part) {
        if (cut.Portions[part].IsNull() || cut.Fields[part].IsNull()) { continue; }
        const auto mesh = cut.Portions[part]->GetMeshData();
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
            const auto p = mesh->Nodes()[n];
            const double expected = squared ? reference.squared(reference.parameter(p.x())) : p.x()+2*p.y()+3*p.z();
            near(cut.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),expected,3e-6,"Independent original FE reference");
            MeshInterpolatedNode origin; require(cut.Portions[part]->OriginNode(n,origin),"Missing curved reference provenance");
            double sum = 0; for (const auto& weight : origin.BaseNodes()) { sum += weight.second; }
            near(sum,1,1e-10,"Curved FE partition of unity");
        }
    }
}
void runReference(int type, bool rational = false, int count = 0,
    const std::vector<int>& orders = {2,2,2}, double weightScale = 1)
{
    std::cout << "CUT_REFERENCE_CHECK " << type << " rational " << rational << " degree " << orders[0] << '\n';
    const auto mesh = fixture(type,rational,count,orders,weightScale);
    const Reference reference{bezier(type),rational,orders[0],curve(type) ? 1.0 : 0.4};
    MeshCutViewManager manager(mesh); manager.SetGeometryOptions(options());
    manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.25)); manager.SetVisibility("cut",7);
    const auto cut = manager.Evaluate("cut",field(mesh,false)); checkValues(cut,reference,false);
    require(cut.Portions[1]->GetMeshData()->NbNodes() > 0,"Empty curved reference section");
    const double u = reference.parameter(0.25);
    if (tetrahedron(type)) {
        const double h = bezier(type) ? 0.2 : 0.4;
        auto section = [&](double q) {
            const double k = reference.transverse(q), shear = rational ? 0.18/reference.denominator(q) : h;
            return k*(k-shear*q)*(1-q)*(1-q)/2;
        };
        auto density = [&](double q) { return section(q)*reference.dx(q); };
        near(cut.Areas[1],section(u),4e-7,"Independent tetra section area");
        near(cut.Volumes[0],integrate(density,u),4e-7,"Independent tetra partial volume");
        near(cut.Volumes[0]+cut.Volumes[2],integrate(density,1),4e-7,"Independent tetra source volume");
    } else {
        for (const auto& p : cut.Portions[1]->GetMeshData()->Nodes()) {
            near(p.x(),0.25,2e-7,"Curved section missed the geometric plane");
            near(curve(type) ? p.y() : p.z(),reference.bow(u),3e-7,"Curved section was replaced by a corner chord");
        }
        if (!curve(type)) {
            auto density = [&](double q) {
                const double span = triangle(type) ? reference.transverse(q)*(1-q) : 1;
                return span*std::hypot(reference.dx(q),reference.dbow(q));
            };
            near(cut.Areas[0],integrate(density,u),4e-7,"Independent curved surface partial area");
            near(cut.Areas[0]+cut.Areas[2],integrate(density,1),4e-7,"Independent curved surface source area");
            const auto slice = Handle(MeshGeometrySlice)::DownCast(cut.Portions[1]); double length = 0;
            for (const auto& line : slice->Lines()) {
                const auto& nodes = slice->GetMeshData()->Nodes(); length += (nodes[line.Nodes[1]]-nodes[line.Nodes[0]]).Modulus();
            }
            near(length,triangle(type) ? reference.transverse(u)*(1-u) : 1,3e-7,"Independent curved surface section length");
        }
    }
    checkValues(manager.Evaluate("cut",field(mesh,true)),reference,true);
    const auto work = manager.Statistics();
    require(work.AnalyticCells == 1 && work.NumericalCells == 0,"Missing analytic reference diagnostics");
    manager.SetPlane("cut",Graphic3d_Vec4d(1,0,0,-0.4)); checkValues(manager.Evaluate("cut",field(mesh,true)),reference,true);
    require(manager.Statistics().SourcePreparations == work.SourcePreparations
        && manager.Statistics().ShapeBuilds == work.ShapeBuilds,"Reference motion rebuilt source geometry");
    if (type == 69 && !count) {
        for (unsigned mask = 0; mask < 8; ++mask) {
            manager.SetVisibility("cut",mask); const auto masked = manager.Evaluate("cut",field(mesh,false));
            for (unsigned part = 0; part < 3; ++part) {
                require(masked.Portions[part].IsNull() == ((mask & (1u << part)) == 0),"Curved surface portion mask changed");
            }
        }
    }
    if (weightScale == 1) {
        writeFixture(mesh,"curved_reference_"+std::to_string(type)+(rational ? "_rational" : "")
            +(count ? "_d"+std::to_string(orders[0])+"x"+std::to_string(orders[1]) : ""));
    }
}
void unsupportedOrder()
{
    const auto mesh = fixture(68,false,8,{7,0,0}); MeshCutViewManager manager(mesh);
    manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.25));
    require(manager.Evaluate("cut").SourceApproximate,"Degree seven was incorrectly labelled analytic");
    manager.SetGeometryOptions(options()); bool rejected = false;
    try { manager.Evaluate("cut"); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected && manager.Statistics().SourcePreparations == 0,"Unsupported analytic order published a source");
    auto numerical = options(); numerical.Mode = MeshCutGeometryMode::Numerical; manager.SetGeometryOptions(numerical);
    require(manager.Evaluate("cut").Portions[1]->GetMeshData()->NbNodes() > 0,"Unsupported-order numerical retry failed");
    writeFixture(mesh,"unsupported_degree_seven");
}
void unsupportedWeightPrecision()
{
    const auto original = fixture(75,true);
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : original->Nodes()) { mesh->AddNodePosition(p); }
    auto topology = original->GenericCells()[0]; topology.RationalWeights[0] *= 1e-15; mesh->AddGenericCell(topology);
    MeshCutViewManager manager(mesh);
    auto preferred = options(); preferred.Mode = MeshCutGeometryMode::AnalyticPreferred;
    preferred.AbsoluteTolerance = 2; manager.SetGeometryOptions(preferred);
    // This intentionally coarse numerical source tests fallback publication, not geometric accuracy.
    manager.SourceGeometry()->GetLastChild()->GetMeshData();
    require(manager.Statistics().NumericalCells == 1 && manager.Statistics().AnalyticCells == 0,
        "Unsupported weight precision was not identified as numerical");
    auto required = preferred; required.Mode = MeshCutGeometryMode::AnalyticRequired; manager.SetGeometryOptions(required);
    bool rejected = false;
    try { manager.SourceGeometry()->GetLastChild()->GetMeshData(); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected && manager.Statistics().SourcePreparations == 0,"Unsupported weight precision published an analytic source");
    writeFixture(mesh,"unsupported_weight_precision");
}
void numericalConvergence(int type, bool rational)
{
    const Reference reference{bezier(type),rational,2,0.4};
    auto density = [&](double u) {
        if (tetrahedron(type)) {
            const double k = reference.transverse(u), shear = rational ? 0.18/reference.denominator(u) : 0.2;
            return reference.dx(u)*k*(k-shear*u)*(1-u)*(1-u)/2;
        }
        const double span = triangle(type) ? reference.transverse(u)*(1-u) : 1;
        return span*std::hypot(reference.dx(u),reference.dbow(u));
    };
    const double expected = integrate(density,1);
    MeshCutViewManager manager(fixture(type,rational));
    manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.25)); manager.SetVisibility("cut",7);
    double previous = 1, first = 0; uint64_t previousPrimitives = 0; bool refined = false;
    for (double tolerance : {0.025,0.007,0.002}) {
        auto numerical = options(); numerical.Mode = MeshCutGeometryMode::Numerical;
        numerical.AbsoluteTolerance = tolerance; manager.SetGeometryOptions(numerical);
        const auto cut = manager.Evaluate("cut"); const auto work = manager.Statistics();
        const auto& measures = tetrahedron(type) ? cut.Volumes : cut.Areas;
        const double error = std::abs(measures[0]+measures[2]-expected);
        require(cut.SourceApproximate && work.NumericalCells == 1,"Numerical reference was incorrectly labelled analytic");
        require(work.SourcePrimitives >= previousPrimitives && error <= previous+1e-10,"Additional curved family regressed under refinement");
        // Dyadic depths can stay unchanged for two adjacent tolerances. Require
        // improvement when the representation actually refines, and overall.
        if (!previousPrimitives) { first = error; }
        else if (work.SourcePrimitives > previousPrimitives) {
            require(error < previous*0.6,"Additional curved family did not converge"); refined = true;
        }
        near(measures[0]+measures[2],expected,tolerance,"Numerical curved reference measure exceeds tolerance");
        previous = error; previousPrimitives = work.SourcePrimitives;
        std::cout << "CUT_REFERENCE_CONVERGENCE " << type << " tolerance " << tolerance << " error " << error << '\n';
    }
    require(refined && previous < first*0.6,"Additional curved family never refined or improved");
}
}
int main()
{
    try {
        for (int type : {21,35,68,75,22,23,28,30,34,69,70,76,77,71,78}) { runReference(type); }
        for (int type : {75,76,77,78}) { runReference(type,true); }
        runReference(68,false,7,{6,0,0}); runReference(75,true,7,{6,0,0});
        runReference(70,false,12,{2,3,0}); runReference(77,true,12,{2,3,0});
        runReference(70,false,21,{6,2,0});
        runReference(75,true,3,{2,0,0},1e-200); runReference(75,true,3,{2,0,0},1e200);
        unsupportedOrder(); unsupportedWeightPrecision();
        numericalConvergence(69,false); numericalConvergence(77,true); numericalConvergence(78,true);
        std::cout << "CUT_REFERENCE_NATIVE_PASS\n"; return 0;
    } catch (const Standard_Failure& error) { std::cerr << "CUT_REFERENCE_FAIL " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "CUT_REFERENCE_FAIL " << error.what() << '\n'; }
    return 1;
}
