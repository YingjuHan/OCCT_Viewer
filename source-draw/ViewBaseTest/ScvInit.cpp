#include <ViewBaseTest/ScvInit.h>

#include <Viewer/WasmViewer.h>

#include <OSD_ThreadPool.hxx>
#include <ViewerTest.hxx>

namespace ViewBaseTest {

// =======================================================================
// function : ScvInit
// =======================================================================
ScvInit::ScvInit()
{
    AddRangeKey(this, RealEpsilon(), RealLast(), "-pixelratio") = &m_pixelRatio;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvInit::ParseArgument(int& theArgIter)
{
    m_args += TCollection_AsciiString(" ") + Argument(theArgIter);
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvInit::Perform()
{
    const int aRes = myDI->Eval(m_args.ToCString());
    if (aRes != 0) {
        return false;
    }

    const int aNbDefThreads = OSD_ThreadPool::DefaultPool()->NbDefaultThreadsToLaunch();

    Handle(AIS_InteractiveContext) aCtx = ViewerTest::GetAISContext();

    Handle(Viewer::WasmViewer) aWebViewer = Handle(Viewer::WasmViewer)::DownCast(
        ViewerTest::CurrentEventManager());
    if (aWebViewer.IsNull() || aWebViewer->GetContext() != aCtx || aWebViewer->View() != ViewerTest::CurrentView()) {
        aWebViewer = new Viewer::WasmViewer(ViewerTest::CurrentView(), aCtx);
    }
    aWebViewer->init();
    if (m_pixelRatio > 0.0) {
        aWebViewer->SetDevicePixelRatio((float)m_pixelRatio);
    }
    ViewerTest::SetEventManager(aWebViewer);

    // restore defaults and ignore web viewer task manager overrides
    OSD_ThreadPool::DefaultPool()->SetNbDefaultThreadsToLaunch(aNbDefThreads);
    return true;
}

}

