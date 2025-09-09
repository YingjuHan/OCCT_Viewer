#include <ViewCaeTest/ScvMeshAnnotation.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshAnnotation
// =======================================================================
ScvMeshAnnotation::ScvMeshAnnotation()
{
    AddVoidKey(this, "+minmax") = [](ScvMeshAnnotation* theCmd, int&) -> bool {
        theCmd->m_toShowMinMax = 1;
        return true;
    };
    AddVoidKey(this, "-minmax") = [](ScvMeshAnnotation* theCmd, int&) -> bool {
        theCmd->m_toShowMinMax = 0;
        return true;
    };
    AddVoidKey(this, "+groups") = [](ScvMeshAnnotation* theCmd, int&) -> bool {
        theCmd->m_toShowGroups = 1;
        return true;
    };
    AddVoidKey(this, "-groups") = [](ScvMeshAnnotation* theCmd, int&) -> bool {
        theCmd->m_toShowGroups = 0;
        return true;
    };
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshAnnotation::Perform()
{
    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();
    if (m_toShowMinMax != -1) {
        m_mesh->SetShowPrsMinMax(aPrsId, m_toShowMinMax == 1);
    }
    if (m_toShowGroups != -1) {
        m_mesh->SetShowGroupsInfo(aPrsId, m_toShowGroups == 1);
    }
    UpdateMesh();
    return true;
}

}
