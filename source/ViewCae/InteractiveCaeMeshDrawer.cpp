#include <ViewCae/InteractiveCaeMeshDrawer.h>

#include <ViewCae/CaeMeshArrowsTools.h>
#include <ViewCae/CaeMeshStreamlinesBuilder.h>

#include <ViewBase/BaseDefaultDrawer.h>

#include <Font_NameOfFont.hxx>

namespace {
static constexpr Quantity_NameOfColor DEFAULT_SHADING_COLOR = Quantity_NOC_WHITE;
static constexpr Quantity_NameOfColor DEFAULT_POINTS_COLOR  = Quantity_NOC_GRAY50;
}

namespace ViewCae {

// =======================================================================
// function : InteractiveCaeMeshDrawer
// =======================================================================
InteractiveCaeMeshDrawer::InteractiveCaeMeshDrawer()
    : myShadingColor(DEFAULT_SHADING_COLOR),
      myPointsColor(DEFAULT_POINTS_COLOR),
      myVectorsColor(DEFAULT_SHADING_COLOR),
      myStreamlinesColor(DEFAULT_SHADING_COLOR),
      myShadingTransparency(0.0f)
{
    SetShadingAspect(new Prs3d_ShadingAspect());
    SetLineAspect(new Prs3d_LineAspect(Quantity_NOC_GRAY80, Aspect_TOL_SOLID, 1.0f));
    SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_POINT, myPointsColor, 5.0f));

    SetFreeBoundaryAspect(new Prs3d_LineAspect(Quantity_NOC_GREEN, Aspect_TOL_SOLID, 1.0f));
    SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_NOC_ORANGE, Aspect_TOL_SOLID, 1.0f));

    SetTextAspect(new Prs3d_TextAspect());
    TextAspect()->SetFont(Font_NOF_SANS_SERIF);
    TextAspect()->Aspect()->SetColor(Quantity_NOC_BLACK);
    TextAspect()->Aspect()->SetTextFontAspect(Font_FontAspect_Regular);
    TextAspect()->SetHeight(ViewBase::BaseDefaultDrawer::m_defTextHeight);
    InitDefaultShading();

    myFillAreaAspect3d = new Graphic3d_AspectFillArea3d(*ShadingAspect()->Aspect());
    myFillAreaAspect3d->SetAlphaMode(Graphic3d_AlphaMode_BlendAuto);
    myFillAreaAspect2d = new Graphic3d_AspectFillArea3d(*ShadingAspect()->Aspect());
    myFillAreaAspect2d->SetAlphaMode(Graphic3d_AlphaMode_BlendAuto);

    myLineAspect3d = new Graphic3d_AspectLine3d(*LineAspect()->Aspect());
    myLineAspect2d = new Graphic3d_AspectLine3d(*LineAspect()->Aspect());

    myFillAreaAspectVectors = new Graphic3d_AspectFillArea3d(*ShadingAspect()->Aspect());
    myFillAreaAspectVectors->SetShaderProgram(ArrowShaderManager::ShaderProgram());

    myElem0dAspect     = new Graphic3d_AspectMarker3d(Aspect_TOM_PLUS, myShadingColor, 1.0f);
    myCentroidAspect2d = new Graphic3d_AspectMarker3d(Aspect_TOM_POINT, myShadingColor, 5.0f);
    myCentroidAspect3d = new Graphic3d_AspectMarker3d(Aspect_TOM_POINT, myShadingColor, 5.0f);

    myAnnotationTopPointAspect = new Graphic3d_AspectMarker3d(Aspect_TOM_O, Quantity_NOC_WHITE, 2.0f);
    myAnnotationPointAspect    = new Graphic3d_AspectMarker3d(Aspect_TOM_O_POINT, Quantity_NOC_WHITE, 2.0f);
    Graphic3d_MaterialAspect aMat;
    myAnnotationFillAreaAspect = new Graphic3d_AspectFillArea3d(Aspect_IS_SOLID,
                                                                Quantity_NOC_WHITE,
                                                                Quantity_NOC_BLACK,
                                                                Aspect_TOL_SOLID,
                                                                1.0f,
                                                                aMat,
                                                                aMat);
    myAnnotationFillAreaAspect->SetPolygonOffsets(Aspect_POM_Fill, 1.0f, 1.0f);

    myStreamlinesTubeAspect = new Graphic3d_AspectFillArea3d(*ShadingAspect()->Aspect());
    myStreamlinesTubeAspect->SetShaderProgram(StreamlinesShaderManager::ShaderProgram(true));
    myStreamlinesTubeAspect->SetShadingModel(Graphic3d_TypeOfShadingModel_Phong);
    myStreamlinesLineAspect = new Graphic3d_AspectLine3d(*LineAspect()->Aspect());
    myStreamlinesLineAspect->SetShaderProgram(StreamlinesShaderManager::ShaderProgram(false));
    myStreamlinesLineAspect->SetShadingModel(Graphic3d_TypeOfShadingModel_Phong);
}

