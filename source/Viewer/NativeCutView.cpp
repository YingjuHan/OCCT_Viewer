#include <Viewer/NativeCutView.h>
#include <Viewer/NativeCutAnalysis.h>
#include <BRepBuilderAPI_Copy.hxx>
#include <AIS_InteractiveContext.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <gp_Pln.hxx>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <V3d_View.hxx>
#include <Graphic3d_CView.hxx>

namespace Viewer {
NativeCutView::NativeCutView(const Handle(RWCae::MeshData)& mesh) : myDisplay(new ViewCae::InteractiveCaeCutView(mesh))
{
    myDisplay->SetOnSynchronize([this] { synchronized(); });
    myDisplay->SetOnDrag([this](AIS_DragAction action) {
        if (action == AIS_DragAction_Start) { myDragSnapshot = myDisplay->Cuts().DumpDefinitionsJson(); event(CutEditEvent::Begin); }
        else if (action == AIS_DragAction_Update) { event(CutEditEvent::Update); }
        else { myDragSnapshot.clear(); event(action == AIS_DragAction_Stop ? CutEditEvent::Commit : CutEditEvent::Abort); }
    });
}
NativeCutView::~NativeCutView() { Close(); }
NativeCutAnalysis& NativeCutView::Analysis()
{
    if (myDisplay.IsNull()) { throw std::invalid_argument("Cut model is closed"); }
    if (!myAnalysis) { myAnalysis.reset(new NativeCutAnalysis(*this)); }
    return *myAnalysis;
}
bool NativeCutView::attempt(const std::function<void()>& action)
{
    try {
        if (myDisplay.IsNull()) { throw std::invalid_argument("Cut model is closed"); }
        action(); myError.clear(); return true;
    } catch (const Standard_Failure& error) { myError = error.GetMessageString(); }
    catch (const std::exception& error) { myError = error.what(); }
    return false;
}
void NativeCutView::event(CutEditEvent value)
{
    ++myGeneration;
    ++myEventCounts[(size_t)value];
    // Client callbacks are notifications; exceptions cannot invalidate publication.
    if (myOnEdit) { try { myOnEdit(value,myGeneration); } catch (...) {} }
}
Handle(RWCae::MeshNVecProperty) NativeCutView::resolveField(const CutFieldBinding& binding) const
{
    if (binding.Name.empty()) { return nullptr; }
    Handle(RWCae::MeshNVecProperty) field;
    for (const auto& candidate : myDisplay->SourceMesh()->Properties()) {
        if (candidate->Name().ToCString() == binding.Name && candidate->IsNodal() == binding.Nodal && candidate->TimeStep() == binding.Step) {
            if (!field.IsNull()) { throw std::invalid_argument("Ambiguous native field binding"); }
            field = candidate;
        }
    }
    if (field.IsNull() || !field->IsInitialized() || binding.Component < 0 || binding.Component >= field->NbRawComponents()
        || field->NbValues() != (int)(binding.Nodal ? myDisplay->SourceMesh()->NbNodes() : myDisplay->SourceMesh()->NbIndexedElems())) {
        throw std::invalid_argument("Missing field, component or source tuple mapping");
    }
    return field;
}
bool NativeCutView::BindField(const CutFieldBinding& binding)
{
    return attempt([&] {
        const auto field = resolveField(binding), previous = myDisplay->Field();
        const int component = myDisplay->Component();
        try { myDisplay->SetField(field,binding.Component); myDisplay->Synchronize(); }
        catch (...) { myDisplay->SetField(previous,component); throw; }
        myFieldBinding = binding; event(CutEditEvent::Update);
    });
}
bool NativeCutView::SetDatum(const std::string& id, const ViewCaeGeom::MeshCutFrame& frame)
{
    return attempt([&] {
        if (id.empty() || id == "global" || id.find('\0') != std::string::npos || !frame.IsValid()) { throw std::invalid_argument("Invalid native datum"); }
        myDatums[id] = frame;
        if (myReferenceDatum == id) { myDisplay->SetReferenceFrame(frame,myReferenceAxesVisible,myReferenceAxesLength); }
        event(CutEditEvent::Update);
    });
}
bool NativeCutView::SetReferenceAxes(const std::string& datum, bool show, double length)
{
    return attempt([&] {
        ViewCaeGeom::MeshCutFrame frame;
        if (datum != "global") {
            const auto found = myDatums.find(datum);
            if (found == myDatums.end()) { throw std::invalid_argument("Unknown reference datum"); } frame = found->second;
        }
        myDisplay->SetReferenceFrame(frame,show,length);
        myReferenceDatum = datum; myReferenceAxesVisible = show; myReferenceAxesLength = length;
    });
}
bool NativeCutView::Edit(const ViewCaeGeom::MeshCutDefinition& definition, const std::string& datum)
{
    return attempt([&] {
        ViewCaeGeom::MeshCutFrame frame;
        if (!datum.empty()) {
            const auto found = myDatums.find(datum);
            if (found == myDatums.end()) { throw std::invalid_argument("Unknown local datum"); }
            frame = found->second;
        }
        auto& cuts = myDisplay->Cuts(); const auto definitions=cuts.Definitions();
        const auto previous=std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id==definition.Id; });
        if(previous==definitions.end()) { throw std::invalid_argument("Unknown native cut edit"); }
        if (!frame.Edit(cuts,definition)) { throw std::invalid_argument("Invalid native cut edit"); }
        if (!Refresh(false)) {
            // The previous numeric definition is already validated against this
            // unchanged source. Restore it without allocating a second validator
            // when failure was caused by a tight cache budget. Old AIS arrays stay
            // published until another complete frame can be prepared.
            if(!ViewCaeGeom::MeshCutFrame().Edit(cuts,*previous)) { throw std::runtime_error("Native cut edit rollback failed"); }
            throw std::runtime_error(myError);
        }
        event(CutEditEvent::Update);
    });
}
bool NativeCutView::BeginDrag()
{
    return attempt([&] {
        if (IsDragging()) { throw std::invalid_argument("A cut drag is already active"); }
        myDragSnapshot = myDisplay->Cuts().DumpDefinitionsJson(); event(CutEditEvent::Begin);
    });
}
bool NativeCutView::EndDrag(bool commit)
{
    return attempt([&] {
        if (!IsDragging()) { throw std::invalid_argument("No active cut drag"); }
        if (!commit && !myDisplay->Cuts().RestoreDefinitionsJson(myDragSnapshot)) { throw std::runtime_error("Cut drag rollback failed"); }
        if (!Refresh(false)) { throw std::runtime_error(myError); }
        myDragSnapshot.clear(); event(commit ? CutEditEvent::Commit : CutEditEvent::Abort);
    });
}
bool NativeCutView::Attach(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& viewport)
{
    return attempt([&] {
        if (context.IsNull()) { throw std::invalid_argument("Native cut view needs an AIS context"); }
        if (!viewport.IsNull() && viewport->View()->IsRemoved()) { throw std::invalid_argument("Native viewport has been removed"); }
        myDisplay->Synchronize(); Detach(); myContext = context; myViewport = viewport;
        if (!myViewport.IsNull() && !myGlobalClip.IsNull()) { myViewport->AddClipPlane(myGlobalClip); myDisplay->SetClipPlanes(nullptr); }
        if (!Refresh(false)) { throw std::runtime_error(myError); }
    });
}
void NativeCutView::Detach()
{
    ClearHandle();
    if (myAnalysis) { myAnalysis->Detach(); }
    if (!myViewport.IsNull() && !myViewport->View()->IsRemoved() && !myGlobalClip.IsNull()) { myViewport->RemoveClipPlane(myGlobalClip); }
    myViewport.Nullify();
    if (myContext.IsNull()) { return; }
    if (!myDisplay.IsNull()) {
        myContext->Remove(myDisplay,false); myContext->Remove(myDisplay->Legend(),false);
    }
    if (!myCadSection.IsNull()) { myContext->Remove(myCadSection,false); }
    for (const auto& entry : myAnnotations) { myContext->Remove(entry.second,false); }
    myContext.Nullify();
}
void NativeCutView::ClearHandle()
{
    if (!myManipulator.IsNull()) {
        if (myManipulator->HasActiveTransformation()) { myManipulator->StopTransform(false); }
        myManipulator->Detach(); myManipulator.Nullify();
    }
    if (!myContext.IsNull()) {
        if (!myPlane.IsNull()) { myContext->Remove(myPlane,false); }
        if (!myQuadric.IsNull()) { myContext->Remove(myQuadric,false); }
    }
    myPlane.Nullify(); myQuadric.Nullify();
    if (IsDragging() && !myDisplay.IsNull()) { EndDrag(false); }
}
bool NativeCutView::ShowHandle(const std::string& id, bool gizmo)
{
    return attempt([&] {
        if (myContext.IsNull()) { throw std::invalid_argument("Cut handle requires an attached native viewport"); }
        const auto definitions = myDisplay->Cuts().Definitions();
        const auto found = std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == id; });
        if (found == definitions.end() || found->Shape == ViewCaeGeom::MeshCutShape::Isosurface) {
            throw std::invalid_argument("Handle requires a geometric cut");
        }
        const auto bounds = myDisplay->Cuts().SourceBounds();
        Handle(ViewCae::InteractiveCaeCutPlane) plane;
        Handle(ViewCae::InteractiveCaeCutQuadric) quadric;
        Handle(AIS_Manipulator) manipulator;
        Handle(AIS_InteractiveObject) guide;
        if (found->Shape == ViewCaeGeom::MeshCutShape::Plane) {
            plane = new ViewCae::InteractiveCaeCutPlane(myDisplay,id,bounds); guide = plane;
            if (gizmo) { manipulator = new ViewCae::InteractiveCaeCutManipulator(plane); }
        } else {
            quadric = new ViewCae::InteractiveCaeCutQuadric(myDisplay,id,bounds); guide = quadric;
            if (gizmo) { manipulator = new ViewCae::InteractiveCaeCutQuadricManipulator(quadric); }
        }
        ClearHandle(); myContext->Display(guide,false);
        if (!manipulator.IsNull()) {
            manipulator->Attach(guide,AIS_Manipulator::OptionsForAttach().SetAdjustPosition(false));
            manipulator->SetPosition(!plane.IsNull() ? plane->Frame() : quadric->Frame());
        }
        myPlane = plane; myQuadric = quadric; myManipulator = manipulator;
    });
}
void NativeCutView::Close()
{
    if (myDisplay.IsNull()) { return; }
    if (myAnalysis) { myAnalysis->Close(); myAnalysis.reset(); }
    Detach(); myDisplay->SetOnDrag(nullptr); myDisplay->SetOnSynchronize(nullptr);
    myDragSnapshot.clear(); myAnnotations.clear(); myCadSection.Nullify(); myCadSource.Nullify();
    myGlobalClip.Nullify(); myDisplay->Cuts().Clear(); myDisplay.Nullify(); event(CutEditEvent::Close);
}
void NativeCutView::adopt(NativeCutView& staged)
{
    // Prepare allocating callbacks before detaching anything from the live view.
    staged.myDisplay->SetOnSynchronize([this] { synchronized(); });
    staged.myDisplay->SetOnDrag([this](AIS_DragAction action) {
        if(action==AIS_DragAction_Start) { myDragSnapshot=myDisplay->Cuts().DumpDefinitionsJson(); event(CutEditEvent::Begin); }
        else if(action==AIS_DragAction_Update) { event(CutEditEvent::Update); }
        else { myDragSnapshot.clear(); event(action==AIS_DragAction_Stop?CutEditEvent::Commit:CutEditEvent::Abort); }
    });
    myDisplay->SetOnDrag(nullptr); myDisplay->SetOnSynchronize(nullptr);
    myDragSnapshot.clear(); Detach();
    myDisplay->Cuts().Clear();
    myDisplay=staged.myDisplay; staged.myDisplay.Nullify();
    myContext=staged.myContext; staged.myContext.Nullify();
    myViewport=staged.myViewport; staged.myViewport.Nullify();
    myDatums=std::move(staged.myDatums); myReferenceDatum=std::move(staged.myReferenceDatum);
    myReferenceAxesVisible=staged.myReferenceAxesVisible; myReferenceAxesLength=staged.myReferenceAxesLength;
    myAnnotations=std::move(staged.myAnnotations); myFieldBinding=std::move(staged.myFieldBinding);
    myCadSection=staged.myCadSection; myCadSource=staged.myCadSource; myCadCut=std::move(staged.myCadCut); mySyncClip=staged.mySyncClip; myGlobalClip=staged.myGlobalClip;
    staged.myCadSection.Nullify(); staged.myCadSource.Nullify(); staged.myGlobalClip.Nullify();
    myAnalysis=std::move(staged.myAnalysis); if(myAnalysis) { myAnalysis->myOwner=this; }
}
bool NativeCutView::Refresh(bool redraw)
{
    return attempt([&] {
        if (!myViewport.IsNull() && myViewport->View()->IsRemoved()) { Detach(); throw std::invalid_argument("Native viewport has been removed"); }
        myDisplay->Synchronize();
        if (myContext.IsNull()) { return; }
        myContext->Display(myDisplay,false);
        // Redisplay unconditionally marks every presentation dirty. Mutable
        // GPU sections already changed their buffers and need only an Update.
        if (myDisplay->RenderStatistics().GpuPlane) { myContext->Update(myDisplay,false); }
        else { myContext->Redisplay(myDisplay,false); }
        if (myDisplay->ContourOptions().ShowLegend) {
            myContext->Display(myDisplay->Legend(),false); myContext->Redisplay(myDisplay->Legend(),false);
        } else { myContext->Erase(myDisplay->Legend(),false); }
        for (const auto& annotation : myAnnotations) { myContext->Display(annotation.second,false); }
        if (!myCadSection.IsNull()) { myContext->Display(myCadSection,false); }
        if (redraw) { myContext->UpdateCurrentViewer(); }
    });
}
void NativeCutView::synchronized()
{
    if (myAnalysis && !myAnalysis->Refresh()) { throw std::runtime_error(myError); }
    // Mouse aids use the same publication path as numeric edits.
    if (!myCadSource.IsNull()) {
        const auto definitions = myDisplay->Cuts().Definitions();
        if (std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == myCadCut; })) {
            ClearCadSection();
        } else {
            const bool clipOn=myGlobalClip.IsNull()||myGlobalClip->IsOn();
            if (!SetCadSection(myCadSource,myCadCut,mySyncClip)) { throw std::runtime_error(myError); }
            if(mySyncClip&&!myGlobalClip.IsNull()) { myGlobalClip->SetOn(clipOn); }
        }
    }
    if (!myContext.IsNull()) {
        if (myDisplay->ContourOptions().ShowLegend) {
            myContext->Display(myDisplay->Legend(),false); myContext->Redisplay(myDisplay->Legend(),false);
        } else { myContext->Erase(myDisplay->Legend(),false); }
    }
}
void NativeCutView::ClearCadSection()
{
    if (!myContext.IsNull() && !myCadSection.IsNull()) { myContext->Remove(myCadSection,false); }
    if (mySyncClip && !myGlobalClip.IsNull()) {
        if (!myViewport.IsNull()) { myViewport->RemoveClipPlane(myGlobalClip); }
        if (!myDisplay.IsNull()) { myDisplay->SetClipPlanes(nullptr); }
        myGlobalClip.Nullify();
    }
    myCadSection.Nullify(); myCadSource.Nullify(); myCadCut.clear(); mySyncClip = false;
}
bool NativeCutView::AddAnnotation(const std::string& id, const ViewCae::MeshCutProbe& probe, const std::string& text)
{
    return attempt([&] {
        if (id.empty() || text.empty() || text.size() > 4096) { throw std::invalid_argument("Invalid cut annotation"); }
        for (int i = 0; i < 3; ++i) { if (!std::isfinite(probe.Position[i])) { throw std::invalid_argument("Nonfinite annotation position"); } }
        Handle(AIS_TextLabel) label = new AIS_TextLabel();
        label->SetText(TCollection_ExtendedString(text.c_str(),true));
        label->SetPosition(gp_Pnt(probe.Position.x(),probe.Position.y(),probe.Position.z()));
        label->SetColor(Quantity_Color(Quantity_NOC_BLACK));
        const auto old = myAnnotations.find(id);
        if (old != myAnnotations.end() && !myContext.IsNull()) { myContext->Remove(old->second,false); }
        myAnnotations[id] = label;
        if (!myContext.IsNull()) { myContext->Display(label,false); }
    });
}
void NativeCutView::ClearAnnotations()
{
    if (!myContext.IsNull()) { for (const auto& entry : myAnnotations) { myContext->Remove(entry.second,false); } }
    myAnnotations.clear();
}
bool NativeCutView::SetGlobalClip(const Graphic3d_Vec4d& plane, bool enabled)
{
    return attempt([&] {
        const double norm = std::hypot(plane.x(),plane.y(),plane.z());
        if (!std::isfinite(norm) || norm <= 0 || !std::isfinite(plane.w())) { throw std::invalid_argument("Invalid global clipping plane"); }
        auto normalized = plane; normalized /= norm;
        Handle(Graphic3d_ClipPlane) clip = new Graphic3d_ClipPlane(normalized); clip->SetOn(enabled);
        Handle(Graphic3d_SequenceOfHClipPlane) planes = new Graphic3d_SequenceOfHClipPlane(); planes->Append(clip);
        if (!myViewport.IsNull()) {
            if (!myGlobalClip.IsNull()) { myViewport->RemoveClipPlane(myGlobalClip); }
            myViewport->AddClipPlane(clip); myDisplay->SetClipPlanes(nullptr);
        } else { myDisplay->SetClipPlanes(planes); }
        myGlobalClip = clip;
        mySyncClip = false;
    });
}
bool NativeCutView::SetCadSection(const TopoDS_Shape& shape, const std::string& id, bool syncClip)
{
    return attempt([&] {
        if (shape.IsNull()) { throw std::invalid_argument("CAD section needs a shape"); }
        // Section algorithms may update shared BREP flags/pcurves. Preserve an
        // independent source snapshot for repeatable sections and persistence.
        const auto preserved=BRepBuilderAPI_Copy(shape,true,false).Shape();
        const auto definitions = myDisplay->Cuts().Definitions();
        auto definition = std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id == id; });
        if (definition == definitions.end()) { throw std::invalid_argument("Unknown CAD section cut"); }
        TopoDS_Shape section;
        if (definition->Shape == ViewCaeGeom::MeshCutShape::Plane) {
            const auto& p = definition->Plane;
            BRepAlgoAPI_Section operation(shape,gp_Pln(p.x(),p.y(),p.z(),p.w()),false); operation.Build();
            if (!operation.IsDone()) { throw std::runtime_error("CAD plane section failed"); } section = operation.Shape();
        } else {
            if (syncClip) { throw std::invalid_argument("Global clipping synchronization requires a plane"); }
            const auto& c = definition->Center;
            Handle(Geom_Surface) tool;
            if (definition->Shape == ViewCaeGeom::MeshCutShape::Sphere) {
                tool = new Geom_SphericalSurface(gp_Ax3(gp_Pnt(c.x(),c.y(),c.z()),gp_Dir(0,0,1)),definition->Radius);
            } else if (definition->Shape == ViewCaeGeom::MeshCutShape::Cylinder) {
                const auto& a = definition->Axis;
                tool = new Geom_CylindricalSurface(gp_Ax3(gp_Pnt(c.x(),c.y(),c.z()),gp_Dir(a.x(),a.y(),a.z())),definition->Radius);
            } else { throw std::invalid_argument("CAD field isosurface section is unsupported"); }
            BRepAlgoAPI_Section operation(shape,tool,false); operation.Build();
            if (!operation.IsDone()) { throw std::runtime_error("CAD curved section failed"); } section = operation.Shape();
        }
        Handle(AIS_Shape) display = new AIS_Shape(section); display->SetColor(Quantity_Color(Quantity_NOC_BLACK)); display->SetWidth(3);
        if (syncClip && !SetGlobalClip(definition->Plane,true)) { throw std::runtime_error(myError); }
        if (!myContext.IsNull()) { if (!myCadSection.IsNull()) { myContext->Remove(myCadSection,false); } myContext->Display(display,false); }
        myCadSection = display; myCadSource = preserved; myCadCut = id; mySyncClip = syncClip;
    });
}
}
