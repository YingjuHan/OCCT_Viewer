#ifndef _INTERACTIVETEXTLABEL_HEADERFILE
#define _INTERACTIVETEXTLABEL_HEADERFILE

#include <ViewBase/BaseDefaultDrawer.h>
#include <ViewBase/IInteractiveObject.h>
#include <ViewBase/InteractiveTextLabelOwner.h>
#include <ViewBase/ScalableTexture2D.h>

#include <AIS_TextLabel.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Prs3d_ShadingAspect.hxx>

namespace ViewBase {

//! Text label presentation.
class InteractiveTextLabel : public AIS_TextLabel, public IInteractiveObject {
    InteractiveTextLabel()                            = delete;
    InteractiveTextLabel(const InteractiveTextLabel&) = delete;
    InteractiveTextLabel& operator=(const InteractiveTextLabel&) = delete;

public:
    //! Calculate label dimensions.
    Standard_EXPORT bool calculateLabelParams(gp_Pnt&                               thePos,
                                              Graphic3d_Vec2d&                      theTextSize,
                                              Graphic3d_Vec2d&                      theRectSize,
                                              const Handle(Graphic3d_Text)&         theText,
                                              const Graphic3d_RenderingParams&      theRendParams,
                                              const Handle(Graphic3d_AspectText3d)& theAsp);

public:
    //! Public constructor
    Standard_EXPORT InteractiveTextLabel(const Handle(ViewBase::InteractiveTextLabelOwner)& theNewId,
                                         const Handle(Prs3d_TextAspect)&                     theDefTextStyle);

    //! Public destructor
    Standard_EXPORT virtual ~InteractiveTextLabel();

    //! Return name of this type.
    virtual const char* TypeName() const override { return "InteractiveTextLabel"; }

    //! Return text wrapping length.
    float Wrapping() const { return myWrappingLength; }

    //! Set text wrapping length.
    void SetWrapping(float theLength)
    {
        if (myWrappingLength != theLength) {
            myWrappingLength = theLength;
            SetToUpdate();
        }
    }

    //! Return attach line aspect.
    const Handle(Graphic3d_AspectLine3d)& AttachLineAspect(bool theTop) const
    {
        return theTop ? myDrawer->HiddenLineAspect()->Aspect() : myDrawer->LineAspect()->Aspect();
    }

    //! Return attach point aspect.
    const Handle(Graphic3d_AspectMarker3d)& AttachPointAspect(bool theTop) const
    {
        return theTop ? myTopAttachAsp : myDrawer->PointAspect()->Aspect();
    }

    //! Return TRUE if 3D point should be drawn; TRUE by default.
    bool ToDrawAttachPoint() const
    {
        return AttachPointAspect(false)->MarkerType() != Aspect_TOM_EMPTY
            || AttachPointAspect(true)->MarkerType() != Aspect_TOM_EMPTY;
    }

    //! Set if 3D point should be drawn or not.
    void SetAttachMarker(Aspect_TypeOfMarker theBackType, Aspect_TypeOfMarker theTopType)
    {
        if (AttachPointAspect(false)->MarkerType() != theBackType
            || AttachPointAspect(true)->MarkerType() != theTopType) {
            AttachPointAspect(false)->SetMarkerType(theBackType);
            AttachPointAspect(true)->SetMarkerType(theTopType);
            SetToUpdate();
        }
    }

    //! Get if 3D point should be drawn or not.
    Aspect_TypeOfMarker AttachMarker() const
    {
        const Aspect_TypeOfMarker aBack = AttachPointAspect(false)->MarkerType();
        const Aspect_TypeOfMarker aTop  = AttachPointAspect(true)->MarkerType();
        return ViewBase::BaseDefaultDrawer::MergedMarker(aBack, aTop);
    }

    //! Set if 3D point should be drawn or not.
    void SetAttachMarker(Aspect_TypeOfMarker theType)
    {
        Aspect_TypeOfMarker aBack = Aspect_TOM_EMPTY;
        Aspect_TypeOfMarker aTop  = Aspect_TOM_EMPTY;
        ViewBase::BaseDefaultDrawer::SplitMarker(theType, aBack, aTop);
        if (aBack != Aspect_TOM_EMPTY && aTop == Aspect_TOM_EMPTY) {
            std::swap(aBack, aTop);
        }
        SetAttachMarker(aBack, aTop);
    }

    //! Return padding to draw text from point position; 20 by default.
    float PointPadding() const { return myPointPadding; }

    //! Set padding to draw text from point.
    void SetPointPadding(float thePadding)
    {
        if (myPointPadding != thePadding) {
            myPointPadding = thePadding;
            if (ToDrawAttachPoint()) {
                SetToUpdate();
            }
        }
    }

    //! Return TRUE if background rectangle should be drawn; FALSE by default.
    bool ToDrawRectangle() const { return myDrawer->ShadingAspect()->Aspect()->InteriorStyle() != Aspect_IS_EMPTY; }

