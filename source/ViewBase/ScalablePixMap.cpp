#include <ViewBase/ScalablePixMap.h>

#include <Message.hxx>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>

//! Get SVG image info.
EM_JS(bool, jsGetImageInfo, (const char *theImageId, int *theW, int *theH, int *theIsScl), {
    const aPath = jsUTF8ToString(theImageId);
    const aSize = Module.getImageSize(aPath);

    Module.HEAPU32[jsPtrToNumber(theW) / 4] = aSize.x;
    Module.HEAPU32[jsPtrToNumber(theH) / 4] = aSize.y;
    Module.HEAPU32[jsPtrToNumber(theIsScl) / 4] = Module.isImageScalable(aPath) ? 1 : 0;
    return aSize.x != 0 || aSize.y != 0;
});

//! Render SVG image with specified resolution.
EM_JS(char *, jsRenderImage, (const char *theImageId, int theW, int theH), {
    return jsNumberToPtr(Module.renderImage(jsUTF8ToString(theImageId), theW, theH));
});
#endif

namespace ViewBase
{

    // =======================================================================
    // function : ScalablePixMap
    // =======================================================================
    ScalablePixMap::ScalablePixMap(const TCollection_AsciiString &theImagePath) : myImagePath(theImagePath)
    {
#if defined(__EMSCRIPTEN__)
        int isScalable = 0;
        jsGetImageInfo(theImagePath.ToCString(), &myBaseSize.x(), &myBaseSize.y(), &isScalable);
        myIsScalable = isScalable == 1;
#endif
        if (myBaseSize == Graphic3d_Vec2i(0))
        {
            Load();
        }
    }

    // =======================================================================
    // function : ~ScalablePixMap
    // =======================================================================
    ScalablePixMap::~ScalablePixMap()
    {
        release();
    }

    // =======================================================================
    // function : Clear
    // =======================================================================
    void ScalablePixMap::Clear()
    {
        release();
    }

    // =======================================================================
    // function : release
    // =======================================================================
    void ScalablePixMap::release() noexcept
    {
        Image_AlienPixMap::Clear();
#if defined(__EMSCRIPTEN__)
        if (myRawData != nullptr)
        {
            free((void *)myRawData);
            myRawData = nullptr;
        }
#endif
    }

// =======================================================================
// function : Load
// =======================================================================
#if defined(__EMSCRIPTEN__)
    bool ScalablePixMap::Load(const Graphic3d_Vec2i &theSize)
    {
        Clear();

        Graphic3d_Vec2i aSize = theSize != Graphic3d_Vec2i(0) ? theSize : myBaseSize;
        Standard_Character *anImgData = nullptr;
        if (aSize != Graphic3d_Vec2i(0))
        {
            anImgData = jsRenderImage(myImagePath.ToCString(), aSize.x(), aSize.y());
        }
        else
        {
            anImgData = emscripten_get_preloaded_image_data(myImagePath.ToCString(), &aSize.x(), &aSize.y());
        }
        if (anImgData == nullptr)
        {
            Message::SendFail() << "Error: image '" << myImagePath << "' is not preloaded/registered";
            return false;
        }

        Image_PixMap::InitWrapper(Image_Format_RGBA, (Standard_Byte *)anImgData, aSize.x(), aSize.y());
        myRawData = anImgData; // will be free() in destructor

        SetTopDown(false);
        Image_PixMap::FlipY(*this);
        return true;
    }
#else
    bool ScalablePixMap::Load(const Graphic3d_Vec2i &)
    {
        return Image_AlienPixMap::Load(nullptr, 0, myImagePath);
    }
#endif

}