#include <ViewCaeTest/ScvSphericalAxes.h>

#include <Message.hxx>
#include <ViewerTest.hxx>

namespace ViewCaeTest {

// =======================================================================
// function : ScvSphericalAxes
// =======================================================================
ScvSphericalAxes::ScvSphericalAxes()
{
    AddKey(this, gp_XYZ(), "-pos") = [](ScvSphericalAxes* theCmd, int&, gp_XYZ theVal) -> bool {
        theCmd->m_position = theVal;
        return true;
    };
    AddKey(this, double(), "-scale") = [](ScvSphericalAxes* theCmd, int&, double theVal) -> bool {
        if (theVal <= Precision::Confusion()) {
            theCmd->Interpretor() << "Error: negative or zero scale.\n";
            return false;
        }
        theCmd->m_scale = theVal;
        return true;
    };
    AddKey(this, "", "-orientation") = &ScvSphericalAxes::parseOrientation;
    AddVoidKey(this, "-angular")     = [](ScvSphericalAxes* theCmd, int& theArgIter) -> bool {
        return theCmd->parseAngularAxes(theCmd->m_angularParams, theArgIter);
    };
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvSphericalAxes::ParseAllArguments()
{
    if (!ViewBaseTest::ViewerCommand::ParseAllArguments()) {
        return false;
    }
    if (m_prsName.IsEmpty() || m_axesObj.IsNull()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : parseOrientation
// =======================================================================
bool ScvSphericalAxes::parseOrientation(const char* theAxis)
{
    TCollection_AsciiString aStrAxis(theAxis);
    aStrAxis.LowerCase();
    m_orientation = ViewCae::AxisTypeConverter::GetValue(aStrAxis.ToCString());
    return m_orientation != AxisType::Unknown;
}

// =======================================================================
// function : parseAngularAxes
// =======================================================================
bool ScvSphericalAxes::parseAngularAxes(std::vector<AngularAxesParams>& theParams, int& theArgIter)
{
    theParams.clear();
    if (theArgIter + 6 >= NbArguments()) {
        return false;
    }

    for (int i = 0; i < 2; i++) {
        AngularAxesParams anAxis;
        anAxis.Label = Argument(theArgIter + (i * 3) + 1);

        TCollection_AsciiString aStrOrientation(Argument(theArgIter + (i * 3) + 2));
        aStrOrientation.LowerCase();
        anAxis.Orientation = ViewCae::ArrowOrientationConverter::GetValue(aStrOrientation.ToCString());
        if (anAxis.Orientation == ArrowOrientation::Unknown) {
            theParams.clear();
            return false;
        }

        TCollection_AsciiString aStrAxis(Argument(theArgIter + (i * 3) + 3));
        aStrAxis.LowerCase();
        anAxis.ZeroAxis = ViewCae::AxisTypeConverter::GetValue(aStrAxis.ToCString());
        if (anAxis.ZeroAxis == AxisType::Unknown) {
            theParams.clear();
            return false;
        }

        theParams.push_back(anAxis);
    }

    theArgIter += 6;
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvSphericalAxes::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);
        if (!ViewBase::IInteractiveObject::FindObjectById(m_axesObj, *m_context, m_prsName)) {
            m_axesObj = new ViewCae::InteractiveSphericalAxes(new ViewBase::InteractiveOwner(m_prsName));
        }
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvSphericalAxes::Perform()
{
    // Apply position and scale from local arguments.
    m_axesObj->SetPosition(m_position);
    m_axesObj->SetScale(m_scale);

    // Apply orientation of longitude/latitude circles
    if (m_orientation > AxisType::Unknown) {
        m_axesObj->SetOrientation(m_orientation);
    }

    // Apply custom angular axes options
    if (!m_angularParams.empty()) {
        for (size_t i = 0; i < 2; i++) {
            const AxisType anAngularAxis = static_cast<AxisType>(i);
            m_axesObj->SetAngularArrowLabel(anAngularAxis, m_angularParams[i].Label);
            m_axesObj->SetAngularArrowDir(anAngularAxis, m_angularParams[i].Orientation);
            m_axesObj->SetAngularArrowAxis(anAngularAxis, m_angularParams[i].ZeroAxis);
        }
    }

    // Display
    ViewerTest::Display(m_prsName, m_axesObj, false);

    return true;
}

}

