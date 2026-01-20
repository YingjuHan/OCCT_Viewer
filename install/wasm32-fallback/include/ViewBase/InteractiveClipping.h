#ifndef INTERACTIVECLIPPING_HEADERFILE
#define INTERACTIVECLIPPING_HEADERFILE

#include <ViewBase/IInteractiveObject.h>
#include <ViewBase/InteractiveClippingOwner.h>

#include <AIS_ClippingPlanes.hxx>
#include <AIS_RedrawProgress.hxx>
#include <AIS_RedrawProgressResult.hxx>

namespace ViewBase
{

    //! Clipping planes presentation.
    class InteractiveClipping : public AIS_ClippingPlanes, public IInteractiveObject
    {
        InteractiveClipping() = delete;
        InteractiveClipping(const InteractiveClipping &) = delete;
        InteractiveClipping &operator=(const InteractiveClipping &) = delete;

    public:
        //! Public constructor
        Standard_EXPORT explicit InteractiveClipping(const Handle(ViewBase::InteractiveClippingOwner) & theNewId);

        //! Public destructor
        Standard_EXPORT virtual ~InteractiveClipping();

        //! Return clipping corner point.
        Standard_EXPORT gp_Pnt GetClippingCorner(bool theIsRel = false) const;

        //! Set clipping corner point.
        Standard_EXPORT void SetClippingCorner(const gp_Pnt &thePnt, bool theIsRel = false);

        //! Return clipping region extent.
        //! - Zero extent means no clipping;
        //! - Finite extent means range clipping;
        //! - Infinite extent means half-space clipping.
        Standard_EXPORT gp_XYZ GetClippingExtent(bool theIsRel = false) const;

        //! Set clipping region extent.
        Standard_EXPORT void SetClippingExtent(const gp_XYZ &theExt, bool theIsRel = false);

        //! Return clipping box configuration.
        Standard_EXPORT Graphic3d_BndBox3d GetClippingBox(bool theIsRel = false) const;

        //! Set clipping box configuration.
        Standard_EXPORT void SetClippingBox(const Graphic3d_BndBox3d &theBox, bool theIsRel = false);

        //! Return TRUE if clipping direction should be flipped; FALSE by default.
        bool ToFlipClipping() const { return myToFlip; }

        //! Set if clipping direction should be flipped.
        void SetFlipClipping(bool theToFlip)
        {
            if (myToFlip != theToFlip)
            {
                myToFlip = theToFlip;
                UpdateClippingBox();
            }
        }

        //! Return TRUE if clipping is enabled; TRUE by default.
        bool IsClippingOn() const { return myIsClipping; }

        //! Set if clipping is enabled.
        Standard_EXPORT void SetClippingOn(bool theToClip);

        //! Return TRUE if capping is enabled; FALSE by default.
        bool IsCappingOn() const { return myIsCapping; }

        //! Set if capping is enabled.
        Standard_EXPORT void SetCappingOn(bool theCapping);

        //! Return TRUE if capping material should inherit object's color; TRUE by default.
        bool ToUseObjectMaterial() const { return myIsObjMat; }

        //! Set if capping material should inherit object's color.
        Standard_EXPORT void SetUseObjectMaterial(bool theToUse);

    public:
        //! Return number of planes within each box dimension (0, -1, 1, or -2, 2),
        //! where negative number means negative extent
        //! (e.g. -1 means clipping in opposite direction,
        //! -2 means building a box in opposite direction from main corner point).
        Standard_EXPORT Graphic3d_Vec3i GetNbPlanesXYZ() const;

        //! Set number of planes within each box dimension.
        Standard_EXPORT void SetNbPlanesXYZ(const Graphic3d_Vec3i &theVec);

    public:
        //! Update capping aspects.
        Standard_EXPORT virtual void UpdateAspects() override;

        //! Callback called by Viewer before redrawing.
        Standard_EXPORT virtual AIS_RedrawProgressResult ProcessRedraw(const Handle(AIS_InteractiveContext) & theCtx,
                                                                       const Handle(V3d_View) & theView,
                                                                       const AIS_RedrawProgress theStage) override;

        //! Compute presentation.
        Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Standard_Integer theMode) override;

