#include <ViewCaeGeom/MeshCutViewManager.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshScalarProperty.h>
#include <ViewCae/InteractiveCaeCutView.h>
#include <ViewCae/CaeMeshTools.h>
#include <ViewCaeGeom/MeshCutIntegration.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

// Windows headers define the historical near qualifier as a macro.
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
    if (std::abs(actual - expected) > 1e-6 || !std::isfinite(actual)) {
        std::cerr << message << ": actual=" << actual << " expected=" << expected << '\n';
        throw std::runtime_error(message);
    }
}

static Handle(RWCae::MeshData) tetra()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const Graphic3d_Vec3& p : {Graphic3d_Vec3(0,0,0), Graphic3d_Vec3(1,0,0),
                                   Graphic3d_Vec3(0,1,0), Graphic3d_Vec3(0,0,1)}) {
        mesh->AddNodePosition(p);
    }
    const RWCae::MeshEntityId nodes[] = {0,1,2,3};
    mesh->AddElement(RWCae::MeshElementTetrahedron(nodes));
    return mesh;
}

static Handle(RWCae::MeshNVecProperty) field(const char* name, bool nodal, const std::vector<double>& values)
{
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty(name, 1, nodal
        ? RWCae::MeshPropertyBase::MappingType::Nodal : RWCae::MeshPropertyBase::MappingType::Elemental);
    result->Init<RWCae::MeshScalarProperty>();
    auto component = Handle(RWCae::MeshScalarProperty)::DownCast(result->GetComponent(0));
    component->Init((int)values.size());
    for (size_t i = 0; i < values.size(); ++i) { component->SetValue(RWCae::MeshPropertyEntityInfo((int)i), values[i]); }
    result->UpdateComponents();
    return result;
}

static double volume(const Handle(RWCae::MeshData)& mesh)
{
    double result = 0.0;
    auto compute = [&](const auto& adaptor) {
        for (RWCae::MeshEntityId i = 0; i < adaptor.NbElements(); ++i) {
            const auto& elem = adaptor.GetElement(i);
            Graphic3d_Vec3d center(0,0,0);
            for (RWCae::MeshElemEntityId n = 0; n < elem.NbNodes(); ++n) {
                center += Graphic3d_Vec3d(mesh->Nodes()[elem[n]]);
            }
            center /= elem.NbNodes();
            for (RWCae::MeshElemEntityId f = 0; f < adaptor.NbFaces(elem); ++f) {
                std::vector<RWCae::MeshEntityId> nodes;
                RWCae::MeshElemType type;
                adaptor.FaceNodes(elem, f, nodes, type);
                const auto a = Graphic3d_Vec3d(mesh->Nodes()[nodes[0]]) - center;
                for (size_t n = 1; n + 1 < nodes.size(); ++n) {
                    const auto b = Graphic3d_Vec3d(mesh->Nodes()[nodes[n]]) - center;
                    const auto c = Graphic3d_Vec3d(mesh->Nodes()[nodes[n+1]]) - center;
                    result += std::abs(a.Dot(Graphic3d_Vec3d::Cross(b, c))) / 6.0;
                }
            }
        }
    };
    RWCae::MeshAdaptorsIterator3dElems::For(compute, mesh);
    return result;
}

