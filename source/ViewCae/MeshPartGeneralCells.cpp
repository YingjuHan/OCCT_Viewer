#include <ViewCae/MeshPartGeneralCells.h>
#include <ViewCae/MeshPresentationScalar.h>
#include <ViewCae/CaeMeshTools.h>
#ifdef OCCTV_USE_VTK
#include <ViewCaeGeom/MeshVtkCutSource.h>
#endif
#include <Graphic3d_Group.hxx>
#include <Image_SupportedFormats.hxx>
#include <algorithm>
#include <cmath>

namespace ViewCae {
std::vector<MeshPartGeneralCells::Primitive> MeshPartGeneralCells::BuildPrimitives(
    const Handle(ViewCaeGeom::MeshGeometry)& geometry)
{
    std::vector<Primitive> result;
#ifdef OCCTV_USE_VTK
    const auto source = geometry->GetLastNotModifierChild();
    const auto mesh = source->GetMeshData();
    if (mesh.IsNull() || mesh->GenericCells().empty()) { return result; }
    ViewCaeGeom::MeshVtkCutSource prepared(0, ViewCaeGeom::MeshCutGeometryOptions());
    prepared.Prepare(mesh, source.get());
    for (RWCae::MeshEntityId index = 0; index < mesh->NbIndexedElems(); ++index) {
        if (mesh->IndexedElements()[index].Type != RWCae::MeshElemType::Generic
            || !source->AcceptElement(index)) { continue; }
        const auto& cell = prepared.GetCell(index);
        auto add = [&](const auto& positions) {
            Primitive primitive;
            primitive.Element = index;
            primitive.Dimension = cell.Geometry->Dimension();
            primitive.Size = (int)positions.size();
            for (int n = 0; n < primitive.Size; ++n) {
                primitive.Vertices[n].Position = positions[n];
                primitive.Vertices[n].Weights = cell.Geometry->Origin(positions[n]);
            }
            result.push_back(std::move(primitive));
        };
        for (size_t t = 0; t < cell.Geometry->Triangles.size(); ++t) {
            bool hidden = false;
            for (size_t f = 0; f < cell.Faces.size(); ++f) {
                const auto& face = cell.Geometry->Faces[f];
                if (cell.Faces[f].Internal && t >= face.FirstTriangle
                    && t < face.FirstTriangle + face.NbTriangles) { hidden = true; break; }
            }
            if (!hidden) { add(cell.Geometry->Triangles[t]); }
        }
        for (const auto& segment : cell.Geometry->Segments) { add(segment); }
        for (const auto& point : cell.Geometry->Points) { add(std::array<Graphic3d_Vec3d, 1>{point}); }
    }
#else
    (void)geometry;
#endif
    return result;
}

double MeshPartGeneralCells::Value(const Primitive& primitive, int vertex,
    const RWCae::MeshPropertyComponent& field)
{
    if (!field.IsDefined()) { return std::numeric_limits<double>::quiet_NaN(); }
    const auto component = field.GetScalarComp();
    if (field.Property->IsElemental() && !field.Property->HasNodeVecComponents()) {
        return component->Value(RWCae::MeshPropertyEntityInfo(primitive.Element));
    }
    double value = 0;
    for (const auto& node : primitive.Vertices.at(vertex).Weights.BaseNodes()) {
        value += node.second * component->Value(RWCae::MeshPropertyEntityInfo(
            field.Property->IsNodal() ? node.first : primitive.Element,
            field.Property->HasNodeVecComponents() ? (int)node.first : -1));
    }
    return value;
}

void MeshPartGeneralCells::update(const Handle(MeshPresentation)& presentation)
{
    const auto scalar = Handle(MeshPresentationScalar)::DownCast(presentation);
    const auto options = scalar->ScalarVisProps();
    const auto primitives = BuildPrimitives(presentation->GetMeshGeometry());
    const auto field = presentation->PropertyData()->Property(presentation->ActiveProperty());
    const auto mapper = presentation->PropertyData()->ColorMapper(field);
    Handle(Image_PixMap) colors;
    double minimum = 0, maximum = 1;
    if (!mapper.IsNull()) {
        field.GetScalarComp()->ValueRange().GetBounds(minimum, maximum);
        colors = mapper->ColorTexture()->GetImage(new Image_SupportedFormats());
    }
    decltype(myArrays) arrays;
    std::vector<int> dimensions;
    for (int dimension = 0; dimension <= 3; ++dimension) {
        if ((dimension == 0 && !options->ToShow0d())
            || (dimension == 1 && !options->ToShow1d())
            || (dimension == 2 && !options->ToShow2d())
            || (dimension == 3 && !options->ToShow3d())) { continue; }
        int count = 0;
        for (const auto& primitive : primitives) {
            if (primitive.Dimension == dimension) { count += primitive.Size; }
        }
        if (!count) { continue; }
        const auto flags = colors.IsNull() ? Graphic3d_ArrayFlags_None : Graphic3d_ArrayFlags_VertexColor;
        Handle(Graphic3d_ArrayOfPrimitives) array;
        if (dimension == 0) { array = new Graphic3d_ArrayOfPoints(count, flags); }
        else if (dimension == 1) { array = new Graphic3d_ArrayOfSegments(count, 0, flags); }
        else { array = new Graphic3d_ArrayOfTriangles(count, 0, flags | Graphic3d_ArrayFlags_VertexNormal); }
        for (const auto& primitive : primitives) {
            if (primitive.Dimension != dimension) { continue; }
            auto normal = dimension > 1 ? Graphic3d_Vec3d::Cross(
                primitive.Vertices[1].Position - primitive.Vertices[0].Position,
                primitive.Vertices[2].Position - primitive.Vertices[0].Position) : Graphic3d_Vec3d(0);
            if (normal.Modulus() > 0) { normal.Normalize(); }
            for (int n = 0; n < primitive.Size; ++n) {
                const auto& p = primitive.Vertices[n].Position;
                const int vertex = array->AddVertex(Graphic3d_Vec3(p));
                if (dimension > 1) { array->SetVertexNormal(vertex, normal.x(), normal.y(), normal.z()); }
                if (!colors.IsNull()) {
                    const auto uv = CaeMeshTools::GetUVCoords(Value(primitive, n, field),
                        minimum, maximum, CaeMeshTools::NbColorscaleColors);
                    const int column = std::clamp((int)(uv.x() * colors->SizeX()), 0, (int)colors->SizeX() - 1);
                    array->SetVertexColor(vertex, colors->PixelColor(column, 0).GetRGB());
                }
            }
        }
        arrays.push_back(array); dimensions.push_back(dimension);
        if (dimension > 1 && (options->PresentationMode() == InteractiveCaeMeshPrsMode::MeshEdges
            || options->PresentationMode() == InteractiveCaeMeshPrsMode::ShadedMeshEdges)) {
            Handle(Graphic3d_ArrayOfSegments) edges = new Graphic3d_ArrayOfSegments(count * 2);
            for (const auto& primitive : primitives) {
                if (primitive.Dimension != dimension) { continue; }
                for (int n = 0; n < 3; ++n) {
                    edges->AddVertex(Graphic3d_Vec3(primitive.Vertices[n].Position));
                    edges->AddVertex(Graphic3d_Vec3(primitive.Vertices[(n + 1) % 3].Position));
                }
            }
            arrays.push_back(edges); dimensions.push_back(dimension + 4);
        }
    }
    myArrays = std::move(arrays); myDimensions = std::move(dimensions);
}

void MeshPartGeneralCells::compute(const Handle(AIS_InteractiveContext)&,
    const Handle(InteractiveCaeMeshDrawer)& drawer, const Handle(Prs3d_Presentation)& presentation,
    const Handle(Prs3d_Presentation)&, float) const
{
    for (size_t i = 0; i < myArrays.size(); ++i) {
        auto group = presentation->NewGroup();
        Handle(Graphic3d_Aspects) aspect;
        if (myDimensions[i] == 0) {
            aspect = new Graphic3d_AspectMarker3d(Aspect_TOM_O_POINT,
                drawer->PointAspect()->Aspect()->Color(), drawer->PointAspect()->Aspect()->Scale());
        } else if (myDimensions[i] > 3) {
            aspect = myDimensions[i] == 7 ? drawer->LineAspect3d() : drawer->LineAspect2d();
        } else if (myDimensions[i] == 1) {
            aspect = new Graphic3d_AspectLine3d(drawer->LineAspect2d()->Color(), Aspect_TOL_SOLID, 2.0);
        } else {
            Handle(Graphic3d_AspectFillArea3d) fill = new Graphic3d_AspectFillArea3d();
            *fill = *(myDimensions[i] == 3 ? drawer->FillAreaAspect3d() : drawer->FillAreaAspect2d());
            fill->SetTextureMapOff();
            aspect = fill;
        }
        group->SetGroupPrimitivesAspect(aspect);
        group->AddPrimitiveArray(myArrays[i]);
    }
}
}
