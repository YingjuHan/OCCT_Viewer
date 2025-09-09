#include <ViewCaeTest/ScvMeshVecProp.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshVecProp
// =======================================================================
ScvMeshVecProp::ScvMeshVecProp()
{
    AddOnOffNoKey(this, "-unset") = &m_toUnset;
    AddOnOffNoKey(this, "-info")  = &m_toShowInfo;

    AddVoidKey(this, "-volume") = [](ScvMeshVecProp* theCmd, int&) -> bool {
        theCmd->m_distrType      = ViewCae::MeshVectorDistrType::Volume;
        theCmd->m_toSetDistrType = true;
        return true;
    };
    AddVoidKey(this, "-surface") = [](ScvMeshVecProp* theCmd, int&) -> bool {
        theCmd->m_distrType      = ViewCae::MeshVectorDistrType::Surface;
        theCmd->m_toSetDistrType = true;
        return true;
    };

    AddRangeKey(this, 0.0, RealLast(), "-scale") = &m_scale;
    AddRangeKey(this, 0.0, 1.0, "-ratio")        = &m_samplingRatio;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshVecProp::Perform()
{
    std::vector<TCollection_AsciiString> anAvailableVecProps = m_mesh->GetAvailableVec3Properties();
    if (m_toShowInfo) {
        if (anAvailableVecProps.empty()) {
            *myDI << "No available vector properties";
        } else {
            *myDI << "Available vector properties ids: ";
            for (const TCollection_AsciiString& aProp : anAvailableVecProps) {
                *myDI << aProp << " ";
            }
        }
    }

    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();

    if (m_toUnset) {
        m_mesh->SetVectorProperty(aPrsId, nullptr);
    }
    if (!m_propId.IsEmpty()) {
        bool hasFound = false;
        for (const TCollection_AsciiString& aProp : anAvailableVecProps) {
            if (m_propId == aProp) {
                hasFound = true;
                break;
            }
        }

        if (!hasFound) {
            *myDI << "Syntax error: wrong index '" << m_propId << "' of vector property";
            return false;
        }

        Handle(RWCae::MeshNVecProperty) aProp = m_mesh->GetPropertyByActiveTimeStep(m_propId);
        m_mesh->SetVectorProperty(aPrsId, aProp);
        m_mesh->SetProperty(aPrsId, aProp);
        m_mesh->SetPropertyComponentIndex(aPrsId, 0);
    }
    if (m_scale >= 0.0) {
        m_mesh->SetVectorScaleCoeff(aPrsId, m_scale);
    }
    if (m_toSetDistrType) {
        m_mesh->SetVectorDistrType(aPrsId, m_distrType);
    }
    if (m_samplingRatio >= 0.0) {
        m_mesh->SetVectorSamplingRatio(aPrsId, m_samplingRatio);
    }

    UpdateMesh();
    return true;
}

}

