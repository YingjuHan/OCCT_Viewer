#include <ViewCae/InteractiveCaeCutPlane.h>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <Precision.hxx>
#include <stdexcept>
#include <algorithm>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>

IMPLEMENT_STANDARD_RTTIEXT(ViewCae::InteractiveCaeCutPlane, AIS_ClippingPlanes)
IMPLEMENT_STANDARD_RTTIEXT(ViewCae::InteractiveCaeCutManipulator, AIS_Manipulator)
IMPLEMENT_STANDARD_RTTIEXT(ViewCae::InteractiveCaeCutQuadric, AIS_Shape)
IMPLEMENT_STANDARD_RTTIEXT(ViewCae::InteractiveCaeCutQuadricManipulator, AIS_Manipulator)

namespace ViewCae {
InteractiveCaeCutPlane::InteractiveCaeCutPlane(const Handle(InteractiveCaeCutView)& theCutView,
    const std::string& theId, const Bnd_Box& theBounds) : myCutView(theCutView), myId(theId)
{
    if (theCutView.IsNull() || theBounds.IsVoid()) { throw std::invalid_argument("Invalid cut positioning aid"); }
    Handle(Graphic3d_SequenceOfHClipPlane) planes = new Graphic3d_SequenceOfHClipPlane();
    planes->Append(new Graphic3d_ClipPlane());
    SetManagedPlanes(planes);
    SetDisplayedPlane(planes->First());
    SetBounds(theBounds);
    SetDatumDisplayMode(Prs3d_DM_WireFrame);
    SetColor(Quantity_Color(Quantity_NOC_GRAY30));
    if (!Refresh()) { throw std::invalid_argument("Unknown cut plane"); }
}

bool InteractiveCaeCutPlane::SetBounds(const Bnd_Box& theBounds)
{
    if (theBounds.IsVoid() || myDragging) { return false; }
    Bnd_Box guideBounds = theBounds;
    const auto extent = theBounds.CornerMax().XYZ() - theBounds.CornerMin().XYZ();
    // A surface mesh still needs a finite selectable positioning aid.
    guideBounds.SetGap(std::max({extent.X(), extent.Y(), extent.Z()}) * 0.05 + Precision::Confusion());
    SetSceneBox(guideBounds);
    SetToUpdate();
    return true;
}

bool InteractiveCaeCutPlane::Refresh()
{
    if (myDragging) { return true; }
    for (const auto& cut : myCutView->Cuts().Definitions()) {
        if (cut.Id == myId && cut.Shape == ViewCaeGeom::MeshCutShape::Plane) {
            ResetTransformation();
            DisplayedPlane()->SetEquation(cut.Plane);
            SetToUpdate();
            return true;
        }
    }
    return false;
}

gp_Ax2 InteractiveCaeCutPlane::Frame() const
{
    const auto plane = DisplayedPlane()->ToPlane().Transformed(LocalTransformation());
    const gp_Pnt center((GetSceneBox().CornerMin().XYZ() + GetSceneBox().CornerMax().XYZ()) * 0.5);
    const auto normal = plane.Axis().Direction();
    const double distance = gp_Vec(plane.Location(), center).Dot(gp_Vec(normal));
    return gp_Ax2(center.Translated(gp_Vec(normal) * -distance), normal);
}

bool InteractiveCaeCutPlane::updateCut(const gp_Pln& thePlane)
{
    double a, b, c, d;
    thePlane.Coefficients(a, b, c, d);
    if (!myCutView->Cuts().SetPlane(myId, Graphic3d_Vec4d(a,b,c,d))) { return false; }
    myCutView->Synchronize();
    if (!GetContext().IsNull()) {
        if (myCutView->RenderStatistics().GpuPlane) { GetContext()->Update(myCutView, false); }
        else { GetContext()->Redisplay(myCutView, false); }
    }
    ++myDragUpdates;
    return true;
}

bool InteractiveCaeCutPlane::ApplyTransformation(bool theCommit)
{
    const auto plane = DisplayedPlane()->ToPlane().Transformed(LocalTransformation());
    if (!updateCut(plane)) { return false; }
    if (theCommit) {
        ResetTransformation();
        DisplayedPlane()->SetEquation(plane);
        SetToUpdate();
        if (!GetContext().IsNull()) { GetContext()->Redisplay(this, false); }
    }
    return true;
}

Standard_Boolean InteractiveCaeCutPlane::ProcessDragging(const Handle(AIS_InteractiveContext)& theCtx,
    const Handle(V3d_View)& theView, const Handle(SelectMgr_EntityOwner)& theOwner,
    const Graphic3d_Vec2i& theFrom, const Graphic3d_Vec2i& theTo, AIS_DragAction theAction)
{
    const bool accepted = AIS_ClippingPlanes::ProcessDragging(theCtx, theView, theOwner, theFrom, theTo, theAction);
    if (!accepted) { return false; }
    myDragging = theAction == AIS_DragAction_Start || theAction == AIS_DragAction_Update;
    if (theAction == AIS_DragAction_Update || theAction == AIS_DragAction_Abort) {
        if (!updateCut(DisplayedPlane()->ToPlane())) { return false; }
    }
    NotifyDrag(theAction);
    return true;
}

InteractiveCaeCutManipulator::InteractiveCaeCutManipulator(const Handle(InteractiveCaeCutPlane)& thePlane)
    : myPlane(thePlane)
{
    SetModeActivationOnDetection(true);
    SetZoomPersistence(true);
    SetSize(75.0f);
    for (int axis = 0; axis < 3; ++axis) {
        SetPart(axis, AIS_MM_Scaling, false);
        SetPart(axis, AIS_MM_TranslationPlane, false);
        SetPart(axis, AIS_MM_Translation, axis == 2);
        SetPart(axis, AIS_MM_Rotation, axis != 2);
    }
}

void InteractiveCaeCutManipulator::Refresh()
{
    if (HasActiveTransformation() || myPlane->IsDragging()) { return; }
    SetPosition(myPlane->Frame());
}

Standard_Boolean InteractiveCaeCutManipulator::ProcessDragging(const Handle(AIS_InteractiveContext)& theCtx,
    const Handle(V3d_View)& theView, const Handle(SelectMgr_EntityOwner)& theOwner,
    const Graphic3d_Vec2i& theFrom, const Graphic3d_Vec2i& theTo, AIS_DragAction theAction)
{
    if (theAction == AIS_DragAction_Stop) {
        StopTransform(true);
        if (!myPlane->ApplyTransformation(true)) { return false; }
        myPlane->NotifyDrag(theAction); return true;
    }
    const bool accepted = AIS_Manipulator::ProcessDragging(theCtx, theView, theOwner, theFrom, theTo, theAction);
    if (accepted && (theAction == AIS_DragAction_Update || theAction == AIS_DragAction_Abort)) {
        if (!myPlane->ApplyTransformation(theAction == AIS_DragAction_Abort)) { return false; }
    }
    if (accepted) { myPlane->NotifyDrag(theAction); }
    return accepted;
}

InteractiveCaeCutQuadric::InteractiveCaeCutQuadric(const Handle(InteractiveCaeCutView)& view,
    const std::string& id, const Bnd_Box& bounds) : AIS_Shape(TopoDS_Shape()), myCutView(view), myId(id), myBounds(bounds)
{
    if (view.IsNull() || bounds.IsVoid() || !Refresh()) { throw std::invalid_argument("Unknown curved cut"); }
    SetDisplayMode(AIS_WireFrame);
    SetColor(Quantity_Color(Quantity_NOC_GRAY30));
}

bool InteractiveCaeCutQuadric::Refresh()
{
    if (myDragging) { return true; }
    for (const auto& cut : myCutView->Cuts().Definitions()) {
        if (cut.Id != myId || (cut.Shape != ViewCaeGeom::MeshCutShape::Sphere
            && cut.Shape != ViewCaeGeom::MeshCutShape::Cylinder)) { continue; }
        myBaseline = cut;
        ResetTransformation();
        const gp_Pnt center(cut.Center.x(),cut.Center.y(),cut.Center.z());
        if (cut.Shape == ViewCaeGeom::MeshCutShape::Sphere) {
            SetShape(BRepPrimAPI_MakeSphere(center,cut.Radius).Shape());
        } else {
            const double length = std::max((myBounds.CornerMax().XYZ()-myBounds.CornerMin().XYZ()).Modulus(),cut.Radius);
            const gp_Dir axis(cut.Axis.x(),cut.Axis.y(),cut.Axis.z());
            SetShape(BRepPrimAPI_MakeCylinder(gp_Ax2(center.Translated(gp_Vec(axis)*(-length/2)),axis),cut.Radius,length).Shape());
        }
        SetToUpdate();
        return true;
    }
    return false;
}

gp_Ax2 InteractiveCaeCutQuadric::Frame() const
{
    return gp_Ax2(gp_Pnt(myBaseline.Center.x(),myBaseline.Center.y(),myBaseline.Center.z()),
        gp_Dir(myBaseline.Axis.x(),myBaseline.Axis.y(),myBaseline.Axis.z()));
}

bool InteractiveCaeCutQuadric::ApplyTransformation(bool commit)
{
    const auto transform = LocalTransformation();
    const auto center = gp_Pnt(myBaseline.Center.x(),myBaseline.Center.y(),myBaseline.Center.z()).Transformed(transform);
    const auto axis = gp_Dir(myBaseline.Axis.x(),myBaseline.Axis.y(),myBaseline.Axis.z()).Transformed(transform);
    const Graphic3d_Vec3d position(center.X(),center.Y(),center.Z());
    const bool ok = IsCylinder() ? myCutView->Cuts().SetCylinder(myId,position,
        Graphic3d_Vec3d(axis.X(),axis.Y(),axis.Z()),myBaseline.Radius)
        : myCutView->Cuts().SetSphere(myId,position,myBaseline.Radius);
    if (!ok) { return false; }
    myCutView->Synchronize();
    ++myDragUpdates;
    if (!GetContext().IsNull()) { GetContext()->Redisplay(myCutView,false); }
    if (commit) {
        myDragging = false;
        Refresh();
        if (!GetContext().IsNull()) { GetContext()->Redisplay(this,false); }
    }
    return true;
}

InteractiveCaeCutQuadricManipulator::InteractiveCaeCutQuadricManipulator(const Handle(InteractiveCaeCutQuadric)& shape)
    : myShape(shape)
{
    SetModeActivationOnDetection(true); SetZoomPersistence(true); SetSize(75.0f);
    for (int axis = 0; axis < 3; ++axis) {
        SetPart(axis,AIS_MM_Scaling,false); SetPart(axis,AIS_MM_TranslationPlane,false);
        SetPart(axis,AIS_MM_Rotation,shape->IsCylinder() && axis != 2);
        // Axial translation/rotation leaves an infinite cylinder unchanged.
        SetPart(axis,AIS_MM_Translation,!shape->IsCylinder() || axis != 2);
    }
}

void InteractiveCaeCutQuadricManipulator::Refresh()
{
    if (!HasActiveTransformation() && !myShape->IsDragging()) { SetPosition(myShape->Frame()); }
}

Standard_Boolean InteractiveCaeCutQuadricManipulator::ProcessDragging(const Handle(AIS_InteractiveContext)& context,
    const Handle(V3d_View)& view, const Handle(SelectMgr_EntityOwner)& owner,
    const Graphic3d_Vec2i& from, const Graphic3d_Vec2i& to, AIS_DragAction action)
{
    if (action == AIS_DragAction_Stop) {
        StopTransform(true);
        if (!myShape->ApplyTransformation(true)) { return false; }
        myShape->NotifyDrag(action); return true;
    }
    const bool accepted = AIS_Manipulator::ProcessDragging(context,view,owner,from,to,action);
    if (accepted && action == AIS_DragAction_Start) { myShape->BeginDrag(); }
    if (accepted && (action == AIS_DragAction_Update || action == AIS_DragAction_Abort)) {
        if (!myShape->ApplyTransformation(action == AIS_DragAction_Abort)) { return false; }
    }
    if (accepted) { myShape->NotifyDrag(action); }
    return accepted;
}
}
