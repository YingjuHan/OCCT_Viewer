#include <Viewer/NativeCutAnalysis.h>
#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <RWCae/MeshGroupProperty.h>

namespace Viewer {
NativeCutAnalysis::~NativeCutAnalysis() { Close(); }
ViewCaeGeom::MeshCutPhysicsReport NativeCutAnalysis::compute(const NativeCutAnalysisRequest& request) const
{
    if (request.Cut.empty() || request.Field.Name.empty() || !std::isfinite(request.ArrowScale) || request.ArrowScale <= 0
        || request.Options.Normal || request.Options.DomainContains || (int)request.Resolution < 0 || (int)request.Resolution > 2
        || (request.DomainSource != "explicit" && request.DomainSource != "element_set" && request.DomainSource != "display_snapshot")) { throw std::invalid_argument("Native physics requires cut/field/positive arrow scale and serializable analytic normals/domains"); }
    const auto field = myOwner->resolveField(request.Field);
    auto options = request.Options; options.SourceGeometry = myOwner->Display()->Cuts().SourceGeometry();
    if (!request.DomainField.Name.empty()) {
        const auto domain = myOwner->resolveField(request.DomainField);
        if (domain.IsNull() || domain->IsNodal() || domain->HasNodeVecComponents() || !domain->IsInitialized()
            || domain->NbValues() != (int)myOwner->Display()->SourceMesh()->NbIndexedElems()
            || request.DomainField.Component < 0 || request.DomainField.Component >= domain->NbRawComponents()
            || !std::isfinite(request.DomainValue)) { throw std::invalid_argument("Element set requires an initialized cell label field with matching tuples"); }
        const auto component = domain->GetRawComponent(request.DomainField.Component);
        const auto group = Handle(RWCae::MeshGroupProperty)::DownCast(component);
        if (request.DomainMask && (group.IsNull() || request.DomainValue < 0 || request.DomainValue >= RWCae::MeshGroupProperty::MaxNbGroups || std::trunc(request.DomainValue) != request.DomainValue)) { throw std::invalid_argument("Group mask domain requires an explicit valid MeshGroupProperty bit"); }
        options.GroupName = "element_set/"+request.DomainField.Name;
        options.DomainContains = [component,group,value=request.DomainValue,mask=request.DomainMask](RWCae::MeshEntityId e) {
            return mask ? RWCae::MeshGroupProperty::IsInGroup(group->GetGroups((int)e),(int)value)
                        : component->Value(RWCae::MeshPropertyEntityInfo(e)) == value;
        };
    }
    if (request.Resolution == CutComponentResolution::Datum) {
        if (request.Datum == "global") { options.OutputFrame = ViewCaeGeom::MeshCutFrame(); }
        else {
            const auto datum = myOwner->Datums().find(request.Datum);
            if (datum == myOwner->Datums().end()) { throw std::invalid_argument("Unknown free-body component coordinate system"); }
            options.OutputFrame = datum->second;
        }
    } else if (request.Resolution == CutComponentResolution::NormalTangential) { options.OutputFrame = ViewCaeGeom::MeshCutFrame(); }
    ViewCaeGeom::MeshCutPhysicsReport report;
    if (request.Beam) {
        const auto definitions = myOwner->Display()->Cuts().Definitions();
        const auto plane = std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == request.Cut; });
        if (plane == definitions.end()) { throw std::invalid_argument("Unknown physical beam cut"); }
        if (plane->FollowDeformation) { options.ReferenceGeometry = myOwner->Display()->Cuts().MaterialReferenceGeometry(request.Cut); }
        report = ViewCaeGeom::MeshCutPhysics::Beam(*plane,field,request.Components,options);
    } else {
        const auto cut = myOwner->Display()->Cuts().EvaluatePhysicalSection(request.Cut,field);
        report = request.Heat ? ViewCaeGeom::MeshCutPhysics::HeatFlux(cut,{{request.Components[0],request.Components[1],request.Components[2]}},options)
            : ViewCaeGeom::MeshCutPhysics::Stress(cut,request.Components,options);
    }
    if (request.Resolution == CutComponentResolution::NormalTangential) {
        auto& frame = report.OutputFrame; frame.X = report.MeanNormal;
        const double length = frame.X.Modulus();
        if (!std::isfinite(length) || length < 1e-12) { throw std::invalid_argument("Cut has no unique mean normal; choose an explicit CSYS"); }
        frame.X /= length;
        auto tangent = request.TangentY;
        for (int c=0;c<3;++c) { if(!std::isfinite(tangent[c])) { throw std::invalid_argument("Invalid free-body tangent direction"); } }
        tangent -= frame.X*tangent.Dot(frame.X);
        if (tangent.Modulus() <= 1e-12) { throw std::invalid_argument("Free-body tangent must not be parallel to the section normal"); }
        frame.Y = tangent/tangent.Modulus(); frame.Z = Graphic3d_Vec3d::Cross(frame.X,frame.Y);
        // Re-sum in the resolved frame: absolute contributions must be taken in that frame.
        auto resolved = request; resolved.Resolution = CutComponentResolution::DefinedFrame; resolved.Options.OutputFrame = frame;
        return compute(resolved);
    }
    return report;
}
std::vector<Handle(AIS_Shape)> NativeCutAnalysis::arrows(const NativeCutAnalysisRequest& request,
    const ViewCaeGeom::MeshCutPhysicsReport& report) const
{
    std::vector<Handle(AIS_Shape)> result;
    if (!request.Overlay || !request.Enabled || myOwner->Display()->Cuts().MultipleSurfacesOnly()) { return result; }
    const auto definitions = myOwner->Display()->Cuts().Definitions();
    if (std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == request.Cut && d.Enabled; })) { return result; }
    const auto& frame = report.OutputFrame;
    std::vector<ViewCaeGeom::MeshCutDistributedResult> total;
    if (!request.Distributed) {
        ViewCaeGeom::MeshCutDistributedResult item; item.Position = report.MomentOrigin; item.Normal = report.MeanNormal; item.Result = report.Total; total.push_back(item);
    } else if (report.Distributed.size() > 1024) { throw std::invalid_argument("Distributed arrows exceed 1024 samples; use one resultant per cut or slicing"); }
    for (const auto& item : request.Distributed ? report.Distributed : total) {
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
        if (request.ComponentVectors) {
            for (int c = 0; c < 3; ++c) {
                Graphic3d_Vec3d force(0), moment(0); force[c] = item.Result.Force[c]; moment[c] = item.Result.Moment[c];
                draw(force,Quantity_NOC_RED); draw(moment,Quantity_NOC_BLUE);
            }
        } else { draw(item.Result.Force,Quantity_NOC_RED); draw(item.Result.Moment,Quantity_NOC_BLUE); }
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
    myRequests[id] = request; myOverlays[id] = overlays;
    if (request.Enabled) { myReports[id] = report; } else { myReports.erase(id); }
}
bool NativeCutAnalysis::Set(const std::string& id,const NativeCutAnalysisRequest& request)
{
    return myOwner->attempt([&] {
        if (id.empty() || id.size() > 256) { throw std::invalid_argument("Invalid native physics request ID"); }
        if (!request.Enabled) {
            const auto definitions = myOwner->Display()->Cuts().Definitions();
            if (std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == request.Cut; })
                || request.Field.Name.empty() || !request.Options.Units.IsValid() || !std::isfinite(request.ArrowScale) || request.ArrowScale <= 0
                || request.Options.Normal || request.Options.DomainContains || !request.Options.FieldFrame.IsValid() || !request.Options.OutputFrame.IsValid()
                || (int)request.Resolution < 0 || (int)request.Resolution > 2 || !std::isfinite(request.DomainValue)
                || (request.Options.NormalSign != 1 && request.Options.NormalSign != -1)) { throw std::invalid_argument("Invalid disabled free-body configuration"); }
            publish(id,request,{},{});
        } else {
            const auto report = compute(request); publish(id,request,report,arrows(request,report));
        }
    });
}
bool NativeCutAnalysis::SetEnabled(const std::string& id,bool enabled)
{
    return myOwner->attempt([&] {
        const auto found = myRequests.find(id); if (found == myRequests.end()) { throw std::invalid_argument("Configure free-body fields before enabling this cut"); }
        auto request = found->second; request.Enabled = enabled;
        if (!Set(id,request)) { throw std::invalid_argument(myOwner->LastError()); }
    });
}
bool NativeCutAnalysis::SetCutEnabled(const std::string& cut,bool enabled)
{
    return myOwner->attempt([&] {
        struct Candidate { std::string Id; NativeCutAnalysisRequest Request; ViewCaeGeom::MeshCutPhysicsReport Report; std::vector<Handle(AIS_Shape)> Overlays; };
        std::vector<Candidate> candidates;
        for (const auto& entry : myRequests) {
            if (entry.second.Cut != cut) { continue; }
            Candidate c; c.Id = entry.first; c.Request = entry.second; c.Request.Enabled = enabled;
            if (enabled) { c.Report = compute(c.Request); c.Overlays = arrows(c.Request,c.Report); }
            candidates.push_back(std::move(c));
        }
        if (enabled && candidates.empty()) { throw std::invalid_argument("Open Free Body options and configure the physical field first"); }
        // Stage every AIS presentation before replacing any live request.
        const auto context = myOwner->myContext;
        if (!context.IsNull()) {
            try { for (const auto& c : candidates) { for (const auto& display : c.Overlays) { context->Display(display,false); } } }
            catch (...) { for (const auto& c : candidates) { for (const auto& display : c.Overlays) { context->Remove(display,false); } } throw; }
            for (const auto& c : candidates) { for (const auto& display : myOverlays[c.Id]) { context->Remove(display,false); } }
        }
        for (const auto& c : candidates) {
            myRequests[c.Id] = c.Request; myOverlays[c.Id] = c.Overlays;
            if (enabled) { myReports[c.Id] = c.Report; } else { myReports.erase(c.Id); }
        }
    });
}
bool NativeCutAnalysis::CaptureDisplayGroup(const std::string& id,const Handle(ViewCae::InteractiveCaeMesh)& mesh,const std::string& presentation)
{
    return myOwner->attempt([&] {
        const auto found = myRequests.find(id);
        if (found == myRequests.end() || mesh.IsNull() || mesh->GetMeshData() != myOwner->Display()->SourceMesh()) { throw std::invalid_argument("Display group must belong to the same source mesh and a configured free body"); }
        const auto prs = mesh->Presentation(presentation.c_str());
        if (prs.IsNull()) { throw std::invalid_argument("Unknown source display-group presentation"); }
        const auto geometry = prs->GetMeshGeometry(); geometry->GetMeshData();
        auto request = found->second; request.DomainField = {}; request.DomainSource = "display_snapshot";
        request.Options.Elements.clear(); request.Options.GroupName = "display/"+presentation;
        if (prs->VisProps()->IsVisible()) {
            for (RWCae::MeshEntityId e=0;e<mesh->GetMeshData()->NbIndexedElems();++e) { if (geometry->AcceptElement(e)) { request.Options.Elements.insert(e); } }
        }
        if (request.Options.Elements.empty()) { throw std::invalid_argument("Selected display group is empty; whole-model fallback is forbidden"); }
        if (!Set(id,request)) { throw std::invalid_argument(myOwner->LastError()); }
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
            if (!entry.second.Enabled || std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == entry.second.Cut && d.Enabled; })) { continue; }
            reports[entry.first] = compute(entry.second); overlays[entry.first] = arrows(entry.second,reports.at(entry.first));
        }
        const auto context = myOwner->myContext;
        if (!context.IsNull()) {
            try { for (const auto& entry : overlays) { for (const auto& display : entry.second) { context->Display(display,false); } } }
            catch (...) { for (const auto& entry : overlays) { for (const auto& display : entry.second) { context->Remove(display,false); } } throw; }
        }
        Detach();
        for (auto entry = myRequests.begin(); entry != myRequests.end();) {
            if (std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == entry->second.Cut; })) { entry = myRequests.erase(entry); } else { ++entry; }
        }
        myReports = std::move(reports); myOverlays = std::move(overlays);
        for(auto entry=myAutomatic.begin();entry!=myAutomatic.end();) {
            if(std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id==entry->first; })) { entry=myAutomatic.erase(entry); } else { ++entry; }
        }
        for(const auto& d:definitions) {
            if(myAutomatic.count(d.Id)) {
                auto& mapping=myAutomatic.at(d.Id).Mapping;
                mapping.Threshold=d.IsoValue; mapping.Visibility=d.Visibility; mapping.Enabled=d.Enabled;
            }
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
            if (!request.DomainField.Name.empty()) { request.DomainField.Step = step; }
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
            const auto definitions=myOwner->Display()->Cuts().Definitions();
            const auto d=std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id==entry.first; });
            if(d==definitions.end()) { throw std::invalid_argument("Automatic definition disappeared on target frame"); }
            entry.second.Mapping.Threshold=d->IsoValue; entry.second.Mapping.Visibility=d->Visibility; entry.second.Mapping.Enabled=d->Enabled;
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
        catch (...) { myOwner->Display()->Cuts().RemoveAutomatic(id,mapping.Provider); throw; }
        myAutomatic[id] = {field,mapping};
    });
}
bool NativeCutAnalysis::SyncAutomatic(const std::string& provider,const std::map<std::string,AutomaticBinding>& bindings)
{
    return myOwner->attempt([&] {
        auto next=myAutomatic;
        for(auto it=next.begin();it!=next.end();) {
            if(it->second.Mapping.Provider==provider) { it=next.erase(it); } else { ++it; }
        }
        std::map<std::string,ViewCaeGeom::MeshCutAutomaticMapping> mappings;
        for(auto entry:bindings) {
            entry.second.Mapping.Provider=provider;
            entry.second.Mapping.Field=myOwner->resolveField(entry.second.Field);
            entry.second.Mapping.Component=entry.second.Field.Component;
            mappings.emplace(entry.first,entry.second.Mapping); next[entry.first]=entry.second;
        }
        auto& cuts=myOwner->Display()->Cuts(); const auto previous=cuts.DumpDefinitionsJson();
        if(!ViewCaeGeom::MeshCutAutomatic::Replace(cuts,provider,mappings)) { throw std::invalid_argument("Invalid automatic metadata snapshot; IDs, fields, ownership or activations conflict"); }
        try { myOwner->Display()->Synchronize(); }
        catch (...) {
            if(!cuts.RestoreDefinitionsJson(previous)) { throw std::runtime_error("Automatic snapshot rollback failed"); }
            throw;
        }
        myAutomatic=std::move(next);
    });
}
bool NativeCutAnalysis::ScanSlices(const std::string& collection,const NativeCutAnalysisRequest& request,
    bool sumOnPath,std::vector<ViewCaeGeom::MeshCutPhysicsReport>& reports)
{
    return myOwner->attempt([&] {
        auto& original=myOwner->Display()->Cuts(); NativeCutView temporary(myOwner->Display()->SourceMesh());
        auto& cuts=temporary.Display()->Cuts();
        if(!cuts.ShareCacheFrom(original) || !cuts.SetCachePolicy(original.CachePolicy()) || !cuts.SetGeometryOptions(original.GeometryOptions())
            || !cuts.SetDeformation(original.DeformationField(),original.DeformationScale())) { throw std::invalid_argument("Cannot stage slice scan configuration"); }
        for(const auto& datum:myOwner->Datums()) { temporary.SetDatum(datum.first,datum.second); }
        auto definitions=original.Definitions();
        std::stable_sort(definitions.begin(),definitions.end(),[](const auto& a,const auto& b) { return a.SliceCoordinate<b.SliceCoordinate; });
        std::vector<ViewCaeGeom::MeshCutPhysicsReport> prepared;
        for(const auto& d:definitions) {
            if(d.SliceCollection!=collection || collection.empty()) { continue; }
            if(!cuts.AddPlane(d.Id,d.Plane) || !cuts.SetPlanePlacement(d.Id,d.Placement)
                || !cuts.SetTraceOptions(d.Id,true,d.PhysicalTrace)) { throw std::invalid_argument("Invalid scan plane/provenance"); }
            auto r=request; r.Cut=d.Id; r.Overlay=false; r.Enabled=true;
            if(sumOnPath) { r.Options.SumPoint=ViewCaeGeom::MeshCutSummationPoint::UserDefined; r.Options.MomentOrigin=d.Placement.Origin; }
            if(!temporary.Analysis().Set("scan",r)) { throw std::invalid_argument(temporary.LastError()); }
            auto report=temporary.Analysis().Reports().at("scan"); report.IntervalBegin=d.SliceBegin; report.IntervalEnd=d.SliceEnd;
            if(!request.Distributed) { report.Distributed.clear(); }
            report.IntervalUnits=d.SliceUnits; prepared.push_back(std::move(report));
            temporary.Analysis().Remove("scan"); cuts.Remove(d.Id);
        }
        if(prepared.empty()) { throw std::invalid_argument("Unknown/empty slice collection"); }
        reports=std::move(prepared);
    });
}
}
