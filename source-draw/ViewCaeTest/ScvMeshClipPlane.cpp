#include <ViewCaeTest/ScvMeshClipPlane.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshClipPlane
// =======================================================================
ScvMeshClipPlane::ScvMeshClipPlane()
{
    AddOnOffNoKey(this, "-unset") = &m_toUnset;

    AddKey(this, Graphic3d_Vec4d(), "-set") = [](ScvMeshClipPlane* theCmd, int&, Graphic3d_Vec4d theVal) -> bool {
        theCmd->m_clipPlane = new Graphic3d_ClipPlane(theVal);
        return true;
    };

    AddOnOffNoKey(this, "-cut") = &m_toCut;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshClipPlane::Perform()
{
    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();
    if (!m_clipPlane.IsNull() || m_toUnset) {
        m_mesh->SetClipPlane(aPrsId, m_clipPlane);
    }
    if (m_toCut != -1) {
        m_mesh->SetCutOnClipPlane(aPrsId, m_toCut == 1);
    }
    UpdateMesh();
    return true;
}

}