static void volumeCut()
{
    auto mesh = tetra();
    MeshCutViewManager manager(mesh);
    require(manager.AddPlane("x", Graphic3d_Vec4d(2,0,0,-0.5)), "AddPlane failed");
    require(!manager.AddPlane("x", Graphic3d_Vec4d(1,0,0,0)), "Duplicate id accepted");
    require(!manager.AddPlane("zero", Graphic3d_Vec4d(0,0,0,0)), "Zero normal accepted");
    require(!manager.SetPlane("x", Graphic3d_Vec4d(1,0,0,std::numeric_limits<double>::infinity())), "Infinite plane accepted");
    auto nodal = field("temperature", true, {0,1,2,3});
    require(manager.SetVisibility("x", 7), "Visibility failed");
    auto result = manager.Evaluate("x", nodal);
    near(volume(result.Portions[0]->GetMeshData()), (1.0 - std::pow(0.75,3)) / 6.0, "Negative volume");
    near(volume(result.Portions[2]->GetMeshData()), std::pow(0.75,3) / 6.0, "Positive volume");
    auto section = result.Portions[1]->GetMeshData();
    require(section->NbIndexedElems() == 1 && section->NbNodes() == 3, "Section triangle missing");
    const auto a = Graphic3d_Vec3d(section->Nodes()[1] - section->Nodes()[0]);
    const auto b = Graphic3d_Vec3d(section->Nodes()[2] - section->Nodes()[0]);
    near(Graphic3d_Vec3d::Cross(a, b).Modulus()/2.0, 0.75*0.75/2.0, "Section area");
    for (int i = 0; i < (int)section->NbNodes(); ++i) {
        const auto& p = section->Nodes()[i];
        near(p.x(), 0.25, "Section not on plane");
        near(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(i)),
             p.x() + 2*p.y() + 3*p.z(), "Nodal interpolation");
        MeshInterpolatedNode origin;
        require(result.Portions[1]->OriginNode(i, origin), "Node provenance missing");
        double weight = 0;
        for (const auto& base : origin.BaseNodes()) { weight += base.second; }
        near(weight, 1.0, "Interpolation weights");
    }
    auto cell = field("pressure", false, {7});
    auto constant = manager.Evaluate("x", cell);
    near(constant.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)), 7, "Constant CellData section");
    for (unsigned mask = 0; mask < 8; ++mask) {
        require(manager.SetVisibility("x", mask), "Valid combination rejected");
        auto combination = manager.Evaluate("x");
        for (unsigned i = 0; i < 3; ++i) {
            require(combination.Portions[i].IsNull() == ((mask & (1u << i)) == 0), "Visibility combination incorrect");
        }
    }
    require(!manager.SetVisibility("x", 8), "Invalid visibility accepted");
    manager.SetVisibility("x", 7);
    require(manager.SetPlane("x", Graphic3d_Vec4d(1,0,0,-0.5)), "Plane update failed");
    auto moved = manager.Evaluate("x", nodal);
    require(moved.Portions[1]->GetMeshData() == section, "Same-topology drag did not reuse connectivity");
    near(volume(moved.Portions[2]->GetMeshData()), 0.125/6.0, "Moved cut stale");
    require(moved.Revision > result.Revision, "Revision not incremented");
    manager.SetPlane("x", Graphic3d_Vec4d(0,1,0,-0.25));
    require(manager.Evaluate("x").Portions[1]->GetMeshData() != section, "Rotation did not rebuild topology");
    manager.SetPlane("x", Graphic3d_Vec4d(1,0,0,-2));
    auto outside = manager.Evaluate("x");
    require(outside.Portions[1]->GetMeshData()->NbNodes() == 0, "Outside section not empty");
    require(outside.Roots[0]->AcceptElement(0) && !outside.Roots[2]->AcceptElement(0), "Retained cells wrong");
    require(manager.Duplicate("x", "copy") && manager.Rename("copy", "Second plane"), "CRUD failed");
    require(manager.EvaluateVisible().size() == 2, "Cuts incorrectly chained");
    manager.SetEnabled("copy", false);
    require(manager.EvaluateVisible().size() == 1, "Disabled cut visible");
    require(manager.Remove("copy") && !manager.Remove("copy"), "Remove failed");
    manager.InvalidateGeometry();
    require(manager.Evaluate("x").Roots[0]->AcceptElement(0), "Geometry invalidation broke cut");
}

static void surfaceCut()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0), Graphic3d_Vec3(1,0,0), Graphic3d_Vec3(0,1,0)}) {
        mesh->AddNodePosition(p);
    }
    const RWCae::MeshEntityId nodes[] = {0,1,2};
    mesh->AddElement(RWCae::MeshElementTriangle(nodes));
    MeshCutViewManager manager(mesh);
    manager.AddPlane("surface", Graphic3d_Vec4d(1,0,0,-0.25));
    manager.SetVisibility("surface", 7);
    auto result = manager.Evaluate("surface", field("temperature", true, {0,1,2}));
    auto slice = Handle(MeshGeometrySlice)::DownCast(result.Portions[1]);
    require(slice->Lines().size() == 1, "Surface section line missing");
    for (auto n : slice->Lines()[0].Nodes) {
        const auto& p = slice->GetMeshData()->Nodes()[n];
        near(p.x(), 0.25, "Surface line not on plane");
        near(result.LineField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)), p.x() + 2*p.y(), "Surface line interpolation");
    }
    require(result.Portions[0]->GetMeshData()->NbIndexedElems() > 0
         && result.Portions[2]->GetMeshData()->NbIndexedElems() > 0, "Surface sides not clipped");
    auto constant = manager.Evaluate("surface", field("pressure", false, {13}));
    near(constant.LineField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)), 13, "CellData line value");
    manager.SetPlane("surface", Graphic3d_Vec4d(0,0,1,0));
    auto coplanar = manager.Evaluate("surface");
    require(coplanar.Portions[1]->GetMeshData()->NbIndexedElems() == 1, "Coplanar surface face lost");
}

