#ifndef _SCALABLETEXTURE2D_HEADERFILE
#define _SCALABLETEXTURE2D_HEADERFILE

#include <ViewBase/ScalablePixMap.h>

#include <Graphic3d_Texture2D.hxx>

namespace ViewBase
{

    //! Texture object for ScalablePixMap image.
    //! This object provides an interface for requesting higher image resolution (e.g. depending on screen pixel density).
    class ScalableTexture2D : public Graphic3d_Texture2D
    {
    public:
        //! Compute round scale factor in percents.
        static int GetRoundScale100(double theScale)
        {
            static constexpr double scale100 = 100.0;
            return static_cast<int>(Round(double(theScale) * scale100));
        }

    public:
        //! Main constructor.
        Standard_EXPORT explicit ScalableTexture2D(const Handle(ViewBase::ScalablePixMap) & thePixMap);

        //! Destructor.
        Standard_EXPORT virtual ~ScalableTexture2D();

        //! Load image.
        Standard_EXPORT virtual Handle(Image_PixMap) GetImage(const Handle(Image_SupportedFormats) & theSupported) override;

        //! Return scalable pixmap.
        Handle(ViewBase::ScalablePixMap) ScalablePixmap() const
        {
            return Handle(ViewBase::ScalablePixMap)::DownCast(myPixMap);
        }

        //! Return image path.
        const TCollection_AsciiString &ImagePath() const
        {
            if (const ViewBase::ScalablePixMap *anImage = dynamic_cast<ViewBase::ScalablePixMap *>(myPixMap.get()))
            {
                return anImage->ImagePath();
            }
            static const TCollection_AsciiString anEmpty;
            return anEmpty;
        }

        //! Return TRUE if image is in vector format.
        bool IsScalableImage() const
        {
            if (const ViewBase::ScalablePixMap *anImage = dynamic_cast<ViewBase::ScalablePixMap *>(myPixMap.get()))
            {
                return anImage->IsScalable();
            }
            return false;
        }

        //! Return base image dimension.
        Graphic3d_Vec2i BaseSize() const
        {
            if (const ViewBase::ScalablePixMap *anImage = dynamic_cast<ViewBase::ScalablePixMap *>(myPixMap.get()))
            {
                return anImage->BaseSize() != Graphic3d_Vec2i(0) ? anImage->BaseSize()
                                                                 : Graphic3d_Vec2i(anImage->SizeXYZ().xy());
            }
            return !myPixMap.IsNull() ? Graphic3d_Vec2i(myPixMap->SizeXYZ().xy()) : Graphic3d_Vec2i(0);
        }

        //! Return resolution scale.
        float ResolutionScale() const { return myResScale; }

        //! Set resolution scale.
        Standard_EXPORT bool SetResolutionScale(float theScale);

    private:
        //! Generate texture id.
        Standard_EXPORT void UpdateTextureId();

    private:
        float myResScale = 1.0f;
    };

}

#endif