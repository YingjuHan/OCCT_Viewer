
#include <ViewCaeTest/ScvMeshAspects.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshAspects
// =======================================================================
ScvMeshAspects::ScvMeshAspects()
{
    AddKey(this, Quantity_Color(), "-shadingcolor") = [](ScvMeshAspects* theCmd, Quantity_Color theVal) -> bool {
        if (!theCmd->m_hasShadingColor) {
            theCmd->m_shadingColor    = theVal;
            theCmd->m_hasShadingColor = true;
            return true;
        }
        return false;
    };
    AddKey(this, Quantity_Color(), "-edgescolor") = [](ScvMeshAspects* theCmd, Quantity_Color theVal) -> bool {
        if (!theCmd->m_hasEdgeColor) {
            theCmd->m_edgeColor    = theVal;
            theCmd->m_hasEdgeColor = true;
            return true;
        }
        return false;
    };
    AddKey(this, Quantity_Color(), "-pointscolor") = [](ScvMeshAspects* theCmd, Quantity_Color theVal) -> bool {
        if (!theCmd->m_hasPointColor) {
            theCmd->m_pointColor    = theVal;
            theCmd->m_hasPointColor = true;
            return true;
        }
        return false;
    };
    AddKey(this, double(), "-edgeswidth") = [](ScvMeshAspects* theCmd, double theVal) -> bool {
        if (theCmd->m_edgeWidth <= 0.0 && theVal > 0.0) {
            theCmd->m_edgeWidth = theVal;
            return true;
        }
        return false;
    };
    AddKey(this, double(), "-pointssize") = [](ScvMeshAspects* theCmd, double theVal) -> bool {
        if (theCmd->m_pointSize <= 0.0 && theVal > 0.0) {
            theCmd->m_pointSize = theVal;
            return true;
        }
        return false;
    };
    AddKey(this, double(), "-transparency") = [](ScvMeshAspects* theCmd, double theVal) -> bool {
        if (theCmd->m_transparency < 0.0 && theVal > -0.0 && theVal <= 1.0) {
            theCmd->m_transparency = theVal;
            return true;
        }
        return false;
    };

    AddOnOffNoKey(this, "-volumeassurface") = &m_volumeAsSurf;

    AddKey(this, "", "-prsmode", "-dispmode") = &ScvMeshAspects::ParsePrsMode;

    AddOnOffNoKey(this, "-visible") = &m_toShowPrs;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvMeshAspects::ParseArgument(int& theArgIter)
{
    if (MeshPrsCommand::ParseArgument(theArgIter)) {
        //
    } else if (ParseMeshElemTypes(Argument(theArgIter))) {
        //
    } else if (ParsePrsMode(Argument(theArgIter))) { // svcmeshdispmode alias
        //
    } else if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);
        if (!ViewBase::IInteractiveObject::FindObjectById(m_mesh, *m_context, m_prsName)) {
            *myDI << "Syntax error: mesh with name '" << m_prsName << "' does not exist\n";
            return false;
        }
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParsePrsMode
// =======================================================================
bool ScvMeshAspects::ParsePrsMode(const char* theStr)
{
    if (m_prsMode != -1) {
        return false;
    }
    TCollection_AsciiString anArg(theStr);
    anArg.LowerCase();
    if (CheckArgName(anArg, "flatshading", "flatshaded", "shaded", "shading") || anArg == "flat") {
        m_prsMode = (int)ViewCae::InteractiveCaeMeshPrsMode::FlatShaded;
    } else if (CheckArgName(anArg, "wireframe", "meshedges")) {
        m_prsMode = (int)ViewCae::InteractiveCaeMeshPrsMode::MeshEdges;
    } else if (CheckArgName(anArg, "shadededges", "edgeshading", "edgeshaded", "shadedmeshedges")) {
        m_prsMode = (int)ViewCae::InteractiveCaeMeshPrsMode::ShadedMeshEdges;
    } else if (CheckArgName(anArg, "shadedfreeedges", "freeedgeshading", "freeedges")) {
        m_prsMode = (int)ViewCae::InteractiveCaeMeshPrsMode::ShadedFreeEdges;
    } else if (CheckArgName(anArg, "shadedsharpedges", "sharpedgeshading", "sharpedges")) {
        m_prsMode = (int)ViewCae::InteractiveCaeMeshPrsMode::ShadedSharpEdges;
    } else if (CheckArgName(anArg, "shrink", "shrinked", "meshshrinked", "shrunk")) {
        m_prsMode = (int)ViewCae::InteractiveCaeMeshPrsMode::ShadedMeshShrinked;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseMeshElemTypes
// =======================================================================
bool ScvMeshAspects::ParseMeshElemTypes(const char* theArg)
{
    unsigned int* aBitMask = nullptr;
    if (theArg[0] == '\0' || theArg[1] == '\0') {
        return false;
    } else if (theArg[0] == '+') {
        aBitMask = &m_elemTypesToEnable;
    } else if (theArg[0] == '-') {
        aBitMask = &m_elemTypesToDisable;
    } else {
        return false;
    }

    TCollection_AsciiString aName(theArg + 1);
    aName.LowerCase();

    unsigned int aBit = 0;
    if (aName == "elems" || aName == "elements") {
        aBit = (unsigned int)ViewCae::InteractiveCaeMeshElementType::AllElemental;
    } else if (aName == "3d") {
        aBit = (unsigned int)ViewCae::InteractiveCaeMeshElementType::Volume;
    } else if (aName == "2d") {
        aBit = (unsigned int)ViewCae::InteractiveCaeMeshElementType::Face;
    } else if (aName == "0d") {
        aBit = (unsigned int)ViewCae::InteractiveCaeMeshElementType::Element0d;
    } else if (aName == "centroid" || aName == "centroids") {
        aBit = (unsigned int)ViewCae::InteractiveCaeMeshElementType::Centroid;
    } else if (aName == "nodes") {
        aBit = (unsigned int)ViewCae::InteractiveCaeMeshElementType::Node;
    } else if (aName == "freenodes") {
        aBit = (unsigned int)ViewCae::InteractiveCaeMeshElementType::FreeNode;
    } else if (aName == "freeedges") {
        aBit = (int)ViewCae::InteractiveCaeMeshElementType::FreeEdges;
    } else if (aName == "sharpedges") {
        aBit = (unsigned int)ViewCae::InteractiveCaeMeshElementType::SharpEdges;
    } else {
        return false;
    }

    *aBitMask |= aBit;
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshAspects::Perform()
{
    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();

    if (m_hasShadingColor) {
        m_mesh->SetShadingColor(aPrsId, m_shadingColor, ViewCae::MeshHighlightingType::None);
    }
    if (m_hasEdgeColor) {
        m_mesh->SetEdgesColor(aPrsId, m_edgeColor, ViewCae::MeshHighlightingType::None);
    }
    if (m_hasPointColor) {
        m_mesh->SetPointsColor(aPrsId, m_pointColor, ViewCae::MeshHighlightingType::None);
    }

    if (m_transparency >= 0.0) {
        m_mesh->SetShadingTransparency(aPrsId, (float)m_transparency, ViewCae::MeshHighlightingType::None);
    }
    if (m_edgeWidth > 0.0) {
        m_mesh->SetEdgesWidth(aPrsId, (float)m_edgeWidth, ViewCae::MeshHighlightingType::None);
    }
    if (m_pointSize > 0.0) {
        m_mesh->SetPointsSize(aPrsId, (float)m_pointSize, ViewCae::MeshHighlightingType::None);
    }

    if (m_volumeAsSurf != -1) {
        m_mesh->SetVolumeAsSurface(aPrsId, m_volumeAsSurf == 1);
    }
    if (m_prsMode != -1) {
        m_mesh->SetPresentationMode(aPrsId, (ViewCae::InteractiveCaeMeshPrsMode)m_prsMode);
    }

    if (m_toShowPrs != -1) {
        m_mesh->SetPrsVisible(aPrsId, m_toShowPrs == 1);
    }

    if (m_elemTypesToEnable != 0 || m_elemTypesToDisable != 0) {
        unsigned int anElementTypes = m_mesh->ElementTypes(aPrsId);
        anElementTypes |= m_elemTypesToEnable;
        anElementTypes &= ~m_elemTypesToDisable;
        m_mesh->SetElementTypes(aPrsId, anElementTypes);
    }

    UpdateMesh();
    return true;
}

}
