#include <ViewBaseTest/PrsBaseCommands.h>

#include <ViewBaseTest/ScvInit.h>
#include <ViewBaseTest/ScvClipping.h>
#include <ViewBaseTest/ScvClone.h>
#include <ViewBaseTest/ScvDrawText.h>
#include <ViewBaseTest/ScvFont.h>
#include <ViewBaseTest/ScvScaleRuler.h>
#include <ViewBaseTest/ScvUnit.h>
#include <ViewBaseTest/ScvViewCube.h>

#include <Draw_PluginMacro.hxx>

namespace ViewBaseTest {

// scvdefaultfonts command definition.
static constexpr const char* SCVDEFAULTFONTS_TCL = R"(
help scvdefaultfonts {
Register default fonts available in Web build.
}
proc scvdefaultfonts {args} {
    vfont -clear
    scvfont [locate_data_file UbuntuMono-R.ttf]     "monospace"
    scvfont [locate_data_file OpenSans-Regular.ttf] "sans-serif"
    scvfont [locate_data_file OpenSans-Bold.ttf]    "sans-serif"
    scvfont [locate_data_file HarmonyOS_Sans_SC_Regular.woff] "cjk"
    vfont -list
}
)";

// ================================================================
// Function : AddCommands
// ================================================================
void PrsBaseCommands::Factory(Draw_Interpretor& theDI)
{
    const char* aGroup = "SC Viewer";

    DrawBase::DrawCommand::Register<ScvInit>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvFont>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvUnit>(theDI, aGroup);

    DrawBase::DrawCommand::Register<ScvClone>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvViewCube>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvScaleRuler>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvClipping>(theDI, aGroup);

    DrawBase::DrawCommand::Register<ScvDrawText>(theDI, aGroup);

    theDI.Eval(SCVDEFAULTFONTS_TCL);
}

}

DPLUGIN(ViewBaseTest::PrsBaseCommands);