#include <RWCaeTest/ScvReadMesh.h>

#include <RWCaeTest/DrawableMeshData.h>

#include <RWBase/EmulatedFileSystem.h>

#include <RWMeshConv/MeshLoader.h>

#include <ViewCae/InteractiveCaeMesh.h>

#include <OSD_Timer.hxx>
#include <ViewerTest.hxx>

namespace RWCaeTest {

// =======================================================================
// function : ScvReadMesh
// =======================================================================
ScvReadMesh::ScvReadMesh() : MeshIOCommand()
{
    AddKey(this, "map.json", "-mapping") = [](ScvReadMesh* theCmd, const char* theVal) -> bool {
        theCmd->m_mappingFilePath = theVal;
        return true;
    };
    AddKey(this, "", "-artifact") = [](ScvReadMesh* theCmd, const char* theVal) -> bool {
        theCmd->m_artifactFilePath = theVal;
        return true;
    };
    AddKey(this, bool(), "-header", "-skipdata") = [](ScvReadMesh* theCmd, bool theVal) -> bool {
        if (theVal) {
            theCmd->m_options |= (int)RWCae::MeshDataImportExportOptions::SKIP_BINARY_DATA;
        } else {
            theCmd->m_options &= ~(int)RWCae::MeshDataImportExportOptions::SKIP_BINARY_DATA;
        }
        return true;
    };
    AddKey(this, bool(), "-skippropdata") = [](ScvReadMesh* theCmd, bool theVal) -> bool {
        if (theVal) {
            theCmd->m_options |= (int)RWCae::MeshDataImportExportOptions::SKIP_PROPERTIES_BINARY_DATA;
        } else {
            theCmd->m_options &= ~(int)RWCae::MeshDataImportExportOptions::SKIP_PROPERTIES_BINARY_DATA;
        }
        return true;
    };
    AddKey(this, bool(), "-savepropreffile") = [](ScvReadMesh* theCmd, bool theVal) -> bool {
        if (theVal) {
            theCmd->m_options |= (int)RWCae::MeshDataImportExportOptions::SAVE_PROPERTY_REF_FILE;
        } else {
            theCmd->m_options &= ~(int)RWCae::MeshDataImportExportOptions::SAVE_PROPERTY_REF_FILE;
        }
        return true;
    };
    AddOnOffNoKey(this, "-display") = [](ScvReadMesh* theCmd, bool theVal) -> bool {
        theCmd->m_toDisplay = theVal ? 1 : 0;
        if (theVal && ViewerTest::GetAISContext().IsNull()) {
            theCmd->Interpretor() << "Syntax error: vinit wasn't called\n";
            return false;
        }
        return true;
    };
    AddOnOffNoKey(this, "-dumptime")     = &m_toDumpTime;
    AddVoidKey(this, "-nodevecasscalar") = [](ScvReadMesh* theCmd) -> bool {
        theCmd->m_nodeVecAsScalar = true;
        return true;
    };
    AddOnOffNoKey(this, "-testcachegeom") = &m_testCacheGenGeom;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvReadMesh::ParseArgument(int& theArgIter)
{
    if (MeshIOCommand::ParseArgument(theArgIter)) {
        return true;
    }

    const char* anArg = Argument(theArgIter);
    if (m_prsName.IsEmpty()) {
        m_prsName = anArg;
    } else if (m_fileName.IsEmpty() && anArg[0] != '\0') {
        m_fileName = anArg;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvReadMesh::Perform()
{
    if (m_prsName.IsEmpty()) {
        TCollection_AsciiString aFileFolder;
        OSD_Path::FolderAndFileFromPath(m_fileName, aFileFolder, m_prsName);
    }

    Handle(RWCae::MeshData) aMeshData;
    {
        Handle(OSD_FileSystem) aFileSystem = RWBase::EmulatedFileSystem::Create(m_fileName);
        if (aFileSystem.IsNull()) {
            return false;
        }

        RWMeshConv::MeshLoader aLoader(m_options);
        aLoader.SetPropsToProcess(m_props);
        aLoader.SetReadNodeVecAsScalar(m_nodeVecAsScalar);
        OSD_Timer aTimer;
        aTimer.Start();
        aMeshData =
            aLoader.OpenMesh(aFileSystem, m_fileName, m_mappingFilePath.ToCString(), m_artifactFilePath.ToCString());
        aTimer.Stop();
        if (aMeshData.IsNull()) {
            return false;
        }

        if (m_toDumpTime) {
            *myDI << aTimer.ElapsedTime();
        }
    }

    // Publish only after a successful import, preserving a live model on failure.
    RemoveObject(m_prsName);
    aMeshData->SetId(m_prsName);
    *myDI << aMeshData->Id() << " "; // print read objects

    Handle(RWCaeTest::DrawableMeshData) aDrawable = new RWCaeTest::DrawableMeshData(aMeshData);
    aDrawable->SetTestCacheGenGeom(m_testCacheGenGeom);
    Draw::Set(m_prsName.ToCString(), aDrawable);
    if (m_toDisplay == -1) {
        m_autoUpdater.Invalidate();
        return true;
    }

    Handle(ViewCae::InteractiveCaeMeshOwner) anOwner  = new ViewCae::InteractiveCaeMeshOwner(aMeshData);
    Handle(ViewCae::InteractiveCaeMesh)      aMeshPrs = new ViewCae::InteractiveCaeMesh(anOwner);

    aMeshPrs->MeshOwner()->SetUseTestCachePrs(m_testCacheGenGeom);

    const TCollection_AsciiString anId = aMeshPrs->MeshOwner()->Id();
    aMeshPrs->SetOnColorMapperChangeCallback(
        [anId](const RWCae::MeshPropertyComponent& theProp, const Handle(ViewCae::MeshColorMapper)& theMapper) {
            (void)anId;
            (void)theProp;
            (void)theMapper;
        });

    m_context->Display(aMeshPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
    aMeshPrs->CreatePresentation(ViewCae::MeshPresentationType::Scalar);
    aMeshPrs->ProcessRedraw(m_context, ViewerTest::CurrentView(), AIS_RedrawProgress_BeforeUpdate);
    if (m_toDisplay == 1) {
        ViewerTest::Display(m_prsName, aMeshPrs);
        for (const Handle(PrsMgr_PresentableObject)& aChildIter : aMeshPrs->Children()) {
            Handle(AIS_InteractiveObject) aChildObj = Handle(AIS_InteractiveObject)::DownCast(aChildIter);
            m_context->Display(aChildObj, false);
        }
    } else {
        RegisterObject(m_prsName, aMeshPrs);
    }
    return true;
}

}

