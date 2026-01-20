#ifndef _SCALABLEPIXMAP_HEADERFILE
#define _SCALABLEPIXMAP_HEADERFILE

#include <Graphic3d_Vec2.hxx>
#include <Image_AlienPixMap.hxx>

namespace ViewBase
{
    //! Image loader class for loading scalable (SVG) images with different resolution
    //! when using JavaScript engine (WebAssembly).
    class ScalablePixMap : public Image_AlienPixMap
    {
    public:
        //! Empty constructor.
        Standard_EXPORT explicit ScalablePixMap(const TCollection_AsciiString &theImagePath);

        //! Destructor.
        Standard_EXPORT virtual ~ScalablePixMap();

        //! Release memory.
        Standard_EXPORT virtual void Clear() override;

        //! Return image path.
        const TCollection_AsciiString &ImagePath() const { return myImagePath; }

        //! Return base dimensions.
        const Graphic3d_Vec2i &BaseSize() const { return myBaseSize; }

        //! Return TRUE for a vector image format.
        bool IsScalable() const { return myIsScalable; }

        //! Load image with specified resolution.
        Standard_EXPORT bool Load(const Graphic3d_Vec2i &theSize = Graphic3d_Vec2i(0));

    private:
        //! Release memory.
        Standard_EXPORT void release() noexcept;

    private:
        const char *myRawData = nullptr;

        TCollection_AsciiString myImagePath;
        Graphic3d_Vec2i myBaseSize;
        bool myIsScalable = false;
    };

}

#endif // _SCALABLEPIXMAP_HEADERFILE