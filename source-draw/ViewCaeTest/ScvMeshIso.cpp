#include <ViewCaeTest/ScvMeshIso.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshIso
// =======================================================================
ScvMeshIso::ScvMeshIso()
{
    AddOnOffNoKey(this, "-clear")  = &m_toClear;
    AddKey(this, double(), "-add") = [](ScvMeshIso* theCmd, double theVal) -> bool {
        theCmd->m_isoValues.Append(theVal);
        return true;
    };

    AddKey(this, Graphic3d_Vec2d(), "-propnodal") = [](ScvMeshIso* theCmd, Graphic3d_Vec2d theVal) -> bool {
        theCmd->m_isElemental = false;
        theCmd->m_prop        = (int)theVal.x();
        theCmd->m_comp        = (int)theVal.y();
        return double(theVal.x()) == theCmd->m_prop && double(theVal.y()) == theCmd->m_comp;
    };
    AddKey(this, Graphic3d_Vec2d(), "-propelemental") = [](ScvMeshIso* theCmd, Graphic3d_Vec2d theVal) -> bool {
        theCmd->m_isElemental = true;
        theCmd->m_prop        = (int)theVal.x();
        theCmd->m_comp        = (int)theVal.y();
        return double(theVal.x()) == theCmd->m_prop && double(theVal.y()) == theCmd->m_comp;
    };
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshIso::Perform()
{
    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();

    if (m_toClear) {
        m_mesh->SetIsoProperty(aPrsId, RWCae::MeshPropertyComponent());
    }

    if (m_prop != -1 && m_comp != -1) {
        const Handle(RWCae::MeshData)&   aMeshData   = m_mesh->GetMeshData();
        const RWCae::MeshNVecProperties& aProperties = aMeshData->Properties();
        int                                anIndex     = -1;
        Handle(RWCae::MeshNVecProperty)  anIsoProp;
        for (const Handle(RWCae::MeshNVecProperty)& aProp : aProperties) {
            if (((m_isElemental && aProp->IsElemental()) || (!m_isElemental && !aProp->IsElemental()))
                && ++anIndex == m_prop) {
                anIsoProp = aProp;
                break;
            }
        }
        m_mesh->SetIsoProperty(aPrsId, RWCae::MeshPropertyComponent(anIsoProp, m_comp));
    }

    if (!m_isoValues.IsEmpty()) {
        std::vector<double> anIsoValues = m_mesh->IsoValues(aPrsId);
        for (NCollection_List<double>::Iterator anIter(m_isoValues); anIter.More(); anIter.Next()) {
            anIsoValues.push_back(anIter.Value());
        }
        m_mesh->SetIsoValues(aPrsId, anIsoValues);
    }

    UpdateMesh();
    return true;
}

}

