#ifndef MESHINFO_HEADERFILE
#define MESHINFO_HEADERFILE

#include <RWCae/MeshData.h>

#include <Graphic3d_Vec3.hxx>

namespace RWCae {
//! Information about cae mesh.
class MeshInfo {
public:
    int nbElems0d = 0;

    int nbElems1d = 0;

    int nbElems2d      = 0;
    int nbTriangles    = 0;
    int nbTriangles2nd = 0;
    int nbQuads        = 0;
    int nbQuads2nd     = 0;
    int nbPolygons     = 0;

    int nbElems3d         = 0;
    int nbTetrahedrons    = 0;
    int nbTetrahedrons2nd = 0;
    int nbPyramids        = 0;
    int nbPyramids2nd     = 0;
    int nbPrisms          = 0;
    int nbPrisms2nd       = 0;
    int nbHexahedrons     = 0;
    int nbHexahedrons2nd  = 0;
    int nbPolyhedrons     = 0;

    int nbElemsSurf    = 0;
    int nbTrisSurf     = 0;
    int nbTris2ndSurf  = 0;
    int nbQuadsSurf    = 0;
    int nbQuads2ndSurf = 0;
    int nbPolsSurf     = 0;

    int nbNodes = 0;

    Graphic3d_Vec3d boundsMin = Graphic3d_Vec3d(RealLast(), RealLast(), RealLast());
    Graphic3d_Vec3d boundsMax = Graphic3d_Vec3d(-RealLast(), -RealLast(), -RealLast());

public:
    //! Empty constructor.
    MeshInfo() {}

    //! Constructor.
    Standard_EXPORT explicit MeshInfo(const Handle(RWCae::MeshData)& theMeshData);
};

} // namespace

#endif // MESHINFO_HEADERFILE
