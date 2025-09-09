#include <ViewBase/InteractiveViewCube.h>

#include <ViewBase/InteractiveViewCubeOwner.h>

#include <AIS_InteractiveContext.hxx>
#include <Prs3d_DatumAspect.hxx>

namespace ViewBase
{

    // ================================================================
    // Function : InteractiveViewCube
    // ================================================================
    InteractiveViewCube::InteractiveViewCube(const Handle(ViewBase::InteractiveViewCubeOwner) & theNewId,
                                             const Handle(Prs3d_TextAspect) & theTextStyle)
    {
        static constexpr int defSize = 60;
        static constexpr double defPadding = 5.0;
        static constexpr double cornerGap = 1.5;

        myOwner = theNewId;
        myTransformPersistence->SetCorner2d(Aspect_TOTP_RIGHT_LOWER);
        myTransformPersistence->SetOffset2d(Graphic3d_Vec2i(int(defSize * cornerGap)));

        myDrawer->SetDatumAspect(new Prs3d_DatumAspect());

        SetAxesPadding(defPadding);

        myDrawer->TextAspect()->Aspect()->SetTextFontAspect(Font_FontAspect_Bold);
        myDrawer->SetDatumAspect(new Prs3d_DatumAspect());
        myDrawer->DatumAspect()->SetTextAspect(theTextStyle);
        myDrawer->DatumAspect()->ShadingAspect(Prs3d_DP_XAxis)->SetColor(Quantity_NOC_RED);
        myDrawer->DatumAspect()->ShadingAspect(Prs3d_DP_YAxis)->SetColor(Quantity_NOC_GREEN);
        myDrawer->DatumAspect()->ShadingAspect(Prs3d_DP_ZAxis)->SetColor(Quantity_NOC_BLUE1);

        SetFixedAnimationLoop(false);
        SetAutoStartAnimation(true);

        SetSize(defSize);
    }

    // ================================================================
    // Function : ~InteractiveViewCube
    // ================================================================
    InteractiveViewCube::~InteractiveViewCube()
    {
        //
    }

}