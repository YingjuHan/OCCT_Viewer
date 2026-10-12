#include <Viewer/NativeCutTimeline.h>
#include <Viewer/NativeCutAnalysis.h>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <TColStd_ListOfInteger.hxx>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace {
bool sameTopology(const Handle(RWCae::MeshData)& a, const Handle(RWCae::MeshData)& b)
{
    if (a->NbNodes() != b->NbNodes() || a->NbIndexedElems() != b->NbIndexedElems()) { return false; }
    std::vector<RWCae::MeshEntityId> left, right;
    for (RWCae::MeshEntityId e = 0; e < a->NbIndexedElems(); ++e) {
        if (a->IndexedElements()[e].Type != b->IndexedElements()[e].Type) { return false; }
        a->ElementNodes(e,left); b->ElementNodes(e,right); if (left != right) { return false; }
        if (a->IndexedElements()[e].Type == RWCae::MeshElemType::Generic) {
            const auto& x = a->GenericCells().at(a->IndexedElements()[e].Index);
            const auto& y = b->GenericCells().at(b->IndexedElements()[e].Index);
            if (x.Type != y.Type || x.Faces != y.Faces || x.Degrees != y.Degrees || x.RationalWeights != y.RationalWeights) { return false; }
        }
    }
    return true;
}
ViewCaeGeom::MeshCutDefinition localDefinition(ViewCaeGeom::MeshCutDefinition definition, const ViewCaeGeom::MeshCutFrame& frame)
{
    if (definition.Shape == ViewCaeGeom::MeshCutShape::Plane) { definition.Plane = frame.PlaneToLocal(definition.Plane); }
    else { definition.Center = frame.PointToLocal(definition.Center); definition.Axis = frame.VectorToLocal(definition.Axis); }
    return definition;
}
}
namespace Viewer {
NativeCutTimeline::NativeCutTimeline(NativeCutView& owner) : myOwner(owner)
{
    if (owner.Display().IsNull()) { throw std::invalid_argument("Timeline requires an open native cut model"); }
    mySource = owner.Display()->SourceMesh();
    myStep = mySource->SeriesSteps().empty() ? -1 : 0;
}
Handle(RWCae::MeshNVecProperty) NativeCutTimeline::resolve(const Handle(RWCae::MeshData)& mesh, const CutFieldBinding& binding) const
{
    if (binding.Name.empty()) { return nullptr; }
    Handle(RWCae::MeshNVecProperty) field;
    for (const auto& candidate : mesh->Properties()) {
        if (candidate->Name().ToCString() == binding.Name && candidate->IsNodal() == binding.Nodal && candidate->TimeStep() == binding.Step) {
            if (!field.IsNull()) { throw std::invalid_argument("Ambiguous coordinated field binding"); } field = candidate;
        }
    }
    if (field.IsNull() || !field->IsInitialized() || binding.Component < 0 || binding.Component >= field->NbRawComponents()
        || field->NbValues() != (int)(binding.Nodal ? mesh->NbNodes() : mesh->NbIndexedElems())) {
        throw std::invalid_argument("Coordinated step has a missing field, component or tuple mapping: "+binding.Name);
    }
    return field;
}
ViewCaeGeom::MeshCutFrame NativeCutTimeline::materialFrame(const CutMaterialFrame& binding, int step,
    const Handle(RWCae::MeshData)& mesh, const Handle(RWCae::MeshNVecProperty)& deformation, double effectiveScale) const
{
    const bool reference = binding.Configuration == CutFrameConfiguration::Reference;
    const auto& source = reference ? (mySource->SeriesSteps().empty() ? mySource : mySource->SeriesSteps().front().Mesh) : mesh;
    const auto anchors = binding.Anchors.find(reference ? -1 : step);
    if (anchors == binding.Anchors.end()) { throw std::invalid_argument("Material frame lacks explicit anchor mapping for this configuration/step"); }
    Graphic3d_Vec3d points[3];
    for (int i = 0; i < 3; ++i) {
        const auto node = anchors->second[i]; if (node >= source->NbNodes()) { throw std::invalid_argument("Material anchor outside source mesh"); }
        points[i] = Graphic3d_Vec3d(source->Nodes()[node]);
        if (!reference && !deformation.IsNull()) {
            Graphic3d_Vec3d displacement; deformation->GetVector(RWCae::MeshPropertyEntityInfo(node),displacement);
            points[i] += displacement*effectiveScale;
        }
    }
    auto unit = [](const Graphic3d_Vec3d& vector) {
        const double length = std::hypot(vector.x(),vector.y(),vector.z());
        if (!std::isfinite(length) || length <= 1e-12) { throw std::invalid_argument("Collinear or nonfinite material frame anchors"); }
        return vector/length;
    };
    ViewCaeGeom::MeshCutFrame frame; frame.Origin = points[0]; frame.X = unit(points[1]-points[0]);
    frame.Y = unit(points[2]-points[0]-frame.X*frame.X.Dot(points[2]-points[0])); frame.Z = Graphic3d_Vec3d::Cross(frame.X,frame.Y);
    if (!frame.IsValid()) { throw std::invalid_argument("Invalid material frame"); } return frame;
}
bool NativeCutTimeline::SetMaterialFrame(const std::string& name, const CutMaterialFrame& binding)
{
    return myOwner.attempt([&] {
        const auto frame = materialFrame(binding,myStep,myOwner.Display()->SourceMesh(),resolve(myOwner.Display()->SourceMesh(),myDeformation),myOwner.Display()->Cuts().EffectiveDeformationScale());
        if (!myOwner.SetDatum(name,frame)) { throw std::invalid_argument(myOwner.LastError()); }
        myMaterialFrames[name] = binding;
    });
}
bool NativeCutTimeline::BindCutFrame(const std::string& id, const std::string& datum)
{
    return myOwner.attempt([&] {
        const auto definitions = myOwner.Display()->Cuts().Definitions();
        const auto found = std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == id; });
        if (found == definitions.end() || found->Shape == ViewCaeGeom::MeshCutShape::Isosurface || !myOwner.Datums().count(datum)) {
            throw std::invalid_argument("Material following requires a geometric cut and an explicit datum");
        }
        myCutFrames[id] = datum;
    });
}
bool NativeCutTimeline::BindDeformation(const CutFieldBinding& binding, double scale)
{
    if (!prepare(myStep,binding,scale)) { return false; }
    myDeformation = binding; myDeformation.Step = myStep; myScale = scale; return true;
}
bool NativeCutTimeline::SwitchStep(int step)
{
    if (!prepare(step,myDeformation,myScale)) { return false; }
    myStep = step; if (!myDeformation.Name.empty()) { myDeformation.Step = step; } return true;
}
bool NativeCutTimeline::prepare(int step, const CutFieldBinding& deformation, double scale)
{
    return myOwner.attempt([&] {
        if (!std::isfinite(scale)) { throw std::invalid_argument("Nonfinite deformation scale"); }
        const auto& series = mySource->SeriesSteps();
        if ((series.empty() && step != -1) || (!series.empty() && (step < 0 || (size_t)step >= series.size()))) {
            throw std::invalid_argument("Coordinated step is outside the series");
        }
        const auto mesh = series.empty() ? mySource : series[step].Mesh;
        if (mesh.IsNull()) { throw std::invalid_argument("Coordinated step has no geometry snapshot"); }
        NativeCutView staged(mesh); const auto candidate = staged.Display();
        auto& oldCuts = myOwner.Display()->Cuts();
        if(!candidate->Cuts().ShareCacheFrom(oldCuts)) { throw std::runtime_error("Cannot share coordinated cache accounting"); }
        auto definitions = oldCuts.Definitions();
        if (myOwner.IsDragging()) {
            ViewCaeGeom::MeshCutViewManager baseline(myOwner.Display()->SourceMesh());
            if(!baseline.ShareCacheFrom(oldCuts)||!baseline.SetCachePolicy(oldCuts.CachePolicy())
                ||!baseline.SetGeometryOptions(oldCuts.GeometryOptions())
                ||!baseline.SetDeformation(oldCuts.DeformationField(),oldCuts.DeformationScale())) { throw std::runtime_error("Cannot stage drag baseline options/accounting"); }
            if (!baseline.RestoreDefinitionsJson(myOwner.myDragSnapshot)) { throw std::runtime_error("Cannot stage the drag baseline"); }
            definitions = baseline.Definitions();
        }
        const bool remesh = !sameTopology(myOwner.Display()->SourceMesh(),mesh);
        auto recovery = oldCuts.RecoveryOptions();
        if (myRecoverySteps.count(step)) { recovery = myRecoverySteps.at(step); }
        else if (remesh && !recovery.Regions.empty()) { throw std::invalid_argument("Remesh requires explicit recovery-region mapping for the target step"); }
        if (!candidate->Cuts().SetGeometryOptions(oldCuts.GeometryOptions()) || !candidate->Cuts().SetRecoveryOptions(recovery)
            || !candidate->Cuts().SetIsoOptions(oldCuts.IsoOptions()) || !candidate->Cuts().SetCachePolicy(oldCuts.CachePolicy())) { throw std::invalid_argument("Invalid target step options"); }
        auto contour = myOwner.FieldBinding(); if (!contour.Name.empty()) { contour.Step = step; }
        const auto field = resolve(mesh,contour);
        const auto oldField = myOwner.Display()->Field();
        if (!oldField.IsNull() && !field.IsNull() && (oldField->Type() != field->Type() || oldField->NbRawComponents() != field->NbRawComponents())) {
            throw std::invalid_argument("Coordinated contour schema changed");
        }
        auto defBinding = deformation; if (!defBinding.Name.empty()) { defBinding.Step = step; }
        const auto displacement = resolve(mesh,defBinding);
        if (!displacement.IsNull() && (!displacement->IsVector3() || !displacement->IsNodal() || !displacement->HasScalarComponents())) {
            throw std::invalid_argument("Coordinated deformation requires a matching nodal 3D vector");
        }
        if (!candidate->Cuts().SetDeformation(displacement,scale)) { throw std::invalid_argument("Invalid coordinated deformation"); }
        auto datums = myOwner.Datums();
        for (const auto& frame : myMaterialFrames) { datums[frame.first] = materialFrame(frame.second,step,mesh,displacement,candidate->Cuts().EffectiveDeformationScale()); }
        for (auto definition : definitions) {
            if (myCutFrames.count(definition.Id)) {
                const auto& name = myCutFrames.at(definition.Id);
                definition = localDefinition(definition,myOwner.Datums().at(name));
                const auto& datum = datums.at(name);
                if (definition.Shape == ViewCaeGeom::MeshCutShape::Plane) { definition.Plane = datum.PlaneToGlobal(definition.Plane); }
                else { definition.Center = datum.PointToGlobal(definition.Center); definition.Axis = datum.VectorToGlobal(definition.Axis); }
            }
            const auto traceStep = myTraceSteps.find(step);
            if (traceStep != myTraceSteps.end() && traceStep->second.count(definition.Id)) {
                const auto& trace = traceStep->second.at(definition.Id); definition.DisplayTrace = trace.first; definition.PhysicalTrace = trace.second;
            } else if (remesh && (!definition.DisplayTrace.Regions.empty() || !definition.PhysicalTrace.Regions.empty())) {
                throw std::invalid_argument("Remesh requires new explicit per-cut trace-region assignments");
            }
            bool added = false;
            switch (definition.Shape) {
                case ViewCaeGeom::MeshCutShape::Plane: added = candidate->Cuts().AddPlane(definition.Id,definition.Plane); break;
                case ViewCaeGeom::MeshCutShape::Sphere: added = candidate->Cuts().AddSphere(definition.Id,definition.Center,definition.Radius); break;
                case ViewCaeGeom::MeshCutShape::Cylinder: added = candidate->Cuts().AddCylinder(definition.Id,definition.Center,definition.Axis,definition.Radius); break;
                case ViewCaeGeom::MeshCutShape::Isosurface: {
                    const auto iso = resolve(mesh,{definition.IsoFieldName,definition.IsoFieldNodal,definition.IsoComponent,step});
                    added = candidate->Cuts().AddIsosurface(definition.Id,iso,definition.IsoComponent,definition.IsoValue); break;
                }
            }
            if (!added || !candidate->Cuts().Rename(definition.Id,definition.Name) || !candidate->Cuts().SetVisibility(definition.Id,definition.Visibility)
                || !candidate->Cuts().SetCellMode(definition.Id,definition.CellMode) || !candidate->Cuts().SetTraceOptions(definition.Id,false,definition.DisplayTrace)
                || !candidate->Cuts().SetTraceOptions(definition.Id,true,definition.PhysicalTrace) || !candidate->Cuts().SetEnabled(definition.Id,definition.Enabled)) {
                throw std::invalid_argument("Target step has an invalid cut definition");
            }
        }
        for (int p = 0; p < 3; ++p) { candidate->SetStyle((ViewCaeGeom::MeshCutPortion)p,myOwner.Display()->Style((ViewCaeGeom::MeshCutPortion)p)); }
        for (const auto& style : myOwner.Display()->StyleOverrides()) { candidate->SetStyle(style.first.first,(ViewCaeGeom::MeshCutPortion)style.first.second,style.second); }
        candidate->SetContourOptions(myOwner.Display()->ContourOptions()); candidate->SetField(field,contour.Component);
        const auto previousVector = myOwner.Display()->VectorField();
        if (!previousVector.IsNull()) {
            const auto vector = resolve(mesh,{previousVector->Name().ToCString(),previousVector->IsNodal(),0,step});
            candidate->SetVectorField(vector,myOwner.Display()->VectorScale(),myOwner.Display()->VectorSampling(),myOwner.Display()->VectorOrientation());
        }
        candidate->SetReferenceFrame(myOwner.ReferenceDatum() == "global" ? ViewCaeGeom::MeshCutFrame() : datums.at(myOwner.ReferenceDatum()),myOwner.ReferenceAxesVisible(),myOwner.ReferenceAxesLength());
        candidate->Synchronize();
        if (myOwner.myAnalysis && !staged.Analysis().PrepareStep(*myOwner.myAnalysis,step,remesh)) { throw std::runtime_error(staged.LastError()); }
        if (!myOwner.myCadSource.IsNull() && !staged.SetCadSection(myOwner.myCadSource,myOwner.myCadCut,myOwner.mySyncClip)) { throw std::runtime_error(staged.LastError()); }
        if (!myOwner.myGlobalClip.IsNull() && !myOwner.mySyncClip && !staged.SetGlobalClip(myOwner.myGlobalClip->GetEquation(),myOwner.myGlobalClip->IsOn())) { throw std::runtime_error(staged.LastError()); }
        const auto context = myOwner.myContext; const auto viewport = myOwner.myViewport;
        // Build every AIS presentation in the live context without redraw first. If any
        // stage fails, staged's destructor removes only the new objects; the old view,
        // drag, bindings and pixels are still intact.
        if (!context.IsNull() && !staged.Attach(context,viewport)) { throw std::runtime_error(staged.LastError()); }
        if (!context.IsNull()) {
            TColStd_ListOfInteger modes; context->ActivatedModes(myOwner.Display(),modes);
            context->Deactivate(candidate);
            for (TColStd_ListOfInteger::Iterator mode(modes); mode.More(); mode.Next()) { context->Activate(candidate,mode.Value()); }
        }
        const auto previous = myOwner.Display(); myOwner.Detach(); myOwner.ClearAnnotations();
        previous->SetOnDrag(nullptr); previous->SetOnSynchronize(nullptr); previous->Cuts().Clear();
        candidate->SetOnSynchronize([owner = &myOwner] { owner->synchronized(); });
        candidate->SetOnDrag([owner = &myOwner](AIS_DragAction action) {
            if (action == AIS_DragAction_Start) { owner->myDragSnapshot = owner->Display()->Cuts().DumpDefinitionsJson(); owner->event(CutEditEvent::Begin); }
            else if (action == AIS_DragAction_Update) { owner->event(CutEditEvent::Update); }
            else { owner->myDragSnapshot.clear(); owner->event(action == AIS_DragAction_Stop ? CutEditEvent::Commit : CutEditEvent::Abort); }
        });
        myOwner.myDisplay = candidate; staged.myDisplay.Nullify();
        myOwner.myAnalysis = std::move(staged.myAnalysis);
        if (myOwner.myAnalysis) { myOwner.myAnalysis->myOwner = &myOwner; }
        myOwner.myDatums = std::move(datums); myOwner.myFieldBinding = contour;
        myOwner.myCadSection = staged.myCadSection; myOwner.myGlobalClip = staged.myGlobalClip;
        myOwner.myCadSource = staged.myCadSource; myOwner.myCadCut = staged.myCadCut; myOwner.mySyncClip = staged.mySyncClip;
        myOwner.myContext = context; myOwner.myViewport = viewport;
        myOwner.myDragSnapshot.clear();
        staged.myContext.Nullify(); staged.myViewport.Nullify();
        myOwner.event(CutEditEvent::Update);
    });
}
bool NativeCutTimeline::Play(double period, bool loop, bool restart)
{
    return myOwner.attempt([&] {
        if (!std::isfinite(period) || period < 0.001 || StepCount() < 2) { throw std::invalid_argument("Animation requires a period >=1 ms and at least two complete steps"); }
        if (restart && !SwitchStep(0)) { throw std::runtime_error(myOwner.LastError()); }
        myPeriod = period; myLoop = loop; myPlaying = true; myUseTimes = false; myElapsed = 0;
    });
}
bool NativeCutTimeline::PlayTime(double speed, bool loop, bool restart)
{
    return myOwner.attempt([&] {
        if (!std::isfinite(speed) || speed <= 0 || StepCount() < 2) { throw std::invalid_argument("Timestamp animation requires a positive speed and at least two steps"); }
        const auto& series = mySource->SeriesSteps();
        for (size_t i = 0; i < series.size(); ++i) {
            if (!std::isfinite(series[i].Time) || (i && series[i].Time <= series[i-1].Time)) { throw std::invalid_argument("Animation timestamps must be finite and strictly increasing"); }
        }
        if (restart && !SwitchStep(0)) { throw std::runtime_error(myOwner.LastError()); }
        myPeriod = speed; myLoop = loop; myPlaying = true; myUseTimes = true;
        myElapsed = series[myStep].Time-series.front().Time;
    });
}
bool NativeCutTimeline::Tick(double elapsed)
{
    if (!myPlaying) { return true; }
    return myOwner.attempt([&] {
        if (!std::isfinite(elapsed) || elapsed < 0 || elapsed > 3600) { throw std::invalid_argument("Invalid animation elapsed time"); }
        if (myUseTimes) {
            const auto& series = mySource->SeriesSteps();
            const double duration = series.back().Time-series.front().Time;
            myElapsed += elapsed*myPeriod;
            if (!std::isfinite(myElapsed)) { myPlaying = false; throw std::invalid_argument("Animation time overflow"); }
            if (myLoop && myElapsed > duration) { myElapsed = std::fmod(myElapsed,duration); }
            int next = 0;
            while ((size_t)(next+1) < series.size() && series[next+1].Time-series.front().Time <= myElapsed) { ++next; }
            if (next != myStep && !SwitchStep(next)) { myPlaying = false; throw std::runtime_error(myOwner.LastError()); }
            if (!myLoop && myElapsed >= duration) { myPlaying = false; }
            return;
        }
        myElapsed += elapsed; if (myElapsed < myPeriod) { return; }
        const auto advance = (size_t)std::floor(myElapsed/myPeriod);
        myElapsed = std::fmod(myElapsed,myPeriod);
        const size_t target = (size_t)myStep+advance;
        const int next = (int)(myLoop ? target%StepCount() : std::min(target,StepCount()-1));
        if (!SwitchStep(next)) { myPlaying = false; throw std::runtime_error(myOwner.LastError()); }
        if (!myLoop && target >= StepCount()-1) { myPlaying = false; }
    });
}
}
