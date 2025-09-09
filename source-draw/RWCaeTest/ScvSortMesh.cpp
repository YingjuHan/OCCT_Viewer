
#include <RWCaeTest/ScvSortMesh.h>

#include <RWCae/MeshTools.h>

#include <RWCaeTest/DrawableMeshData.h>

#include <Message.hxx>

namespace RWCaeTest {

// =======================================================================
// function : ScvSortMesh
// =======================================================================
ScvSortMesh::ScvSortMesh() : DrawBase::DrawCommand()
{
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvSortMesh::ParseArgument(int& theArgIter)
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
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvSortMesh::Perform()
{
    Handle(RWCaeTest::DrawableMeshData) aDrawable = Handle(RWCaeTest::DrawableMeshData)::DownCast(
        Draw::GetExisting(m_prsName.ToCString()));

    const Handle(RWCae::MeshData)& aMeshData = aDrawable->MeshData();

    RWCae::MeshTools::SortMeshData(aMeshData);

    return true;
}

}
