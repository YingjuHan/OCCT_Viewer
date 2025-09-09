
#ifndef _OFFSCREENVIEWER_HEADERFILE
#define _OFFSCREENVIEWER_HEADERFILE

#include <AIS_InteractiveContext.hxx>

namespace ViewBase
{
    //! Auxiliary class to initialize an offscreen instance of OCCT 3D Viewer for generating screenshots.
    class OffscreenViewer : public Standard_Transient
    {
    public:
        //! Empty constructor.
        Standard_EXPORT OffscreenViewer();

        //! Return viewer initialization state.
        bool IsInitialized() const { return myIsInitialized; }

        //! Initialize offscreen viewer.
        //! @param[in] theDriver graphics driver
        //! @param[in] theWinSize view dimensions
        //! @return FALSE in case of initialization error
        Standard_EXPORT bool InitOffscreenViewer(const Handle(Graphic3d_GraphicDriver) & theDriver,
                                                 const Graphic3d_Vec2i &theWinSize);

        //! Return view instance.
        const Handle(V3d_View) & View() const { return myView; }

        //! Return AIS context.
        const Handle(AIS_InteractiveContext) & Context() const { return myContext; }

        //! Print information about graphics context.
        Standard_EXPORT void DumpGlInfo();

    private:
        //! Create offscreen window.
        Standard_EXPORT Handle(Aspect_Window) createWindow(const Handle(Aspect_DisplayConnection) & theDisp,
                                                           const Graphic3d_Vec2i &theWinSize);

        //! Create V3d_View instance.
        Standard_EXPORT void createView(const Handle(Aspect_Window) & theWindow);

    private:
        //! Default text resolution.
        static constexpr int DEF_FONT_RESOLUTION = 96;
        //! Offscreen window size.
        static constexpr int DEF_WIN_SIZE = 64;
        //! Default number of MSAA samples.
        static constexpr int DEF_MSAA = 4;

    private:
        Handle(V3d_Viewer) myViewer;
        Handle(V3d_View) myView;
        Handle(AIS_InteractiveContext) myContext;
        bool myIsInitialized;
    };

}

#endif