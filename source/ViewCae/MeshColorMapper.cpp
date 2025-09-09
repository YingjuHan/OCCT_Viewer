#include <ViewCae/MeshColorMapper.h>

#include <ViewCae/CaeMeshTools.h>

#include <Graphic3d_TextureParams.hxx>
#include <Image_SupportedFormats.hxx>

namespace ViewCae {

// =======================================================================
// function : MeshColorMapper
// =======================================================================
MeshColorMapper::MeshColorMapper()
{
    Handle(Image_PixMap) anImage = new Image_PixMap();

    myColorTexture = new Graphic3d_Texture2D(anImage);
    myColorTexture->SetMipmapsGeneration(false);
    myColorTexture->GetParams()->SetRepeat(false);
}

// =======================================================================
// function : ~MeshColorMapper
// =======================================================================
MeshColorMapper::~MeshColorMapper()
{
    //
}

// =======================================================================
// function : SetMeshProperty
// =======================================================================
void MeshColorMapper::SetMeshProperty(const Handle(RWCae::MeshNVecProperty)& theProperty,
                                      int                                      theComponent,
                                      bool                                     theToUpdate)
{
    myMeshProperty          = theProperty;
    myMeshPropertyComponent = theComponent;

    if (!theToUpdate) {
        return;
    }

    UpdateColorTexture();

    if (myOnPropertyUpdateCallback != nullptr) {
        myOnPropertyUpdateCallback(this);
    }
}

// =======================================================================
// function : ValuesRange
// =======================================================================
Bnd_Range MeshColorMapper::ValuesRange() const
{
    Bnd_Range aRes;
    if (myMeshProperty.IsNull()) {
        return aRes;
    }

    const Handle(RWCae::MeshScalarPropertyBase)& aComp = myMeshProperty->GetRawComponent(myMeshPropertyComponent);
    return aComp->ValueRange();
}

// =======================================================================
// function : UpdateColorTexture
// =======================================================================
void MeshColorMapper::UpdateColorTexture()
{
    if (myMeshProperty.IsNull()) {
        return;
    }

    updateColorTexture();

    myColorTexture->UpdateRevision();
    if (myOnTextureUpdateCallback != nullptr) {
        myOnTextureUpdateCallback();
    }
}

// =======================================================================
// function : updateColorTexture
// =======================================================================
void MeshColorMapper::updateColorTexture()
{
    static constexpr double byteMax255f = 255.0;
    static constexpr int    nbImgRows2  = 2;

    const int aNbColors = CaeMeshTools::NbColorscaleColors;

    double aSCRangeFrom = 0.0;
    double aSCRangeTo   = 1.0;

    const Graphic3d_Vec3d anOut    = Graphic3d_Vec3d(0.5, 0.5, 0.5);
    Graphic3d_Vec3d       aScale00 = Graphic3d_Vec3d(0.0, 0.0, 1.0);
    Graphic3d_Vec3d       aScale05 = Graphic3d_Vec3d(0.0, 1.0, 0.0);
    Graphic3d_Vec3d       aScale10 = Graphic3d_Vec3d(1.0, 0.0, 0.0);

    const double aRange    = Max(aSCRangeTo - aSCRangeFrom, 0.01);
    const double aRangeInv = 1.0 / aRange;
    const double aMiddle   = aSCRangeFrom + 0.5 * aRange;
    const double aFrom     = aSCRangeFrom;

    Handle(Image_PixMap) anImage = myColorTexture->GetImage(new Image_SupportedFormats());
    if (anImage.IsNull() || !anImage->InitTrash(Image_Format_RGBA, aNbColors, nbImgRows2)) {
        return;
    }

    anImage->SetTopDown(false);
    for (size_t aColIter = 0; aColIter < size_t(aNbColors); ++aColIter) {
        const double    aValue = double(aColIter) / double(aNbColors - 1);
        Graphic3d_Vec3d aColorVal;
        if (aValue < aSCRangeFrom || aValue > aSCRangeTo) {
            aColorVal = anOut;
        } else if (aValue < aMiddle) {
            const double aHalfVal = (aValue - aFrom) * 2.0 * aRangeInv;
            aColorVal             = aScale00 * (1.0 - aHalfVal) + aScale05 * aHalfVal;
        } else {
            const double aHalfVal = (aValue - aFrom) * 2.0 * aRangeInv - 1.0;
            aColorVal             = aScale05 * (1.0 - aHalfVal) + aScale10 * aHalfVal;
        }

        Image_ColorRGBA& aColor0 = anImage->ChangeValue<Image_ColorRGBA>(0, aColIter);
        aColor0.r()              = Standard_Byte(byteMax255f * aColorVal.r());
        aColor0.g()              = Standard_Byte(byteMax255f * aColorVal.g());
        aColor0.b()              = Standard_Byte(byteMax255f * aColorVal.b());
        aColor0.a()              = 0xFF;
        Image_ColorRGBA& aColor1 = anImage->ChangeValue<Image_ColorRGBA>(1, aColIter);
        aColor1.r()              = Standard_Byte(byteMax255f * anOut.r());
        aColor1.g()              = Standard_Byte(byteMax255f * anOut.g());
        aColor1.b()              = Standard_Byte(byteMax255f * anOut.b());
        aColor1.a()              = 0xFF;
    }
}

} // namespace
