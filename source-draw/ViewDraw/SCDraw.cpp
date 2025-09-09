
#include <ViewDraw/SCDraw.h>

#include <ViewDraw/ScvRead.h>

#include <LodCaeTest/LodCaeCommands.h>

#include <RWCaeTest/RWCaeCommands.h>
#include <RWShapeTest/RWShapeCommands.h>

#include <ViewBaseTest/PrsBaseCommands.h>
#include <ViewCaeTest/PrsCaeCommands.h>
#include <ViewShapeTest/PrsShapeCommands.h>

#include <SimMeshDraw.h>

#include <Draw_PluginMacro.hxx>

namespace ViewDraw {

// ================================================================
// Function : Factory
// ================================================================
void SCDraw::Factory(Draw_Interpretor& theDI)
{
    RWCaeTest::RWCaeCommands::Factory(theDI);
    RWShapeTest::RWShapeCommands::Factory(theDI);

    ViewBaseTest::PrsBaseCommands::Factory(theDI);
    ViewCaeTest::PrsCaeCommands::Factory(theDI);
    ViewShapeTest::PrsShapeCommands::Factory(theDI);

    LodCaeTest::LodCaeCommands::Factory(theDI);
    SimMeshDraw::Factory(theDI);

    DrawBase::DrawCommand::Register<ScvRead>(theDI, "SC Viewer");
}

}

DPLUGIN(ViewDraw::SCDraw);

