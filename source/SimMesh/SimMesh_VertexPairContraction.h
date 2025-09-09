#ifndef SIMMESH_VERTEXPAIRCONTRACTION_HEADERFILE
#define SIMMESH_VERTEXPAIRCONTRACTION_HEADERFILE

#include <SimMesh_IdList.h>

//! Struct containing info necessary to apply a vertex pair contraction.
struct SimMesh_VertexPairContraction {
    SimMesh_Id Vert1 = 0;
    SimMesh_Id Vert2 = 0;

    SimMesh_IdList DeadTriangles;
    SimMesh_IdList DeltaTriangles;
    size_t         DeltaTrianglesPivot = 0;

    SimMesh_IdList DeadEdges;
    SimMesh_IdList DeltaEdges;
    size_t         DeltaEdgesPivot = 0;
};

#endif // SIMMESH_VERTEXPAIRCONTRACTION_HEADERFILE