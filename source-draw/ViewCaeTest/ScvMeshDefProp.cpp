#include <ViewCaeTest/ScvMeshDefProp.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshDefProp
// =======================================================================
ScvMeshDefProp::ScvMeshDefProp()
{
    AddOnOffNoKey(this, "-unset")                = &m_toUnset;
    AddOnOffNoKey(this, "-info")                 = &m_toShowInfo;
    AddRangeKey(this, 0.0, RealLast(), "-scale") = &m_scale;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshDefProp::Perform()
{
    std::vector<TCollection_AsciiString> anAvailableDefProps = m_mesh->GetAvailableVec3Properties();
    if (m_toShowInfo) {
        if (anAvailableDefProps.empty()) {
            *myDI << "No available deformed properties";
        } else {
            *myDI << "Available deformed properties ids: ";
            for (const TCollection_AsciiString& aProp : anAvailableDefProps) {
                *myDI << aProp << " ";
            }
        }
    }

    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();

    if (m_toUnset) {
        m_mesh->SetDeformedProperty(aPrsId, nullptr);
    }
    if (!m_propId.IsEmpty()) {
        bool hasFound = false;
        for (const TCollection_AsciiString& aProp : anAvailableDefProps) {
            if (m_propId == aProp) {
                hasFound = true;
                break;
            }
        }

        if (!hasFound) {
            *myDI << "Syntax error: wrong index '" << m_propId << "' of deformed property";
            return false;
        }

        Handle(RWCae::MeshNVecProperty) aDefProp = m_mesh->GetPropertyByActiveTimeStep(m_propId);
        m_mesh->SetDeformedProperty(aPrsId, aDefProp);
    }

    if (m_scale >= 0.0) {
        m_mesh->SetDeformedScaleCoeff(aPrsId, m_scale);
    }

    UpdateMesh();
    return true;
}

}
