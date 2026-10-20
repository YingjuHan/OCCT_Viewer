#include <ViewCaeTest/CutViewCommands.h>
#include <ViewCae/InteractiveCaeCutView.h>
#include <ViewCae/InteractiveCaeCutPlane.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <ViewCaeGeom/MeshCutUnion.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <RWMeshConv/MeshLoader.h>
#include <ViewerTest.hxx>
#include <ViewerTest_EventManager.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <gp_Ax1.hxx>
#include <Draw.hxx>
#include <Image_AlienPixMap.hxx>
#include <Viewer/NativeCutView.h>
#include <Viewer/NativeCutTimeline.h>
#include <Viewer/NativeCutAnalysis.h>
#include <Viewer/NativeCutSession.h>
#include <ViewCaeGeom/MeshCutSlicing.h>
#include <DBRep.hxx>

#include <cmath>
#include <map>
#include <stdexcept>
#include <set>
#include <limits>
#include <sstream>
namespace ViewCaeTest { void RegisterCutBenchmarkCommands(Draw_Interpretor&); }

namespace {
int64_t cutInt64(const std::string& argument)
{
    const size_t start = !argument.empty() && (argument[0] == '-' || argument[0] == '+') ? 1 : 0;
    if (argument.size() == start || argument.find_first_not_of("0123456789",start) != std::string::npos) {
        throw std::invalid_argument("Trace region IDs require signed 64-bit decimal integers");
    }
    size_t consumed = 0; const auto value = std::stoll(argument,&consumed);
    if (consumed != argument.size()) { throw std::invalid_argument("Invalid trace region ID"); } return value;
}
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
    RWVtk::VtkImportReport ImportReport;
    uint64_t Generation = 0;
    std::string Source;
    Handle(RWCae::MeshData) Mesh;
    Handle(ViewCae::InteractiveCaeCutView) View;
    std::shared_ptr<Viewer::NativeCutView> Native;
    std::shared_ptr<Viewer::NativeCutTimeline> Timeline;
    std::shared_ptr<Viewer::NativeCutSession> Session;
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
    if (model.Native) { model.Native->ClearHandle(); }
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
    size_t interiorWhite = 0;
    uint64_t checksum = 14695981039346656037ull;
    std::set<unsigned> colors;
    for (size_t y = 0; y < image.SizeY(); ++y) {
        int first = -1, last = -1;
        for (size_t x = 0; x < image.SizeX(); ++x) {
            auto color = image.PixelColor((int)x, (int)y).GetRGB();
            const unsigned r = (unsigned)std::lround(color.Red()*255);
            const unsigned g = (unsigned)std::lround(color.Green()*255);
            const unsigned b = (unsigned)std::lround(color.Blue()*255);
            const unsigned min = std::min({r,g,b}), max = std::max({r,g,b});
            for (unsigned channel : {r,g,b}) { checksum = (checksum ^ channel) * 1099511628211ull; }
            if (min < 245) { ++visible; colors.insert((r << 16) | (g << 8) | b); if (first < 0) { first = (int)x; } last = (int)x; }
            if (max - min > 30) { ++colored; }
        }
        // Convex opaque fixture silhouettes must have no white interior holes.
        // The two-pixel boundary margin excludes silhouette antialiasing.
        for (int x = first+2; first >= 0 && x <= last-2; ++x) {
            const auto c = image.PixelColor(x,(int)y).GetRGB();
            if (std::min({c.Red(),c.Green(),c.Blue()}) > 0.98) { ++interiorWhite; }
        }
    }
    di << "visible " << (int)visible << " colored " << (int)colored << " colors " << (int)colors.size() << " interior_white " << (int)interiorWhite
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

Standard_Integer imageCompare(Draw_Interpretor& di, Standard_Integer argc, const char** argv)
{
    if (argc != 3) { di << "scvcutimagecompare reference.png actual.png"; return 1; }
    Image_AlienPixMap reference, actual;
    if (!reference.Load(argv[1]) || !actual.Load(argv[2]) || reference.SizeX() != actual.SizeX()
        || reference.SizeY() != actual.SizeY() || reference.IsEmpty()) {
        di << "Images require matching non-empty dimensions"; return 1;
    }
    uint64_t changed = 0, sum = 0, silhouette = 0; int maximum = 0;
    for (size_t y = 0; y < reference.SizeY(); ++y) { for (size_t x = 0; x < reference.SizeX(); ++x) {
        const auto a = reference.PixelColor((int)x,(int)y).GetRGB(), b = actual.PixelColor((int)x,(int)y).GetRGB();
        const double av[] = {a.Red(),a.Green(),a.Blue()}, bv[] = {b.Red(),b.Green(),b.Blue()};
        silhouette += (std::min({av[0],av[1],av[2]}) < 0.96) != (std::min({bv[0],bv[1],bv[2]}) < 0.96);
        bool different = false;
        for (int c = 0; c < 3; ++c) {
            const int delta = std::abs((int)std::lround(av[c]*255)-(int)std::lround(bv[c]*255));
            sum += delta; maximum = std::max(maximum,delta); different |= delta != 0;
        }
        changed += different;
    } }
    di << "changed " << std::to_string(changed).c_str() << " mean " << (double)sum/(reference.SizeX()*reference.SizeY()*3)
       << " maximum " << maximum << " silhouette " << (double)silhouette;
    return 0;
}

Standard_Integer cutView(Draw_Interpretor& di, Standard_Integer argc, const char** argv)
{
    try {
        if (argc < 3) { throw std::invalid_argument("scvcutview name action arguments"); }
        const std::string name(argv[1]), action(argv[2]);
        if ((action == "read" || action == "replace") && argc == 4) {
            if (action == "read" && models.count(name)) { throw std::invalid_argument("Cut model already exists"); }
            const std::string path(argv[3]);
            CutModel model;
            RWMeshConv::MeshLoader loader;
            model.Mesh = loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(), argv[3]);
            model.ImportReport = loader.VtkReport();
            if (model.Mesh.IsNull()) {
                throw std::runtime_error(model.ImportReport.errors.empty() ? "VTK import failed" : model.ImportReport.errors.front());
            }
            model.Native = std::make_shared<Viewer::NativeCutView>(model.Mesh);
            model.Timeline = std::make_shared<Viewer::NativeCutTimeline>(*model.Native);
            model.Session = std::make_shared<Viewer::NativeCutSession>(*model.Native,*model.Timeline);
            model.View = model.Native->Display();
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
                if (!model.Native->Attach(ViewerTest::GetAISContext(),ViewerTest::CurrentView())) { throw std::runtime_error(model.Native->LastError()); }
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
        auto printProbe = [&](const ViewCae::MeshCutProbe& probe) {
            di << "position {" << probe.Position.x() << " " << probe.Position.y() << " " << probe.Position.z()
               << "} element " << (int)probe.SourceElement << " values {";
            for (double v : probe.Values) { di << v << " "; } di << "} nodes {";
            for (const auto& n : probe.SourceNodes) { di << "{" << (int)n.first << " " << n.second << "} "; } di << "}";
        };
        auto refreshStep = [&] {
            const bool changed = model.View != model.Native->Display();
            model.View = model.Native->Display(); model.Mesh = model.View->SourceMesh(); model.Field = model.View->Field();
            model.Component = model.View->Component(); model.Bounds = model.View->Cuts().SourceBounds();
            if (changed) { model.Plane.Nullify(); model.Quadric.Nullify(); model.Manipulator.Nullify(); model.EditedCut.clear(); }
            const auto context = ViewerTest::GetAISContext();
            if (!context.IsNull() && context->IsDisplayed(model.View)) {
                ViewerTest::Display(name.c_str(),model.View,false);
                context->UpdateCurrentViewer();
            }
        };
        if(action=="sessionsave" && (argc==4||argc==5)) {
            const auto cad=argc==5?std::string(argv[4]):model.Session->CadSource();
            if(!model.Session->Save(argv[3],model.Source,cad)) { throw std::invalid_argument(model.Native->LastError()); }
            model.Source=model.Session->Source(); return 0;
        } else if(action=="sessionload" && argc==4) {
            if(!model.Session->Load(argv[3])) { throw std::invalid_argument(model.Native->LastError()); }
            model.Source=model.Session->Source(); model.Generation=++nextGeneration; refreshStep(); return 0;
        } else if(action=="sessioninfo" && argc==3) {
            di<<"version 3 source"; di.AppendElement(model.Source.c_str()); di<<" cad"; di.AppendElement(model.Session->CadSource().c_str()); return 0;
        } else if(action=="cacheinfo" && argc==3) {
            const auto policy=cuts.CachePolicy(); const auto stats=cuts.CacheStatistics();
            di<<"sourceLimit "<<std::to_string(policy.SourceBytes).c_str()<<" resultLimit "<<std::to_string(policy.ResultBytes).c_str()<<" sourceReserved "<<std::to_string(stats.SourceReserved).c_str()<<" resultReserved "<<std::to_string(stats.ResultReserved).c_str()<<" sourceEstimate "<<std::to_string(cuts.EstimatedSourceBytes()).c_str()<<" resultEstimate "<<std::to_string(cuts.EstimatedResultBytes(model.Field)).c_str()<<" evictions "<<std::to_string(stats.Evictions).c_str()<<" request "<<std::to_string(cuts.RequestGeneration()).c_str(); return 0;
        } else if(action=="cachepolicy" && argc==5) {
            const int s=cutInteger(argv[3]),r=cutInteger(argv[4]); if(s<=0||r<=0) { throw std::invalid_argument("Cache MiB limits must be positive"); }
            if(!cuts.SetCachePolicy({(uint64_t)s*1024*1024,(uint64_t)r*1024*1024})) { throw std::invalid_argument("Cache limit is below currently retained geometry reservations"); } return 0;
        } else if(action=="request" && argc==3) { cuts.BeginRequest(std::make_shared<ViewCaeGeom::MeshCutCancellation>()); return 0;
        } else if(action=="cancelrequest" && argc==3) { cuts.CancelRequest(); return 0;
        } else if (action == "physics" && argc == 5) {
            if(!model.Native->Analysis().SetJson(argv[3],argv[4])) { throw std::invalid_argument(model.Native->LastError()); } ok=true;
        } else if(action == "physicsremove" && argc == 4) {
            if(!model.Native->Analysis().Remove(argv[3])) { throw std::invalid_argument(model.Native->LastError()); } ok=true;
        } else if(action == "physicsreport" && argc == 4) {
            const auto& reports=model.Native->Analysis().Reports(); const auto r=reports.find(argv[3]);
            if(r==reports.end()) { throw std::invalid_argument("Unknown physical report"); }
            const auto& t=r->second.Total;
            di<<"area "<<t.Area<<" force {"<<t.Force.x()<<" "<<t.Force.y()<<" "<<t.Force.z()<<"} moment {"<<t.Moment.x()<<" "<<t.Moment.y()<<" "<<t.Moment.z()<<"} heatFlow "<<t.HeatFlow<<" samples "<<(int)r->second.Distributed.size()<<" overlays "<<(int)model.Native->Analysis().Overlays().at(argv[3]).size(); return 0;
        } else if(action == "physicscsv" && argc == 4) {
            const auto& reports=model.Native->Analysis().Reports(); const auto r=reports.find(argv[3]); if(r==reports.end()) { throw std::invalid_argument("Unknown physical report"); }
            di<<r->second.Csv().c_str(); return 0;
        } else if(action == "automatic" && argc == 12) {
            ViewCaeGeom::MeshCutAutomaticMapping mapping;
            if(std::string(argv[4])=="crack") { mapping.Kind=ViewCaeGeom::MeshCutAutomaticKind::CrackDistance; }
            else if(std::string(argv[4])=="density") { mapping.Kind=ViewCaeGeom::MeshCutAutomaticKind::OptimizationDensity; }
            else { throw std::invalid_argument("Automatic kind must be crack/density"); }
            mapping.Threshold=cutReal(argv[7]); mapping.Sign=cutInteger(argv[8]); mapping.Minimum=cutReal(argv[9]); mapping.Maximum=cutReal(argv[10]);
            const int visibility=cutInteger(argv[11]); if(visibility<0||visibility>7) { throw std::invalid_argument("Invalid automatic visibility"); } mapping.Visibility=visibility;
            if(!model.Native->Analysis().Automatic(argv[3],{argv[5],true,cutInteger(argv[6]),model.Timeline->Step()},mapping)) { throw std::invalid_argument(model.Native->LastError()); } ok=true;
        } else if (action == "timeinfo" && argc == 3) {
            di << "step " << model.Timeline->Step() << " count " << (int)model.Timeline->StepCount() << " playing " << (int)model.Timeline->IsPlaying() << " times {";
            for (const auto& step : model.Timeline->Source()->SeriesSteps()) { di << step.Time << " "; } di << "}"; return 0;
        } else if (action == "step" && argc == 4) {
            if (!model.Timeline->SwitchStep(cutInteger(argv[3]))) { throw std::invalid_argument(model.Native->LastError()); }
            refreshStep(); return 0;
        } else if (action == "timeddeform" && argc == 5) {
            Viewer::CutFieldBinding binding; if (std::string(argv[3]) != "none") { binding = {argv[3],true,0,model.Timeline->Step()}; }
            if (!model.Timeline->BindDeformation(binding,cutReal(argv[4]))) { throw std::invalid_argument(model.Native->LastError()); }
            refreshStep(); return 0;
        } else if (action == "materialframe" && argc >= 9 && (argc-5)%4 == 0) {
            Viewer::CutMaterialFrame frame;
            if (std::string(argv[4]) == "reference") { frame.Configuration = Viewer::CutFrameConfiguration::Reference; }
            else if (std::string(argv[4]) == "updated") { frame.Configuration = Viewer::CutFrameConfiguration::Updated; }
            else { throw std::invalid_argument("Material frame configuration must be reference/updated"); }
            for (int i = 5; i < argc; i += 4) {
                const int step = cutInteger(argv[i]); std::array<RWCae::MeshEntityId,3> nodes;
                for (int n = 0; n < 3; ++n) { const int node = cutInteger(argv[i+1+n]); if (node < 0) { throw std::invalid_argument("Negative material anchor"); } nodes[n] = node; }
                if (!frame.Anchors.emplace(step,nodes).second) { throw std::invalid_argument("Duplicate material anchor step"); }
            }
            ok = model.Timeline->SetMaterialFrame(argv[3],frame); if (!ok) { throw std::invalid_argument(model.Native->LastError()); }
        } else if (action == "follow" && argc == 5) {
            ok = model.Timeline->BindCutFrame(argv[3],argv[4]); if (!ok) { throw std::invalid_argument(model.Native->LastError()); }
        } else if (action == "play" && argc == 7) {
            const bool timestamps = cutBoolean(argv[6]);
            ok = timestamps ? model.Timeline->PlayTime(cutReal(argv[3]),cutBoolean(argv[4]),cutBoolean(argv[5]))
                : model.Timeline->Play(cutReal(argv[3]),cutBoolean(argv[4]),cutBoolean(argv[5]));
            if (!ok) { throw std::invalid_argument(model.Native->LastError()); } refreshStep(); return 0;
        } else if (action == "pause" && argc == 3) { model.Timeline->Pause(); return 0;
        } else if (action == "tick" && argc == 4) {
            if (!model.Timeline->Tick(cutReal(argv[3]))) { throw std::invalid_argument(model.Native->LastError()); }
            refreshStep(); return 0;
        } else if (action == "slicing" && argc >= 5) {
            const auto definitions = cuts.Definitions();
            const auto base = std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == argv[3]; });
            if (base == definitions.end() || base->Shape != ViewCaeGeom::MeshCutShape::Plane) { throw std::invalid_argument("Slicing requires a selected plane"); }
            auto definition = *base; definition.Id += "_"+std::string(argv[4])+"_"+std::to_string(model.Native->Generation());
            std::vector<ViewCaeGeom::MeshCutSliceSample> samples; const std::string mode(argv[4]);
            auto count = [&](const char* value) { const int n = cutInteger(value); if (n < 1 || n > 10000) { throw std::invalid_argument("Slice count outside 1..10000"); } return (size_t)n; };
            if (mode == "regular" && argc == 8) { samples = ViewCaeGeom::MeshCutSlicing::Regular(definition,cutReal(argv[5]),cutReal(argv[6]),count(argv[7])); }
            else if (mode == "range" && argc == 8) { samples = ViewCaeGeom::MeshCutSlicing::Range(definition,cutReal(argv[5]),cutReal(argv[6]),count(argv[7])); }
            else if (mode == "rotational" && argc == 14) {
                samples = ViewCaeGeom::MeshCutSlicing::Rotational(definition,Graphic3d_Vec3d(cutReal(argv[5]),cutReal(argv[6]),cutReal(argv[7])),
                    Graphic3d_Vec3d(cutReal(argv[8]),cutReal(argv[9]),cutReal(argv[10])),cutReal(argv[11]),cutReal(argv[12]),count(argv[13]));
            } else if (mode == "path" && argc >= 14 && (argc-8)%3 == 0) {
                std::vector<Graphic3d_Vec3d> points; for (int i = 8; i < argc; i += 3) { points.emplace_back(cutReal(argv[i]),cutReal(argv[i+1]),cutReal(argv[i+2])); }
                samples = ViewCaeGeom::MeshCutSlicing::Path(definition.Id,points,Graphic3d_Vec3d(cutReal(argv[5]),cutReal(argv[6]),cutReal(argv[7])));
            } else { throw std::invalid_argument("Invalid slicing mode/parameters"); }
            ViewCaeGeom::MeshCutViewManager staged(model.Mesh);
            if (!staged.RestoreDefinitionsJson(cuts.DumpDefinitionsJson()) || !staged.SetDeformation(cuts.DeformationField(),cuts.DeformationScale())) { throw std::invalid_argument("Cannot stage slicing configuration"); }
            for (const auto& sample : samples) {
                if (!staged.AddPlane(sample.Definition.Id,sample.Definition.Plane)
                    || !staged.SetPlanePlacement(sample.Definition.Id,sample.Definition.Placement)) { throw std::invalid_argument("Cannot stage slice definition"); }
                staged.Evaluate(sample.Definition.Id,model.Field);
            }
            if (!cuts.RestoreDefinitionsJson(staged.DumpDefinitionsJson())) { throw std::invalid_argument("Cannot publish staged slicing definitions"); }
            ok = true;
        } else if (action == "datums" && argc == 3) {
            di.AppendElement("global");
            for (const auto& datum : model.Native->Datums()) { di.AppendElement(datum.first.c_str()); }
            return 0;
        } else if (action == "placement" && argc == 13) {
            ViewCaeGeom::MeshCutPlanePlacement p;
            if (!p.Define(Graphic3d_Vec3d(cutReal(argv[4]),cutReal(argv[5]),cutReal(argv[6])),
                Graphic3d_Vec3d(cutReal(argv[7]),cutReal(argv[8]),cutReal(argv[9])),
                Graphic3d_Vec3d(cutReal(argv[10]),cutReal(argv[11]),cutReal(argv[12])))) {
                throw std::invalid_argument("Origin must be finite; normal and axis2 must be nonzero and perpendicular");
            }
            ok = cuts.SetPlanePlacement(argv[3],p);
        } else if (action == "fromcsys" && argc == 6) {
            ViewCaeGeom::MeshCutFrame frame;
            const std::string datum(argv[4]);
            if (datum != "global") {
                const auto datumEntry = model.Native->Datums().find(datum);
                if (datumEntry == model.Native->Datums().end()) { throw std::invalid_argument("Unknown datum"); }
                frame = datumEntry->second;
            }
            const int axis = cutInteger(argv[5]);
            if (axis < 1 || axis > 3) { throw std::invalid_argument("CSYS normal axis must be 1, 2 or 3"); }
            const std::array<Graphic3d_Vec3d,3> axes = {frame.X,frame.Y,frame.Z};
            ViewCaeGeom::MeshCutPlanePlacement p;
            if (!p.Define(frame.Origin,axes[axis-1],axes[axis%3])) { throw std::invalid_argument("Invalid CSYS"); }
            ok = cuts.SetPlanePlacement(argv[3],p);
        } else if (action == "translate" && argc == 5) {
            ok = cuts.TranslatePlane(argv[3],cutReal(argv[4]));
        } else if (action == "absrotate" && argc == 6) {
            ok = cuts.RotatePlane(argv[3],cutInteger(argv[4]),cutReal(argv[5]));
        } else if (action == "datum" && argc == 16) {
            ViewCaeGeom::MeshCutFrame frame;
            for (int i = 0; i < 3; ++i) {
                frame.Origin[i] = cutReal(argv[4+i]); frame.X[i] = cutReal(argv[7+i]);
                frame.Y[i] = cutReal(argv[10+i]); frame.Z[i] = cutReal(argv[13+i]);
            }
            if (!model.Native->SetDatum(argv[3],frame)) { throw std::invalid_argument(model.Native->LastError()); }
            ok = true;
        } else if (action == "localmove" && (argc == 9 || argc == 12)) {
            auto definitions = cuts.Definitions();
            auto definition = std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == argv[3]; });
            if (definition == definitions.end()) { throw std::invalid_argument("Unknown local cut"); }
            if (definition->Shape == ViewCaeGeom::MeshCutShape::Plane && argc == 9) {
                definition->Plane = Graphic3d_Vec4d(cutReal(argv[5]),cutReal(argv[6]),cutReal(argv[7]),cutReal(argv[8]));
            } else if ((definition->Shape == ViewCaeGeom::MeshCutShape::Sphere || definition->Shape == ViewCaeGeom::MeshCutShape::Cylinder) && argc == 12) {
                definition->Center = Graphic3d_Vec3d(cutReal(argv[5]),cutReal(argv[6]),cutReal(argv[7]));
                definition->Axis = Graphic3d_Vec3d(cutReal(argv[8]),cutReal(argv[9]),cutReal(argv[10])); definition->Radius = cutReal(argv[11]);
            } else { throw std::invalid_argument("Local edit arguments do not match the cut shape"); }
            if (!model.Native->Edit(*definition,argv[4])) { throw std::invalid_argument(model.Native->LastError()); }
            ok = true;
        } else if (action == "axes" && argc == 6) {
            ok = model.Native->SetReferenceAxes(argv[3],cutBoolean(argv[4]),cutReal(argv[5]));
            if (!ok) { throw std::invalid_argument(model.Native->LastError()); }
        } else if (action == "displaymode" && (argc == 3 || argc == 4)) {
            using Mode = ViewCae::InteractiveCaeMeshPrsMode;
            if (argc == 3) {
                const auto mode = model.View->Style((ViewCaeGeom::MeshCutPortion)0).Mode;
                bool mixed = false;
                for (int i = 0; i < 3; ++i) {
                    const auto part = (ViewCaeGeom::MeshCutPortion)i;
                    mixed |= model.View->Style(part).Mode != mode;
                    for (const auto& cut : cuts.Definitions()) { mixed |= model.View->EffectiveStyle(cut.Id,part).Mode != mode; }
                }
                di << (mixed ? "mixed" : mode == Mode::FlatShaded ? "cloud" : mode == Mode::MeshEdges ? "wireframe"
                    : mode == Mode::ShadedMeshEdges ? "cloud_edges" : "custom");
                return 0;
            }
            const std::string mode(argv[3]);
            if (mode != "cloud" && mode != "wireframe" && mode != "cloud_edges") {
                throw std::invalid_argument("Display mode: cloud, wireframe or cloud_edges");
            }
            model.View->SetMeshDisplayMode(mode == "cloud" ? Mode::FlatShaded : mode == "wireframe" ? Mode::MeshEdges : Mode::ShadedMeshEdges);
            ok = true;
        } else if (action == "cutstyle" && (argc == 5 || argc == 8 || argc == 11)) {
            const int portion = cutInteger(argv[4]);
            if (portion < 0 || portion > 2) { throw std::invalid_argument("Style portion must be 0, 1 or 2"); }
            const auto part = (ViewCaeGeom::MeshCutPortion)portion;
            const std::string owner(argv[3]);
            const bool defaults = owner == "defaults";
            const auto definitions = cuts.Definitions();
            if (!defaults && std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == owner; })) {
                throw std::invalid_argument("Unknown cut style owner");
            }
            auto style = defaults ? model.View->Style(part) : model.View->EffectiveStyle(owner,part);
            if (argc == 5) {
                const bool overridden = !defaults && model.View->StyleOverrides().count({owner,portion}) != 0;
                di << "transparency " << style.Transparency << " width " << style.LineWidth << " mode " << (int)style.Mode
                   << " color {" << style.Color.Red() << " " << style.Color.Green() << " " << style.Color.Blue()
                   << "} override " << (int)overridden; return 0;
            }
            style.Transparency = (float)cutReal(argv[5]); style.LineWidth = (float)cutReal(argv[6]);
            style.Mode = (ViewCae::InteractiveCaeMeshPrsMode)cutInteger(argv[7]);
            if (argc == 11) {
                const Graphic3d_Vec3d rgb(cutReal(argv[8]),cutReal(argv[9]),cutReal(argv[10]));
                for (int i = 0; i < 3; ++i) {
                    if (rgb[i] < 0 || rgb[i] > 1) { throw std::invalid_argument("Style RGB components must be in [0,1]"); }
                }
                style.Color = Quantity_Color(rgb.x(),rgb.y(),rgb.z(),Quantity_TOC_RGB);
            }
            if (defaults) { model.View->SetStyle(part,style); } else { model.View->SetStyle(owner,part,style); }
            ok = true;
        } else if (action == "clearstyle" && argc == 5) {
            const auto definitions = cuts.Definitions();
            if (std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == argv[3]; })) {
                throw std::invalid_argument("Unknown cut style owner");
            }
            model.View->ClearStyle(argv[3],(ViewCaeGeom::MeshCutPortion)cutInteger(argv[4])); ok = true;
        } else if (action == "contour" && (argc == 3 || argc == 8)) {
            auto options = model.View->ContourOptions();
            if (argc == 3) {
                di << "automatic " << (int)options.AutomaticRange << " minimum " << options.Minimum << " maximum " << options.Maximum
                   << " legend " << (int)options.ShowLegend << " intervals " << options.Intervals << " state " << (int)model.View->ContourState(); return 0;
            }
            options.AutomaticRange = cutBoolean(argv[3]); options.Minimum = cutReal(argv[4]); options.Maximum = cutReal(argv[5]);
            options.ShowLegend = cutBoolean(argv[6]); options.Intervals = cutInteger(argv[7]); model.View->SetContourOptions(options); ok = true;
        } else if (action == "probe" && argc == 7) {
            const int index = cutInteger(argv[5]), portion = cutInteger(argv[4]);
            if (index < 0) { throw std::invalid_argument("Negative probe index"); }
            const auto probe = model.View->Probe(argv[3],(ViewCaeGeom::MeshCutPortion)portion,index,cutBoolean(argv[6]));
            printProbe(probe); return 0;
        } else if (action == "picked" && argc == 3) {
            const auto context = ViewerTest::GetAISContext();
            if (context.IsNull()) { throw std::invalid_argument("Picking requires a view"); }
            const auto owner = Handle(ViewCae::MeshCutPickOwner)::DownCast(context->DetectedOwner());
            printProbe(model.View->Probe(owner)); return 0;
        } else if (action == "annotation" && argc == 9) {
            const int index = cutInteger(argv[5]); if (index < 0) { throw std::invalid_argument("Negative annotation probe"); }
            const auto probe = model.View->Probe(argv[3],(ViewCaeGeom::MeshCutPortion)cutInteger(argv[4]),index,cutBoolean(argv[6]));
            ok = model.Native->AddAnnotation(argv[7],probe,argv[8]);
        } else if (action == "annotationsclear" && argc == 3) {
            model.Native->ClearAnnotations(); ok = true;
        } else if (action == "vectors" && (argc == 4 || argc == 10)) {
            if (argc == 4 && std::string(argv[3]) == "none") { model.View->SetVectorField(nullptr); ok = true; }
            else if (argc == 10) {
                const std::string mapping(argv[3]); if (mapping != "point" && mapping != "cell") { throw std::invalid_argument("Invalid vector mapping"); }
                Handle(RWCae::MeshNVecProperty) vector;
                for (const auto& candidate : model.Mesh->Properties()) {
                    if (candidate->Name() == argv[4] && candidate->IsNodal() == (mapping == "point") && candidate->TimeStep() == cutInteger(argv[5])) {
                        if (!vector.IsNull()) { throw std::invalid_argument("Ambiguous vector field"); } vector = candidate;
                    }
                }
                if (vector.IsNull()) { throw std::invalid_argument("Vector field not found"); }
                model.View->SetVectorField(vector,cutReal(argv[6]),cutReal(argv[7]),cutBoolean(argv[8]));
                if (!cutBoolean(argv[9])) { model.View->SetVectorField(nullptr); } ok = true;
            }
        } else if (action == "pickmode" && argc == 4) {
            const int mode = cutInteger(argv[3]); if (mode < 0 || mode > 2 || ViewerTest::GetAISContext().IsNull()) { throw std::invalid_argument("Pick mode requires view and 0/1/2"); }
            ViewerTest::GetAISContext()->Deactivate(model.View);
            if (mode) { ViewerTest::GetAISContext()->Activate(model.View,mode); } ok = true;
        } else if (action == "cadsection" && argc == 6) {
            ok = model.Native->SetCadSection(DBRep::Get(argv[3]),argv[4],cutBoolean(argv[5]));
            if (!ok) { throw std::invalid_argument(model.Native->LastError()); }
        } else if (action == "globalclip" && argc == 8) {
            ok = model.Native->SetGlobalClip(Graphic3d_Vec4d(cutReal(argv[3]),cutReal(argv[4]),cutReal(argv[5]),cutReal(argv[6])),cutBoolean(argv[7]));
        } else if (action == "cadclear" && argc == 3) {
            model.Native->ClearCadSection(); ok = true;
        } else if (action == "importreport" && argc == 3) {
            const auto& report = model.ImportReport;
            auto messages = [&](const std::vector<std::string>& values) {
                di << " {";
                for (const auto& value : values) { di.AppendElement(value.c_str()); }
                di << "}";
            };
            di << "succeeded " << (int)report.succeeded << " source"; di.AppendElement(report.source.c_str());
            di << " stagedSource"; di.AppendElement(report.stagedSource.c_str());
            di << " format"; di.AppendElement(report.format.c_str());
            di << " backend"; di.AppendElement(report.backend.c_str());
            di << " points " << (int)report.points << " cells " << (int)report.cells << " fields " << (int)report.fields;
            di << " errors"; messages(report.errors);
            di << " warnings"; messages(report.warnings);
            di << " attempts {";
            for (const auto& attempt : report.attempts) {
                di << " {backend"; di.AppendElement(attempt.backend.c_str());
                di << " succeeded " << (int)attempt.succeeded << " errors"; messages(attempt.errors);
                di << " warnings"; messages(attempt.warnings); di << "}";
            }
            di << "}";
            return 0;
        } else if (action == "nativeinfo" && argc == 3) {
            const auto counts = model.Native->EventCounts();
            di << "generation " << std::to_string(model.Native->Generation()).c_str() << " dragging " << (int)model.Native->IsDragging();
            const char* names[] = {"begin","update","commit","abort","close"};
            for (int i = 0; i < 5; ++i) { di << " " << names[i] << " " << std::to_string(counts[i]).c_str(); }
            return 0;
        } else if (action == "info" && argc == 3) {
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
                    const auto& p = cut.Placement;
                    di << " origin {" << p.Origin.x() << " " << p.Origin.y() << " " << p.Origin.z()
                       << "} normal {" << p.Normal.x() << " " << p.Normal.y() << " " << p.Normal.z()
                       << "} axis2 {" << p.Axis2.x() << " " << p.Axis2.y() << " " << p.Axis2.z()
                       << "} offset " << p.Offset << " angle " << p.Angle << " rotation_axis " << p.RotationAxis;
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
        } else if (action == "union" && argc == 3) {
            const auto display = cuts.EvaluatePresentation(model.Field);
            di << "unified " << (int)display->Unified << " volume " << display->Volume << " area " << display->Area
               << " primitives " << std::to_string(display->Provenance.size()).c_str() << " provenance {";
            for (const auto& primitive : display->Provenance) {
                di << "{cut"; di.AppendElement(primitive.CutId.c_str());
                di << " portion " << (int)primitive.Portion << " source " << std::to_string(primitive.SourceElement).c_str()
                   << " cap " << (int)primitive.Cap << " dimension " << primitive.Dimension
                   << " primitive " << std::to_string(primitive.Primitive).c_str() << "} ";
            }
            di << "}"; return 0;
        } else if (action == "style" && (argc == 4 || argc == 5)) {
            const std::string portion(argv[3]); const int index = portion == "negative" ? 0 : portion == "on" ? 1 : portion == "positive" ? 2 : -1;
            if (index < 0) { throw std::invalid_argument("Style portion: negative, on or positive"); }
            const auto part = (ViewCaeGeom::MeshCutPortion)index; auto style = model.View->Style(part);
            if (argc == 4) { di << "transparency " << style.Transparency; return 0; }
            style.Transparency = (float)cutReal(argv[4]); model.View->SetStyle(part,style); ok = true;
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
        } else if (action == "renderbackend" && (argc == 3 || argc == 4)) {
            if (argc == 3) { di << (model.View->RenderBackend() == ViewCae::MeshCutRenderBackend::Auto ? "auto" : "cpu"); return 0; }
            const std::string backend = argv[3];
            if (backend != "auto" && backend != "cpu") { throw std::invalid_argument("Render backend must be auto or cpu"); }
            model.View->SetRenderBackend(backend == "auto" ? ViewCae::MeshCutRenderBackend::Auto : ViewCae::MeshCutRenderBackend::Cpu);
            ok = true;
        } else if (action == "renderinfo" && argc == 3) {
            const auto& stats = model.View->RenderStatistics();
            const auto& phases = model.View->Cuts().LastTimings();
            const auto cache = model.View->Cuts().TraceCacheStatistics();
            di << "backend " << (stats.GpuPlane ? "gpu_plane" : "cpu") << " fallback";
            di.AppendElement(stats.FallbackReason.c_str());
            di << " body_builds " << (double)stats.BodyBuilds << " slice_reallocations " << (double)stats.SliceReallocations
               << " slice_evaluations " << (double)stats.SliceEvaluations
               << " submitted_body_bytes " << (double)stats.SubmittedBodyBytes << " submitted_slice_bytes " << (double)stats.SubmittedSliceBytes
               << " source_ms " << stats.SourceMs << " slice_ms " << stats.SliceMs << " fields_ms " << stats.FieldsMs
               << " presentation_ms " << stats.PresentationMs
               << " geometry_slice_ms " << phases.SliceMs << " trace_ms " << phases.TraceMs
               << " bvh_ms " << phases.BvhMs << " node_prepare_ms " << phases.NodePrepareMs
               << " cell_cut_ms " << phases.CellCutMs << " merge_ms " << phases.MergeMs
               << " candidates " << (double)phases.Candidates << " fallbacks " << (double)phases.Fallbacks;
            di << " trace_origins_ms " << phases.TraceOriginsMs << " trace_classify_ms " << phases.TraceClassifyMs
               << " trace_prepare_ms " << phases.TracePrepareMs << " trace_maps_ms " << phases.TraceMapsMs
               << " trace_fill_ms " << phases.TraceFillMs << " trace_general_ms " << phases.TraceGeneralMs;
            di << " interior_hits_total " << (double)cache.Hits << " interior_misses_total " << (double)cache.Misses
               << " interior_evictions_total " << (double)cache.Evictions;
            return 0;
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
        } else if ((action == "range" || action == "motionrange") && argc == 4) {
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
                const auto& p = cut.Placement;
                const double shift = action == "motionrange" ? p.CurrentNormal().Dot(p.Origin) : 0;
                di << lower-shift << " " << upper-shift;
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
                removeHandle(model);
                if (!model.Native->ShowHandle(argv[3],gizmo)) { throw std::invalid_argument(model.Native->LastError()); }
                model.EditedCut = argv[3]; model.Plane = model.Native->PlaneHandle(); model.Quadric = model.Native->QuadricHandle();
                model.Manipulator = model.Native->Manipulator();
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
            if (!model.Native->BindField({})) { throw std::invalid_argument(model.Native->LastError()); }
            model.Field.Nullify(); model.Component = 0; ok = true;
        } else if ((action == "add" || action == "move") && argc == 8) {
            Graphic3d_Vec4d plane(cutReal(argv[4]), cutReal(argv[5]), cutReal(argv[6]), cutReal(argv[7]));
            ok = action == "add" ? cuts.AddPlane(argv[3], plane) : cuts.SetPlane(argv[3], plane);
        } else if ((action == "sphereadd" || action == "spheremove") && argc == 8) {
            const Graphic3d_Vec3d center(cutReal(argv[4]),cutReal(argv[5]),cutReal(argv[6]));
            ok = action == "sphereadd" ? cuts.AddSphere(argv[3],center,cutReal(argv[7]))
                : cuts.SetSphere(argv[3],center,cutReal(argv[7]));
            if (!ok) { throw std::invalid_argument("Sphere requires a positive radius and supported mesh topology; check id and center"); }
        } else if ((action == "cylinderadd" || action == "cylindermove") && argc == 11) {
            const Graphic3d_Vec3d center(cutReal(argv[4]),cutReal(argv[5]),cutReal(argv[6]));
            const Graphic3d_Vec3d axis(cutReal(argv[7]),cutReal(argv[8]),cutReal(argv[9]));
            ok = action == "cylinderadd" ? cuts.AddCylinder(argv[3],center,axis,cutReal(argv[10]))
                : cuts.SetCylinder(argv[3],center,axis,cutReal(argv[10]));
            if (!ok) { throw std::invalid_argument("Cylinder requires a positive radius, nonzero axis and supported mesh topology; check id and center"); }
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
        } else if (action == "copy" && argc == 5) {
            ok = cuts.Duplicate(argv[3], argv[4]);
            if (ok) {
                for (int portion = 0; portion < 3; ++portion) {
                    const auto foundStyle = model.View->StyleOverrides().find({argv[3],portion});
                    if (foundStyle != model.View->StyleOverrides().end()) {
                        model.View->SetStyle(argv[4],(ViewCaeGeom::MeshCutPortion)portion,foundStyle->second);
                    }
                }
            }
        }
        else if (action == "rename" && argc == 5) { ok = cuts.Rename(argv[3], argv[4]); }
        else if (action == "remove" && argc == 4) {
            ok = cuts.Remove(argv[3]);
            if (ok) { for (int portion = 0; portion < 3; ++portion) { model.View->ClearStyle(argv[3],(ViewCaeGeom::MeshCutPortion)portion); } }
        }
        else if (action == "snapshot" && argc == 3) { di << cuts.DumpDefinitionsJson().c_str(); return 0; }
        else if (action == "restore" && argc == 4) { ok = cuts.RestoreDefinitionsJson(argv[3]); }
        else if (action == "deform" && (argc == 3 || argc == 5 || argc == 6)) {
            if (argc == 3) {
                const auto& field = cuts.DeformationField();
                di << "field"; di.AppendElement(field.IsNull() ? "none" : field->Name().ToCString());
                di << " step " << (field.IsNull() ? -1 : field->TimeStep()) << " scale " << cuts.DeformationScale()
                   << " effectiveScale " << cuts.EffectiveDeformationScale();
                return 0;
            }
            const int step = argc == 6 ? cutInteger(argv[5]) : -1;
            const double scale = cutReal(argv[4]);
            Handle(RWCae::MeshNVecProperty) displacement;
            if (std::string(argv[3]) != "none") {
                for (const auto& field : model.Mesh->Properties()) {
                    if (field->Name() != argv[3] || field->TimeStep() != step || !field->IsNodal() || !field->IsVector3()) { continue; }
                    if (!displacement.IsNull()) { throw std::invalid_argument("Ambiguous nodal deformation field"); }
                    displacement = field;
                }
                if (displacement.IsNull()) { throw std::invalid_argument("Deformation requires a matching nodal 3D vector field"); }
            }
            // Cancel a held plane edit before invalidating its geometry chains.
            cancelHandleDrag(model);
            if (std::string(argv[3]) == "none") { ok = cuts.SetDeformation(nullptr, 0.0); }
            else { ok = cuts.SetDeformation(displacement,scale); }
            if (ok) {
                model.Bounds = cuts.SourceBounds();
                if (!model.Plane.IsNull()) { model.Plane->SetBounds(model.Bounds); }
                if (!model.Quadric.IsNull()) { model.Quadric->SetBounds(model.Bounds); }
                di << "effectiveScale " << cuts.EffectiveDeformationScale();
            }
        }
        else if (action == "trace" && (argc == 5 || argc == 6 || argc == 8)) {
            const std::string role(argv[4]);
            if (role != "display" && role != "physical") { throw std::invalid_argument("Trace role: display or physical"); }
            ViewCaeGeom::MeshCutTraceOptions options; bool found = false;
            for (const auto& cut : cuts.Definitions()) {
                if (cut.Id == argv[3]) { options = role == "physical" ? cut.PhysicalTrace : cut.DisplayTrace; found = true; break; }
            }
            if (!found) { throw std::invalid_argument("Unknown trace cut ID"); }
            const char* policies[] = {"lowest","negative","positive","region"};
            if (argc == 5) {
                di << "policy " << policies[(int)options.Policy] << " region " << std::to_string(options.Region).c_str() << " regions {";
                for (auto region : options.Regions) { di << std::to_string(region).c_str() << " "; }
                di << "}"; return 0;
            }
            int policy = -1; for (int i = 0; i < 4; ++i) { if (std::string(argv[5]) == policies[i]) { policy = i; } }
            if (policy < 0 || (policy == 3) != (argc == 8)) { throw std::invalid_argument("Region trace requires an ID and explicit per-element assignments"); }
            options.Policy = (ViewCaeGeom::MeshCutTracePolicy)policy;
            if (policy == 3) {
                options.Region = cutInt64(argv[6]); options.Regions.clear();
                std::istringstream ids(argv[7]); std::string id;
                while (ids >> id) { options.Regions.push_back(cutInt64(id)); }
            }
            ok = cuts.SetTraceOptions(argv[3],role == "physical",options);
            if (!ok) { throw std::invalid_argument("Trace selection is unavailable or ambiguous; check policy, region count and interface topology"); }
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
                    if (!model.Native->BindField({argv[4],mapping == "point",component,step})) { throw std::invalid_argument(model.Native->LastError()); }
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
            if (!model.Native->Attach(ViewerTest::GetAISContext(),ViewerTest::CurrentView())) { throw std::runtime_error(model.Native->LastError()); }
            return 0;
        } else if (action == "clear" && argc == 3) {
            removeHandle(model);
            model.Native->Close();
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
            if (!model.Native->Refresh(false)) { throw std::runtime_error(model.Native->LastError()); }
            ViewerTest::GetAISContext()->UpdateCurrentViewer();
        }
        return 0;
    } catch (const Standard_Failure& error) { di << "Error: " << error.GetMessageString(); }
    catch (const std::exception& error) { di << "Error: " << error.what(); }
    return 1;
}
}