static void projectedCells()
{
    auto mesh = tetra();
    mesh->AddNodePosition(Graphic3d_Vec3(0,0,-1));
    const RWCae::MeshEntityId second[] = {0,2,1,4};
    mesh->AddElement(RWCae::MeshElementTetrahedron(second));
    MeshCutViewManager manager(mesh);
    manager.AddPlane("x", Graphic3d_Vec4d(1,0,0,-0.25));
    auto cell = field("pressure", false, {2,6});
    manager.SetCellMode("x", MeshCutCellMode::Projected);
    auto result = manager.Evaluate("x", cell);
    require(result.SourceField->IsNodal() && result.Fields[1]->IsNodal(), "Projection not nodal");
    double minimum, maximum;
    require(result.SourceField->GetComponent(0)->ValueRange().GetBounds(minimum, maximum), "Projected contour range missing");
    near(minimum, 2, "Projected contour minimum");
    near(maximum, 6, "Projected contour maximum");
    near(result.SourceField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)), 4, "Shared node projection");
    near(result.SourceField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(3)), 2, "Unshared node projection");
    for (int i = 0; i < (int)result.Portions[1]->GetMeshData()->NbNodes(); ++i) {
        const double z = result.Portions[1]->GetMeshData()->Nodes()[i].z();
        near(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(i)), 4 - 2*z, "Projected contour interpolation");
    }
    auto cached = result.SourceField;
    manager.SetPlane("x", Graphic3d_Vec4d(1,0,0,-0.5));
    require(manager.Evaluate("x", cell).SourceField == cached, "Projection recalculated during drag");
    cell->GetComponent(0)->SetValue(RWCae::MeshPropertyEntityInfo(0), 10);
    manager.InvalidateFields();
    near(manager.Evaluate("x", cell).SourceField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)), 8, "Field cache stale");
}

static void displayPrimitives()
{
    Handle(ViewCae::InteractiveCaeCutView) view = new ViewCae::InteractiveCaeCutView(tetra());
    view->Cuts().AddPlane("x", Graphic3d_Vec4d(1,0,0,-0.25));
    view->Cuts().SetVisibility("x", 7);
    view->SetField(field("temperature", true, {0,1,2,3}));
    view->Synchronize();
    require(view->Presentations().size() == 3, "Three portion presentations not created");
    for (size_t i = 0; i < view->Presentations().size(); ++i) {
        auto prs = view->Presentations()[i];
        prs->GetParts();
        auto triangles = ViewCae::CaeMeshTools::CreateShadedMesh(prs->GetMeshGeometry(), prs->GetGeomScalarProps(), false,
            i == 1 ? ViewCae::MeshShadedType::FACES : ViewCae::MeshShadedType::VOLUME_SURFACE, 1.0f);
        require(!triangles.IsNull() && triangles->VertexNumber() > 0 && triangles->HasVertexTexels(),
                "Cut contour triangles not generated");
    }
    view->Cuts().SetVisibility("x", 0);
    view->Synchronize();
    require(view->Presentations().empty() && view->LineArrays().empty(), "Hidden cut retains primitives");
    view->Cuts().SetEnabled("x", false);
    view->Synchronize();
    require(view->Presentations().size() == 1, "Disabling all cuts did not restore the source model");
    view->Presentations()[0]->GetParts();
    auto full = ViewCae::CaeMeshTools::CreateShadedMesh(view->Presentations()[0]->GetMeshGeometry(),
        view->Presentations()[0]->GetGeomScalarProps(), false, ViewCae::MeshShadedType::VOLUME_SURFACE, 1.0f);
    require(!full.IsNull() && full->VertexNumber() > 0 && full->HasVertexTexels(), "Uncut source contour missing");

    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0), Graphic3d_Vec3(1,0,0), Graphic3d_Vec3(0,1,0)}) { mesh->AddNodePosition(p); }
    const RWCae::MeshEntityId nodes[] = {0,1,2};
    mesh->AddElement(RWCae::MeshElementTriangle(nodes));
    view = new ViewCae::InteractiveCaeCutView(mesh);
    view->Cuts().AddPlane("surface", Graphic3d_Vec4d(1,0,0,-0.25));
    view->SetField(field("temperature", true, {0,1,2}));
    view->Synchronize();
    require(view->LineArrays().size() == 1 && view->LineArrays()[0]->VertexNumber() == 2
         && view->LineArrays()[0]->HasVertexTexels(), "Colored line primitive missing");
    auto line = view->LineArrays()[0];
    require(line->VertexTexel(1).X() != line->VertexTexel(2).X(), "Line contour coordinates not interpolated");
    view->SetField(field("pressure", false, {7}));
    view->Synchronize();
    line = view->LineArrays()[0];
    near(line->VertexTexel(1).X(), line->VertexTexel(2).X(), "Constant cell line changes color");
}

