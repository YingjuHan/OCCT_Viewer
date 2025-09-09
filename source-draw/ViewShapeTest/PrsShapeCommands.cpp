#include <ViewShapeTest/PrsShapeCommands.h>

#include <ViewShapeTest/ScvAspects.h>
#include <ViewShapeTest/ScvDimension.h>
#include <ViewShapeTest/ScvDisplay.h>
#include <ViewShapeTest/ScvSelmode.h>
#include <ViewShapeTest/ScvSelect.h>
#include <ViewShapeTest/ScvSelState.h>

#include <Draw_PluginMacro.hxx>

namespace ViewShapeTest {

// ================================================================
// Function : Factory
// ================================================================
void PrsShapeCommands::Factory(Draw_Interpretor& theDI)
{
    const char* aGroup = "SC Viewer Shape Prs";
    DrawBase::DrawCommand::Register<ScvAspects>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvDimension>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvDisplay>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvSelmode>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvSelState>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvSelect>(theDI, aGroup);

    theDI.Add("scvdispmode",
              "Alias to scvaspects prsName -prsMode Mode",
              __FILE__,
              &DrawBase::DrawCommand::Invoke<ScvAspects>,
              aGroup);
}

}

DPLUGIN(ViewShapeTest::PrsShapeCommands);
