#include <ViewCaeTest/MeshPrsCommand.h>

#include <ViewerTest.hxx>

namespace ViewCaeTest {

// =======================================================================
// function : MeshPrsCommand
// =======================================================================
MeshPrsCommand::MeshPrsCommand()
{
    AddKey(this, int(), "-prs") = &m_prsIndex;

    AddKey(this, "", "-prop") = [](MeshPrsCommand* theCmd, int&, const char* theVal) -> bool {
        theCmd->m_propId = theVal;
        return true;
    };
}

// =======================================================================
// function : ~MeshPrsCommand
// =======================================================================
MeshPrsCommand::~MeshPrsCommand()
{
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool MeshPrsCommand::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }
    if (m_prsName.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool MeshPrsCommand::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);
        if (!ViewBase::IInteractiveObject::FindObjectById(m_mesh, *m_context, m_prsName)) {
            *myDI << "Syntax error: mesh object not found\n";
            return false;
        }
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : UpdateMesh
// =======================================================================
void MeshPrsCommand::UpdateMesh()
{
    int aMask = m_mesh->ProcessRedraw(m_context, ViewerTest::CurrentView(), AIS_RedrawProgress_BeforeUpdate);
    if ((aMask & AIS_RedrawProgressResult_NeedRedisplay) != 0) {
        m_context->Redisplay(m_mesh, false);
    }
}

}
