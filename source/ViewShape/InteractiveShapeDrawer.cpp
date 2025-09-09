#include <ViewShape/InteractiveShapeDrawer.h>

namespace ViewShape
{

    // =======================================================================
    // function : InteractiveShapeDrawer
    // =======================================================================
    InteractiveShapeDrawer::InteractiveShapeDrawer()
    {
        myTransparentMat = new XCAFDoc_VisMaterial();
        mySelectedMat = new XCAFDoc_VisMaterial();
        SetSemitransparentColor(Quantity_ColorRGBA(0.5f, 1.0f, 1.0f, 0.0f));
        SetSelectedColor(Quantity_ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f));
    }

    // =======================================================================
    // function : ~InteractiveShapeDrawer
    // =======================================================================
    InteractiveShapeDrawer::~InteractiveShapeDrawer()
    {
        //
    }

    // =======================================================================
    // function : SetSemiTransparentColor
    // =======================================================================
    void InteractiveShapeDrawer::SetSemitransparentColor(const Quantity_ColorRGBA &theColor)
    {
        XCAFDoc_VisMaterialPBR aPbrMar;
        aPbrMar.IsDefined = true;
        aPbrMar.BaseColor = theColor;
        aPbrMar.Metallic = 0.0f;
        aPbrMar.Roughness = 1.0f;
        aPbrMar.EmissiveFactor.SetValues(0.3f, 0.5f, 0.5f);
        myTransparentMat->SetPbrMaterial(aPbrMar);
    }

    // =======================================================================
    // function : SetSelectedColor
    // =======================================================================
    void InteractiveShapeDrawer::SetSelectedColor(const Quantity_ColorRGBA &theColor)
    {
        XCAFDoc_VisMaterialPBR aPbrMar;
        aPbrMar.IsDefined = true;
        aPbrMar.BaseColor = theColor;
        aPbrMar.Metallic = 1.0f;
        aPbrMar.Roughness = 1.0f;
        mySelectedMat->SetPbrMaterial(aPbrMar);
    }

} // namespace