static void definitionsAndDeformation()
{
    auto mesh = tetra();
    MeshCutViewManager manager(mesh);
    manager.AddPlane("x", Graphic3d_Vec4d(1,0,0,-0.25));
    manager.SetVisibility("x", 7);
    manager.Rename("x", "Primary cut");
    manager.Duplicate("x", "disabled");
    manager.SetEnabled("disabled", false);
    manager.SetCellMode("disabled", MeshCutCellMode::Projected);
    const auto json = manager.DumpDefinitionsJson();
    require(!manager.RestoreDefinitionsJson("{\"version\":1,\"cuts\":[{}]}") && manager.DumpDefinitionsJson() == json,
            "Invalid restore changed current cuts");
    manager.Remove("x");
    require(manager.RestoreDefinitionsJson(json) && manager.DumpDefinitionsJson() == json, "Definition round trip failed");
    require(manager.AddPlane("oblique", Graphic3d_Vec4d(2,3,7,-0.456789123456789)), "Oblique plane rejected");
    manager.SetPlane("x", Graphic3d_Vec4d(0.34506636574889995,0.7399972093835984,0.5773502691896258,-0.3324827688644248));
    const auto obliqueJson = manager.DumpDefinitionsJson();
    for (int i = 0; i < 20; ++i) {
        require(manager.RestoreDefinitionsJson(obliqueJson) && manager.DumpDefinitionsJson() == obliqueJson,
            "Repeated oblique-plane restoration changes floating-point coefficients");
    }
    manager.Remove("oblique");
    manager.SetPlane("x", Graphic3d_Vec4d(1,0,0,-0.25));
    Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("displacement", 3,
        RWCae::MeshPropertyBase::MappingType::Nodal, RWCae::MeshNVecPropertyType::Vector);
    displacement->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 3; ++c) {
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c));
        component->Init(4);
        for (int n = 0; n < 4; ++n) { component->SetValue(RWCae::MeshPropertyEntityInfo(n), c == 0 ? 1 : 0); }
    }
    displacement->UpdateComponents();
    require(manager.SetDeformation(displacement, 1.0), "Valid deformation rejected");
    const double shift = manager.EffectiveDeformationScale();
    require(shift > 0, "Deformation scale is zero");
    near(manager.SourceBounds().CornerMin().X(), shift, "Deformed source bounds stale");
    near(manager.SourceBounds().CornerMax().X(), shift+1, "Deformed source upper bounds stale");
    auto source = manager.SourceGeometry();
    auto sourcePoint = mesh->Nodes()[0];
    source->ModifyNode(0, sourcePoint);
    near(sourcePoint.x(), shift, "Uncut source ignores deformation");
    manager.SetPlane("x", Graphic3d_Vec4d(1,0,0,-0.25-shift));
    auto result = manager.Evaluate("x", field("temperature", true, {0,1,2,3}));
    near(volume(result.Portions[2]->GetMeshData()), std::pow(0.75,3)/6.0, "Deformed clip volume");
    auto section = result.Portions[1]->GetMeshData();
    for (int n = 0; n < (int)section->NbNodes(); ++n) {
        const auto& p = section->Nodes()[n];
        near(p.x(), 0.25+shift, "Cut ignores deformed positions");
        near(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),
             p.x()-shift+2*p.y()+3*p.z(), "Deformed nodal interpolation");
    }
    require(manager.SetDeformation(nullptr, 0.0), "Deformation reset failed");
    near(manager.SourceBounds().CornerMin().X(), 0, "Source bounds ignored deformation reset");
    sourcePoint = mesh->Nodes()[0];
    manager.SourceGeometry()->ModifyNode(0, sourcePoint);
    near(sourcePoint.x(), 0, "Uncut source deformation cache stale");
    manager.SetPlane("x", Graphic3d_Vec4d(1,0,0,-0.25));
    near(volume(manager.Evaluate("x").Portions[2]->GetMeshData()), std::pow(0.75,3)/6.0, "Undeformed cache stale");
}

