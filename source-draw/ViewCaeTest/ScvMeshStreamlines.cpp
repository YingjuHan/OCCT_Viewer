#include <ViewCaeTest/ScvMeshStreamlines.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshStreamlines
// =======================================================================
ScvMeshStreamlines::ScvMeshStreamlines()
{
    AddKey(this, gp_XYZ(), "-planeorigin") = [](ScvMeshStreamlines* theCmd, int&, gp_XYZ theVal) -> bool {
        theCmd->m_plane.Origin     = theVal;
        theCmd->m_toSetPlaneOrigin = true;
        return true;
    };

    AddKey(this, gp_XYZ(), "-planeside") = [](ScvMeshStreamlines* theCmd, int&, gp_XYZ theVal) -> bool {
        theCmd->m_plane.Side     = theVal;
        theCmd->m_toSetPlaneSide = true;
        return true;
    };

    AddKey(this, gp_XYZ(), "-planeup") = [](ScvMeshStreamlines* theCmd, int&, gp_XYZ theVal) -> bool {
        theCmd->m_plane.Up     = theVal;
        theCmd->m_toSetPlaneUp = true;
        return true;
    };

    AddKey(this, int(), "-regular") = &m_isRegular;

    AddKey(this, int(), "-nbstreamlines") = &m_nbStreamlines;

    AddKey(this, int(), "-ustreamlines") = &m_uStreamlines;

    AddKey(this, int(), "-vstreamlines") = &m_vStreamlines;

    AddKey(this, double(), "-stepcoeff") = &m_stepCoeff;

    AddKey(this, double(), "-lengthcoeff") = &m_lengthCoeff;

    AddKey(this, double(), "-thickness") = &m_thickness;

    AddOnOffNoKey(this, "-anim") = &m_toAnim;

    AddKey(this, double(), "-animspeed") = &m_animSpeed;

    AddKey(this, double(), "-animtrail") = &m_animTrail;

    AddKey(this, int(), "-animfreq") = &m_animFreq;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshStreamlines::Perform()
{
    std::vector<TCollection_AsciiString> anAvailableVecProps = m_mesh->GetAvailableVec3Properties();

    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();

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
        if (!aProp->IsNodal()) {
            *myDI << "Syntax error: vector property should be nodal";
            return false;
        }
        m_mesh->SetStreamlinesProperty(aPrsId, aProp);
        m_mesh->SetProperty(aPrsId, aProp);
        m_mesh->SetPropertyComponentIndex(aPrsId, 0);
    }

    ViewCae::MeshStreamlinesPlane aPrevPlane = m_mesh->StreamlinesPlane(aPrsId);
    if (m_toSetPlaneOrigin) {
        aPrevPlane.Origin = m_plane.Origin;
    }
    if (m_toSetPlaneSide) {
        aPrevPlane.Side = m_plane.Side;
    }
    if (m_toSetPlaneUp) {
        aPrevPlane.Up = m_plane.Up;
    }
    m_mesh->SetStreamlinesPlane(aPrsId, aPrevPlane);

    if (m_isRegular != -1) {
        m_mesh->SetIsRegularStreamlines(aPrsId, m_isRegular == 1);
    }

    if (m_uStreamlines != -1 || m_vStreamlines != -1) {
        Graphic3d_Vec2i aUVStreamlines = m_mesh->UVStreamlines(aPrsId);
        if (m_uStreamlines != -1) {
            aUVStreamlines.x() = m_uStreamlines;
        }

        if (m_vStreamlines != -1) {
            aUVStreamlines.y() = m_vStreamlines;
        }
        m_mesh->SetUVStreamlines(aPrsId, aUVStreamlines);
    }

    if (m_nbStreamlines != -1) {
        m_mesh->SetNbStreamlines(aPrsId, m_nbStreamlines);
    }

    if (m_stepCoeff > 0.0) {
        m_mesh->SetStreamlinesStepCoeff(aPrsId, m_stepCoeff);
    }

    if (m_lengthCoeff > 0.0) {
        m_mesh->SetStreamlinesLengthCoeff(aPrsId, m_lengthCoeff);
    }

    if (m_thickness >= 0.0) {
        m_mesh->SetStreamlinesThickness(aPrsId, m_thickness);
    }

    if (m_toAnim != -1) {
        m_mesh->SetStreamlinesAnimated(aPrsId, m_toAnim == 1);
    }

    if (m_animSpeed >= 0.0) {
        m_mesh->SetStreamlinesAnimationSpeed(aPrsId, (float)m_animSpeed);
    }

    if (m_animTrail >= 0.0) {
        m_mesh->SetStreamlinesAnimationTrail(aPrsId, (float)m_animTrail);
    }

    if (m_animFreq > 0) {
        m_mesh->SetStreamlinesAnimationFrequency(aPrsId, m_animFreq);
    }

    UpdateMesh();
    return true;
}

}

