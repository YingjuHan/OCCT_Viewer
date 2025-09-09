
#include <ViewCaeTest/ScvMeshSelMode.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshSelMode
// =======================================================================
ScvMeshSelMode::ScvMeshSelMode()
{
    AddVoidKey(this, "-presentation", "presentation") = [](ScvMeshSelMode* theCmd) -> bool {
        theCmd->m_selMode |= (int)ViewCae::InteractiveCaeMeshSelMode::Presentation;
        return true;
    };
    AddVoidKey(this, "-element", "element") = [](ScvMeshSelMode* theCmd) -> bool {
        theCmd->m_selMode |= (int)ViewCae::InteractiveCaeMeshSelMode::Elements;
        return true;
    };
    AddVoidKey(this, "-node", "node") = [](ScvMeshSelMode* theCmd) -> bool {
        theCmd->m_selMode |= (int)ViewCae::InteractiveCaeMeshSelMode::Nodes;
        return true;
    };
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvMeshSelMode::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }
    if (m_mesh.IsNull()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvMeshSelMode::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_mesh.IsNull()) {
        const char* aPrsName = Argument(theArgIter);
        if (!ViewBase::InteractiveOwner::FindPrsOwnerById(m_mesh, *m_context, aPrsName)) {
            *myDI << "Syntax error: mesh object '" << aPrsName << "' not found\n";
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
bool ScvMeshSelMode::Perform()
{
    m_context->ClearSelected(false);
    m_mesh->SetSelectionMode(m_selMode);
    return true;
}

}