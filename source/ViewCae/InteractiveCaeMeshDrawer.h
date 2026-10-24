

#ifndef INTERACTIVECAEMESHDRAWER_HEADERFILE
#define INTERACTIVECAEMESHDRAWER_HEADERFILE

#include <ViewCae/MeshPart.h>

#include <Graphic3d_AspectFillArea3d.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Graphic3d_Texture2D.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Prs3d_TextAspect.hxx>

namespace ViewCae {
//! Class defining presentation attributes for ViewCae::InteractiveCaeMesh object.
class InteractiveCaeMeshDrawer : public Prs3d_Drawer {
public:
    //! Default constructor.
    Standard_EXPORT InteractiveCaeMeshDrawer();

    //! Destructor.
    Standard_EXPORT virtual ~InteractiveCaeMeshDrawer();

    //! Update aspects with values from other drawer.
    Standard_EXPORT void UpdateAspects(const Handle(Prs3d_Drawer)& theDrawer);

    //! Update texture of the specified aspect.
    Standard_EXPORT void UpdateAspectTexture(const Handle(Graphic3d_Aspects)&   theAspect,
                                             const Handle(Graphic3d_Texture2D)& theTexture,
                                             const float                        theTextureTransp);

    //! Enables/disables texture mapping for fill area aspects.
    Standard_EXPORT void SetFillAreaTextureMapOn(bool theToEnable);

    //! Enables/disables texture mapping for elem0d aspect.
    Standard_EXPORT void SetElem0dTextureMapOn(bool theToEnable);

    //! Enables/disables texture mapping for point aspect.
    Standard_EXPORT void SetPointTextureMapOn(bool theToEnable);

    //! Enables/disables texture mapping for vector aspect.
    Standard_EXPORT void SetVectorTextureMapOn(bool theToEnable);

    //! Enables/disables texture mapping for streamlines aspect.
    Standard_EXPORT void SetStreamlinesTextureMapOn(bool theToEnable);

    //! Enables/disables polygon offset.
    Standard_EXPORT void SetPolygonOffsetOn(bool theToEnable);

    //! Fill area aspect for mesh 3D data.
    const Handle(Graphic3d_AspectFillArea3d)& FillAreaAspect3d() const { return myFillAreaAspect3d; }

    //! Line aspect for mesh 3D data.
    const Handle(Graphic3d_AspectLine3d)& LineAspect3d() const { return myLineAspect3d; }

    //! Fill area aspect for mesh 2D data.
    const Handle(Graphic3d_AspectFillArea3d)& FillAreaAspect2d() const { return myFillAreaAspect2d; }

    //! Line aspect for mesh 2D data.
    const Handle(Graphic3d_AspectLine3d)& LineAspect2d() const { return myLineAspect2d; }

    //! Fill area aspect for vector mesh presentation.
    const Handle(Graphic3d_AspectFillArea3d)& FillAreaAspectVectors() const { return myFillAreaAspectVectors; }

    //! Marker aspect for 0D elements.
    const Handle(Graphic3d_AspectMarker3d)& Elem0dAspect() const { return myElem0dAspect; }

    //! Line aspect for centroid points.
    const Handle(Graphic3d_AspectMarker3d)& CentroidAspect3d() const { return myCentroidAspect3d; }

    //! Line aspect for centroid points.
    const Handle(Graphic3d_AspectMarker3d)& CentroidAspect2d() const { return myCentroidAspect2d; }

    //! Marker aspect for top-layer annotation points.
    const Handle(Graphic3d_AspectMarker3d)& AnnotationTopPointAspect() const { return myAnnotationTopPointAspect; }

    //! Marker aspect for default-layer annotation points.
    const Handle(Graphic3d_AspectMarker3d)& AnnotationPointAspect() const { return myAnnotationPointAspect; }

    //! Fill area aspect for annotation text background.
    const Handle(Graphic3d_AspectFillArea3d)& AnnotationFillAreaAspect() const { return myAnnotationFillAreaAspect; }

    //! Fill area aspect for mesh streamlines.
    const Handle(Graphic3d_AspectFillArea3d)& StreamlinesTubeAspect() const { return myStreamlinesTubeAspect; }