static void resultants()
{
    MeshCutViewManager manager(tetra());
    manager.AddPlane("x", Graphic3d_Vec4d(1,0,0,-0.25));
    Handle(RWCae::MeshNVecProperty) stress = new RWCae::MeshNVecProperty("stress", 6,
        RWCae::MeshPropertyBase::MappingType::Nodal, RWCae::MeshNVecPropertyType::Tensor);
    stress->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 6; ++c) {
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(stress->GetComponent(c));
        component->Init(4);
        const double xx[] = {2,2,4,5};
        for (int n = 0; n < 4; ++n) {
            component->SetValue(RWCae::MeshPropertyEntityInfo(n), c == 0 ? xx[n] : c == 1 ? 3 : c == 2 ? 4 : 0);
        }
    }
    stress->UpdateComponents();
    const std::array<int,9> components = {0,3,4,3,1,5,4,5,2};
    auto cut = manager.Evaluate("x", stress);
    auto resultant = MeshCutIntegration::Stress(cut, components, Graphic3d_Vec3d(0.0));
    near(resultant.Area, 0.28125, "Stress integration area");
    near(resultant.Force.x(), 0.9140625, "Integrated nodal stress force");
    near(resultant.Force.y(), 0.0, "Unexpected transverse force");
    near(resultant.Moment.y(), 0.24609375, "Stress moment quadrature");
    near(resultant.Moment.z(), -0.23291015625, "Stress moment quadrature");
    auto translated = MeshCutIntegration::Stress(cut, components, Graphic3d_Vec3d(0,1,0));
    near(translated.Moment.z(), resultant.Moment.z()+resultant.Force.x(), "Moment reference point ignored");
    auto scalarFlux = field("heat_flux", true, {1,1,5,1});
    auto flow = MeshCutIntegration::HeatFlux(manager.Evaluate("x", scalarFlux), {0,0,0});
    near(flow.HeatFlow, 0.5625, "Integrated heat flow");
    manager.SetPlane("x", Graphic3d_Vec4d(-1,0,0,0.25));
    auto reversed = MeshCutIntegration::Stress(manager.Evaluate("x", stress), components, Graphic3d_Vec3d(0.0));
    near(reversed.Force.x(), -resultant.Force.x(), "Plane normal not respected by resultant");
    bool rejected = false;
    try { MeshCutIntegration::Stress(cut, {9,3,4,3,1,5,4,5,2}, Graphic3d_Vec3d(0.0)); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Invalid stress mapping accepted");
}

