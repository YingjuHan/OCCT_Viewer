#include <RWCaeTest/ScvMeshInfo.h>

#include <RWCaeTest/DrawableMeshData.h>

#include <Viewer/Utils/CaeMeshInfo.h>

#include <Message.hxx>

namespace RWCaeTest {

// =======================================================================
// function : ScvMeshInfo
// =======================================================================
ScvMeshInfo::ScvMeshInfo()
{
    AddOnOffNoKey(this, "-force", "-forced", "-recompute") = &m_toRecomputeAABB;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvMeshInfo::ParseAllArguments()
{
    if (!DrawCommand::ParseAllArguments()) {
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
bool ScvMeshInfo::ParseArgument(int& theArgIter)
{
    if (DrawCommand::ParseArgument(theArgIter)) {
        return true;
    }
    if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);

        Handle(RWCaeTest::DrawableMeshData) aDrawable = Handle(RWCaeTest::DrawableMeshData)::DownCast(
            Draw::GetExisting(m_prsName.ToCString()));
        if (aDrawable.IsNull()) {
            Message::SendFail() << "Error: object '" << m_prsName << "' doesn't exist or isn't CAE mesh";
            return false;
        }
        return true;
    }
    return false;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshInfo::Perform()
{
    Handle(RWCaeTest::DrawableMeshData) aDrawable = Handle(RWCaeTest::DrawableMeshData)::DownCast(
        Draw::GetExisting(m_prsName.ToCString()));

    const Handle(RWCae::MeshData)& aMeshData = aDrawable->MeshData();
    if (m_toRecomputeAABB) {
        aMeshData->UpdateCachedMinMax();
    }

    Viewer::CaeMeshInfo anInfo(aMeshData);
    *myDI << "Mesh statistics:"
          << "\nNodes: " << anInfo.nbNodes << "\n0d elements: " << anInfo.nbElems0d
          << "\n1d elements: " << anInfo.nbElems1d << "\n2d elements: " << anInfo.nbElems2d
          << "\n  Triangles: " << anInfo.nbTriangles << "\n  2nd order triangles: " << anInfo.nbTriangles2nd
          << "\n  Quads: " << anInfo.nbQuads << "\n  2nd order quads: " << anInfo.nbQuads2nd
          << "\n  Polygons: " << anInfo.nbPolygons << "\n3d elements: " << anInfo.nbElems3d
          << "\n  Tetrahedra: " << anInfo.nbTetrahedrons << "\n  2nd order tetrahedra: " << anInfo.nbTetrahedrons2nd
          << "\n  Pyramids: " << anInfo.nbPyramids << "\n  2nd order pyramids: " << anInfo.nbPyramids2nd
          << "\n  Prisms: " << anInfo.nbPrisms << "\n  2nd order prisms: " << anInfo.nbPrisms2nd
          << "\n  Hexahedra: " << anInfo.nbHexahedrons << "\n  2nd order hexahedra: " << anInfo.nbHexahedrons2nd
          << "\n  Polyhedra: " << anInfo.nbPolyhedrons << "\nOuter surface elements: " << anInfo.nbElemsSurf
          << "\n  Triangles: " << anInfo.nbTrisSurf << "\n  2nd order triangles: " << anInfo.nbTris2ndSurf
          << "\n  Quads: " << anInfo.nbQuadsSurf << "\n  2nd order quads: " << anInfo.nbQuads2ndSurf
          << "\n  Polygons: " << anInfo.nbPolsSurf << "\nAxes-aligned bounding box"
          << "\nX-range: " << anInfo.boundsMin.x() << " " << anInfo.boundsMax.x()
          << "\nY-range: " << anInfo.boundsMin.y() << " " << anInfo.boundsMax.y()
          << "\nZ-range: " << anInfo.boundsMin.z() << " " << anInfo.boundsMax.z() << "\n";
    return true;
}

}