    //! Set if background rectangle should be drawn or not.
    void SetDrawRectangle(bool theToDraw)
    {
        if (ToDrawRectangle() != theToDraw) {
            myDrawer->ShadingAspect()->Aspect()->SetInteriorStyle(theToDraw ? Aspect_IS_SOLID : Aspect_IS_EMPTY);
            SetToUpdate();
        }
    }

    //! Return padding to draw rectangle; 5 by default.
    float RectanglePadding() const { return myRectPadding; }

    //! Set padding to draw rectangle.
    void SetRectanglePadding(float thePadding)
    {
        if (myRectPadding != thePadding) {
            myRectPadding = thePadding;
            if (ToDrawRectangle()) {
                SetToUpdate();
            }
        }
    }

    //! Return true if attach point is defined.
    bool HasAttachPoint() const { return !Precision::IsInfinite(myAttachPoint.X()); }

    //! Return optional attach point.
    const gp_Pnt& AttachPoint() const { return myAttachPoint; }

    //! Return optional attach point.
    void SetAttachPoint(const gp_Pnt& thePnt)
    {
        if (!myAttachPoint.IsEqual(thePnt, Precision::Confusion())) {
            myAttachPoint = thePnt;
            SetToUpdate();
        }
    }

    //! Return TRUE if icon image is defined.
    bool HasIcon() const { return !myIconTexture.IsNull(); }

    //! Return icon texture.
    const Handle(ViewBase::ScalableTexture2D)& Icon() const { return myIconTexture; }

    //! Set new icon texture.
    void SetIcon(const Handle(ViewBase::ScalableTexture2D)& theImage)
    {
        if (myIconTexture == theImage) {
            return;
        }

        myIconTexture = theImage;
        if (!myIconTexture.IsNull()) {
            myIconTexture->SetResolutionScale(myDevicePixelRatio);
        }
        myIconAsp->SetTextureMap(theImage);
        SetToUpdate();
    }

public:
    //! Draw rectangle.
    Standard_EXPORT Handle(Graphic3d_ArrayOfTriangles) DrawRectangle(int theExp = 0);

    //! Return label transform persistence.
    const Handle(Graphic3d_TransformPers)& LabelTransformPersistence() const { return myLabTrsfPers; }

public:
    //! Reset color to defaults.
    virtual void UnsetColor() override
    {
        SetColor(Quantity_NOC_BLACK);
        hasOwnColor = false;
    }

public:
    //! Update presentation to new pixel scale ratio.
    Standard_EXPORT virtual bool OnPixelScaleChange(const Handle(AIS_InteractiveObject)& theObj,
                                                    float                                theRatio) override;

    //! Update current state.
    Standard_EXPORT virtual void UpdateAspects() override;

private:
    //! Return TRUE for supported display modes (only mode 0 is supported).
    virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const override { return theMode == 0; }

    //! Compute presentation.
    Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                         const Handle(Prs3d_Presentation)&         thePrs,
                                         const Standard_Integer                    theMode) override;

    //! Compute selection.
    Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSel,
                                                  const Standard_Integer             theMode) override;

    //! Draw 3D point marker.
    Standard_EXPORT void drawPoint(const Handle(Prs3d_Presentation)& thePrs,
                                   const Handle(Graphic3d_Aspects)&  theAspect);

    //! Draw 3D attach line.
    Standard_EXPORT void drawAttachLine(const Handle(Prs3d_Presentation)& thePrs,
                                        const Handle(Graphic3d_Aspects)&  theAspect);

    //! Draw rectangle.
    Standard_EXPORT void drawRectangle(const Handle(Prs3d_Presentation)& thePrs);

    //! Update text element.
    Standard_EXPORT void updateText();

    //! Draw text label.
    Standard_EXPORT void drawText(const Handle(Prs3d_Presentation)& thePrs);

    //! Calculate text position.
    gp_Pnt getTextPoint() const;

    //! Draw icon image.
    Standard_EXPORT void drawIcon(const Handle(Prs3d_Presentation)& thePrs);

private:
    //! Calculate presentation corner.
    Standard_EXPORT Graphic3d_Vec2i GetPrsCorner(bool theIsFullLen) const;

private:
    static constexpr int defaultOffset = 10;

private:
    Handle(Graphic3d_TransformPers)  myLabTrsfPers;
    Handle(Graphic3d_Aspects)        myShadowAsp;
    Handle(Graphic3d_AspectMarker3d) myTopAttachAsp;

    Handle(Graphic3d_Aspects)            myIconAsp;
    Handle(ViewBase::ScalableTexture2D) myIconTexture;
    Graphic3d_Vec2i                      myIconSize;

    gp_Pnt myAttachPoint = gp_Pnt(RealLast(), RealLast(), RealLast());

    float myWrappingLength = -1.0f;
    float myRectPadding    = 5.0f;
    float myPointPadding   = 20.0f;

    Handle(Graphic3d_Text) myTextElem;
    gp_Pnt                 myTextBBoxPos;
    Graphic3d_Vec2d        myTextBBoxSize;
};

}

#endif // _INTERACTIVETEXTLABEL_HEADERFILE