// =======================================================================
// function : ~InteractiveCaeMeshDrawer
// =======================================================================
InteractiveCaeMeshDrawer::~InteractiveCaeMeshDrawer()
{
    //
}

// =======================================================================
// function : InitDefaultShading
// =======================================================================
void InteractiveCaeMeshDrawer::InitDefaultShading()
{
    const Handle(Prs3d_ShadingAspect)& aShadeAsp = ShadingAspect();

    Graphic3d_MaterialAspect aShadeMat(Graphic3d_NameOfMaterial_Plastified);
    aShadeMat.SetAmbientColor(Quantity_Color(Graphic3d_Vec3(0.3f)));
    aShadeMat.SetDiffuseColor(Quantity_Color(Graphic3d_Vec3(0.7f)));
    aShadeMat.SetSpecularColor(Quantity_Color(Graphic3d_Vec3(0.0f)));
    aShadeMat.SetEmissiveColor(Quantity_Color(Graphic3d_Vec3(0.0f)));
    aShadeAsp->SetMaterial(aShadeMat);
    aShadeAsp->SetColor(myShadingColor);
    aShadeAsp->Aspect()->SetInteriorStyle(Aspect_IS_SOLID);
    aShadeAsp->Aspect()->SetFaceCulling(Graphic3d_TypeOfBackfacingModel_DoubleSided);
    aShadeAsp->Aspect()->SetShadingModel(Graphic3d_TypeOfShadingModel_Phong);
}

// =======================================================================
// function : UpdateAspects
// =======================================================================
void InteractiveCaeMeshDrawer::UpdateAspects(const Handle(Prs3d_Drawer)& theDrawer)
{
    auto aDrawer = dynamic_cast<InteractiveCaeMeshDrawer*>(theDrawer.get());
    if (aDrawer == nullptr) {
        return;
    }

    SetShadingColor(aDrawer->ShadingColor());
    SetPointsColor(aDrawer->PointsColor());
    SetEdgesColor(aDrawer->EdgesColor());
    SetVectorsColor(aDrawer->VectorsColor());
    SetStreamlinesColor(aDrawer->StreamlinesColor());
    SetEdgesWidth(aDrawer->EdgesWidth());
    SetPointsSize(aDrawer->PointsSize());
    SetShadingTransparency(aDrawer->ShadingTransparency());
    SetModulateTexture(aDrawer->ToModulateTexture());
    ShadingAspect()->Aspect()->SetPolygonOffset(aDrawer->ShadingAspect()->Aspect()->PolygonOffset());
}

// =======================================================================
// function : Aspect
// =======================================================================
const Handle(Graphic3d_Aspects)& InteractiveCaeMeshDrawer::Aspect(MeshPartType theType) const
{
    switch (theType) {
        case MeshPartType::Triangles3d: return myFillAreaAspect3d;
        case MeshPartType::Triangles2d: return myFillAreaAspect2d;
        case MeshPartType::Edges3d: return myLineAspect3d;
        case MeshPartType::Edges2d: return myLineAspect2d;
        case MeshPartType::SharpEdges3d:
        case MeshPartType::SharpEdges2d: return FaceBoundaryAspect()->Aspect();
        case MeshPartType::FreeEdges3d:
        case MeshPartType::FreeEdges2d: return FreeBoundaryAspect()->Aspect();
        case MeshPartType::Elems0d: return myElem0dAspect;
        case MeshPartType::Points: return PointAspect()->Aspect();
        case MeshPartType::Centroids3d: return myCentroidAspect3d;
        case MeshPartType::Centroids2d: return myCentroidAspect2d;
        case MeshPartType::Vectors: return myFillAreaAspectVectors;
        default: break;
    }

    static Handle(Graphic3d_Aspects) aNullAspect;
    return aNullAspect;
}

