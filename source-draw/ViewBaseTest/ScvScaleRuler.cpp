#include <ViewBaseTest/ScvScaleRuler.h>

#include <ViewBase/InteractiveScaleRuler.h>

#include <Viewer/WasmViewer.h>

#include <ViewerTest.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>

Standard_IMPORT ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();

namespace ViewBaseTest {

// =======================================================================
// function : ScvScaleRuler
// =======================================================================
ScvScaleRuler::ScvScaleRuler()
{
    AddRangeKey(this, gp_XY(RealEpsilon(), RealEpsilon()), gp_XY(RealLast(), RealLast()), "-size") = &m_baseSize;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvScaleRuler::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }
    if (m_name.IsEmpty()) {
        *myDI << "Syntax error: wrong number of parameters";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvScaleRuler::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_name.IsEmpty()) {
        m_name = Argument(theArgIter);
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvScaleRuler::Perform()
{
    Handle(ViewBase::InteractiveScaleRuler) aRuler;
    if (const Handle(AIS_InteractiveObject)* anAISObject = GetMapOfAIS().Seek2(m_name)) {
        aRuler = Handle(ViewBase::InteractiveScaleRuler)::DownCast(*anAISObject);
    }
    if (aRuler.IsNull()) {
        Handle(Viewer::WasmViewer) aWebViewer = Handle(Viewer::WasmViewer)::DownCast(
            ViewerTest::CurrentEventManager());

        Handle(Prs3d_TextAspect) aTextStyle = !aWebViewer.IsNull()
                                                ? aWebViewer->DefaultTextStyle()
                                                : ViewerTest::GetAISContext()->DefaultDrawer()->TextAspect();

        aRuler = new ViewBase::InteractiveScaleRuler(new ViewBase::InteractiveScaleRulerOwner(m_name), aTextStyle);
        if (!aWebViewer.IsNull()) {
            aRuler->OnPixelScaleChange(aRuler, aWebViewer->DevicePixelRatio());
        }
    }
    if (m_baseSize.X() > 0.0) {
        aRuler->SetBaseSize(Graphic3d_Vec2((float)m_baseSize.X(), (float)m_baseSize.Y()));
    }

    ViewerTest::Display(m_name, aRuler, false);
    return true;
}

}

