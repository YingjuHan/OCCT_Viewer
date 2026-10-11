
#include <ViewCaeTest/PrsCaeCommands.h>
#include <ViewCaeTest/CutViewCommands.h>

#include <ViewCaeTest/ScvMesh2DGrid.h>
#include <ViewCaeTest/ScvMeshActiveProp.h>
#include <ViewCaeTest/ScvMeshAnimation.h>
#include <ViewCaeTest/ScvMeshAnnotation.h>
#include <ViewCaeTest/ScvMeshAspects.h>
#include <ViewCaeTest/ScvMeshBlock.h>
#include <ViewCaeTest/ScvMeshClipPlane.h>
#include <ViewCaeTest/ScvMeshDefProp.h>
#include <ViewCaeTest/ScvMeshIso.h>
#include <ViewCaeTest/ScvMeshPrs.h>
#include <ViewCaeTest/ScvMeshRemoveProps.h>
#include <ViewCaeTest/ScvMeshResetCounters.h>
#include <ViewCaeTest/ScvMeshSelMode.h>
#include <ViewCaeTest/ScvMeshSlice.h>
#include <ViewCaeTest/ScvMeshStreamlines.h>
#include <ViewCaeTest/ScvMeshTestGroup.h>
#include <ViewCaeTest/ScvMeshVecProp.h>
#include <ViewCaeTest/ScvSphericalAxes.h>

#include <ViewCaeTest/ScvColorScale.h>

#include <Draw_PluginMacro.hxx>

namespace ViewCaeTest {

// ================================================================
// Function : Factory
// ================================================================
void PrsCaeCommands::Factory(Draw_Interpretor& theDI)
{
    const char* aGroup = "SC Viewer CAE Prs";
    CutViewCommands::Factory(theDI);
    DrawBase::DrawCommand::Register<ScvColorScale>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMesh2DGrid>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshActiveProp>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshAnimation>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshAnnotation>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshAspects>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshBlock>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshClipPlane>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshDefProp>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshIso>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshPrs>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshRemoveProps>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshResetCounters>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshSelMode>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshSlice>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshStreamlines>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshTestGroup>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshVecProp>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvSphericalAxes>(theDI, aGroup);

    theDI.Add("scvmeshdispmode",
              "Alias to scvmeshaspects prsName -prsMode Mode",
              __FILE__,
              &DrawBase::DrawCommand::Invoke<ScvMeshAspects>,
              aGroup);
    theDI.Add("scvmeshelemtype",
              "Alias to scvmeshaspects",
              __FILE__,
              &DrawBase::DrawCommand::Invoke<ScvMeshAspects>,
              aGroup);
    theDI.Add("scvmeshvisprops",
              "Alias to scvmeshaspects",
              __FILE__,
              &DrawBase::DrawCommand::Invoke<ScvMeshAspects>,
              aGroup);
}

}

DPLUGIN(ViewCaeTest::PrsCaeCommands);
