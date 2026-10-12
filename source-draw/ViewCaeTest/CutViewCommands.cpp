#include <ViewCaeTest/CutViewCommands.h>
#include <ViewCae/InteractiveCaeCutView.h>
#include <ViewCae/InteractiveCaeCutPlane.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <RWVtk/VtkLegacyReader.h>
#include <RWVtk/VtkPolyDataReader.h>
#include <RWVtk/VtkSeriesReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>
#ifdef OCCTV_USE_VTK
#include <RWVtk/VtkNativeReader.h>
#endif
#include <ViewerTest.hxx>
#include <ViewerTest_EventManager.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <gp_Ax1.hxx>
#include <Draw.hxx>
#include <Image_AlienPixMap.hxx>

#include <cmath>
#include <map>
#include <stdexcept>
#include <set>
#include <limits>

namespace {
int cutInteger(const char* argument)
{
    Standard_Real value;
    if (!Draw::ParseReal(argument, value) || !std::isfinite(value) || std::trunc(value) != value
        || value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
        throw std::invalid_argument(std::string("Invalid integer: ") + argument);
    }
    return (int)value;
}

double cutReal(const char* argument)
{
    Standard_Real value;
    if (!Draw::ParseReal(argument, value) || !std::isfinite(value)) {
        throw std::invalid_argument(std::string("Invalid finite number: ") + argument);
    }
    return value;
}

bool cutBoolean(const char* argument)
{
    const int value = cutInteger(argument);
    if (value != 0 && value != 1) { throw std::invalid_argument("Boolean value must be 0 or 1"); }
    return value != 0;
}

struct CutModel {
    uint64_t Generation = 0;
    std::string Source;
    Handle(RWCae::MeshData) Mesh;
    Handle(ViewCae::InteractiveCaeCutView) View;
    Handle(RWCae::MeshNVecProperty) Field;
    int Component = 0;
    Bnd_Box Bounds;
    Handle(ViewCae::InteractiveCaeCutPlane) Plane;
    Handle(ViewCae::InteractiveCaeCutQuadric) Quadric;
    Handle(AIS_Manipulator) Manipulator;
    std::string EditedCut;
};
std::map<std::string, CutModel> models;
uint64_t nextGeneration = 0;

void cancelHandleDrag(CutModel& model)
{
    if ((!model.Plane.IsNull() && model.Plane->IsDragging())
        || (!model.Quadric.IsNull() && model.Quadric->IsDragging())
        || (!model.Manipulator.IsNull() && model.Manipulator->HasActiveTransformation())) {
        const auto event = ViewerTest::CurrentEventManager();
        const auto context = ViewerTest::GetAISContext();
        const auto view = ViewerTest::CurrentView();
        if (!event.IsNull() && !context.IsNull() && !view.IsNull()) {
            event->ResetViewInput();
            event->FlushViewEvents(context, view, true);
        }
    }
}

void removeHandle(CutModel& model)
{
    cancelHandleDrag(model);
    if (!model.Manipulator.IsNull()) { model.Manipulator->Detach(); model.Manipulator.Nullify(); }
    if (!model.Plane.IsNull() && !model.Plane->GetContext().IsNull()) {
        model.Plane->GetContext()->Remove(model.Plane, false);
    }
    model.Plane.Nullify();
    if (!model.Quadric.IsNull() && !model.Quadric->GetContext().IsNull()) {
        model.Quadric->GetContext()->Remove(model.Quadric,false);
    }
    model.Quadric.Nullify();
    model.EditedCut.clear();
}

void refreshHandle(CutModel& model)
{
    if ((model.Plane.IsNull() && model.Quadric.IsNull()) || (!model.Plane.IsNull() && model.Plane->IsDragging())
        || (!model.Quadric.IsNull() && model.Quadric->IsDragging())
        || (!model.Manipulator.IsNull() && model.Manipulator->HasActiveTransformation())) { return; }
    if (!model.Plane.IsNull()) {
        if (!model.Plane->Refresh()) { removeHandle(model); return; }
        auto manipulator = Handle(ViewCae::InteractiveCaeCutManipulator)::DownCast(model.Manipulator);
        if (!manipulator.IsNull()) { manipulator->Refresh(); }
        if (!model.Plane->GetContext().IsNull()) { model.Plane->GetContext()->Redisplay(model.Plane,false); }
    } else {
        if (!model.Quadric->Refresh()) { removeHandle(model); return; }
        auto manipulator = Handle(ViewCae::InteractiveCaeCutQuadricManipulator)::DownCast(model.Manipulator);
        if (!manipulator.IsNull()) { manipulator->Refresh(); }
        if (!model.Quadric->GetContext().IsNull()) { model.Quadric->GetContext()->Redisplay(model.Quadric,false); }
    }
    if (!ViewerTest::CurrentEventManager().IsNull()) { ViewerTest::CurrentEventManager()->ResetPreviousMoveTo(); }
}

Standard_Integer imageStats(Draw_Interpretor& di, Standard_Integer argc, const char** argv)
{
    if (argc != 2) { di << "scvcutimagestats filename"; return 1; }
    Image_AlienPixMap image;
    if (!image.Load(argv[1])) { di << "Cannot load screenshot"; return 1; }
    size_t visible = 0, colored = 0;
    uint64_t checksum = 14695981039346656037ull;
    std::set<unsigned> colors;
    for (size_t y = 0; y < image.SizeY(); ++y) {
        for (size_t x = 0; x < image.SizeX(); ++x) {
            auto color = image.PixelColor((int)x, (int)y).GetRGB();
            const unsigned r = (unsigned)std::lround(color.Red()*255);
            const unsigned g = (unsigned)std::lround(color.Green()*255);
            const unsigned b = (unsigned)std::lround(color.Blue()*255);
            const unsigned min = std::min({r,g,b}), max = std::max({r,g,b});
            for (unsigned channel : {r,g,b}) { checksum = (checksum ^ channel) * 1099511628211ull; }
            if (min < 245) { ++visible; colors.insert((r << 16) | (g << 8) | b); }
            if (max - min > 30) { ++colored; }
        }
    }
    di << "visible " << (int)visible << " colored " << (int)colored << " colors " << (int)colors.size()
       << " checksum " << std::to_string(checksum).c_str();
    return 0;
}

double measure(const Handle(RWCae::MeshData)& mesh, const Handle(ViewCaeGeom::MeshGeometry)& filter, bool volume)
{
    double result = 0.0;
    auto compute = [&](const auto& adaptor) {
        for (RWCae::MeshEntityId i = 0; i < adaptor.NbElements(); ++i) {
            const auto& elem = adaptor.GetElement(i);
            if (!filter.IsNull() && !filter->AcceptElement(elem.GlobalIndex())) { continue; }
            Graphic3d_Vec3d center(0,0,0);
            for (RWCae::MeshElemEntityId n = 0; n < elem.NbNodes(); ++n) { center += Graphic3d_Vec3d(mesh->Nodes()[elem[n]]); }
            center /= elem.NbNodes();
            for (RWCae::MeshElemEntityId f = 0; f < adaptor.NbFaces(elem); ++f) {
                std::vector<RWCae::MeshEntityId> nodes;
                RWCae::MeshElemType type;
                adaptor.FaceNodes(elem, f, nodes, type);
                const auto a = Graphic3d_Vec3d(mesh->Nodes()[nodes[0]]) - center;
                for (size_t n = 1; n + 1 < nodes.size(); ++n) {
                    const auto b = Graphic3d_Vec3d(mesh->Nodes()[nodes[n]]) - center;
                    const auto c = Graphic3d_Vec3d(mesh->Nodes()[nodes[n+1]]) - center;
                    const auto cross = Graphic3d_Vec3d::Cross(b, c);
                    result += volume ? std::abs(a.Dot(cross))/6.0 : Graphic3d_Vec3d::Cross(b-a,c-a).Modulus()/2.0;
                }
            }
        }
    };
    if (volume) { RWCae::MeshAdaptorsIterator3dElems::For(compute, mesh); }
    else { RWCae::MeshAdaptorsIterator2dElems::For(compute, mesh); }
    return result;
}

Standard_Integer cutView(Draw_Interpretor& di, Standard_Integer argc, const char** argv)
{
    try {
        if (argc < 3) { throw std::invalid_argument("scvcutview name action arguments"); }
        const std::string name(argv[1]), action(argv[2]);
        if ((action == "read" || action == "replace") && argc == 4) {
            if (action == "read" && models.count(name)) { throw std::invalid_argument("Cut model already exists"); }
            const std::string path(argv[3]);
            const auto ends = [&path](const std::string& ext) { return path.size() >= ext.size() && path.compare(path.size()-ext.size(), ext.size(), ext) == 0; };
            CutModel model;
            if (ends(".vtm.series")) {
                RWVtk::VtkSeriesReader reader;
                if (!reader.Perform(argv[3])) { throw std::runtime_error("VTK series import failed"); }
                model.Mesh = reader.GetMergedMeshData();
            } else if (ends(".vtm")) {
                RWVtk::VtkMultiBlockDataReader reader;
                if (!reader.Perform(argv[3])) { throw std::runtime_error("VTK composite import failed"); }
                model.Mesh = reader.GetMergedMeshData();
            } else if (ends(".vtk")) {
                RWVtk::VtkLegacyReader reader;
                if (reader.Perform(argv[3])) { model.Mesh = reader.GetMeshData(); }
            } else if (ends(".vtp")) {
                RWVtk::VtkPolyDataReader reader;
                if (reader.Perform(argv[3])) { model.Mesh = reader.GetMeshData(); }
            } else {
                RWVtk::VtkUnstructuredGridReader reader;
                if (reader.Perform(argv[3])) { model.Mesh = reader.GetMeshData(); }
            }
#ifdef OCCTV_USE_VTK
            if (model.Mesh.IsNull()) {
                RWVtk::VtkNativeReader reader;
                if (!reader.Perform(argv[3])) {
                    throw std::runtime_error(reader.Report().errors.empty() ? "Native VTK import failed" : reader.Report().errors.front());
                }
                model.Mesh = reader.GetMeshData();
            }
#endif
            if (model.Mesh.IsNull()) { throw std::runtime_error("VTK import failed; general cells require OCCTV_USE_VTK"); }
            model.View = new ViewCae::InteractiveCaeCutView(model.Mesh);
            for (RWCae::MeshEntityId n = 0; n < model.Mesh->NbNodes(); ++n) {
                const auto& p = model.Mesh->Nodes()[n];
                model.Bounds.Add(gp_Pnt(p.x(), p.y(), p.z()));
            }
            model.Source = path;
            const auto old = models.find(name);
            bool displayed = false;
            if (old != models.end()) {
                removeHandle(old->second);
                const auto context = ViewerTest::GetAISContext();
                if (!context.IsNull()) {
                    displayed = context->IsDisplayed(old->second.View);
                    context->Remove(old->second.View, false);
                }
            }
            model.Generation = ++nextGeneration;
            models[name] = model;
            if (displayed) {
                model.View->Synchronize();
                ViewerTest::Display(name.c_str(), model.View, true);
            }
            di << "points " << (int)model.Mesh->NbNodes() << " cells " << (int)model.Mesh->NbIndexedElems()
               << " fields " << model.Mesh->Properties().Size();
            return 0;
        }
        auto found = models.find(name);
        if (found == models.end()) { throw std::invalid_argument("Unknown cut model"); }
        auto& model = found->second;
        auto& cuts = model.View->Cuts();
        bool ok = false;
        if (action == "info" && argc == 3) {
            di << "generation " << std::to_string(model.Generation).c_str() << " source";
            di.AppendElement(model.Source.c_str());
            di << " points " << (int)model.Mesh->NbNodes() << " cells " << (int)model.Mesh->NbIndexedElems();
            return 0;
        } else if (action == "defaults" && argc == 3) {
            ok = cuts.EnsureDefaultPlanes();
        } else if (action == "list" && argc == 3) {
            for (const auto& cut : cuts.Definitions()) {
                di << "{id"; di.AppendElement(cut.Id.c_str());
                di << " name"; di.AppendElement(cut.Name.c_str());
                di << " enabled " << (int)cut.Enabled << " visibility " << (int)cut.Visibility;
                if (cut.Shape == ViewCaeGeom::MeshCutShape::Plane) {
                    di << " shape plane plane {" << cut.Plane.x() << " " << cut.Plane.y() << " "
                       << cut.Plane.z() << " " << cut.Plane.w() << "}";
                } else if (cut.Shape == ViewCaeGeom::MeshCutShape::Sphere || cut.Shape == ViewCaeGeom::MeshCutShape::Cylinder) {
                    di << " shape " << (cut.Shape == ViewCaeGeom::MeshCutShape::Sphere ? "sphere" : "cylinder")
                       << " center {" << cut.Center.x() << " " << cut.Center.y() << " " << cut.Center.z()
                       << "} axis {" << cut.Axis.x() << " " << cut.Axis.y() << " " << cut.Axis.z()
                       << "} radius " << cut.Radius;
                } else {
                    di << " shape isosurface field"; di.AppendElement(cut.IsoFieldName.c_str());
                    di << " mapping " << (cut.IsoFieldNodal ? "point" : "cell") << " component " << cut.IsoComponent
                       << " step " << cut.IsoTimeStep << " value " << cut.IsoValue;
                }
                di << " cellmode " << (cut.CellMode == ViewCaeGeom::MeshCutCellMode::Constant ? "constant" : "projected")
                   << " selected " << (int)(model.EditedCut == cut.Id) << "} ";
            }
            return 0;
        } else if (action == "stats" && argc == 3) {
            const auto work = cuts.Statistics();
            auto value = [&](const char* key, uint64_t number) { di << key << " " << std::to_string(number).c_str() << " "; };
            value("epoch",work.SourceEpoch); value("preparations",work.SourcePreparations);
            value("preparedCells",work.PreparedCells); value("shapeBuilds",work.ShapeBuilds);
            value("bvhBuilds",work.BvhBuilds); value("boxTests",work.BoxTests);
            value("candidates",work.CandidateCells); value("booleans",work.BooleanOperations);
            value("filteredFaces",work.FilteredFaces); value("unmergedGroups",work.UnmergedFaceGroups);
            value("sourcePrimitives",work.SourcePrimitives); value("faceDepth",work.FaceSubdivisionDepth);
            value("analyticCells",work.AnalyticCells); value("numericalCells",work.NumericalCells);
            di << "analyticResidual " << work.AnalyticResidual << " ";
            di << "geometryTolerance " << work.GeometryTolerance << " storageResolution " << work.StorageResolution;
            return 0;
        } else if (action == "isooptions" && argc == 3) {
            const auto& options = cuts.IsoOptions();
            di << "absolute " << options.AbsoluteTolerance << " relative " << options.RelativeTolerance
               << " minimumDepth " << options.MinimumDepth << " limit " << options.SubdivisionLimit
               << " primitives " << std::to_string(options.MaxPrimitives).c_str(); return 0;
        } else if (action == "isooptions" && argc == 8) {
            auto options = cuts.IsoOptions();
            options.AbsoluteTolerance = cutReal(argv[3]); options.RelativeTolerance = cutReal(argv[4]);
            options.MinimumDepth = cutInteger(argv[5]); options.SubdivisionLimit = cutInteger(argv[6]);
            const auto limit = cutInteger(argv[7]); if (limit < 1) { throw std::invalid_argument("Isosurface budget must be positive"); }
            options.MaxPrimitives = (uint64_t)limit; ok = cuts.SetIsoOptions(options);
        } else if (action == "geometry" && argc == 3) {
            const auto& options = cuts.GeometryOptions();
            di << "absolute " << options.AbsoluteTolerance << " relative " << options.RelativeTolerance
               << " faceLimit " << options.FaceSubdivisionLimit << " lineLimit " << options.LineSubdivisionLimit
               << " primitives " << std::to_string(options.MaxSourcePrimitives).c_str();
            return 0;
        } else if (action == "geometry" && argc == 8) {
            auto options = cuts.GeometryOptions();
            options.AbsoluteTolerance = cutReal(argv[3]); options.RelativeTolerance = cutReal(argv[4]);
            options.FaceSubdivisionLimit = cutInteger(argv[5]); options.LineSubdivisionLimit = cutInteger(argv[6]);
            const auto limit = cutInteger(argv[7]);
            if (limit < 1) { throw std::invalid_argument("Source primitive limit must be positive"); }
            options.MaxSourcePrimitives = (uint64_t)limit;
            ok = cuts.SetGeometryOptions(options);
        } else if (action == "geometrymode" && (argc == 3 || argc == 4)) {
            const char* modes[] = {"numerical","preferred","required"};
            if (argc == 3) { di << modes[(int)cuts.GeometryOptions().Mode]; return 0; }
            auto options = cuts.GeometryOptions();
            for (int mode = 0; mode < 3; ++mode) {
                if (std::string(argv[3]) == modes[mode]) { options.Mode = (ViewCaeGeom::MeshCutGeometryMode)mode; ok = cuts.SetGeometryOptions(options); break; }
            }
        } else if (action == "presentation" && argc == 4) {
            const auto& revisions = model.View->SynchronizedRevisions();
            const auto revision = revisions.find(argv[3]);
            di << "synchronizations " << (int)model.View->SynchronizationCount()
               << " revision " << (int)(revision == revisions.end() ? 0 : revision->second)
               << " portions " << (int)model.View->Presentations().size()
               << " lines " << (int)model.View->LineArrays().size();
            return 0;
        } else if (action == "fields" && argc == 3) {
            for (const auto& field : model.Mesh->Properties()) {
                di << "{name"; di.AppendElement(field->Name().ToCString());
                di << " mapping " << (field->IsNodal() ? "point" : "cell") << " components " << field->NbRawComponents()
                   << " kind " << (field->Type() == RWCae::MeshNVecPropertyType::Tensor ? "tensor" : field->IsVector() ? "vector" : "scalar")
                   << " step " << field->TimeStep() << " active " << (int)(field == model.Field)
                   << " component " << model.Component << "} ";
            }
            return 0;
        } else if (action == "bounds" && argc == 3) {
            const auto min = model.Bounds.CornerMin(), max = model.Bounds.CornerMax();
            di << min.X() << " " << min.Y() << " " << min.Z() << " " << max.X() << " " << max.Y() << " " << max.Z();
            return 0;
        } else if (action == "range" && argc == 4) {
            for (const auto& cut : cuts.Definitions()) {
                if (cut.Id != argv[3]) { continue; }
                if (cut.Shape == ViewCaeGeom::MeshCutShape::Isosurface) {
                    double lower, upper;
                    if (!cuts.IsoRange(cut.Id, lower, upper)) { throw std::invalid_argument("Invalid isosurface field binding"); }
                    di << lower << " " << upper;
                    return 0;
                }
                if (cut.Shape == ViewCaeGeom::MeshCutShape::Sphere || cut.Shape == ViewCaeGeom::MeshCutShape::Cylinder) {
                    const double extent = (model.Bounds.CornerMax().XYZ()-model.Bounds.CornerMin().XYZ()).Modulus();
                    di << 1e-6 << " " << std::max(extent,cut.Radius);
                    return 0;
                }
                const auto min = model.Bounds.CornerMin(), max = model.Bounds.CornerMax();
                double lower = 0.0, upper = 0.0;
                for (int i = 0; i < 3; ++i) {
                    const double a = cut.Plane[i] * min.Coord(i+1), b = cut.Plane[i] * max.Coord(i+1);
                    lower += std::min(a,b); upper += std::max(a,b);
                }
                di << lower << " " << upper;
                return 0;
            }
        } else if (action == "handle" && (argc == 5 || argc == 6)) {
            const auto ctx = ViewerTest::GetAISContext();
            if (ctx.IsNull()) { throw std::runtime_error("Initialize a view with vinit first"); }
            const bool show = cutBoolean(argv[4]);
            const bool gizmo = argc == 5 || cutBoolean(argv[5]);
            if (!show) { removeHandle(model); }
            else if (model.EditedCut != argv[3] || (model.Plane.IsNull() && model.Quadric.IsNull())
                || gizmo != !model.Manipulator.IsNull()) {
                Handle(ViewCae::InteractiveCaeCutPlane) plane;
                Handle(ViewCae::InteractiveCaeCutQuadric) quadric;
                for (const auto& cut : cuts.Definitions()) {
                    if (cut.Id != argv[3]) { continue; }
                    if (cut.Shape == ViewCaeGeom::MeshCutShape::Plane) {
                        plane = new ViewCae::InteractiveCaeCutPlane(model.View,argv[3],model.Bounds);
                    } else if (cut.Shape == ViewCaeGeom::MeshCutShape::Sphere || cut.Shape == ViewCaeGeom::MeshCutShape::Cylinder) {
                        quadric = new ViewCae::InteractiveCaeCutQuadric(model.View,argv[3],model.Bounds);
                    }
                    break;
                }
                if (plane.IsNull() && quadric.IsNull()) { throw std::invalid_argument("Cut has no geometric positioning aid"); }
                removeHandle(model);
                model.EditedCut = argv[3]; model.Plane = plane; model.Quadric = quadric;
                Handle(AIS_InteractiveObject) guide = !plane.IsNull() ? Handle(AIS_InteractiveObject)(plane)
                    : Handle(AIS_InteractiveObject)(quadric);
                ctx->Display(guide,false);
                if (gizmo) {
                    if (!plane.IsNull()) { model.Manipulator = new ViewCae::InteractiveCaeCutManipulator(plane); }
                    else { model.Manipulator = new ViewCae::InteractiveCaeCutQuadricManipulator(quadric); }
                    model.Manipulator->Attach(guide,AIS_Manipulator::OptionsForAttach().SetAdjustPosition(false));
                    model.Manipulator->SetPosition(!plane.IsNull() ? plane->Frame() : quadric->Frame());
                }
                if (!ViewerTest::CurrentEventManager().IsNull()) { ViewerTest::CurrentEventManager()->SetAllowDragging(true); }
            } else { refreshHandle(model); }
            ctx->UpdateCurrentViewer();
            return 0;
        } else if (action == "handleinfo" && argc == 3) {
            di << "id"; di.AppendElement(model.EditedCut.c_str());
            di << " gizmo " << (int)!model.Manipulator.IsNull();
            di << " updates " << (!model.Plane.IsNull() ? model.Plane->DragUpdates()
                    : !model.Quadric.IsNull() ? model.Quadric->DragUpdates() : 0)
               << " dragging " << (int)((!model.Plane.IsNull() && model.Plane->IsDragging())
                   || (!model.Quadric.IsNull() && model.Quadric->IsDragging())
                   || (!model.Manipulator.IsNull() && model.Manipulator->HasActiveTransformation()));
            const auto ctx = ViewerTest::GetAISContext();
            const auto detected = ctx.IsNull() || !ctx->HasDetected() ? Handle(AIS_InteractiveObject)() : ctx->DetectedInteractive();
            di << " detected " << (!detected.IsNull() && detected == model.Plane ? "plane"
                : (!detected.IsNull() && detected == model.Quadric ? "quadric"
                : (!detected.IsNull() && detected == model.Manipulator ? "manipulator" : "none")));
            di << " mode " << (model.Manipulator.IsNull() ? 0 : (int)model.Manipulator->ActiveMode())
               << " axis " << (model.Manipulator.IsNull() ? -1 : model.Manipulator->ActiveAxisIndex());
            if (!model.Plane.IsNull() || !model.Quadric.IsNull()) {
                const auto point = !model.Plane.IsNull() ? model.Plane->Frame().Location() : model.Quadric->Frame().Location();
                di << " origin {" << point.X() << " " << point.Y() << " " << point.Z() << "}";
            }
            return 0;
        } else if (action == "project" && argc == 6) {
            if (ViewerTest::CurrentView().IsNull()) { throw std::runtime_error("Initialize a view with vinit first"); }
            int x, y;
            ViewerTest::CurrentView()->Convert(cutReal(argv[3]), cutReal(argv[4]), cutReal(argv[5]), x, y);
            di << x << " " << y;
            return 0;
        } else if (action == "mouse" && argc == 6) {
            const auto event = ViewerTest::CurrentEventManager();
            const auto ctx = ViewerTest::GetAISContext(); const auto view = ViewerTest::CurrentView();
            if (event.IsNull() || ctx.IsNull() || view.IsNull()) { throw std::runtime_error("Initialize a view with vinit first"); }
            const std::string phase(argv[3]);
            const Graphic3d_Vec2i point(cutInteger(argv[4]), cutInteger(argv[5]));
            if (phase == "move") { event->UpdateMousePosition(point, event->PressedMouseButtons(), Aspect_VKeyFlags_NONE, false); }
            else if (phase == "down") { event->UpdateMouseButtons(point, Aspect_VKeyMouse_LeftButton, Aspect_VKeyFlags_NONE, false); }
            else if (phase == "up") { event->UpdateMouseButtons(point, Aspect_VKeyMouse_NONE, Aspect_VKeyFlags_NONE, false); }
            else if (phase == "abort") {
                event->OnObjectDragged(ctx, view, AIS_DragAction_Abort);
                event->UpdateMouseButtons(point, Aspect_VKeyMouse_NONE, Aspect_VKeyFlags_NONE, false);
            } else { throw std::invalid_argument("Mouse phase: move, down, up, abort"); }
            event->FlushViewEvents(ctx, view, true);
            return 0;
        } else if (action == "fit" && argc == 3) {
            if (ViewerTest::CurrentView().IsNull()) { throw std::runtime_error("Initialize a view with vinit first"); }
            ViewerTest::CurrentView()->FitAll(model.Bounds, 0.12, true);
            return 0;
        } else if (action == "rotate" && (argc == 6 || argc == 9)) {
            const int axis = cutInteger(argv[4]);
            if (axis < 0 || axis > 2) { throw std::invalid_argument("Rotation axis index: 0, 1, or 2"); }
            for (const auto& cut : cuts.Definitions()) {
                if (cut.Id != argv[3]) { continue; }
                if (cut.Shape == ViewCaeGeom::MeshCutShape::Sphere || cut.Shape == ViewCaeGeom::MeshCutShape::Cylinder) {
                    gp_Pnt center(cut.Center.x(),cut.Center.y(),cut.Center.z());
                    gp_Dir normal(cut.Axis.x(),cut.Axis.y(),cut.Axis.z());
                    const gp_Pnt pivot = argc == 9 ? gp_Pnt(cutReal(argv[6]),cutReal(argv[7]),cutReal(argv[8])) : center;
                    const gp_Ax1 rotation(pivot,gp_Dir(axis == 0 ? 1 : 0,axis == 1 ? 1 : 0,axis == 2 ? 1 : 0));
                    const double angle = cutReal(argv[5])*std::acos(-1.0)/180.0;
                    center.Rotate(rotation,angle); normal.Rotate(rotation,angle);
                    ok = cut.Shape == ViewCaeGeom::MeshCutShape::Sphere
                        ? cuts.SetSphere(cut.Id,Graphic3d_Vec3d(center.X(),center.Y(),center.Z()),cut.Radius)
                        : cuts.SetCylinder(cut.Id,Graphic3d_Vec3d(center.X(),center.Y(),center.Z()),
                            Graphic3d_Vec3d(normal.X(),normal.Y(),normal.Z()),cut.Radius);
                    break;
                }
                if (cut.Shape != ViewCaeGeom::MeshCutShape::Plane) {
                    throw std::invalid_argument("Rotation requires a plane cut; use isovalue for an isosurface");
                }
                gp_Pln plane(cut.Plane.x(), cut.Plane.y(), cut.Plane.z(), cut.Plane.w());
                const gp_Pnt center((model.Bounds.CornerMin().XYZ() + model.Bounds.CornerMax().XYZ()) * 0.5);
                const gp_Dir normal = plane.Axis().Direction();
                const gp_Pnt pivot = argc == 9 ? gp_Pnt(cutReal(argv[6]),cutReal(argv[7]),cutReal(argv[8]))
                    : center.Translated(gp_Vec(normal) * -gp_Vec(plane.Location(),center).Dot(gp_Vec(normal)));
                const gp_Dir direction(axis == 0 ? 1 : 0, axis == 1 ? 1 : 0, axis == 2 ? 1 : 0);
                plane.Rotate(gp_Ax1(pivot, direction), cutReal(argv[5]) * std::acos(-1.0)/180.0);
                double a,b,c,d; plane.Coefficients(a,b,c,d);
                ok = cuts.SetPlane(argv[3], Graphic3d_Vec4d(a,b,c,d));
                break;
            }
        } else if (action == "field" && argc == 4 && std::string(argv[3]) == "none") {
            model.Field.Nullify(); model.Component = 0; model.View->SetField(nullptr); ok = true;
        } else if ((action == "add" || action == "move") && argc == 8) {
            Graphic3d_Vec4d plane(cutReal(argv[4]), cutReal(argv[5]), cutReal(argv[6]), cutReal(argv[7]));
            ok = action == "add" ? cuts.AddPlane(argv[3], plane) : cuts.SetPlane(argv[3], plane);
        } else if ((action == "sphereadd" || action == "spheremove") && argc == 8) {
            const Graphic3d_Vec3d center(cutReal(argv[4]),cutReal(argv[5]),cutReal(argv[6]));
            ok = action == "sphereadd" ? cuts.AddSphere(argv[3],center,cutReal(argv[7]))
                : cuts.SetSphere(argv[3],center,cutReal(argv[7]));
            if (!ok) { throw std::invalid_argument("Sphere requires a positive radius and supported mesh topology; check id and center (general cells need OCCTV_USE_VTK)"); }
        } else if ((action == "cylinderadd" || action == "cylindermove") && argc == 11) {
            const Graphic3d_Vec3d center(cutReal(argv[4]),cutReal(argv[5]),cutReal(argv[6]));
            const Graphic3d_Vec3d axis(cutReal(argv[7]),cutReal(argv[8]),cutReal(argv[9]));
            ok = action == "cylinderadd" ? cuts.AddCylinder(argv[3],center,axis,cutReal(argv[10]))
                : cuts.SetCylinder(argv[3],center,axis,cutReal(argv[10]));
            if (!ok) { throw std::invalid_argument("Cylinder requires a positive radius, nonzero axis and supported mesh topology; check id and center (general cells need OCCTV_USE_VTK)"); }
        } else if (action == "radius" && argc == 5) {
            for (const auto& cut : cuts.Definitions()) {
                if (cut.Id != argv[3]) { continue; }
                ok = cut.Shape == ViewCaeGeom::MeshCutShape::Sphere ? cuts.SetSphere(cut.Id,cut.Center,cutReal(argv[4]))
                    : cut.Shape == ViewCaeGeom::MeshCutShape::Cylinder
                        && cuts.SetCylinder(cut.Id,cut.Center,cut.Axis,cutReal(argv[4]));
                break;
            }
        } else if ((action == "isoadd" || action == "isomove") && (argc == 8 || argc == 9)) {
            const std::string mapping(argv[4]);
            if (mapping != "point" && mapping != "cell") { throw std::invalid_argument("Field mapping: point or cell"); }
            const int step = argc == 9 ? cutInteger(argv[8]) : -1;
            Handle(RWCae::MeshNVecProperty) boundField;
            for (const auto& field : model.Mesh->Properties()) {
                if (field->Name() == argv[5] && field->IsNodal() == (mapping == "point") && field->TimeStep() == step) {
                    if (!boundField.IsNull() && boundField != field) { throw std::invalid_argument("Ambiguous isosurface field"); }
                    boundField = field;
                }
            }
            if (boundField.IsNull()) { throw std::invalid_argument("Isosurface field not found"); }
            ok = action == "isoadd" ? cuts.AddIsosurface(argv[3], boundField, cutInteger(argv[6]), cutReal(argv[7]))
                : cuts.SetIsosurface(argv[3], boundField, cutInteger(argv[6]), cutReal(argv[7]));
            if (!ok) { throw std::invalid_argument("Invalid isosurface: unique finite scalar component and first-order mesh required"); }
        } else if (action == "isovalue" && argc == 5) {
            ok = cuts.SetIsoValue(argv[3], cutReal(argv[4]));
        } else if (action == "visibility" && argc == 5) {
            const int mask = cutInteger(argv[4]);
            ok = mask >= 0 && cuts.SetVisibility(argv[3], (unsigned)mask);
        } else if (action == "enable" && argc == 5) {
            ok = cuts.SetEnabled(argv[3], cutBoolean(argv[4]));
        } else if (action == "copy" && argc == 5) { ok = cuts.Duplicate(argv[3], argv[4]); }
        else if (action == "rename" && argc == 5) { ok = cuts.Rename(argv[3], argv[4]); }
        else if (action == "remove" && argc == 4) { ok = cuts.Remove(argv[3]); }
        else if (action == "snapshot" && argc == 3) { di << cuts.DumpDefinitionsJson().c_str(); return 0; }
        else if (action == "restore" && argc == 4) { ok = cuts.RestoreDefinitionsJson(argv[3]); }
        else if (action == "deform" && (argc == 5 || argc == 6)) {
            const int step = argc == 6 ? cutInteger(argv[5]) : -1;
            if (std::string(argv[3]) == "none") { ok = cuts.SetDeformation(nullptr, 0.0); }
            else {
                for (const auto& field : model.Mesh->Properties()) {
                    if (field->Name() == argv[3] && field->TimeStep() == step) {
                        ok = cuts.SetDeformation(field, cutReal(argv[4]));
                        break;
                    }
                }
            }
            if (ok) {
                cancelHandleDrag(model);
                model.Bounds = cuts.SourceBounds();
                if (!model.Plane.IsNull()) { model.Plane->SetBounds(model.Bounds); }
                if (!model.Quadric.IsNull()) { model.Quadric->SetBounds(model.Bounds); }
                di << "effectiveScale " << cuts.EffectiveDeformationScale();
            }
        }
        else if (action == "tensor" && (argc == 3 || argc == 6)) {
            const char* measures[] = {"components","mises","pressure","maxprincipal","midprincipal","minprincipal","tresca"};
            const char* layouts[] = {"xx-yy-zz-xy-xz-yz","xx-yy-zz-xy-yz-xz","row-major","column-major"};
            auto options = cuts.RecoveryOptions();
            if (argc == 3) {
                di << "measure " << measures[(int)options.Tensor.Measure] << " layout " << layouts[(int)options.Tensor.Layout]
                   << " shear " << (options.Tensor.EngineeringShear ? "engineering" : "tensor"); return 0;
            }
            int measure = -1, layout = -1;
            for (int i = 0; i < 7; ++i) { if (std::string(argv[3]) == measures[i]) { measure = i; } }
            for (int i = 0; i < 4; ++i) { if (std::string(argv[4]) == layouts[i]) { layout = i; } }
            const std::string shear(argv[5]);
            if (measure < 0 || layout < 0 || (shear != "tensor" && shear != "engineering")) { throw std::invalid_argument("Invalid tensor measure, layout or shear convention"); }
            options.Tensor.Measure = (ViewCaeGeom::MeshCutTensorMeasure)measure;
            options.Tensor.Layout = (ViewCaeGeom::MeshCutTensorLayout)layout;
            options.Tensor.EngineeringShear = shear == "engineering";
            if (!model.Field.IsNull() && model.Field->Type() == RWCae::MeshNVecPropertyType::Tensor && measure != 0) {
                ViewCaeGeom::DeriveCutTensorField(model.Field,options.Tensor);
            }
            ok = cuts.SetRecoveryOptions(options);
        }
        else if (action == "recovery" && (argc == 3 || argc == 6 || argc == 7)) {
            if (argc == 3) {
                const auto& options = cuts.RecoveryOptions();
                di << "threshold " << options.ThresholdPercent << " before " << (int)options.InvariantsBeforeAveraging << " regions {";
                for (auto region : options.Regions) { di << std::to_string(region).c_str() << " "; }
                di << "}";
                return 0;
            }
            ViewCaeGeom::MeshCutRecoveryOptions options;
            options.Tensor = cuts.RecoveryOptions().Tensor;
            options.ThresholdPercent = cutReal(argv[3]);
            const int before = cutInteger(argv[4]);
            if (before != 0 && before != 1) { throw std::invalid_argument("Recovery before-invariant flag must be 0 or 1"); }
            options.InvariantsBeforeAveraging = before != 0;
            if (std::string(argv[5]) == "keep") {
                if (argc == 7) { throw std::invalid_argument("Retaining region IDs does not take a field step"); }
                options.Regions = cuts.RecoveryOptions().Regions;
            } else if (std::string(argv[5]) != "none") {
                const int step = argc == 7 ? cutInteger(argv[6]) : -1;
                Handle(RWCae::MeshNVecProperty) regions;
                for (const auto& candidate : model.Mesh->Properties()) {
                    if (candidate->Name() == argv[5] && candidate->IsElemental() && candidate->TimeStep() == step) {
                        if (!regions.IsNull()) { throw std::invalid_argument("Ambiguous recovery region field"); }
                        regions = candidate;
                    }
                }
                if (regions.IsNull() || !regions->IsInitialized() || !regions->HasScalarComponents()
                    || regions->NbComponents() != 1 || regions->NbValues() != (int)model.Mesh->NbIndexedElems()) {
                    throw std::invalid_argument("Recovery regions require a matching elemental scalar field");
                }
                for (int e = 0; e < regions->NbValues(); ++e) {
                    const double region = regions->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e));
                    if (!std::isfinite(region) || region != std::trunc(region)
                        || region < -std::ldexp(1.0,63) || region >= std::ldexp(1.0,63)) {
                        throw std::invalid_argument("Recovery region IDs must be finite signed 64-bit integers");
                    }
                    options.Regions.push_back((int64_t)region);
                }
            } else if (argc == 7) { throw std::invalid_argument("A region step requires a region field"); }
            ok = cuts.SetRecoveryOptions(options);
        }
        else if (action == "cellmode" && argc == 5) {
            const std::string mode(argv[4]);
            if (mode != "constant" && mode != "projected") { throw std::invalid_argument("Cell mode: constant or projected"); }
            ok = cuts.SetCellMode(argv[3], mode == "constant" ? ViewCaeGeom::MeshCutCellMode::Constant : ViewCaeGeom::MeshCutCellMode::Projected);
        } else if (action == "field" && (argc == 6 || argc == 7)) {
            const std::string mapping(argv[3]);
            if (mapping != "point" && mapping != "cell") { throw std::invalid_argument("Field mapping: point or cell"); }
            const int step = argc == 7 ? cutInteger(argv[6]) : -1;
            for (const auto& field : model.Mesh->Properties()) {
                if (field->Name() == argv[4] && field->IsNodal() == (mapping == "point") && field->TimeStep() == step) {
                    const int component = cutInteger(argv[5]);
                    model.View->SetField(field, component);
                    model.Field = field;
                    model.Component = component;
                    ok = true;
                    break;
                }
            }
        } else if ((action == "freebody" && (argc == 14 || argc == 15))
                   || (action == "heatflow" && (argc == 8 || argc == 9))) {
            const bool stress = action == "freebody";
            const int baseCount = stress ? 14 : 8;
            const int step = argc > baseCount ? cutInteger(argv[baseCount]) : -1;
            Handle(RWCae::MeshNVecProperty) field;
            for (const auto& candidate : model.Mesh->Properties()) {
                if (candidate->Name() == argv[4] && candidate->TimeStep() == step) {
                    if (!field.IsNull()) { throw std::invalid_argument("Ambiguous integration field name"); }
                    field = candidate;
                }
            }
            if (field.IsNull()) { throw std::invalid_argument("Integration field not found"); }
            const auto cut = cuts.EvaluatePhysical(argv[3], field);
            ViewCaeGeom::MeshCutResultant integral;
            if (stress) {
                const int xx = cutInteger(argv[5]), yy = cutInteger(argv[6]), zz = cutInteger(argv[7]);
                const int xy = cutInteger(argv[8]), xz = cutInteger(argv[9]), yz = cutInteger(argv[10]);
                integral = ViewCaeGeom::MeshCutIntegration::Stress(cut, {xx,xy,xz,xy,yy,yz,xz,yz,zz},
                    Graphic3d_Vec3d(cutReal(argv[11]), cutReal(argv[12]), cutReal(argv[13])));
            } else {
                integral = ViewCaeGeom::MeshCutIntegration::HeatFlux(cut,
                    {cutInteger(argv[5]),cutInteger(argv[6]),cutInteger(argv[7])});
            }
            di << "area " << integral.Area << " force {" << integral.Force.x() << " " << integral.Force.y() << " "
               << integral.Force.z() << "} moment {" << integral.Moment.x() << " " << integral.Moment.y() << " "
               << integral.Moment.z() << "} heatFlow " << integral.HeatFlow;
            return 0;
        } else if (action == "eval" && argc == 4) {
            auto result = cuts.Evaluate(argv[3], model.Field);
            const int displayComponent = result.TensorDerived ? 0 : model.Component;
            di << "visible " << (int)(result.Definition.Enabled ? result.Definition.Visibility : 0)
               << " revision " << (int)result.Revision
               << " sourceGeometry " << (result.SourceApproximate ? "numerical" : "analytic")
               << " isoFieldTolerance " << result.IsoFieldTolerance << " isoFieldResidual " << result.IsoFieldResidual
               << " isoGeometryResidual " << result.IsoGeometryResidual << " isoDepth " << result.IsoSubdivisionDepth
               << " isoPrimitives " << std::to_string(result.IsoPrimitives).c_str();
            const char* keys[] = {"negative", "section", "positive"};
            for (size_t i = 0; i < result.Portions.size(); ++i) {
                di << " " << keys[i] << " {";
                if (result.Portions[i].IsNull()) { di << "cells 0 nodes 0 volume 0 area 0 values {}"; }
                else {
                    auto mesh = result.Portions[i]->GetMeshData();
                    const bool curved = result.BoundaryMesh;
                    const double vol = curved ? result.Volumes[i]
                        : i == 1 ? 0.0 : measure(mesh, nullptr, true) + measure(model.Mesh, result.Roots[i], true);
                    di << "cells " << (int)mesh->NbIndexedElems() << " nodes " << (int)mesh->NbNodes()
                       << " volume " << vol << " area " << (curved ? result.Areas[i] : measure(mesh, nullptr, false))
                       << " meshKind " << (curved ? "boundary" : "cells") << " values {";
                    auto field = result.Fields[i];
                    if (!field.IsNull() && field->IsNodal()) {
                        for (int n = 0; n < (int)mesh->NbNodes(); ++n) {
                            const auto& p = mesh->Nodes()[n];
                            di << "{" << p.x() << " " << p.y() << " " << p.z() << " "
                               << field->GetRawComponent(displayComponent)->Value(RWCae::MeshPropertyEntityInfo(n)) << "} ";
                        }
                    } else if (!field.IsNull() && field->HasNodeVecComponents()) {
                        for (int e = 0; e < (int)mesh->NbIndexedElems(); ++e) {
                            std::vector<RWCae::MeshEntityId> nodes;
                            mesh->ElementNodes(e,nodes);
                            for (auto n : nodes) {
                                const auto& p = mesh->Nodes()[n];
                                di << "{" << e << " " << (int)n << " " << p.x() << " " << p.y() << " " << p.z() << " "
                                   << field->GetRawComponent(displayComponent)->Value(RWCae::MeshPropertyEntityInfo(e,n)) << "} ";
                            }
                        }
                    } else if (!field.IsNull()) {
                        for (int e = 0; e < (int)mesh->NbIndexedElems(); ++e) {
                            di << field->GetRawComponent(displayComponent)->Value(RWCae::MeshPropertyEntityInfo(e)) << " ";
                        }
                    }
                    di << "} valueMapping " << (field.IsNull() ? "none" : field->IsNodal() ? "point"
                        : field->HasNodeVecComponents() ? "element-node" : "element");
                }
                di << "}";
            }
            auto slice = Handle(ViewCaeGeom::MeshGeometrySlice)::DownCast(result.Portions[1]);
            di << " lines " << (slice.IsNull() ? 0 : (int)slice->Lines().size()) << " lineValues {";
            if (!slice.IsNull() && !result.LineField.IsNull()) {
                for (size_t l = 0; l < slice->Lines().size(); ++l) {
                    for (auto n : slice->Lines()[l].Nodes) {
                        di << result.LineField->GetRawComponent(displayComponent)->Value(RWCae::MeshPropertyEntityInfo(
                            result.LineField->IsNodal() ? (int)n : (int)l,
                            result.LineField->HasNodeVecComponents() ? (int)n : -1)) << " ";
                    }
                }
            }
            di << "}";
            return 0;
        } else if (action == "display" && argc == 3) {
            if (ViewerTest::GetAISContext().IsNull()) { throw std::runtime_error("Initialize a view with vinit first"); }
            model.View->Synchronize();
            ViewerTest::Display(name.c_str(), model.View, true);
            return 0;
        } else if (action == "clear" && argc == 3) {
            removeHandle(model);
            if (!ViewerTest::GetAISContext().IsNull()) { ViewerTest::GetAISContext()->Remove(model.View, true); }
            models.erase(found);
            return 0;
        }
        if (!ok) {
            di << "Error: Invalid cut-view operation or arguments. Received:";
            for (int i = 0; i < argc; ++i) { di << " " << argv[i]; }
            di << "\nUse 'help scvcutview' for command syntax.";
            return 1;
        }
        refreshHandle(model);
        if (!ViewerTest::GetAISContext().IsNull() && ViewerTest::GetAISContext()->IsDisplayed(model.View)) {
            model.View->Synchronize();
            ViewerTest::GetAISContext()->Redisplay(model.View, true);
        }
        return 0;
    } catch (const Standard_Failure& error) { di << "Error: " << error.GetMessageString(); }
    catch (const std::exception& error) { di << "Error: " << error.what(); }
    return 1;
}
}

