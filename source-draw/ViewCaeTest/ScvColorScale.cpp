
#include <ViewCaeTest/ScvColorScale.h>

#include <DrawBase/DrawPrinters.h>

#include <ViewCae/ColorScaleTools.h>
#include <ViewCae/InteractiveColorScaleOwner.h>

#include <Viewer/WasmViewer.h>

#include <ViewerTest.hxx>

namespace ViewCaeTest {

// =======================================================================
// function : ScvColorScale
// =======================================================================
ScvColorScale::ScvColorScale()
{
    AddRangeKey(this, 1, IntegerLast(), "-nbintervals") = &m_nbIntervals;

    AddVoidKey(this, "-attachprop") = &ScvColorScale::ParseAttachProperty;

    AddOnOffNoKey(this, "-toskipemptylabels", "-skipemptylabels", "-skipempty") = &m_toSkipEmptyLabels;

    AddOnOffNoKey(this, "-horizontal") = &m_isHorizontal;

    AddOnOffNoKey(this, "-logarithmic", "-logscale", "-log") = &m_isLogarithmic;

    AddVoidKey(this, "-colors") = &ScvColorScale::ParseColorList;

    AddKey(this, "", "-attach") = [](ScvColorScale* theCmd, int&, const char* theVal) -> bool {
        TCollection_AsciiString aMeshName(theVal);
        if (!ViewBase::IInteractiveObject::FindObjectById(theCmd->m_mesh, *theCmd->m_context, aMeshName)) {
            theCmd->Interpretor() << "Syntax error: mesh " << aMeshName << " not found\n";
            return false;
        }
        return true;
    };

    AddKey(this, Quantity_ColorRGBA(), "-coloroutofrange", "-colorout") =
        [](ScvColorScale* theCmd, int&, Quantity_ColorRGBA theVal) -> bool {
        theCmd->m_colorOutOfRange    = theVal;
        theCmd->m_hasColorOutOfRange = true;
        return true;
    };
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvColorScale::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_colorScale.IsNull()) {
        ParseColorscaleName(Argument(theArgIter));
    } else {
        if (*Argument(theArgIter) == '\0') {
            m_args += TCollection_AsciiString(" \"\""); // preserve empty strings
        } else {
            m_args += TCollection_AsciiString(" ") + Argument(theArgIter);
        }
        ++m_nbArgs;
    }
    return true;
}

// =======================================================================
// function : ParseColorList
// =======================================================================
bool ScvColorScale::ParseColorList(int& theArgIter)
{
    Quantity_ColorRGBA aColorRgba;
    for (;;) {
        if (!ParseRgbaColorIterator(myNbArgs, myArgVec, theArgIter, aColorRgba)) {
            return !m_customColors.IsEmpty();
        }

        m_customColors.Append(aColorRgba);
    }
}

// =======================================================================
// function : ParseAttachProperty
// =======================================================================
bool ScvColorScale::ParseAttachProperty(int& theArgIter)
{
    static constexpr int attachPropNbArgs = 3;
    if (theArgIter + attachPropNbArgs >= myNbArgs) {
        return false;
    }

    TCollection_AsciiString aMeshName(Argument(++theArgIter));
    if (!ViewBase::IInteractiveObject::FindObjectById(m_mesh, *m_context, aMeshName)) {
        *myDI << "Syntax error: mesh " << aMeshName << " not found\n";
        return false;
    }

    TCollection_AsciiString                  aPropId = Argument(++theArgIter);
    const Handle(RWCae::MeshNVecProperty)& aProp   = m_mesh->GetMeshData()->GetPropertyById(aPropId);
    if (aProp.IsNull()) {
        *myDI << "Syntax error: property " << aPropId << " not found\n";
        return false;
    }
    int aCompIndex = -1;
    if (!Draw::ParseInteger(Argument(++theArgIter), aCompIndex) || !aProp->EnabledComponents().Contains(aCompIndex)) {
        *myDI << "Syntax error: property component " << aCompIndex << " not found\n";
        return false;
    }
    m_prop = RWCae::MeshPropertyComponent(aProp, aCompIndex);
    return true;
}