static void isosurfaceCut()
{
    auto mesh = tetra();
    MeshCutViewManager manager(mesh);
    auto cutting = field("cut_x", true, {0,1,0,0});
    auto contour = field("temperature", true, {0,1,2,3});
    require(manager.AddIsosurface("iso", cutting, 0, 0.25), "Isosurface creation failed");
    require(!manager.AddIsosurface("iso", cutting, 0, 0.5), "Duplicate isosurface id accepted");
    require(!manager.SetPlane("iso", Graphic3d_Vec4d(1,0,0,0)), "Isosurface treated as plane");
    require(!manager.AddIsosurface("bad", cutting, 9, 0.5), "Invalid iso component accepted");
    require(!manager.SetIsoValue("iso", std::numeric_limits<double>::infinity()), "Infinite iso threshold accepted");
    require(!manager.AddIsosurface("bad", field("short", true, {1}), 0, 0.5), "Wrong iso tuple count accepted");
    require(!manager.AddIsosurface("bad", field("nan", true, {0,1,2,std::numeric_limits<double>::quiet_NaN()}), 0, 0.5),
        "Missing cutting values accepted");
    double minimum, maximum;
    require(manager.IsoRange("iso", minimum, maximum), "Iso range missing");
    near(minimum, 0, "Iso minimum"); near(maximum, 1, "Iso maximum");
    manager.SetVisibility("iso", 7);
    auto result = manager.Evaluate("iso", contour);
    auto section = result.Portions[1]->GetMeshData();
    require(section->NbIndexedElems() == 1 && section->NbNodes() == 3, "Iso triangle missing");
    near(volume(result.Portions[0]->GetMeshData()), (1-std::pow(0.75,3))/6, "Iso below volume");
    near(volume(result.Portions[2]->GetMeshData()), std::pow(0.75,3)/6, "Iso above volume");
    for (int n = 0; n < (int)section->NbNodes(); ++n) {
        const auto& p = section->Nodes()[n];
        near(p.x(), 0.25, "Iso geometry field changed");
        near(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),
            p.x()+2*p.y()+3*p.z(), "Iso contour interpolation");
        MeshInterpolatedNode origin;
        require(result.Portions[1]->OriginNode(n, origin), "Iso provenance missing");
        double sum = 0;
        for (const auto& base : origin.BaseNodes()) { sum += base.second; }
        near(sum, 1, "Iso provenance weights");
    }
    auto pressure = field("pressure", false, {7});
    auto constant = manager.Evaluate("iso", pressure);
    require(constant.Portions[1]->GetMeshData() == section, "Contour change rebuilt fixed iso geometry");
    near(constant.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)), 7, "Iso CellData constant");
    manager.SetCellMode("iso", MeshCutCellMode::Projected);
    auto projected = manager.Evaluate("iso", pressure);
    require(projected.Fields[1]->IsNodal(), "Iso projected contour not nodal");
    near(projected.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)), 7, "Iso CellData projected");
    for (unsigned mask = 0; mask < 8; ++mask) {
        manager.SetVisibility("iso", mask);
        const auto combination = manager.Evaluate("iso", contour);
        for (unsigned i = 0; i < 3; ++i) {
            require(combination.Portions[i].IsNull() == ((mask & (1u << i)) == 0), "Iso mask incorrect");
        }
    }
    manager.SetVisibility("iso", 7);
    require(manager.SetIsoValue("iso", 0.5), "Iso movement failed");
    near(volume(manager.Evaluate("iso").Portions[2]->GetMeshData()), 0.125/6, "Iso moved geometry stale");
    cutting->GetComponent(0)->SetValue(RWCae::MeshPropertyEntityInfo(1), 2);
    manager.InvalidateFields();
    auto updated = manager.Evaluate("iso", contour);
    for (const auto& p : updated.Portions[1]->GetMeshData()->Nodes()) { near(p.x(), 0.25, "Mutated iso field cache stale"); }
    require(manager.Duplicate("iso", "copy"), "Iso copy failed");
    require(manager.Definitions()[0].Shape == MeshCutShape::Isosurface, "Iso copy lost shape");
    require(manager.SetIsoValue("copy", 1), "Copied iso value failed");
    const auto json = manager.DumpDefinitionsJson();
    manager.Remove("copy");
    require(manager.RestoreDefinitionsJson(json) && manager.DumpDefinitionsJson() == json, "Iso JSON round trip failed");
    auto document = json;
    const auto pos = document.find("cut_x");
    require(pos != std::string::npos, "Iso binding not serialized");
    document.replace(pos, 5, "missing");
    require(!manager.RestoreDefinitionsJson(document) && manager.DumpDefinitionsJson() == json, "Unknown iso field load not atomic");
    document = json;
    const auto shapePos = document.find("isosurface");
    document.replace(shapePos, 10, "unknown");
    require(!manager.RestoreDefinitionsJson(document) && manager.DumpDefinitionsJson() == json, "Unknown cut shape accepted");
    const auto version1 = "{\"version\":1,\"cuts\":[{\"id\":\"x\",\"name\":\"X\",\"plane\":[1,0,0,-0.25],"
        "\"visibility\":7,\"enabled\":true,\"cellMode\":\"constant\"}]}";
    require(manager.RestoreDefinitionsJson(version1) && manager.Definitions()[0].Shape == MeshCutShape::Plane,
        "Version-one plane definitions not migrated");

    MeshCutViewManager cellManager(mesh);
    auto cells = field("cell_cut", false, {4});
    require(cellManager.AddIsosurface("cell", cells, 0, 3), "Elemental iso creation failed");
    cellManager.SetVisibility("cell", 7);
    auto allAbove = cellManager.Evaluate("cell");
    require(!allAbove.Roots[0]->AcceptElement(0) && allAbove.Roots[2]->AcceptElement(0), "Iso retained source cells wrong");
    require(allAbove.Portions[1]->GetMeshData()->NbNodes() == 0, "Outside iso not empty");
    require(cellManager.SetIsoValue("cell", 5), "Cell threshold failed");
    auto allBelow = cellManager.Evaluate("cell");
    require(allBelow.Roots[0]->AcceptElement(0) && !allBelow.Roots[2]->AcceptElement(0), "Iso sides did not reverse");

    require(manager.RestoreDefinitionsJson(json), "Iso restore after plane migration failed");
    bool rejected = false;
    try { MeshCutIntegration::HeatFlux(manager.Evaluate("iso", contour), {0,0,0}); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Iso integration silently used stale plane normal");
}

