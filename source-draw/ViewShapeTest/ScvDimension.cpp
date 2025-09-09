
#include <ViewShapeTest/ScvDimension.h>

#include <ViewDim/InteractiveAngleDimension.h>
#include <ViewDim/InteractiveLengthDimension.h>
#include <ViewDim/InteractiveRadiusDimension.h>
#include <ViewDim/InteractiveDimensionOwner.h>

#include <Viewer/WasmViewer.h>

#include <ViewerTest.hxx>

namespace ViewShapeTest {

// =======================================================================
// function : ScvDimension
// =======================================================================
ScvDimension::ScvDimension()
{
    AddVoidKey(this, "-length", "-len") = [](ScvDimension* theCmd) -> bool {
        theCmd->m_dimType = PrsDim_KOD_LENGTH;
        return true;
    };
    AddVoidKey(this, "-angle") = [](ScvDimension* theCmd) -> bool {
        theCmd->m_dimType = PrsDim_KOD_PLANEANGLE;
        return true;
    };
    AddVoidKey(this, "-radius") = [](ScvDimension* theCmd) -> bool {
        theCmd->m_dimType = PrsDim_KOD_RADIUS;
        return true;
    };
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvDimension::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_prsName.IsEmpty()) {
        ParsePrsName(Argument(theArgIter));
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
// function : ParsePrsName
// =======================================================================
void ScvDimension::ParsePrsName(Standard_CString theName)
{
    m_prsName = theName;
    if (ViewBase::IInteractiveObject::FindObjectById(m_prsObj, *m_context, m_prsName)) {
        m_dimType = m_prsObj->KindOfDimension();
    }

    m_args += TCollection_AsciiString(" ") + theName;
    ++m_nbArgs;
}

//! Build dimension object.
static Handle(PrsDim_Dimension) buildDimension(const Handle(PrsDim_Dimension)& theOld,
                                               const TCollection_AsciiString&  theName,
                                               PrsDim_KindOfDimension          theType)
{
    Handle(PrsDim_Dimension) aDimPrs;
    switch (theType) {
        case PrsDim_KOD_LENGTH: {
            aDimPrs = dynamic_cast<ViewDim::InteractiveLengthDimension*>(theOld.get());
            if (aDimPrs.IsNull()) {
                aDimPrs = new ViewDim::InteractiveLengthDimension(new ViewDim::InteractiveDimensionOwner(theName));
            }
            break;
        }
        case PrsDim_KOD_PLANEANGLE: {
            aDimPrs = dynamic_cast<ViewDim::InteractiveAngleDimension*>(theOld.get());
            if (aDimPrs.IsNull()) {
                aDimPrs = new ViewDim::InteractiveAngleDimension(new ViewDim::InteractiveDimensionOwner(theName));
            }
            break;
        }
        case PrsDim_KOD_RADIUS: {
            aDimPrs = dynamic_cast<ViewDim::InteractiveRadiusDimension*>(theOld.get());
            if (aDimPrs.IsNull()) {
                aDimPrs = new ViewDim::InteractiveRadiusDimension(new ViewDim::InteractiveDimensionOwner(theName));
            }
            break;
        }
        default: {
            return Handle(PrsDim_Dimension)();
        }
    }
    return aDimPrs;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvDimension::Perform()
{
    if (m_prsName.IsEmpty()) {
        return false;
    }

    m_prsObj = buildDimension(m_prsObj, m_prsName, m_dimType);

    ViewBase::IInteractiveObject* anIObj = dynamic_cast<ViewBase::IInteractiveObject*>(m_prsObj.get());
    if (anIObj == nullptr) {
        return false;
    }

    m_prsObj->Attributes()->SetupOwnDimensionAspect(ViewerTest::GetAISContext()->DefaultDrawer());

    Handle(Viewer::WasmViewer) aWebViewer = Handle(Viewer::WasmViewer)::DownCast(
        ViewerTest::CurrentEventManager());
    if (!aWebViewer.IsNull()) {
        anIObj->OnPixelScaleChange(m_prsObj, aWebViewer->DevicePixelRatio());
    }
    ViewerTest::Display(m_prsName, m_prsObj, false);

    const int aRes = myDI->Eval(m_args.ToCString());
    if (aRes != 0) {
        return false;
    }
    return true;
}

}