void ViewCaeTest::CutViewCommands::Factory(Draw_Interpretor& di)
{
    di.Add("scvcutimagestats", "scvcutimagestats image.png: count non-white and colored pixels", __FILE__,
        imageStats, "SC Viewer CAE Prs");
    di.Add("scvcutview", "scvcutview name read/replace file | info | defaults | add/move id A B C D | visibility id mask\n"
        "  | isoadd/isomove id point/cell field rawComponent value [timeStep] | isovalue id value\n"
        "  | sphereadd/spheremove id centerX centerY centerZ radius | radius id radius\n"
        "  | cylinderadd/cylindermove id centerX centerY centerZ axisX axisY axisZ radius\n"
        "  | field point/cell name rawComponent [timeStep] | cellmode id constant/projected\n"
        "  | recovery [thresholdPercent beforeInvariant0/1 regionField/none/keep [timeStep]]\n"
        "  | tensor [components/mises/pressure/maxprincipal/midprincipal/minprincipal/tresca layout tensor/engineering]\n"
        "    layout: xx-yy-zz-xy-xz-yz, xx-yy-zz-xy-yz-xz, row-major, column-major\n"
        "  | copy id newId | rename id name | enable id 0/1 | remove id | eval id | display | clear\n"
        "  | snapshot | restore json | deform property/none normalizedScale [timeStep]\n"
        "  | freebody id field xx yy zz xy xz yz originX originY originZ [timeStep]\n"
        "  | heatflow id field componentX componentY componentZ [timeStep]\n"
        "  | list | fields | bounds | range id | stats | presentation id | rotate id axisIndex degrees [originX originY originZ]\n"
        "  | geometry [absoluteTolerance relativeTolerance faceLimit lineLimit maxSourcePrimitives]\n"
        "  | isooptions [fieldAbsoluteTolerance fieldRelativeTolerance minimumDepth limit maxPrimitives]\n"
        "  | geometrymode [numerical/preferred/required]\n"
        "  | handle id 0/1 [gizmo0/1] | handleinfo | fit | field none\n"
        "  | project worldX worldY worldZ | mouse move/down/up/abort pixelX pixelY",
        __FILE__, cutView, "SC Viewer CAE Prs");
}