static void isosurfaceSurfaceAndCells()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0), Graphic3d_Vec3(1,0,0), Graphic3d_Vec3(0,1,0)}) { mesh->AddNodePosition(p); }
    const RWCae::MeshEntityId nodes[] = {0,1,2};
    mesh->AddElement(RWCae::MeshElementTriangle(nodes));
    Handle(ViewCae::InteractiveCaeCutView) view = new ViewCae::InteractiveCaeCutView(mesh);
    auto& manager = view->Cuts();
    require(manager.AddIsosurface("iso", field("cut_x", true, {0,1,0}), 0, 0.25), "Surface iso creation failed");
    manager.SetVisibility("iso", 7);
    auto result = manager.Evaluate("iso", field("temperature", true, {0,1,2}));
    auto slice = Handle(MeshGeometrySlice)::DownCast(result.Portions[1]);
    require(slice->Lines().size() == 1, "Iso surface line missing");
    for (auto n : slice->Lines()[0].Nodes) {
        const auto& p = slice->GetMeshData()->Nodes()[n];
        near(p.x(), 0.25, "Surface iso position");
        near(result.LineField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)), p.x()+2*p.y(), "Iso line nodal value");
    }
    result = manager.Evaluate("iso", field("pressure", false, {13}));
    near(result.LineField->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)), 13, "Iso line CellData");
    view->SetField(field("temperature", true, {0,1,2}));
    view->Synchronize();
    require(view->LineArrays().size() == 1 && view->LineArrays()[0]->HasVertexTexels(), "Iso line contour primitive missing");

    mesh = tetra();
    mesh->AddNodePosition(Graphic3d_Vec3(0,0,-1));
    const RWCae::MeshEntityId second[] = {0,2,1,4};
    mesh->AddElement(RWCae::MeshElementTetrahedron(second));
    MeshCutViewManager cells(mesh);
    require(cells.AddIsosurface("cell", field("cut_pressure", false, {2,6}), 0, 3), "Projected iso creation failed");
    cells.SetVisibility("cell", 7);
    auto cut = cells.Evaluate("cell", field("temperature", true, {0,1,2,3,-3}));
    require(cut.Portions[1]->GetMeshData()->NbNodes() == 3, "Projected elemental iso not generated");
    for (const auto& p : cut.Portions[1]->GetMeshData()->Nodes()) { near(p.z(), 0.5, "Equal-weight iso projection"); }
    near(volume(cut.Portions[0]->GetMeshData()), std::pow(0.5,3)/6, "Projected iso below volume");

    Handle(RWCae::MeshNVecProperty) vector = new RWCae::MeshNVecProperty("cell_vector", 3,
        RWCae::MeshPropertyBase::MappingType::Elemental, RWCae::MeshNVecPropertyType::Vector);
    vector->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 3; ++c) {
        auto values = Handle(RWCae::MeshScalarProperty)::DownCast(vector->GetComponent(c));
        values->Init(2);
        values->SetValue(RWCae::MeshPropertyEntityInfo(0), c == 0 ? 2 : 0);
        values->SetValue(RWCae::MeshPropertyEntityInfo(1), c == 0 ? -6 : 0);
    }
    vector->UpdateComponents();
    require(cells.AddIsosurface("magnitude", vector, 0, 3), "Vector magnitude iso rejected");
    const auto magnitude = cells.Evaluate("magnitude").Portions[1]->GetMeshData();
    for (const auto& p : magnitude->Nodes()) { near(p.z(), 0.5, "Iso magnitude projected components instead of scalar"); }
    require(magnitude->NbNodes() == 3, "Scalar magnitude projection vanished");
    require(cells.SetIsosurface("magnitude", vector, 1, 0), "Vector raw component binding rejected");
    for (const auto& p : cells.Evaluate("magnitude").Portions[1]->GetMeshData()->Nodes()) {
        near(p.z(), 0.5, "Wrong raw component binding");
    }
}

static void isosurfaceStepsAndDeformation()
{
    auto mesh = tetra();
    MeshCutViewManager manager(mesh);
    auto first = field("cut_x", true, {0,1,0,0});
    auto second = field("cut_x", true, {0,2,0,0});
    first->SetTime(0.5, 0); second->SetTime(1.5, 1);
    require(manager.AddIsosurface("step0", first, 0, 0.25), "Iso step-zero creation failed");
    require(manager.AddIsosurface("step1", second, 0, 0.25), "Iso step-one creation failed");
    auto step0 = manager.Evaluate("step0", second);
    for (const auto& p : step0.Portions[1]->GetMeshData()->Nodes()) { near(p.x(), 0.25, "Contour step moved fixed iso step"); }
    for (const auto& p : manager.Evaluate("step1", first).Portions[1]->GetMeshData()->Nodes()) {
        near(p.x(), 0.125, "Iso bound timestep ignored");
    }
    auto duplicateBinding = field("cut_x", true, {0,3,0,0});
    duplicateBinding->SetTime(0.5, 0);
    const auto json = manager.DumpDefinitionsJson();
    require(!manager.SetIsosurface("step0", duplicateBinding, 0, 0.25)
        && manager.DumpDefinitionsJson() == json, "Ambiguous binding changed definition");
    require(manager.RestoreDefinitionsJson(json), "Iso step JSON failed");

    Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("displacement", 3,
        RWCae::MeshPropertyBase::MappingType::Nodal, RWCae::MeshNVecPropertyType::Vector);
    displacement->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 3; ++c) {
        auto values = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c));
        values->Init(4);
        for (int n = 0; n < 4; ++n) { values->SetValue(RWCae::MeshPropertyEntityInfo(n), c == 0 ? 1 : 0); }
    }
    displacement->UpdateComponents();
    require(manager.SetDeformation(displacement, 1), "Iso deformation rejected");
    const double shift = manager.EffectiveDeformationScale();
    auto deformed = manager.Evaluate("step0", first);
    for (int n = 0; n < (int)deformed.Portions[1]->GetMeshData()->NbNodes(); ++n) {
        const auto& p = deformed.Portions[1]->GetMeshData()->Nodes()[n];
        near(p.x(), 0.25+shift, "Iso ignores deformed geometry");
        near(deformed.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)), 0.25, "Deformed iso field not interpolated");
    }
    require(manager.SetDeformation(nullptr, 0), "Iso deformation reset rejected");
    for (const auto& p : manager.Evaluate("step0").Portions[1]->GetMeshData()->Nodes()) { near(p.x(), 0.25, "Iso deformation reset stale"); }
}

