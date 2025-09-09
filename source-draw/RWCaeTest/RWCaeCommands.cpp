
#include <RWCaeTest/RWCaeCommands.h>

#include <RWCaeTest/ScvCompareMeshes.h>
#include <RWCaeTest/ScvExportMesh.h>
#include <RWCaeTest/ScvFarfieldJsonMesh.h>
#include <RWCaeTest/ScvMeshInfo.h>
#include <RWCaeTest/ScvMeshPropDependency.h>
#include <RWCaeTest/ScvNearfieldPhaseMesh.h>
#include <RWCaeTest/ScvReadMapping.h>
#include <RWCaeTest/ScvReadMesh.h>
#include <RWCaeTest/ScvReadMeshProperties.h>
#include <RWCaeTest/ScvSortMesh.h>
#include <RWCaeTest/ScvTestMesh.h>
#include <RWCaeTest/ScvTestMeshProp.h>

#include <Draw_PluginMacro.hxx>

namespace RWCaeTest {

// ================================================================
// Function : Factory
// ================================================================
void RWCaeCommands::Factory(Draw_Interpretor& theDI)
{
    const char* aGroup = "SC Viewer CAE RW";
    DrawBase::DrawCommand::Register<ScvCompareMeshes>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvExportMesh>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvFarfieldJsonMesh>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshInfo>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvMeshPropDependency>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvNearfieldPhaseMesh>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvReadMapping>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvReadMesh>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvReadMeshProperties>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvSortMesh>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvTestMesh>(theDI, aGroup);
    DrawBase::DrawCommand::Register<ScvTestMeshProp>(theDI, aGroup);
}

}

DPLUGIN(RWCaeTest::RWCaeCommands);