// =======================================================================
// function : SetFillAreaTextureMapOn
// =======================================================================
void InteractiveCaeMeshDrawer::SetFillAreaTextureMapOn(bool theToEnable)
{
    ShadingAspect()->Aspect()->SetTextureMapOn(theToEnable);
    myFillAreaAspect3d->SetTextureMapOn(theToEnable);
    myFillAreaAspect2d->SetTextureMapOn(theToEnable);
    Quantity_Color aShadingColor = theToEnable && !myToModulateTexture ? DEFAULT_SHADING_COLOR : myShadingColor;
    myFillAreaAspect3d->SetColor(aShadingColor);
    myFillAreaAspect2d->SetColor(aShadingColor);
}

// =======================================================================
// function : SetElem0dTextureMapOn
// =======================================================================
void InteractiveCaeMeshDrawer::SetElem0dTextureMapOn(bool theToEnable)
{
    myElem0dAspect->SetTextureMapOn(theToEnable);
    myCentroidAspect3d->SetTextureMapOn(theToEnable);
    myCentroidAspect2d->SetTextureMapOn(theToEnable);
    Quantity_Color aShadingColor = theToEnable && !myToModulateTexture ? DEFAULT_SHADING_COLOR : myShadingColor;
    myElem0dAspect->SetColor(aShadingColor);
    myCentroidAspect3d->SetColor(aShadingColor);
    myCentroidAspect2d->SetColor(aShadingColor);
}

// =======================================================================
// function : SetPointTextureMapOn
// =======================================================================
void InteractiveCaeMeshDrawer::SetPointTextureMapOn(bool theToEnable)
{
    PointAspect()->Aspect()->SetTextureMapOn(theToEnable);
    Quantity_Color aPointsColor = theToEnable && !myToModulateTexture ? DEFAULT_POINTS_COLOR : myPointsColor;
    PointAspect()->SetColor(aPointsColor);
}

// =======================================================================
// function : SetVectorTextureMapOn
// =======================================================================
void InteractiveCaeMeshDrawer::SetVectorTextureMapOn(bool theToEnable)
{
    myFillAreaAspectVectors->SetTextureMapOn(theToEnable);
    Quantity_Color aVectorShadingColor = theToEnable && !myToModulateTexture ? DEFAULT_SHADING_COLOR : myVectorsColor;
    myFillAreaAspectVectors->SetColor(aVectorShadingColor);
}

// =======================================================================
// function : SetStreamlinesTextureMapOn
// =======================================================================
void InteractiveCaeMeshDrawer::SetStreamlinesTextureMapOn(bool theToEnable)
{
    myStreamlinesTubeAspect->SetTextureMapOn(theToEnable);
    myStreamlinesLineAspect->SetTextureMapOn(theToEnable);
    Quantity_Color aColor = theToEnable && !myToModulateTexture ? DEFAULT_SHADING_COLOR : myStreamlinesColor;
    myStreamlinesTubeAspect->SetColor(aColor);
    myStreamlinesLineAspect->SetColor(aColor);
}

// =======================================================================
// function : SetPolygonOffsetOn
// =======================================================================
void InteractiveCaeMeshDrawer::SetPolygonOffsetOn(bool theToEnable)
{
    Graphic3d_PolygonOffset anOffset;
    anOffset.Mode = theToEnable ? Aspect_POM_Fill : Aspect_POM_Off;

    ShadingAspect()->Aspect()->SetPolygonOffset(anOffset);
    myFillAreaAspect3d->SetPolygonOffset(anOffset);
    myFillAreaAspect2d->SetPolygonOffset(anOffset);
}

// =======================================================================
// function : SetShadingColor
// =======================================================================
void InteractiveCaeMeshDrawer::SetShadingColor(const Quantity_Color& theColor)
{
    myShadingColor = theColor;
    SetColor(theColor);
    if (!ShadingAspect()->Aspect()->ToMapTexture() || myToModulateTexture) {
        ShadingAspect()->SetColor(theColor);
    }
    if (!myFillAreaAspect3d->ToMapTexture() || myToModulateTexture) {
        myFillAreaAspect3d->SetColor(theColor);
    }
    if (!myFillAreaAspect2d->ToMapTexture() || myToModulateTexture) {
        myFillAreaAspect2d->SetColor(theColor);
    }
    if (!myCentroidAspect3d->ToMapTexture() || myToModulateTexture) {
        myCentroidAspect3d->SetColor(theColor);
    }
    if (!myCentroidAspect2d->ToMapTexture() || myToModulateTexture) {
        myCentroidAspect2d->SetColor(theColor);
    }
    if (!myElem0dAspect->ToMapTexture() || myToModulateTexture) {
        myElem0dAspect->SetColor(theColor);
    }
}