void ViewCaeTest::CutViewCommands::Factory(Draw_Interpretor& di)
{
    ViewCaeTest::RegisterCutBenchmarkCommands(di);
    di.Add("scvcutimagestats", "scvcutimagestats image.png: count non-white and colored pixels", __FILE__,
        imageStats, "SC Viewer CAE Prs");
    di.Add("scvcutimagecompare", "scvcutimagecompare reference.png actual.png: RGB byte differences", __FILE__,
        imageCompare, "SC Viewer CAE Prs");
    di.Add("scvcutview", "scvcutview name read/replace file | info | importreport | defaults | add/move id A B C D | visibility id mask\n"
        "  | isoadd/isomove id point/cell field rawComponent value [timeStep] | isovalue id value\n"
        "  | sphereadd/spheremove id centerX centerY centerZ radius | radius id radius\n"
        "  | cylinderadd/cylindermove id centerX centerY centerZ axisX axisY axisZ radius\n"
        "  | field point/cell name rawComponent [timeStep] | cellmode id constant/projected\n"
        "  | recovery [thresholdPercent beforeInvariant0/1 regionField/none/keep [timeStep]]\n"
        "  | tensor [components/mises/pressure/maxprincipal/midprincipal/minprincipal/tresca layout tensor/engineering]\n"
        "  | trace id display/physical [lowest/negative/positive | region regionId {elementRegionIds}]\n"
        "  | union (display OR volume, exposed area and primitive provenance) | style negative/on/positive [transparency]\n"
        "    layout: xx-yy-zz-xy-xz-yz, xx-yy-zz-xy-yz-xz, row-major, column-major\n"
        "  | copy id newId | rename id name | enable id 0/1 | remove id | eval id | display | clear\n"
        "  | snapshot | restore json | deform [property/none normalizedScale [timeStep]]\n"
        "  | freebody id field xx yy zz xy xz yz originX originY originZ [timeStep]\n"
        "  | heatflow id field componentX componentY componentZ [timeStep]\n"
        "  | list | fields | bounds | range id | stats | presentation id | rotate id axisIndex degrees [originX originY originZ]\n"
        "  | geometry [absoluteTolerance relativeTolerance faceLimit lineLimit maxSourcePrimitives]\n"
        "  | isooptions [fieldAbsoluteTolerance fieldRelativeTolerance minimumDepth limit maxPrimitives]\n"
        "  | geometrymode [numerical/preferred/required]\n"
        "  | renderbackend [auto/cpu] | renderinfo (submitted bytes are not measured GPU traffic)\n"
        "  | handle id 0/1 [gizmo0/1] | handleinfo | fit | field none\n"
        "  | datum name originX originY originZ Xx Xy Xz Yx Yy Yz Zx Zy Zz\n"
        "  | datums | placement id ox oy oz nx ny nz axis2x axis2y axis2z | fromcsys id datum normalAxis1/2/3\n"
        "  | translate id absoluteOffset | absrotate id localAxis2/3 degrees | motionrange id\n"
        "  | timeinfo | step index | timeddeform name/none scale | play period/speed loop restart timestamps | pause | tick seconds\n"
        "  | physics requestId explicitMappingJson | physicsremove/physicsreport/physicscsv requestId\n"
        "  | automatic id crack/density nodalField component threshold sign minimum maximum visibility\n"
        "  | materialframe name reference/updated {step n0 n1 n2}... | follow cut datum\n"
        "  | slicing plane regular/range start spacing/end count | rotational ax ay az ox oy oz start end count | path ux uy uz {x y z}...\n"
        "  | localmove id datum a b c d (plane) / cx cy cz ax ay az radius (quadric)\n"
        "  | axes datum/global show length | nativeinfo | pickmode 0/1/2\n"
        "  | sessionsave file ?CAD-brep-source? | sessionload file | sessioninfo\n"
        "  | cutstyle id/defaults portion [transparency width mode [r g b]] | clearstyle id portion\n"
        "  | displaymode [cloud/wireframe/cloud_edges] (all portions; preserves other style settings)\n"
        "  | contour [automatic min max legend intervals] | probe id portion index node0/1\n"
        "  | vectors none / mapping field step scale sampling orientation show\n"
        "  | annotation id portion index node0/1 annotationId text | annotationsclear\n"
        "  | cadsection shape cut syncClip | cadclear | globalclip a b c d enabled\n"
        "  | project worldX worldY worldZ | mouse move/down/up/abort pixelX pixelY",
        __FILE__, cutView, "SC Viewer CAE Prs");
}
