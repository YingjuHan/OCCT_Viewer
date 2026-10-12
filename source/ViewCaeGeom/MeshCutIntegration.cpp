#include <ViewCaeGeom/MeshCutIntegration.h>
#include <RWCae/MeshAdaptorsIterator.h>
#ifdef OCCTV_USE_VTK
#include <vtkGenericCell.h>
#include <vtkSmartPointer.h>
#endif

#include <cmath>
#include <stdexcept>

namespace {
bool isVolume(const Handle(RWCae::MeshData)& mesh, RWCae::MeshEntityId index)
{
    const auto& cell = mesh->IndexedElements()[index];
    if (cell.Type == RWCae::MeshElemType::Generic) {
#ifdef OCCTV_USE_VTK
        auto concrete = vtkSmartPointer<vtkCell>::Take(vtkGenericCell::InstantiateCell(mesh->GenericCells().at(cell.Index).Type));
        return concrete && concrete->GetCellDimension() == 3;
#else
        return false;
#endif
    }
    return (int)cell.Type >= RWCae::MeshElem3dLowerType && (int)cell.Type <= RWCae::MeshElem3dUpperType;
}
template<size_t N, class Evaluate>
ViewCaeGeom::MeshCutResultant integrate(const ViewCaeGeom::MeshCutResult& cut,
    const std::array<int, N>& components, Evaluate evaluate)
{
    if (cut.PresentationOnly) { throw std::invalid_argument("Physical integration requires an independent numerical cut"); }
    if (cut.Definition.Shape != ViewCaeGeom::MeshCutShape::Plane) {
        throw std::invalid_argument("Non-planar section integration requires local-normal mapping");
    }
    constexpr size_t on = (size_t)ViewCaeGeom::MeshCutPortion::On;
    if (cut.Portions[on].IsNull() || cut.Roots[on].IsNull() || cut.Fields[on].IsNull()) {
        throw std::invalid_argument("Integration requires a visible section and a result field");
    }
    const auto field = !cut.PhysicalFields[on].IsNull() ? cut.PhysicalFields[on] : cut.Fields[on];
    if (cut.DisplayRecovered && cut.PhysicalFields[on].IsNull()) {
        throw std::invalid_argument("Physical integration cannot use display-recovered fields");
    }
    for (int c : components) {
        if (c < 0 || c >= field->NbComponents()) { throw std::invalid_argument("Invalid integration component mapping"); }
    }
    const auto mesh = cut.Portions[on]->GetMeshData();
    const auto source = cut.Roots[on]->GetMeshData();
    if (mesh.IsNull() || source.IsNull()) { throw std::invalid_argument("Integration requires section and source mesh data"); }
    const auto& plane = cut.Definition.Plane;
    const Graphic3d_Vec3d normal(plane.x(), plane.y(), plane.z());
    ViewCaeGeom::MeshCutResultant result;
    // Degree-two triangular quadrature integrates moments of linear nodal stress exactly.
    constexpr double weights[3][3] = {{2.0/3,1.0/6,1.0/6},{1.0/6,2.0/3,1.0/6},{1.0/6,1.0/6,2.0/3}};
    auto process = [&](const auto& adaptor) {
        for (RWCae::MeshEntityId e = 0; e < adaptor.NbElements(); ++e) {
            const auto& elem = adaptor.GetElement(e);
            RWCae::MeshEntityId origin = 0;
            if (!cut.Portions[on]->OriginElement(ViewCaeGeom::MeshGeomElem(e, adaptor.GetElementType(), false), origin)
                || origin >= source->NbIndexedElems()
                || !isVolume(source,origin)) {
                throw std::invalid_argument("Only volume-cell stress/flux sections can be integrated without section mapping");
            }
            for (RWCae::MeshElemEntityId i = 1; i+1 < elem.NbNodes(); ++i) {
                const RWCae::MeshEntityId nodes[] = {elem[0], elem[i], elem[i+1]};
                Graphic3d_Vec3d positions[3] = {Graphic3d_Vec3d(mesh->Nodes()[nodes[0]]),
                    Graphic3d_Vec3d(mesh->Nodes()[nodes[1]]), Graphic3d_Vec3d(mesh->Nodes()[nodes[2]])};
                const double area = Graphic3d_Vec3d::Cross(positions[1]-positions[0], positions[2]-positions[0]).Modulus()/2;
                if (area == 0.0) { continue; }
                result.Area += area;
                for (const auto& w : weights) {
                    std::array<double, N> values{};
                    Graphic3d_Vec3d position(0.0);
                    for (int n = 0; n < 3; ++n) {
                        position += positions[n]*w[n];
                        const RWCae::MeshPropertyEntityInfo entity(field->IsNodal() ? nodes[n] : elem.GlobalIndex(),
                            field->HasNodeVecComponents() ? (int)nodes[n] : -1);
                        for (size_t c = 0; c < N; ++c) {
                            const double value = field->GetComponent(components[c])->Value(entity);
                            if (!std::isfinite(value)) { throw std::invalid_argument("Integration field has missing or nonfinite values"); }
                            values[c] += value*w[n];
                        }
                    }
                    evaluate(result, normal, position, values, area/3);
                }
            }
        }
    };
    RWCae::MeshAdaptorsIterator2dElems::For(process, mesh);
    auto slice = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(cut.Portions[on]);
    if (!slice.IsNull() && !slice->Lines().empty()) {
        throw std::invalid_argument("Shell/beam section fields require explicit section mapping");
    }
    return result;
}
}

namespace ViewCaeGeom {
MeshCutResultant MeshCutIntegration::Stress(const MeshCutResult& cut,
    const std::array<int, 9>& components, const Graphic3d_Vec3d& momentOrigin)
{
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(momentOrigin[i])) { throw std::invalid_argument("Moment origin must be finite"); }
    }
    return integrate(cut, components, [&momentOrigin](MeshCutResultant& result, const Graphic3d_Vec3d& normal,
        const Graphic3d_Vec3d& position, const std::array<double, 9>& values, double weight) {
        Graphic3d_Vec3d traction(0.0);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) { traction[i] += values[3*i+j]*normal[j]; }
        }
        result.Force += traction*weight;
        result.Moment += Graphic3d_Vec3d::Cross(position-momentOrigin, traction)*weight;
    });
}

MeshCutResultant MeshCutIntegration::HeatFlux(const MeshCutResult& cut, const std::array<int, 3>& components)
{
    return integrate(cut, components, [](MeshCutResultant& result, const Graphic3d_Vec3d& normal,
        const Graphic3d_Vec3d&, const std::array<double, 3>& values, double weight) {
        result.HeatFlow += Graphic3d_Vec3d(values[0], values[1], values[2]).Dot(normal)*weight;
    });
}
}
