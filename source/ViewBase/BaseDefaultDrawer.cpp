#include <ViewBase/BaseDefaultDrawer.h>

#include <ViewBase/UnitManager.h>

#include <Font_NameOfFont.hxx>
#include <Prs3d_DatumAspect.hxx>
#include <Prs3d_DimensionAspect.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PlaneAspect.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Prs3d_TextAspect.hxx>

namespace ViewBase
{
    BaseDefaultDrawer::BaseDefaultDrawer()
    {
        myZLayer = Graphic3d_ZLayerId_Default;
        myDispMode = 0;

        SetupOwnDefaults();

        // we preload only sans-serif files
        // 我们只预加载无衬线字体文件
        // @c TextAspect()->SetFont(Font_NOF_SERIF);
        TextAspect()->SetFont(Font_NOF_SANS_SERIF);
        TextAspect()->Aspect()->SetTextFontAspect(Font_FontAspect_Regular);

        DimensionAspect()->TextAspect()->SetFont(Font_NOF_SANS_SERIF);
        DimensionAspect()->MakeText3d(false);
        DimensionAspect()->MakeTextShaded(false);
        DimensionAspect()->ArrowAspect()->SetZoomable(false);
        DimensionAspect()->ArrowAspect()->SetLength(m_defDimArrowLen);

        UpdateUnits();

        SetFaceBoundaryUpperContinuity(GeomAbs_G2);

        Graphic3d_MaterialAspect aMat(Graphic3d_NameOfMaterial_Brass);
        ShadingAspect()->SetMaterial(aMat);
        ShadingAspect()->Aspect()->SetEdgeColor(Quantity_NOC_GRAY60);

        Handle(Prs3d_LineAspect) aLineAspect = HiddenLineAspect();
        aLineAspect->SetColor(Quantity_NOC_GRAY20);
        aLineAspect->SetWidth(1.0);
        aLineAspect->SetTypeOfLine(Aspect_TOL_DASH);

        // customizing the drawer for trihedrons and planes
        // 为三面角和面定制绘制器
        Handle(Prs3d_DatumAspect) aTrihAspect = DatumAspect();
        const double aLength = 100.0;
        aTrihAspect->SetAxisLength(aLength, aLength, aLength);
        const Quantity_Color aTrihColor = Quantity_NOC_LIGHTSTEELBLUE4;
        aTrihAspect->LineAspect(Prs3d_DatumParts_XAxis)->SetColor(aTrihColor);
        aTrihAspect->LineAspect(Prs3d_DatumParts_YAxis)->SetColor(aTrihColor);
        aTrihAspect->LineAspect(Prs3d_DatumParts_ZAxis)->SetColor(aTrihColor);

        Handle(Prs3d_PlaneAspect) aPlaneAspect = PlaneAspect();
        const double aPlaneLength = 200.0;
        aPlaneAspect->SetPlaneLength(aPlaneLength, aPlaneLength);
        aPlaneAspect->EdgesAspect()->SetColor(Quantity_NOC_SKYBLUE);

        const Quantity_ColorRGBA aDefColor(0.243137f, 0.243137f, 0.243137f, 1.0f);

        XCAFDoc_VisMaterialPBR aPbrMar;
        aPbrMar.IsDefined = true;
        aPbrMar.BaseColor = aDefColor;
        myVisMaterial = new XCAFDoc_VisMaterial();
        myVisMaterial->SetPbrMaterial(aPbrMar);
    }

    BaseDefaultDrawer::~BaseDefaultDrawer()
    {
        //
    }

    void BaseDefaultDrawer::UpdateUnits()
    {
        const Handle(ViewBase::UnitManager) &aMgr = ViewBase::UnitManager::Instance();

        const TCollection_AsciiString &aSysLen = aMgr->GetSystemUnit(QuantityType::Length);
        const TCollection_AsciiString &aDispLen = aMgr->GetDisplayUnit(QuantityType::Length);
        SetDimLengthModelUnits(aSysLen);
        SetDimLengthDisplayUnits(aDispLen);

        static constexpr double defDimSize = 0.006;
        const double aDimSize = aMgr->SystemFromSI(QuantityType::Length, defDimSize);
        DimensionAspect()->SetExtensionSize(aDimSize);
        DimensionAspect()->SetArrowTailSize(aDimSize);
        if (DimensionAspect()->ArrowAspect()->IsZoomable())
        {
            DimensionAspect()->ArrowAspect()->SetLength(aDimSize);
        }

        // static constexpr double defDimFit = 0.001;
        // const double aDimFit = aMgr->SystemFromSI(QuantityType::Length, defDimFit);
        // DimensionAspect()->SetNonZoomableFitScale(aDimFit);
    }

} // namespace