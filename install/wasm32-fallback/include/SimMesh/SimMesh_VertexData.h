#ifndef SIMMESH_VERTEXDATA_HEADERFILE
#define SIMMESH_VERTEXDATA_HEADERFILE

#include <SimMesh_Attributes.h>
#include <SimMesh_IdList.h>
#include <SimMesh_Quadric.h>

//! Data associated with a vertex.
//! It contains the list of adjacent triangle IDs,
//! vertex attributes and the quadric corresponding to this set of
//! vertex position and attributes.
struct SimMesh_VertexData {
    SimMesh_IdList     Triangles;
    SimMesh_IdList     Edges;
    SimMesh_Attributes Attrib;
    SimMesh_Quadric    Quadric;

    explicit SimMesh_VertexData(const SimMesh_Attributes& theAttrib) : Triangles(0), Edges(0), Attrib(theAttrib) {}
};

#endif // SIMMESH_VERTEXDATA_HEADERFILE
