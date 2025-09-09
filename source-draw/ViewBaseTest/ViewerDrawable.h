#ifndef ViewBASETEST_VIEWERDRAWABLE_HEADERFILE
#define ViewBASETEST_VIEWERDRAWABLE_HEADERFILE

#include <AIS_InteractiveObject.hxx>
#include <Draw_Color.hxx>
#include <Draw_Drawable3D.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>

namespace ViewBaseTest {

//! Interface for extending Draw_Drawable3D with 3D viewer callbacks.
class ViewerDrawable {
public:
    //! Display drawable presentation in 3D Viewer.
    virtual bool Display(const PrsMgr_DisplayStatus           theStatus,
                         const Handle(AIS_InteractiveObject)& theOldPrs = Handle(AIS_InteractiveObject)()) const = 0;

    //! Print drawable presentation information.
    virtual void PrintInfo(Standard_OStream& theStream) const = 0;

public:
    //! Draw array of segments.
    Standard_EXPORT static void DrawSegments(Draw_Display&                            theDisp,
                                             const Handle(Graphic3d_ArrayOfSegments)& theSegs,
                                             const Draw_Color&                        theColor);
};

}

#endif // ViewBASETEST_VIEWERDRAWABLE_HEADERFILE