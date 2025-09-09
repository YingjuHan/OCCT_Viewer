
#include <ViewCaeTest/ScvMeshAnimation.h>

#include <ViewerTest.hxx>
#include <ViewerTest_EventManager.hxx>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshAnimation
// =======================================================================
ScvMeshAnimation::ScvMeshAnimation()
{
    AddKey(this, int(), "-time") = [](ScvMeshAnimation* theCmd, int theVal) -> bool {
        if (theVal < -1) {
            return false;
        }
        theCmd->m_timeStep = theVal;
        return true;
    };

    AddVoidKey(this, "-start") = [](ScvMeshAnimation* theCmd, int&) -> bool {
        theCmd->m_toStartAnim = 1;
        return true;
    };

    AddVoidKey(this, "-restart") = [](ScvMeshAnimation* theCmd, int&) -> bool {
        theCmd->m_timeStep    = -1;
        theCmd->m_toStartAnim = 1;
        return true;
    };

    AddVoidKey(this, "-stop") = [](ScvMeshAnimation* theCmd, int&) -> bool {
        theCmd->m_toStartAnim = 0;
        return true;
    };

    AddKey(this, double(), "-duration") = [](ScvMeshAnimation* theCmd, double theVal) -> bool {
        if (theVal <= 0.0) {
            return false;
        }
        theCmd->m_duration = theVal;
        return true;
    };

    AddOnOffNoKey(this, "-loop") = &m_toLoop;

    AddOnOffNoKey(this, "-info") = &m_toShowInfo;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshAnimation::Perform()
{
    std::vector<std::pair<int, double>> aTimeSteps = m_mesh->MeshOwner()->AvailableTimeSteps();

    if (m_toShowInfo) {
        *myDI << "Active time step: " << m_mesh->ActiveTimeStep() << "\n";
        *myDI << "Available time steps: ";
        for (int i = 0; i < (int)aTimeSteps.size(); i++) {
            if (i > 0) {
                *myDI << ", ";
            }
            *myDI << "(" << aTimeSteps[i].first << ", " << aTimeSteps[i].second << ")";
        }
        *myDI << "\n";
    }

    if (m_timeStep > -2) {
        bool isValid = false;
        for (const std::pair<int, double>& aTimeStep : aTimeSteps) {
            if (m_timeStep == aTimeStep.first || m_timeStep == -1) {
                m_timeStep = aTimeStep.first;
                isValid    = true;
                break;
            }
        }

        if (!isValid) {
            *myDI << "Error: invalid timestamp '" << m_timeStep << "'";
            return false;
        }

        m_mesh->SetActiveTimeStep(m_timeStep);
        UpdateMesh();
    }

    if (m_duration > 0.0 && m_mesh->Animation()->IsStopped()) {
        m_mesh->Animation()->SetOwnDuration(m_duration);
    }

    if (m_toLoop != -1) {
        m_mesh->Animation()->SetLoopPlayback(m_toLoop == 1);
    }

    if (m_toStartAnim == 1) {
        m_mesh->Animation()->StartTimer(0.0, 1.0, true);
        ViewerTest::CurrentEventManager()->FlushViewEvents(m_context, ViewerTest::CurrentView(), true);
    } else if (m_toStartAnim == 0) {
        m_mesh->Animation()->Stop();
    }

    return true;
}

}