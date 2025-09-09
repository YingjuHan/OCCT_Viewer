#ifndef _CAEMESHINFO_HEADERFILE
#define _CAEMESHINFO_HEADERFILE

#include "VecN.h"

#include <RWCae/MeshData.h>

namespace Viewer {
//! Information about cae mesh.
class CaeMeshInfo {
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

    Vec3 boundsMin = Vec3::InvalidPoint();
    Vec3 boundsMax = Vec3(Vec3::InvalidPoint().Multiplied(-1.0));

public:
    //! Empty constructor.
    CaeMeshInfo() {}

    //! Constructor.
    Standard_EXPORT CaeMeshInfo(const Handle(RWCae::MeshData)& theMeshData);
};

} // namespace

#endif // _CAEMESHINFO_HEADERFILE

