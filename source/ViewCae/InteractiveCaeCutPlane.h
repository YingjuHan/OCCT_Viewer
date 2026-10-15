#ifndef _INTERACTIVECAECUTPLANE_HEADERFILE
#define _INTERACTIVECAECUTPLANE_HEADERFILE

#include <ViewCae/InteractiveCaeCutView.h>
#include <AIS_ClippingPlanes.hxx>
#include <AIS_Manipulator.hxx>
#include <AIS_Shape.hxx>

namespace ViewCae {
//! Selectable positioning aid. It never clips the model on the GPU.
class InteractiveCaeCutPlane : public AIS_ClippingPlanes {
    DEFINE_STANDARD_RTTIEXT(InteractiveCaeCutPlane, AIS_ClippingPlanes)
public:
    Standard_EXPORT InteractiveCaeCutPlane(const Handle(InteractiveCaeCutView)& theCutView,
        const std::string& theId, const Bnd_Box& theBounds);
    Standard_EXPORT bool Refresh();
    Standard_EXPORT bool SetBounds(const Bnd_Box& theBounds);
    Standard_EXPORT gp_Ax2 Frame() const;
    Standard_EXPORT bool ApplyTransformation(bool theCommit);
    bool IsDragging() const { return myDragging; }
    int DragUpdates() const { return myDragUpdates; }
    void NotifyDrag(AIS_DragAction action) { myCutView->NotifyDrag(action); }
    Standard_EXPORT Standard_Boolean ProcessDragging(const Handle(AIS_InteractiveContext)& theCtx,
        const Handle(V3d_View)& theView, const Handle(SelectMgr_EntityOwner)& theOwner,
        const Graphic3d_Vec2i& theFrom, const Graphic3d_Vec2i& theTo, AIS_DragAction theAction) override;

private:
    bool updateCut(const gp_Pln& thePlane);
    Handle(InteractiveCaeCutView) myCutView;
    std::string myId;
    bool myDragging = false;
    int myDragUpdates = 0;
};

//! Recomputes mesh intersections and fields during each OCCT manipulation update.
class InteractiveCaeCutManipulator : public AIS_Manipulator {
    DEFINE_STANDARD_RTTIEXT(InteractiveCaeCutManipulator, AIS_Manipulator)
public:
    Standard_EXPORT explicit InteractiveCaeCutManipulator(const Handle(InteractiveCaeCutPlane)& thePlane);
    Standard_EXPORT void Refresh();
    Standard_EXPORT Standard_Boolean ProcessDragging(const Handle(AIS_InteractiveContext)& theCtx,
        const Handle(V3d_View)& theView, const Handle(SelectMgr_EntityOwner)& theOwner,
        const Graphic3d_Vec2i& theFrom, const Graphic3d_Vec2i& theTo, AIS_DragAction theAction) override;

private:
    Handle(InteractiveCaeCutPlane) myPlane;
};

//! Wireframe analytic guide; the attached manipulator drives CPU intersections.
class InteractiveCaeCutQuadric : public AIS_Shape {
    DEFINE_STANDARD_RTTIEXT(InteractiveCaeCutQuadric, AIS_Shape)
public:
    Standard_EXPORT InteractiveCaeCutQuadric(const Handle(InteractiveCaeCutView)& theView,
        const std::string& theId, const Bnd_Box& theBounds);
    Standard_EXPORT bool Refresh();
    void SetBounds(const Bnd_Box& theBounds) { if (!theBounds.IsVoid()) { myBounds = theBounds; } }
    Standard_EXPORT gp_Ax2 Frame() const;
    Standard_EXPORT bool ApplyTransformation(bool theCommit);
    void BeginDrag() { myDragging = true; }
    bool IsDragging() const { return myDragging; }
    int DragUpdates() const { return myDragUpdates; }
    void NotifyDrag(AIS_DragAction action) { myCutView->NotifyDrag(action); }
    bool IsCylinder() const { return myBaseline.Shape == ViewCaeGeom::MeshCutShape::Cylinder; }
private:
    Handle(InteractiveCaeCutView) myCutView;
    std::string myId;
    Bnd_Box myBounds;
    ViewCaeGeom::MeshCutDefinition myBaseline;
    bool myDragging = false;
    int myDragUpdates = 0;
};

class InteractiveCaeCutQuadricManipulator : public AIS_Manipulator {
    DEFINE_STANDARD_RTTIEXT(InteractiveCaeCutQuadricManipulator, AIS_Manipulator)
public:
    Standard_EXPORT explicit InteractiveCaeCutQuadricManipulator(const Handle(InteractiveCaeCutQuadric)& theShape);
    Standard_EXPORT void Refresh();
    Standard_EXPORT Standard_Boolean ProcessDragging(const Handle(AIS_InteractiveContext)& theCtx,
        const Handle(V3d_View)& theView, const Handle(SelectMgr_EntityOwner)& theOwner,
        const Graphic3d_Vec2i& theFrom, const Graphic3d_Vec2i& theTo, AIS_DragAction theAction) override;
private:
    Handle(InteractiveCaeCutQuadric) myShape;
};
}
#endif