    //! Line aspect for mesh streamlines.
    const Handle(Graphic3d_AspectLine3d)& StreamlinesLineAspect() const { return myStreamlinesLineAspect; }

    //! Return aspect assigned to specified mesh presentation part.
    Standard_EXPORT const Handle(Graphic3d_Aspects)& Aspect(MeshPartType theType) const;

public:
    //! Return shading color, Quantity_NOC_WHITE by default.
    const Quantity_Color& ShadingColor() const { return myShadingColor; }

    //! Set shading color.
    Standard_EXPORT void SetShadingColor(const Quantity_Color& theColor);

    //! Return edges color, Quantity_NOC_GRAY80 by default.
    const Quantity_Color& EdgesColor() const { return LineAspect()->Aspect()->Color(); }

    //! Set edges color.
    Standard_EXPORT void SetEdgesColor(const Quantity_Color& theColor);
    Standard_EXPORT void SetEdgesType(Aspect_TypeOfLine type);

    //! Return points color, Quantity_NOC_GRAY50 by default.
    const Quantity_Color& PointsColor() const { return myPointsColor; }

    //! Set points color.
    Standard_EXPORT void SetPointsColor(const Quantity_Color& theColor);

    //! Return vectors color, Quantity_NOC_WHITE by default.
    const Quantity_Color& VectorsColor() const { return myVectorsColor; }

    //! Set vectors color.
    Standard_EXPORT void SetVectorsColor(const Quantity_Color& theColor);

    //! Return streamlines color, Quantity_NOC_WHITE by default.
    const Quantity_Color& StreamlinesColor() const { return myStreamlinesColor; }

    //! Set streamlines color.
    Standard_EXPORT void SetStreamlinesColor(const Quantity_Color& theColor);

    //! Return edges width, 1 by default.
    float EdgesWidth() const { return LineAspect()->Aspect()->Width(); }

    //! Set edges width.
    Standard_EXPORT void SetEdgesWidth(const float theWidth);

    //! Return points size, 5 by default.
    float PointsSize() const { return PointAspect()->Aspect()->Scale(); }

    //! Set points size.
    void SetPointsSize(const float theSize) { PointAspect()->SetScale(theSize); }

    //! Return shading transparency, 0.0 by default.
    float ShadingTransparency() const { return myShadingTransparency; }

    //! Set shading transparency.
    Standard_EXPORT void SetShadingTransparency(const float theValue, const bool toUpdateValue = true);

    //! Check if texture is modulated with defined colors.
    bool ToModulateTexture() const { return myToModulateTexture; }

    //! Define if texture should be modulated with defined colors.
    void SetModulateTexture(bool theToUse) { myToModulateTexture = theToUse; }

protected:
    //! Initialize shading aspects.
    void InitDefaultShading();

protected:
    Handle(Graphic3d_AspectFillArea3d) myFillAreaAspect3d;
    Handle(Graphic3d_AspectLine3d)     myLineAspect3d;
    Handle(Graphic3d_AspectFillArea3d) myFillAreaAspect2d;
    Handle(Graphic3d_AspectLine3d)     myLineAspect2d;
    Handle(Graphic3d_AspectFillArea3d) myFillAreaAspectVectors;

    Handle(Graphic3d_AspectFillArea3d) myStreamlinesTubeAspect;
    Handle(Graphic3d_AspectLine3d)     myStreamlinesLineAspect;

    Handle(Graphic3d_AspectMarker3d) myElem0dAspect;
    Handle(Graphic3d_AspectMarker3d) myCentroidAspect2d;
    Handle(Graphic3d_AspectMarker3d) myCentroidAspect3d;

    Handle(Graphic3d_AspectMarker3d)   myAnnotationTopPointAspect;
    Handle(Graphic3d_AspectMarker3d)   myAnnotationPointAspect;
    Handle(Graphic3d_AspectFillArea3d) myAnnotationFillAreaAspect;

    Quantity_Color myShadingColor;
    Quantity_Color myPointsColor;
    Quantity_Color myVectorsColor;
    Quantity_Color myStreamlinesColor;
    float          myShadingTransparency;

    bool myToModulateTexture = false;
};

} // namespace

#endif // INTERACTIVECAEMESHDRAWER_HEADERFILE
