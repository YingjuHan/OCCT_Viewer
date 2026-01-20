#ifndef SIMMESH_VERTEXDATALIST_HEADERFILE
#define SIMMESH_VERTEXDATALIST_HEADERFILE

#include <SimMesh_VertexData.h>
#include <SimMesh_TargetType.h>

//! This class represents the list of vertex data assigned to a vertex
class SimMesh_VertexDataList : public std::vector<SimMesh_VertexData> {
public:
    //! Adds all quadrics in the list to the given sum
    Standard_EXPORT void SumQuadric(SimMesh_Quadric& theSum) const;

    //! Finds the vertex data containing the given triangle ID
    Standard_EXPORT const_iterator FindByTriangleId(const SimMesh_Id theTriaId) const;

    //! Finds the vertex data containing the given triangle ID
    Standard_EXPORT iterator ChangeByTriangleId(const SimMesh_Id theTriaId);

    //! Finds the vertex data containing the given edge ID
    Standard_EXPORT const_iterator FindByEdgeId(const SimMesh_Id theEdgeId) const;

    //! Finds the vertex data containing the given edge ID
    Standard_EXPORT iterator ChangeByEdgeId(const SimMesh_Id theEdgeId);

    //! Merge vertex data from the other list:<br>
    //! - For vertex data containing dead triangles just join triangles lists.<br>
    //!   Update attributes according to interpolation parameter theInterParam.<br>
    //! - Add remaining vertex data.
    Standard_EXPORT void Merge(SimMesh_VertexDataList&  theOther,
                               const SimMesh_IdList&    theDeadTrias,
                               const SimMesh_IdList&    theDeadEdges,
                               const SimMesh_TargetType theTargetType,
                               const double             theInterParam);

private:
    //! Merge edges from other list.
    void mergeEdges(SimMesh_VertexDataList& theOther, const SimMesh_IdList& theDeadEdges);

    //! Merge triangles from other list.
    void mergeTriangles(SimMesh_VertexDataList&  theOther,
                        const SimMesh_IdList&    theDeadTrias,
                        const SimMesh_TargetType theTargetType,
                        const double             theInterParam);
};

#endif // SIMMESH_VERTEXDATALIST_HEADERFILE