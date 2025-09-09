#include <LodCaeTest/LodCaeCommands.h>

#include <LodCaeTest/ScvMeshSimplify.h>

#include <Draw_PluginMacro.hxx>

namespace LodCaeTest {

// ================================================================
// Function : Factory
// ================================================================
void LodCaeCommands::Factory(Draw_Interpretor& theDI)
{
    const char* aGroup = "SC Viewer CAE Prs";
    DrawBase::DrawCommand::Register<ScvMeshSimplify>(theDI, aGroup);
}

}

DPLUGIN(LodCaeTest::LodCaeCommands);