static void isosurfaceBoundaryAndQuadratic()
{
    auto mesh = tetra();
    mesh->AddNodePosition(Graphic3d_Vec3(0,0,-1));
    const RWCae::MeshEntityId second[] = {0,2,1,4};
    mesh->AddElement(RWCae::MeshElementTetrahedron(second));
    MeshCutViewManager manager(mesh);
    require(manager.AddIsosurface("boundary", field("z", true, {0,0,0,1,-1}), 0, 0), "Boundary iso failed");
    manager.SetVisibility("boundary", 7);
    auto result = manager.Evaluate("boundary");
    near(volume(result.Portions[0]->GetMeshData()), 1.0/6, "Iso coincident-face lower volume");
    near(volume(result.Portions[2]->GetMeshData()), 1.0/6, "Iso coincident-face upper volume");
    require(result.Portions[1]->GetMeshData()->NbNodes() >= 3, "Iso coincident face lost");

    Handle(RWCae::MeshData) quadratic = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0), Graphic3d_Vec3(1,0,0), Graphic3d_Vec3(0,1,0),
                         Graphic3d_Vec3(0.5,0,0), Graphic3d_Vec3(0.5,0.5,0), Graphic3d_Vec3(0,0.5,0)}) {
        quadratic->AddNodePosition(p);
    }
    const RWCae::MeshEntityId nodes[] = {0,1,2,3,4,5};
    quadratic->AddElement(RWCae::MeshElementTriangle2nd(nodes));
    MeshCutViewManager highOrder(quadratic);
    require(!highOrder.AddIsosurface("unsupported", field("x", true, {0,1,0,0.5,0.5,0}), 0, 0.25)
        && highOrder.Definitions().empty(), "Quadratic iso silently used linear interpolation");
}

static void defaultPlanes()
{
    MeshCutViewManager manager(tetra());
    require(manager.EnsureDefaultPlanes(), "Default planes failed");
    const auto definitions = manager.Definitions();
    require(definitions.size() == 3 && manager.EvaluateVisible().empty(), "Default planes unexpectedly clip the source");
    for (int axis = 0; axis < 3; ++axis) {
        require(!definitions[axis].Enabled && definitions[axis].Shape == MeshCutShape::Plane, "Default plane state wrong");
        near(definitions[axis].Plane[axis], 1, "Default plane normal");
        near(definitions[axis].Plane.w(), -0.5, "Default plane center");
    }
    manager.SetEnabled("y", true);
    manager.SetPlane("y", Graphic3d_Vec4d(0,1,0,-0.25));
    const auto snapshot = manager.DumpDefinitionsJson();
    require(manager.EnsureDefaultPlanes() && manager.DumpDefinitionsJson() == snapshot, "Defaults overwrote existing cuts");
    manager.Clear();
    require(manager.EnsureDefaultPlanes() && manager.Definitions().size() == 3, "Defaults not recreated after clear");
    Handle(RWCae::MeshData) empty = new RWCae::MeshData();
    MeshCutViewManager emptyManager(empty);
    require(!emptyManager.EnsureDefaultPlanes() && emptyManager.Definitions().empty(), "Empty mesh produced invalid planes");
}

int main()
{
    try {
        volumeCut();
        surfaceCut();
        projectedCells();
        displayPrimitives();
        definitionsAndDeformation();
        resultants();
        isosurfaceCut();
        isosurfaceSurfaceAndCells();
        isosurfaceStepsAndDeformation();
        isosurfaceBoundaryAndQuadratic();
        defaultPlanes();
        std::cout << "Cut view numerical tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