// =======================================================================
// function : SetEdgesColor
// =======================================================================
void InteractiveCaeMeshDrawer::SetEdgesColor(const Quantity_Color& theColor)
{
    LineAspect()->SetColor(theColor);
    myLineAspect3d->SetColor(theColor);
    myLineAspect2d->SetColor(theColor);
}

// =======================================================================
// function : SetPointsColor
// =======================================================================
void InteractiveCaeMeshDrawer::SetPointsColor(const Quantity_Color& theColor)
{
    myPointsColor = theColor;
    if (!PointAspect()->Aspect()->ToMapTexture() || myToModulateTexture) {
        PointAspect()->SetColor(theColor);
    }
}

// =======================================================================
// function : SetVectorsColor
// =======================================================================
void InteractiveCaeMeshDrawer::SetVectorsColor(const Quantity_Color& theColor)
{
    myVectorsColor = theColor;
    if (!myFillAreaAspectVectors->ToMapTexture() || myToModulateTexture) {
        myFillAreaAspectVectors->SetColor(theColor);
    }
}

// =======================================================================
// function : SetStreamlinesColor
// =======================================================================
void InteractiveCaeMeshDrawer::SetStreamlinesColor(const Quantity_Color& theColor)
{
    myStreamlinesColor = theColor;
    if (!myStreamlinesTubeAspect->ToMapTexture() || myToModulateTexture) {
        myStreamlinesTubeAspect->SetColor(theColor);
    }
    if (!myStreamlinesLineAspect->ToMapTexture() || myToModulateTexture) {
        myStreamlinesLineAspect->SetColor(theColor);
    }
}

// =======================================================================
// function : SetEdgesWidth
// =======================================================================
void InteractiveCaeMeshDrawer::SetEdgesWidth(const float theWidth)
{
    LineAspect()->SetWidth(theWidth);
    myLineAspect3d->SetWidth(theWidth);
    myLineAspect2d->SetWidth(theWidth);
}

// =======================================================================
// function : SetShadingTransparency
// =======================================================================
void InteractiveCaeMeshDrawer::SetShadingTransparency(const float theValue, const bool toUpdateValue)
{
    if (toUpdateValue) {
        myShadingTransparency = theValue;
    }

    SetTransparency(theValue);

    ShadingAspect()->SetTransparency(theValue);

    myFillAreaAspect3d->ChangeFrontMaterial().SetTransparency(theValue);
    myFillAreaAspect3d->SetInteriorColor(Quantity_ColorRGBA(myFillAreaAspect3d->InteriorColor(), 1.0f - theValue));
    myFillAreaAspect3d->ChangeBackMaterial().SetTransparency(theValue);
    myFillAreaAspect3d->SetBackInteriorColor(
        Quantity_ColorRGBA(myFillAreaAspect3d->BackInteriorColor(), 1.0f - theValue));

    myFillAreaAspect2d->ChangeFrontMaterial().SetTransparency(theValue);
    myFillAreaAspect2d->SetInteriorColor(Quantity_ColorRGBA(myFillAreaAspect2d->InteriorColor(), 1.0f - theValue));
    myFillAreaAspect2d->ChangeBackMaterial().SetTransparency(theValue);
    myFillAreaAspect2d->SetBackInteriorColor(
        Quantity_ColorRGBA(myFillAreaAspect2d->BackInteriorColor(), 1.0f - theValue));
}

// =======================================================================
// function : UpdateAspectTexture
// =======================================================================
void InteractiveCaeMeshDrawer::UpdateAspectTexture(const Handle(Graphic3d_Aspects)&   theAspect,
                                                   const Handle(Graphic3d_Texture2D)& theTexture,
                                                   const float                        theTextureTransp)
{
    const Handle(Graphic3d_TextureSet)& aTextureSet = theAspect->TextureSet();
    if (aTextureSet.IsNull() || theTexture.IsNull()) {
        theAspect->SetAlphaMode(Graphic3d_AlphaMode_BlendAuto);
        theAspect->SetTextureMap(theTexture);
        return;
    }

    theAspect->SetAlphaMode((theTextureTransp < 0.01f || theAspect->FrontMaterial().Transparency() > 0.01f)
                                ? Graphic3d_AlphaMode_BlendAuto
                                : (theTextureTransp < 0.99f ? Graphic3d_AlphaMode_Blend : Graphic3d_AlphaMode_Mask));
    if (aTextureSet->First() == theTexture) {
        return;
    }

    aTextureSet->SetFirst(theTexture);
    theAspect->SetTextureSet(aTextureSet);
}

} // namespace