// =======================================================================
// function : ParseColorscaleName
// =======================================================================
void ScvColorScale::ParseColorscaleName(Standard_CString theName)
{
    m_prsName = theName;
    if (!ViewBase::IInteractiveObject::FindObjectById(m_colorScale, *m_context, m_prsName)) {
        m_colorScale = new ViewCae::InteractiveColorScale(new ViewCae::InteractiveColorScaleOwner(m_prsName));
        m_colorScale->SetZLayer(Graphic3d_ZLayerId_TopOSD);
        Handle(Viewer::WasmViewer) aWebViewer = Handle(Viewer::WasmViewer)::DownCast(
            ViewerTest::CurrentEventManager());
        if (!aWebViewer.IsNull()) {
            m_colorScale->OnPixelScaleChange(m_colorScale, aWebViewer->DevicePixelRatio());
        }
        m_context->SetTransformPersistence(m_colorScale,
                                           new Graphic3d_TransformPers(Graphic3d_TMF_2d, Aspect_TOTP_LEFT));

        ViewerTest::Display(m_prsName, m_colorScale, false);
    }

    m_args += TCollection_AsciiString(" ") + theName;
    ++m_nbArgs;
}

// =======================================================================
// function : AttachColorScale
// =======================================================================
void ScvColorScale::AttachColorScale()
{
    m_colorScale->Attach(m_mesh, m_prop, [](const Handle(ViewCae::MeshColorMapper)& theColorMapper) {
        if (Handle(ViewCae::MeshColorScaleMapper) aColorScaleMapper = Handle(ViewCae::MeshColorScaleMapper)::DownCast(
                theColorMapper)) {
            ViewCae::ColorScaleTools::UpdateColorScales(ViewerTest::GetAISContext(),
                                                         aColorScaleMapper->ColorScaleObject());
        } else {
            ViewCae::ColorScaleTools::UpdateColorScales(ViewerTest::GetAISContext());
        }
    });
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvColorScale::Perform()
{
    int aNbOwnArgs = 0;
    if (m_nbIntervals > 0) {
        ++aNbOwnArgs;
        m_colorScale->SetNumberOfIntervals(m_nbIntervals);
    }
    if (m_toSkipEmptyLabels != -1) {
        ++aNbOwnArgs;
        m_colorScale->SetSkipEmptyLabels(m_toSkipEmptyLabels == 1);
    }
    if (m_isHorizontal != -1) {
        ++aNbOwnArgs;
        m_colorScale->SetHorizontal(m_isHorizontal == 1);
    }
    if (m_isLogarithmic != -1) {
        ++aNbOwnArgs;
        m_colorScale->SetLogarithmic(m_isLogarithmic == 1);
    }
    if (m_hasColorOutOfRange) {
        ++aNbOwnArgs;
        m_colorScale->SetOutOfRangeColor(m_colorOutOfRange);
    }
    if (!m_mesh.IsNull()) {
        ++aNbOwnArgs;
        AttachColorScale();
    }
    if (!m_customColors.IsEmpty() && !SetCustomColors(*myDI)) {
        return false;
    }

    static constexpr int cmdNbArgs2 = 2;
    if (m_nbArgs > cmdNbArgs2) {
        const int aRes = myDI->Eval(m_args.ToCString());
        if (aRes != 0) {
            return false;
        }
    }

    m_colorScale->UpdateColorMap(true);

    if (m_nbArgs == cmdNbArgs2) {
        m_colorScale->SetToUpdate();
    }
    if (aNbOwnArgs == 0 && m_nbArgs <= cmdNbArgs2) {
        PrintParams(*myDI);
    }
    return true;
}

// =======================================================================
// function : SetCustomColors
// =======================================================================
bool ScvColorScale::SetCustomColors(Draw_Interpretor& theDI)
{
    if (m_customColors.IsEmpty()) {
        return true;
    }
    if (m_customColors.Length() != m_colorScale->GetNumberOfIntervals()) {
        theDI << "Error: not enough arguments! You should provide color names or RGB color "
                 "values for every interval of the "
              << m_colorScale->GetNumberOfIntervals() << " intervals";
        return false;
    }

    m_colorScale->SetCustomColors(m_customColors);
    m_colorScale->SetColorType(Aspect_TOCSD_USER);
    return true;
}

// =======================================================================
// function : PrintParams
// =======================================================================
void ScvColorScale::PrintParams(Draw_Interpretor& theDI) const
{
    using namespace DrawBase::DrawPrinters;

    theDI << "title:           " << m_colorScale->GetTitle() << "\n";
    theDI << "height:          " << m_colorScale->GetBaseHeight() << "\n";
    theDI << "breadth:         " << m_colorScale->GetBaseBreadth() << "\n";
    theDI << "maxSize:         " << m_colorScale->GetMaxBaseHeight() << "\n";
    theDI << "nbIntervals:     " << m_colorScale->GetNumberOfIntervals() << "\n";
    theDI << "labelSize:       " << m_colorScale->GetTextHeight() << "\n";
    theDI << "isSmooth:        " << (m_colorScale->IsSmoothTransition() ? "1" : "0") << "\n";
    theDI << "isLogarithmic:   " << (m_colorScale->IsLogarithmic() ? "1" : "0") << "\n";
    theDI << "isLabelAtBorder: " << (m_colorScale->IsLabelAtBorder() ? "1" : "0") << "\n";
    theDI << "isReversed:      " << (m_colorScale->IsReversed() ? "1" : "0") << "\n";
    theDI << "isCustomColors:  " << (m_colorScale->GetColorType() == Aspect_TOCSD_USER ? "1" : "0") << "\n";
    theDI << "skipEmptyLabels: " << (m_colorScale->ToSkipEmptyLabels() ? "1" : "0") << "\n";
    theDI << "defaultRange:    " << m_colorScale->GetDefaultRangeMin() << " " << m_colorScale->GetDefaultRangeMax()
          << "\n";
    theDI << "displayRange:    " << m_colorScale->GetMin() << " " << m_colorScale->GetMax() << "\n";
    theDI << "selectedRange:   " << m_colorScale->GetSelectedRangeFrom() << " " << m_colorScale->GetSelectedRangeTo()
          << "\n";
    theDI << "outOfRangeColor: " << m_colorScale->GetOutOfRangeColor() << "\n";
    if (m_colorScale->GetColorType() == Aspect_TOCSD_USER) {
        theDI << "customColors:    ";
        for (const Quantity_Color& anIter : m_colorScale->GetColors()) {
            theDI << anIter << " ";
        }
        theDI << "\ncustomLabels:    ";
        for (const TCollection_ExtendedString& anIter : m_colorScale->Labels()) {
            theDI << "\"" << anIter << "\" ";
        }
        theDI << "\n";
    } else {
        Quantity_Color aMinColor;
        Quantity_Color aMaxColor;
        m_colorScale->ColorRange(aMinColor, aMaxColor);
        theDI << "minColor:        " << aMinColor << "\nmaxColor:        " << aMaxColor << "\n";

        theDI << "colors:          ";
        NCollection_Sequence<Quantity_ColorRGBA> aColors;
        for (int anIntervalIter = 1; anIntervalIter <= m_colorScale->GetNumberOfIntervals(); ++anIntervalIter) {
            if (m_colorScale->IsReversed()) {
                aColors.Prepend(m_colorScale->GetIntervalColor(anIntervalIter));
            } else {
                aColors.Append(m_colorScale->GetIntervalColor(anIntervalIter));
            }
        }
        for (const Quantity_ColorRGBA& anIter : aColors) {
            theDI << anIter << " ";
        }
        theDI << "\n";
    }
}

}
