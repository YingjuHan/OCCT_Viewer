#include <ViewBaseTest/ScvViewCube.h>

#include <ViewBase/InteractiveViewCube.h>

#include <Viewer/WasmViewer.h>

#include <ViewerTest.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>

Standard_IMPORT ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();

namespace ViewBaseTest {

// =======================================================================
// function : ScvViewCube
// =======================================================================
ScvViewCube::ScvViewCube()
{
    AddRangeKey(this, RealEpsilon(), RealLast(), "-size") = &m_baseSize;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvViewCube::ParseAllArguments()
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
bool ScvViewCube::ParseArgument(int& theArgIter)
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
bool ScvViewCube::Perform()
{
    Handle(ViewBase::InteractiveViewCube) aViewCube;
    if (const Handle(AIS_InteractiveObject)* anAISObject = GetMapOfAIS().Seek2(m_name)) {
        aViewCube = Handle(ViewBase::InteractiveViewCube)::DownCast(*anAISObject);
    }
    if (aViewCube.IsNull()) {
        Handle(Viewer::WasmViewer) aWebViewer = Handle(Viewer::WasmViewer)::DownCast(
            ViewerTest::CurrentEventManager());

        Handle(Prs3d_TextAspect) aTextStyle = !aWebViewer.IsNull()
                                                ? aWebViewer->DefaultTextStyle()
                                                : ViewerTest::GetAISContext()->DefaultDrawer()->TextAspect();

        aViewCube = new ViewBase::InteractiveViewCube(new ViewBase::InteractiveViewCubeOwner(m_name), aTextStyle);
        aViewCube->SetViewAnimation(ViewerTest::CurrentEventManager()->ViewAnimation());
        aViewCube->SetFixedAnimationLoop(false);
        aViewCube->SetAutoStartAnimation(true);
        if (!aWebViewer.IsNull()) {
            aViewCube->OnPixelScaleChange(aViewCube, aWebViewer->DevicePixelRatio());
        }
    }
    if (m_baseSize > 0.0) {
        aViewCube->SetSize(m_baseSize);
    }

    ViewerTest::Display(m_name, aViewCube, false);
    return true;
}

}

