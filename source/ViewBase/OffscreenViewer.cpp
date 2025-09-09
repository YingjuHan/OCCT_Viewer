#ifdef _WIN32
#include <windows.h>
#endif

#include "OffscreenViewer.h"

#include <ViewBase/BaseDefaultDrawer.h>
#include <ViewBase/BaseViewer.h>
#include <ViewBase/BaseSelection.h>

#include <AIS_InteractiveContext.hxx>
#include <Image_AlienPixMap.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <Message.hxx>
#include <OSD.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#include <array>

#ifdef _WIN32
#include <WNT_WClass.hxx>
#include <WNT_Window.hxx>
#elif defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#include <Aspect_NeutralWindow.hxx>
#else
#include <Xw_Window.hxx>
#include <X11/Xlib.h>
#endif

namespace ViewBase
{
    OffscreenViewer::OffscreenViewer() : myIsInitialized(false)
    {
        //
    }

    Handle(Aspect_Window) OffscreenViewer::createWindow(const Handle(Aspect_DisplayConnection) & theDisp,
                                                        const Graphic3d_Vec2i &theWinSize)
    {
        (void)theDisp;

        // create offscreen window
        const TCollection_AsciiString aWinName("OCCT offscreen window");
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
        Handle(Aspect_NeutralWindow) aWindow = new Aspect_NeutralWindow();
        aWindow->SetSize(theWinSize.x(), theWinSize.y());
#elif defined(_WIN32)
        static const TCollection_AsciiString aClassName("OffscreenClass");
        // empty callback!
        static const Handle(WNT_WClass) aWinClass = new WNT_WClass(aClassName.ToCString(), NULL, 0);

        Handle(WNT_Window) aWindow = new WNT_Window(aWinName.ToCString(),
                                                    aWinClass,
                                                    0x80000000L, // WS_POPUP
                                                    DEF_WIN_SIZE,
                                                    DEF_WIN_SIZE,
                                                    DEF_WIN_SIZE,
                                                    DEF_WIN_SIZE,
                                                    Quantity_NOC_BLACK);
        aWindow->SetVirtual(true);
        aWindow->SetPos(0, 0, theWinSize.x(), theWinSize.y());
#elif defined(__APPLE__)
        Handle(Cocoa_Window) aWindow =
            new Cocoa_Window(aWinName.ToCString(), DEF_WIN_SIZE, DEF_WIN_SIZE, theWinSize.x(), theWinSize.y());
#else
        Handle(Xw_Window) aWindow =
            new Xw_Window(theDisp, aWinName.ToCString(), DEF_WIN_SIZE, DEF_WIN_SIZE, theWinSize.x(), theWinSize.y());
#endif
        aWindow->SetVirtual(true);
        return aWindow;
    }

    void OffscreenViewer::createView(const Handle(Aspect_Window) & theWindow)
    {
        // create 3D view from offscreen window
        myView = new V3d_View(myViewer);
        myView->SetImmediateUpdate(false);
        // @c myView->SetBackgroundColor (Quantity_NOC_BLACK);
        myView->SetBackgroundColor(Quantity_NOC_GRAY10);
        // @c myView->Camera()->SetProjectionType (Graphic3d_Camera::Projection_Perspective);
        myView->Camera()->SetFOV2d(360.0f);
        myView->Camera()->SetFOVy(45.0f);
        myView->SetWindow(theWindow);

        const Handle(Graphic3d_GraphicDriver) &aDriver = myView->Viewer()->Driver();

        Graphic3d_TypeOfShadingModel aShadeModel = aDriver->InquireLimit(Graphic3d_TypeOfLimit_HasPBR) == 1
                                                       ? Graphic3d_TOSM_PBR
                                                       : Graphic3d_TOSM_FRAGMENT;
        myView->SetShadingModel(aShadeModel);

        const int anMaxMsaa = aDriver->InquireLimit(Graphic3d_TypeOfLimit_MaxMsaa);
        Graphic3d_RenderingParams &aRendParams = myView->ChangeRenderingParams();
        aRendParams.NbMsaaSamples = anMaxMsaa >= DEF_MSAA ? DEF_MSAA : 0;

        // setup default IBL cubemap
        ViewBase::BaseViewer::SetupCubemapIBL(myView);
    }

