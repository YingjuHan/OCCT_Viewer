#include <RWShapeTest/RWShapeCommands.h>

#include <RWShapeTest/ScvExplode.h>
#include <RWShapeTest/ScvLoad.h>
#include <RWShapeTest/ScvReadShape.h>

#include <Draw_PluginMacro.hxx>

namespace RWShapeTest {

// ================================================================
// Function : Factory
// ================================================================
void RWShapeCommands::Factory(Draw_Interpretor& theDI)
{
    const char* aGroup = "SC Viewer Shape RW";
    DrawBase::DrawCommand::Register<ScvExplode>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvLoad>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvReadShape>(theDI, aGroup);
}

}

DPLUGIN(RWShapeTest::RWShapeCommands);