        //! Drag object in the viewer.
        Standard_EXPORT virtual Standard_Boolean ProcessDragging(const Handle(AIS_InteractiveContext) & theCtx,
                                                                 const Handle(V3d_View) & theView,
                                                                 const Handle(SelectMgr_EntityOwner) & theOwner,
                                                                 const Graphic3d_Vec2i &theDragFrom,
                                                                 const Graphic3d_Vec2i &theDragTo,
                                                                 const AIS_DragAction theAction) override;

        //! Handle context changes.
        Standard_EXPORT virtual void SetContext(const Handle(AIS_InteractiveContext) & theCtx) override;

        //! Return name of this type.
        virtual const char *TypeName() const override { return "InteractiveClipping"; }

        //! Update clipping box configuration.
        Standard_EXPORT void UpdateClippingBox();

        //! Return TRUE for dimensions defining invalid range.
        NCollection_Vec3<bool> IsVoidXYZ() const
        {
            return NCollection_Vec3<bool>(Abs(myClipExtent.X()) <= gp::Resolution(),
                                          Abs(myClipExtent.Y()) <= gp::Resolution(),
                                          Abs(myClipExtent.Z()) <= gp::Resolution());
        }

        //! Return TRUE for dimensions defining infinite clipping range.
        NCollection_Vec3<bool> IsInfiniteXYZ() const
        {
            return NCollection_Vec3<bool>(Precision::IsInfinite(myClipExtent.X()),
                                          Precision::IsInfinite(myClipExtent.Y()),
                                          Precision::IsInfinite(myClipExtent.Z()));
        }

        //! Return TRUE for dimensions defining open minimum range.
        NCollection_Vec3<bool> IsOpenMinXYZ() const
        {
            return NCollection_Vec3<bool>(Precision::IsNegativeInfinite(myClipExtent.X()),
                                          Precision::IsNegativeInfinite(myClipExtent.Y()),
                                          Precision::IsNegativeInfinite(myClipExtent.Z()));
        }

        //! Return TRUE for dimensions defining open maximum range.
        NCollection_Vec3<bool> IsOpenMaxXYZ() const
        {
            return NCollection_Vec3<bool>(Precision::IsPositiveInfinite(myClipExtent.X()),
                                          Precision::IsPositiveInfinite(myClipExtent.Y()),
                                          Precision::IsPositiveInfinite(myClipExtent.Z()));
        }

        //! Return view/scene AABB.
        Standard_EXPORT Bnd_Box GetViewSceneBox() const;

        //! Convert point to relative scene box.
        Standard_EXPORT gp_Pnt PntToRelative(const gp_Pnt &thePnt) const;

        //! Convert point from relative scene box.
        Standard_EXPORT gp_Pnt PntFromRelative(const gp_Pnt &thePnt) const;

        //! Convert range to relative scene box.
        Standard_EXPORT gp_XYZ RangeToRelative(const gp_XYZ &theVec) const;

        //! Convert range from relative scene box.
        Standard_EXPORT gp_XYZ RangeFromRelative(const gp_XYZ &theVec) const;

        //! Find dragged min/max range.
        Standard_EXPORT void FindDraggedMinMax(gp_Pnt &theNewMin, gp_Pnt &theNewMax);

        //! Set clipping corner/extent to dragged min/max range.
        Standard_EXPORT void SetDraggedMinMax(const gp_Pnt &theNewMin, const gp_Pnt &theaNewMax);

    protected:
        //! Calculate a new extent for specified dimension.
        Standard_EXPORT void calculateDimExtent(const Graphic3d_Vec3i &theNbPlanesVec,
                                                gp_XYZ &theExtent,
                                                int theComp,
                                                const Bnd_Box &theViewBox) const;

    private:
        gp_Pnt myClipCorner = gp_XYZ(0.0, 0.0, 0.0); //!< clipping box corner
        gp_XYZ myClipExtent = gp_XYZ(0.0, 0.0, 0.0); //!< clipping box extent, defining clipping direction
        gp_XYZ myClipFinExt = gp_XYZ(0.0, 0.0, 0.0); //!< backed up finite extent

        bool myIsClipping = true;
        bool myToFlip = false;
        bool myIsCapping = false;
        bool myIsObjMat = true;

        Bnd_Box myViewBoxTest;
    };

}

#endif // INTERACTIVECLIPPING_HEADERFILE
