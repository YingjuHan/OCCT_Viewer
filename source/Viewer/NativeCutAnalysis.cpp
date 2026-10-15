#include <Viewer/NativeCutAnalysis.h>
#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace Viewer {
NativeCutAnalysis::~NativeCutAnalysis() { Close(); }
ViewCaeGeom::MeshCutPhysicsReport NativeCutAnalysis::compute(const NativeCutAnalysisRequest& request) const
{
    if (request.Cut.empty() || request.Field.Name.empty() || !std::isfinite(request.ArrowScale) || request.ArrowScale <= 0
        || request.Options.Normal) { throw std::invalid_argument("Native physics requires cut/field/positive arrow scale and serializable analytic normals"); }
    const auto field = myOwner->resolveField(request.Field);
    auto options = request.Options; options.SourceGeometry = myOwner->Display()->Cuts().SourceGeometry();
    if (request.Beam) {
        const auto definitions = myOwner->Display()->Cuts().Definitions();
        const auto plane = std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == request.Cut; });
        if (plane == definitions.end()) { throw std::invalid_argument("Unknown physical beam cut"); }
        return ViewCaeGeom::MeshCutPhysics::Beam(*plane,field,request.Components,options);
    }
    const auto cut = myOwner->Display()->Cuts().EvaluatePhysical(request.Cut,field);
    if (request.Heat) { return ViewCaeGeom::MeshCutPhysics::HeatFlux(cut,{{request.Components[0],request.Components[1],request.Components[2]}},options); }
    return ViewCaeGeom::MeshCutPhysics::Stress(cut,request.Components,options);
}
std::vector<Handle(AIS_Shape)> NativeCutAnalysis::arrows(const NativeCutAnalysisRequest& request,
    const ViewCaeGeom::MeshCutPhysicsReport& report) const
{
    std::vector<Handle(AIS_Shape)> result;
    if (!request.Overlay) { return result; }
    const auto& frame = request.Options.OutputFrame;
    for (const auto& item : report.Distributed) {
        const auto position = frame.PointToGlobal(item.Position/request.Options.Units.LengthToSI);
        auto draw = [&](Graphic3d_Vec3d vector, Quantity_NameOfColor color) {
            vector = frame.VectorToGlobal(vector)*request.ArrowScale;
            const double length = vector.Modulus(); if (length <= 1e-12) { return; }
            if (!std::isfinite(length)) { throw std::invalid_argument("Resultant arrow overflows model units"); }
            const auto direction = vector/length;
            auto side = Graphic3d_Vec3d::Cross(direction,std::abs(direction.z()) < 0.9 ? Graphic3d_Vec3d(0,0,1) : Graphic3d_Vec3d(0,1,0)); side /= side.Modulus();
            const auto tip = position+vector;
            auto point = [](const Graphic3d_Vec3d& p) { return gp_Pnt(p.x(),p.y(),p.z()); };
            BRep_Builder builder; TopoDS_Compound shape; builder.MakeCompound(shape);
            builder.Add(shape,BRepBuilderAPI_MakeEdge(point(position),point(tip)).Shape());
            for (double sign : {-1.0,1.0}) { builder.Add(shape,BRepBuilderAPI_MakeEdge(point(tip),point(tip-vector*0.2+side*(sign*length*0.08))).Shape()); }
            Handle(AIS_Shape) display = new AIS_Shape(shape); display->SetColor(Quantity_Color(color)); display->SetWidth(3); result.push_back(display);
        };
        draw(item.Result.Force,Quantity_NOC_RED); draw(item.Result.Moment,Quantity_NOC_BLUE);
        if (request.Heat) { draw(item.Normal*item.Result.HeatFlow,Quantity_NOC_ORANGE); }
    }
    return result;
}
void NativeCutAnalysis::publish(const std::string& id, const NativeCutAnalysisRequest& request,
    const ViewCaeGeom::MeshCutPhysicsReport& report,const std::vector<Handle(AIS_Shape)>& overlays)
{
    const auto context = myOwner->myContext;
    if (!context.IsNull()) {
        try { for (const auto& display : overlays) { context->Display(display,false); } }
        catch (...) { for (const auto& display : overlays) { context->Remove(display,false); } throw; }
        if (myOverlays.count(id)) { for (const auto& display : myOverlays.at(id)) { context->Remove(display,false); } }
    }
    myRequests[id] = request; myReports[id] = report; myOverlays[id] = overlays;
}
bool NativeCutAnalysis::Set(const std::string& id,const NativeCutAnalysisRequest& request)
{
    return myOwner->attempt([&] {
        if (id.empty() || id.size() > 256) { throw std::invalid_argument("Invalid native physics request ID"); }
        const auto report = compute(request); publish(id,request,report,arrows(request,report));
    });
}
bool NativeCutAnalysis::Remove(const std::string& id)
{
    return myOwner->attempt([&] {
        if (!myRequests.count(id)) { throw std::invalid_argument("Unknown physical request"); }
        if (!myOwner->myContext.IsNull()) { for (const auto& display : myOverlays[id]) { myOwner->myContext->Remove(display,false); } }
        myRequests.erase(id); myReports.erase(id); myOverlays.erase(id);
    });
}
bool NativeCutAnalysis::Refresh()
{
    return myOwner->attempt([&] {
        const auto definitions = myOwner->Display()->Cuts().Definitions();
        std::map<std::string,ViewCaeGeom::MeshCutPhysicsReport> reports;
        std::map<std::string,std::vector<Handle(AIS_Shape)>> overlays;
        for (const auto& entry : myRequests) {
            if (std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == entry.second.Cut; })) { continue; }
            reports[entry.first] = compute(entry.second); overlays[entry.first] = arrows(entry.second,reports.at(entry.first));
        }
        const auto context = myOwner->myContext;
        if (!context.IsNull()) {
            try { for (const auto& entry : overlays) { for (const auto& display : entry.second) { context->Display(display,false); } } }
            catch (...) { for (const auto& entry : overlays) { for (const auto& display : entry.second) { context->Remove(display,false); } } throw; }
        }
        Detach();
        for (auto entry = myRequests.begin(); entry != myRequests.end();) {
            if (!reports.count(entry->first)) { entry = myRequests.erase(entry); } else { ++entry; }
        }
        myReports = std::move(reports); myOverlays = std::move(overlays);
        for(auto entry=myAutomatic.begin();entry!=myAutomatic.end();) {
            if(std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id==entry->first; })) { entry=myAutomatic.erase(entry); } else { ++entry; }
        }
    });
}
void NativeCutAnalysis::Detach()
{
    if (myOwner->myContext.IsNull()) { return; }
    for (const auto& entry : myOverlays) { for (const auto& display : entry.second) { myOwner->myContext->Remove(display,false); } }
}
void NativeCutAnalysis::Close() { Detach(); myRequests.clear(); myReports.clear(); myOverlays.clear(); mySteps.clear(); myAutomatic.clear(); }
bool NativeCutAnalysis::PrepareStep(const NativeCutAnalysis& source,int step,bool remesh)
{
    return myOwner->attempt([&] {
        mySteps = source.mySteps;
        for (const auto& entry : source.Requests()) {
            auto request = entry.second;
            if (mySteps.count(step) && mySteps.at(step).count(entry.first)) { request = mySteps.at(step).at(entry.first); }
            else if (remesh && (!request.Options.Elements.empty() || !request.Options.Sections.empty() || !request.Options.ElementNormals.empty())) { throw std::invalid_argument("Remeshed physics needs an explicit per-step group/section/normal mapping"); }
            request.Field.Step = step;
            for (auto& section : request.Options.Sections) {
                for (auto& layer : section.second.Layers) {
                    if(layer.Field.IsNull()) { throw std::invalid_argument("Missing step layer binding"); }
                    layer.Field = myOwner->resolveField({layer.Field->Name().ToCString(),layer.Field->IsNodal(),0,step});
                }
            }
            if (!Set(entry.first,request)) { throw std::invalid_argument(myOwner->LastError()); }
        }
        myAutomatic = source.myAutomatic;
        for (auto& entry : myAutomatic) {
            entry.second.Field.Step = step;
            entry.second.Mapping.Field = myOwner->resolveField(entry.second.Field);
            ViewCaeGeom::MeshCutViewManager check(myOwner->Display()->SourceMesh());
            ViewCaeGeom::MeshCutAutomatic::Add(check,entry.first,entry.second.Mapping);
        }
    });
}
bool NativeCutAnalysis::Automatic(const std::string& id,const CutFieldBinding& field,ViewCaeGeom::MeshCutAutomaticMapping mapping)
{
    return myOwner->attempt([&] {
        mapping.Field = myOwner->resolveField(field); mapping.Component = field.Component;
        ViewCaeGeom::MeshCutAutomatic::Add(myOwner->Display()->Cuts(),id,mapping);
        try { myOwner->Display()->Synchronize(); }
        catch (...) { myOwner->Display()->Cuts().Remove(id); throw; }
        myAutomatic[id] = {field,mapping};
    });
}
}
