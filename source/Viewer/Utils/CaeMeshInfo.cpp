#include "CaeMeshInfo.h"

#include <RWCae/MeshAdaptor.h>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : CaeMeshInfo
// ================================================================
CaeMeshInfo::CaeMeshInfo(const Handle(RWCae::MeshData)& theMeshData)
{
    nbElems0d = theMeshData->NbElems0d();

    nbElems1d = 0;

    nbTriangles    = theMeshData->NbElems(RWCae::MeshElemType::Triangle);
    nbTriangles2nd = theMeshData->NbElems(RWCae::MeshElemType::Triangle2nd);
    nbQuads        = theMeshData->NbElems(RWCae::MeshElemType::Quad);
    nbQuads2nd     = theMeshData->NbElems(RWCae::MeshElemType::Quad2nd);
    nbPolygons     = theMeshData->NbElems(RWCae::MeshElemType::Polygon);
    nbElems2d      = nbTriangles + nbTriangles2nd + nbQuads + nbQuads2nd + nbPolygons;

    nbTetrahedrons    = theMeshData->NbElems(RWCae::MeshElemType::Tetrahedron);
    nbTetrahedrons2nd = theMeshData->NbElems(RWCae::MeshElemType::Tetrahedron2nd);
    nbPyramids        = theMeshData->NbElems(RWCae::MeshElemType::Pyramid);
    nbPyramids2nd     = theMeshData->NbElems(RWCae::MeshElemType::Pyramid2nd);
    nbPrisms          = theMeshData->NbElems(RWCae::MeshElemType::Prism);
    nbPrisms2nd       = theMeshData->NbElems(RWCae::MeshElemType::Prism2nd);
    nbHexahedrons     = theMeshData->NbElems(RWCae::MeshElemType::Hexahedron);
    nbHexahedrons2nd  = theMeshData->NbElems(RWCae::MeshElemType::Hexahedron2nd);
    nbPolyhedrons     = theMeshData->NbElems(RWCae::MeshElemType::Polyhedron);
    nbElems3d = nbTetrahedrons + nbTetrahedrons2nd + nbPyramids + nbPyramids2nd + nbPrisms + nbPrisms2nd + nbHexahedrons
              + nbHexahedrons2nd + nbPolyhedrons;

    nbTrisSurf     = theMeshData->NbSurfElems(RWCae::MeshElemType::Triangle);
    nbTris2ndSurf  = theMeshData->NbSurfElems(RWCae::MeshElemType::Triangle2nd);
    nbQuadsSurf    = theMeshData->NbSurfElems(RWCae::MeshElemType::Quad);
    nbQuads2ndSurf = theMeshData->NbSurfElems(RWCae::MeshElemType::Quad2nd);
    nbPolsSurf     = theMeshData->NbSurfElems(RWCae::MeshElemType::Polygon);
    nbElemsSurf    = nbTrisSurf + nbTris2ndSurf + nbQuadsSurf + nbQuads2ndSurf + nbPolsSurf;

    nbNodes = theMeshData->NbNodes();

    const Bnd_Box& anAABB = theMeshData->CachedMinMax();
    if (!anAABB.IsVoid()) {
        gp_Pnt pntMin(anAABB.CornerMin());
        gp_Pnt pntMax(anAABB.CornerMax());
        boundsMin.SetValues(pntMin.X(), pntMin.Y(), pntMin.Z());
        boundsMax.SetValues(pntMax.X(), pntMax.Y(), pntMax.Z());
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(CaeMeshInfo)
{
    emscripten::value_object<CaeMeshInfo>("CaeMeshInfo")
        .field("nbElems0d", &CaeMeshInfo::nbElems0d)
        .field("nbElems1d", &CaeMeshInfo::nbElems1d)
        .field("nbElems2d", &CaeMeshInfo::nbElems2d)
        .field("nbTriangles", &CaeMeshInfo::nbTriangles)
        .field("nbTriangles2nd", &CaeMeshInfo::nbTriangles2nd)
        .field("nbQuads", &CaeMeshInfo::nbQuads)
        .field("nbQuads2nd", &CaeMeshInfo::nbQuads2nd)
        .field("nbPolygons", &CaeMeshInfo::nbPolygons)
        .field("nbTetrahedrons", &CaeMeshInfo::nbTetrahedrons)
        .field("nbTetrahedrons2nd", &CaeMeshInfo::nbTetrahedrons2nd)
        .field("nbPyramids", &CaeMeshInfo::nbPyramids)
        .field("nbPyramids2nd", &CaeMeshInfo::nbPyramids2nd)
        .field("nbPrisms", &CaeMeshInfo::nbPrisms)
        .field("nbPrisms2nd", &CaeMeshInfo::nbPrisms2nd)
        .field("nbHexahedrons", &CaeMeshInfo::nbHexahedrons)
        .field("nbHexahedrons2nd", &CaeMeshInfo::nbHexahedrons2nd)
        .field("nbPolyhedrons", &CaeMeshInfo::nbPolyhedrons)
        .field("nbElems3d", &CaeMeshInfo::nbElems3d)
        .field("nbNodes", &CaeMeshInfo::nbNodes)
        .field("boundsMin", &CaeMeshInfo::boundsMin)
        .field("boundsMax", &CaeMeshInfo::boundsMax);
}
#endif

} // namespace

