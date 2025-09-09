
#include <RWCaeTest/ScvExportMesh.h>

#include <RWCaeTest/DrawableMeshData.h>

#include <Message.hxx>
#include <OSD_Timer.hxx>
#include <Draw_ProgressIndicator.hxx>

namespace RWCaeTest {

// =======================================================================
// function : ScvExportMesh
// =======================================================================
ScvExportMesh::ScvExportMesh() : MeshIOCommand()
{
    AddOnOffNoKey(this, "-singleprecision") = &m_toIsSinglePrecision;

    AddKey(this, Graphic3d_Vec3i(), "-draco") = [](ScvExportMesh* theCmd, Graphic3d_Vec3i theVal) -> bool {
        theCmd->m_toUseDracoCompression = true;
        theCmd->m_dracoNodesQuantBits   = theVal.x();
        theCmd->m_dracoPropsQuantBits   = theVal.y();
        theCmd->m_dracoNbParts          = theVal.z();
        return true;
    };

    AddKey(this, bool(), "-skipgeomdata") = [](ScvExportMesh* theCmd, bool theVal) -> bool {
        if (theVal) {
            theCmd->m_options |= (int)RWCae::MeshDataImportExportOptions::SKIP_GEOMETRY_BINARY_DATA;
        } else {
            theCmd->m_options &= ~(int)RWCae::MeshDataImportExportOptions::SKIP_GEOMETRY_BINARY_DATA;
        }
        return true;
    };

    AddKey(this, bool(), "-skippropdata") = [](ScvExportMesh* theCmd, bool theVal) -> bool {
        if (theVal) {
            theCmd->m_options |= (int)RWCae::MeshDataImportExportOptions::SKIP_PROPERTIES_BINARY_DATA;
        } else {
            theCmd->m_options &= ~(int)RWCae::MeshDataImportExportOptions::SKIP_PROPERTIES_BINARY_DATA;
        }
        return true;
    };

    AddOnOffNoKey(this, "-dumptime") = &m_toDumpTime;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvExportMesh::ParseArgument(int& theArgIter)
{
    if (DrawCommand::ParseArgument(theArgIter)) {
        return true;
    }

    if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);

        Handle(RWCaeTest::DrawableMeshData) aDrawable = Handle(RWCaeTest::DrawableMeshData)::DownCast(
            Draw::GetExisting(m_prsName.ToCString()));
        if (aDrawable.IsNull()) {
            Message::SendFail() << "Error: object '" << m_prsName << "' doesn't exist or isn't CAE mesh";
            return false;
        }
    } else if (m_fileName.IsEmpty()) {
        m_fileName = Argument(theArgIter);
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvExportMesh::Perform()
{
    Handle(RWCaeTest::DrawableMeshData) aDrawable = Handle(RWCaeTest::DrawableMeshData)::DownCast(
        Draw::GetExisting(m_prsName.ToCString()));

    Handle(Draw_ProgressIndicator) aProgress = new Draw_ProgressIndicator(*myDI);

    RWCae::MeshBinWriter aWriter(aDrawable->MeshData());
    aWriter.SetOptions(m_options);
    aWriter.SetPropsToProcess(m_props);
    if (m_toIsSinglePrecision != -1) {
        aWriter.SetToForceSinglePrecision(m_toIsSinglePrecision == 1);
    }
    if (m_toUseDracoCompression) {
        aWriter.SetUseDracoCompression(
            RWCae::MeshDracoCompressionInfo(m_dracoNodesQuantBits, m_dracoPropsQuantBits, m_dracoNbParts));
    }
    OSD_Timer aTimer;
    aTimer.Start();
    if (!aWriter.Perform(m_fileName, aProgress->Start())) {
        *myDI << "Error: failed to write mesh data to '" << m_fileName << "'";
        return false;
    }
    aTimer.Stop();
    if (m_toDumpTime) {
        *myDI << aTimer.ElapsedTime();
    }

    return true;
}

}
