#include <ViewBaseTest/ScvDrawText.h>

#include <ViewerTest.hxx>

namespace ViewBaseTest {

// =======================================================================
// function : ScvDrawText
// =======================================================================
ScvDrawText::ScvDrawText()
{
    AddOnOffNoKey(this, "-drawrect", "-drawrectangle", "-rect") = &m_toDrawRect;

    AddRangeKey(this, 1, IntegerLast(), "-attachmarkerscale") = &m_attachMarkerScale;

    AddRangeKey(this, 0.0, RealLast(), "-rectpadding", "-rectanglepadding") = &m_rectPadding;

    AddRangeKey(this, 0.0, RealLast(), "-pointpadding") = &m_pointPadding;

    AddKey(this, gp_XYZ(), "-attachpoint") = &m_attachPnt;

    AddVoidKey(this, "-noattachpoint") = [](ScvDrawText* theCmd, int&) -> bool {
        theCmd->m_noAttachPoint = true;
        return true;
    };
    AddKey(this, "", "-icon", "-image") = [](ScvDrawText* theCmd, int&, const char* theVal) -> bool {
        theCmd->m_toSetIconImage = true;
        theCmd->m_iconImage.Nullify();
        TCollection_AsciiString aPath(theVal);
        if (!aPath.IsEmpty()) {
            theCmd->m_iconImage = new ViewBase::ScalablePixMap(aPath);
        }
        return true;
    };
    AddKey(this, "", "-attachmarker") = &ScvDrawText::ParseAttachMarker;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvDrawText::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_prsObject.IsNull()) {
        ParseObjectName(Argument(theArgIter));
    } else {
        if (*Argument(theArgIter) == '\0') {
            m_args += TCollection_AsciiString(" \"\""); // preserve empty strings
        } else {
            m_args += TCollection_AsciiString(" {") + Argument(theArgIter) + "}";
        }
        ++m_nbArgs;
    }
    return true;
}

// =======================================================================
// function : ParseAttachMarker
// =======================================================================
bool ScvDrawText::ParseAttachMarker(Standard_CString theMarker)
{
    TCollection_AsciiString aTypeStr(theMarker);
    aTypeStr.LowerCase();
    m_attachMarkerType = Aspect_TOM_EMPTY;
    m_attachMarkerImage.Nullify();
    if (aTypeStr == "off" || aTypeStr == "0") {
        m_attachMarker = true;
    } else if (aTypeStr == "on" || aTypeStr == "1") {
        m_attachMarker     = true;
        m_attachMarkerType = Aspect_TOM_O_PLUS;
    } else if (ViewerTest::ParseMarkerType(aTypeStr.ToCString(), m_attachMarkerType, m_attachMarkerImage)) {
        m_attachMarker = true;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseObjectName
// =======================================================================
void ScvDrawText::ParseObjectName(Standard_CString theName)
{
    m_prsName = theName;
    if (!ViewBase::IInteractiveObject::FindObjectById(m_prsObject, *m_context, m_prsName)) {
        Handle(Prs3d_TextAspect) aTextStyle = ViewerTest::GetAISContext()->DefaultDrawer()->TextAspect();

        m_prsObject = new ViewBase::InteractiveTextLabel(new ViewBase::InteractiveTextLabelOwner(m_prsName),
                                                          aTextStyle);
        ViewerTest::Display(m_prsName, m_prsObject, false);
    }

    m_args += TCollection_AsciiString(" ") + theName;
    ++m_nbArgs;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvDrawText::Perform()
{
    const int aNbOwnArgs = SetParameters();

    static constexpr int cmdNbArgs2 = 2;
    if (m_nbArgs > cmdNbArgs2) {
        const int aRes = myDI->Eval(m_args.ToCString());
        if (aRes != 0) {
            return false;
        }
    } else if (aNbOwnArgs == 0) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }

    if (m_nbArgs == cmdNbArgs2) {
        m_prsObject->SetToUpdate();
    }
    ViewerTest::GetAISContext()->Redisplay(m_prsObject, false, true);
    return true;
}

// =======================================================================
// function : SetParameters
// =======================================================================
int ScvDrawText::SetParameters()
{
    int aNbOwnArgs = 0;
    if (m_toDrawRect != -1) {
        m_prsObject->SetDrawRectangle(m_toDrawRect == 1);
        ++aNbOwnArgs;
    }
    if (m_rectPadding >= 0.0) {
        m_prsObject->SetRectanglePadding(float(m_rectPadding));
        ++aNbOwnArgs;
    }
    if (m_attachMarker) {
        m_prsObject->SetAttachMarker(m_attachMarkerType);
        m_prsObject->AttachPointAspect(true)->SetMarkerImage(m_attachMarkerImage.IsNull()
                                                                 ? Handle(Graphic3d_MarkerImage)()
                                                                 : new Graphic3d_MarkerImage(m_attachMarkerImage));
        ++aNbOwnArgs;
    }
    if (m_attachMarkerScale != -1) {
        m_prsObject->AttachPointAspect(false)->SetMarkerScale((float)m_attachMarkerScale);
        m_prsObject->AttachPointAspect(true)->SetMarkerScale((float)m_attachMarkerScale);
        ++aNbOwnArgs;
    }
    if (m_pointPadding >= 0.0) {
        m_prsObject->SetPointPadding(float(m_pointPadding));
        ++aNbOwnArgs;
    }
    if (!Precision::IsInfinite(m_attachPnt.X())) {
        m_prsObject->SetAttachPoint(gp_Pnt(m_attachPnt));
        ++aNbOwnArgs;
    }
    if (m_noAttachPoint) {
        m_prsObject->SetAttachPoint(gp_Pnt(RealLast(), RealLast(), RealLast()));
        ++aNbOwnArgs;
    }
    if (m_toSetIconImage) {
        Handle(ViewBase::ScalableTexture2D) aTexture = new ViewBase::ScalableTexture2D(m_iconImage);
        m_prsObject->SetIcon(aTexture);
        ++aNbOwnArgs;
    }
    return aNbOwnArgs;
}

}