    bool OffscreenViewer::InitOffscreenViewer(const Handle(Graphic3d_GraphicDriver) & theDriver,
                                              const Graphic3d_Vec2i &theWinSize)
    {
        myIsInitialized = false;
        try
        {
            OCC_CATCH_SIGNALS

            // create graphic driver
            Handle(Graphic3d_GraphicDriver) aDriver = theDriver;

            /* @code Handle(Aspect_DisplayConnection) aDispConn = new Aspect_DisplayConnection();
            Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver(aDispConn, true);
            aDriver->ChangeOptions().ffpEnable         = false;
            aDriver->ChangeOptions().swapInterval      = 0; @endcode */
            // @c aDriver->ChangeOptions().contextCompatible = false;

            // create viewer and AIS context
            myViewer = new V3d_Viewer(aDriver);
            myContext = new AIS_InteractiveContext(myViewer);

            Handle(ViewBase::BaseDefaultDrawer) aDefDrawer = new ViewBase::BaseDefaultDrawer();
            myContext->SetDefaultDrawer(aDefDrawer);
            for (int aStyleIter = Prs3d_TypeOfHighlight_Selected; aStyleIter < Prs3d_TypeOfHighlight_NB; ++aStyleIter)
            {
                myContext->HighlightStyle((Prs3d_TypeOfHighlight)aStyleIter)->SetLink(aDefDrawer);
            }
            myContext->SetSelection(new ViewBase::BaseSelection());

            // light sources setup
            ViewBase::BaseViewer::SetupLightSources(myViewer);
            // @c myViewer->SetDefaultLights();
            // @c myViewer->SetLightOn();
            {
                Graphic3d_RenderingParams aRendParams = myViewer->DefaultRenderingParams();
                // @c aRendParams.NbMsaaSamples = DEF_MSAA;
                aRendParams.IsShadowEnabled = false;
                aRendParams.BaseResolution = DEF_FONT_RESOLUTION;
                aRendParams.Resolution = DEF_FONT_RESOLUTION;
                myViewer->SetDefaultRenderingParams(aRendParams);

                // disable immediate layers for sync with WasmViewer::setupRenderingParams()
                const std::array<Graphic3d_ZLayerId, 2> aTopLayers = {Graphic3d_ZLayerId_Top, Graphic3d_ZLayerId_Topmost};
                for (Graphic3d_ZLayerId aLayerIter : aTopLayers)
                {
                    Graphic3d_ZLayerSettings aLayer = myViewer->ZLayerSettings(aLayerIter);
                    aLayer.SetImmediate(false);
                    myViewer->SetZLayerSettings(aLayerIter, aLayer);
                }
            }

            // create offscreen window
            Handle(Aspect_Window) aWindow = createWindow(aDriver->GetDisplayConnection(), theWinSize);

            // create 3D view from offscreen window
            createView(aWindow);

            myIsInitialized = true;

            static bool toDumpFirstGl = true;
            if (toDumpFirstGl)
            {
                toDumpFirstGl = false;
                DumpGlInfo();
            }
        }
        catch (const Standard_Failure &theErr)
        {
            Message::SendFail() << "Offscreen Viewer creation FAILED:\n"
                                << theErr;
            myIsInitialized = false;
            return false;
        }
        return true;
    }

    void OffscreenViewer::DumpGlInfo()
    {
        TColStd_IndexedDataMapOfStringString aGlCapsDict;
        myView->DiagnosticInformation(aGlCapsDict, Graphic3d_DiagnosticInfo_Basic);

        // beautify output
        {
            TCollection_AsciiString *aGlVer = aGlCapsDict.ChangeSeek("GLversion");
            TCollection_AsciiString *aGlslVer = aGlCapsDict.ChangeSeek("GLSLversion");
            if (aGlVer != NULL && aGlslVer != NULL)
            {
                *aGlVer = *aGlVer + " [GLSL: " + *aGlslVer + "]";
                aGlslVer->Clear();
            }
        }

        TCollection_AsciiString anInfo = "Offscreen OpenGL info:\n";
        for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aGlCapsDict); aValueIter.More(); aValueIter.Next())
        {
            if (!aValueIter.Value().IsEmpty())
            {
                anInfo += TCollection_AsciiString("  ") + aValueIter.Key() + ": " + aValueIter.Value() + "\n";
            }
        }
        Message::SendInfo(anInfo);
    }

}