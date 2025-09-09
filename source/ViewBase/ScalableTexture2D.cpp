#include <ViewBase/ScalableTexture2D.h>

#include <Graphic3d_TextureParams.hxx>
#include <Message.hxx>

namespace ViewBase
{

    ScalableTexture2D::ScalableTexture2D(const Handle(ViewBase::ScalablePixMap) & thePixMap)
        : Graphic3d_Texture2D(thePixMap)
    {
        SetMipmapsGeneration(false);
        GetParams()->SetModulate(false);
        GetParams()->SetRepeat(false);
        GetParams()->SetFilter(Graphic3d_TOTF_BILINEAR);
        GetParams()->SetTextureUnit(Graphic3d_TextureUnit_BaseColor);

        UpdateTextureId();
    }

    ScalableTexture2D::~ScalableTexture2D()
    {
        //
    }

    void ScalableTexture2D::UpdateTextureId()
    {
        TCollection_AsciiString aPath = ImagePath();
        if (aPath.IsEmpty())
        {
            return;
        }

        if (!IsScalableImage())
        {
            myTexId = aPath;
            return;
        }

        const int aScale = GetRoundScale100(myResScale);

        myTexId = aPath + "@" + aScale;
    }

    bool ScalableTexture2D::SetResolutionScale(float theScale)
    {
        if (myResScale == theScale)
        {
            return false;
        }

        const int aScaleOld = GetRoundScale100(myResScale);
        const int aScaleNew = GetRoundScale100(theScale);
        myResScale = theScale;
        if (aScaleNew == aScaleOld)
        {
            return false;
        }

        UpdateTextureId();
        return true;
    }

    Handle(Image_PixMap) ScalableTexture2D::GetImage(const Handle(Image_SupportedFormats) & theSupported)
    {
        Handle(ViewBase::ScalablePixMap) anImage = ScalablePixmap();
        if (anImage.IsNull())
        {
            return Graphic3d_Texture2D::GetImage(theSupported);
        }

        if (!anImage->IsScalable() && Graphic3d_Vec2i(anImage->SizeXYZ().xy()) != Graphic3d_Vec2i(0))
        {
            myIsTopDown = anImage->IsTopDown();
            return anImage;
        }

        const double aScale = anImage->IsScalable() ? 0.01 * double(GetRoundScale100(myResScale)) : 1.0;

        Graphic3d_Vec2i aSize(Graphic3d_Vec2d(BaseSize()) * aScale);
        anImage->Load(aSize);
        myIsTopDown = anImage->IsTopDown();
        return anImage;
    }

}
