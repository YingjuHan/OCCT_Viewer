
#include <ViewCaeTest/ScvMeshRemoveProps.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshRemoveProps
// =======================================================================
ScvMeshRemoveProps::ScvMeshRemoveProps()
{
    //
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshRemoveProps::Perform()
{
    m_mesh->RemoveProperties(true);
    UpdateMesh();
    return true;
}

}
