#ifndef SIMMESH_MODEL_HEADERFILE
#define SIMMESH_MODEL_HEADERFILE

#include <SimMesh_Edge.h>
#include <SimMesh_Triangle.h>
#include <SimMesh_Vertex.h>
#include <SimMesh_VertexDataList.h>
#include <SimMesh_VertexPairContraction.h>

#include <Bnd_B3f.hxx>
#include <Standard_OutOfRange.hxx>

//! This class describes the mesh model prepared for simplification.
//!
//! The method SimMesh::ModelFromShape() is a sample of filling in of the model
//! based on a triangulated shape.
//!
//! The method SimMesh::ShapeFromModel is a sample of how the results of
//! simplification can be extracted to create a new shape with triangulation.
class SimMesh_Model : public Standard_Transient {
public:
    //! Main constructor.
    //! @param[in] theNbVertsReserve amount of expected vertexes to reserve memory
    //! @param[in] theNbTriaReserve amount of expected triangles to reserve memory
    //! @param[in] theNbEdgesReserve amount of expected edges to reserve memory
    Standard_EXPORT explicit SimMesh_Model(size_t theNbVertsReserve,
                                           size_t theNbTriaReserve  = 0,
                                           size_t theNbEdgesReserve = 0);

    //! Empty constructor.
    Standard_EXPORT SimMesh_Model();

    //! Return TRUE if vertex data includes normal attribute; TRUE by default.
    bool IsVertexNormals() const { return myIsVertNormals; }

    //! Set if vertex data includes normal attribute.
    void SetVertexNormals(bool theIsNormals) { myIsVertNormals = theIsNormals; }

    //! Return TRUE if vertex data includes texture coordinates attribute; FALSE by default.
    bool IsVertexTextureCoords() const { return myIsVertTCoords; }

    //! Set if vertex data includes texture coordinates attribute.
    void SetVertexTextureCoords(bool theIsTCoords) { myIsVertTCoords = theIsTCoords; }

    //! Return TRUE if vertex data includes color attribute; FALSE by default.
    bool IsVertexColors() const { return myIsVertColors; }

    //! Set if vertex data includes color attribute.
    void SetVertexColors(bool theIsColors) { myIsVertColors = theIsColors; }

    //! Return TRUE if triangle data includes color attribute; FALSE by default.
    bool IsTriangleColors() const { return myIsTriaColors; }

    //! Set if triangle data includes color attribute.
    void SetTriangleColors(bool theIsColors) { myIsTriaColors = theIsColors; }

    //! Return number of vertices.
    size_t GetNbVertices() const { return myVertices.size(); }

    //! Return number of triangles.
    size_t GetNbTriangles() const { return myTriangles.size(); }

    //! Return number of edges.
    size_t GetNbEdges() const { return myEdges.size(); }

    //! Return vertex.
    const SimMesh_Vertex& GetVertex(SimMesh_Id theIndex) const { return myVertices[theIndex]; }

    //! Return triangle.
    const SimMesh_Triangle& GetTriangle(SimMesh_Id theIndex) const { return myTriangles[theIndex]; }

    //! Return edge.
    const SimMesh_Edge& GetEdge(SimMesh_Id theIndex) const { return myEdges[theIndex]; }

public:
    //! Return vertex data.
    const SimMesh_VertexDataList& GetVertexDataList(SimMesh_Id theIndex) const { return myVertDataLists[theIndex]; }

    //! Return vertex data.
    SimMesh_VertexDataList& ChangeVertexDataList(SimMesh_Id theIndex) { return myVertDataLists[theIndex]; }

    //! Return vertex attributes associated with the given triangle ID.
    //! Gets the first data if the triangle ID is not given.
    const SimMesh_Attributes& GetVertexAttrib(SimMesh_Id theIndex, SimMesh_Id theTriaId = SimMesh_InvalidId) const
    {
        return theTriaId != SimMesh_InvalidId ? (GetVertexDataList(theIndex).FindByTriangleId(theTriaId)->Attrib)
                                              : (GetVertexDataList(theIndex).begin()->Attrib);
    }

    //! Return non-const vertex attributes associated with the given triangle ID.
    //! Gets the first data if the triangle ID is not given.
    SimMesh_Attributes& ChangeVertexAttrib(SimMesh_Id theIndex, SimMesh_Id theTriaId = SimMesh_InvalidId)
    {
        return theTriaId != SimMesh_InvalidId ? (ChangeVertexDataList(theIndex).ChangeByTriangleId(theTriaId)->Attrib)
                                              : (ChangeVertexDataList(theIndex).begin()->Attrib);
    }

    //! Return triangle color.
    const SimMesh_Color& GetTriangleColor(SimMesh_Id theIndex) const { return myTriaColors[theIndex]; }

    //! Return triangle color.
    SimMesh_Color& ChangeTriangleColor(SimMesh_Id theIndex) { return myTriaColors[theIndex]; }

    //! Return the group ID of the given triangle
    int GetTriangleGroup(SimMesh_Id theIndex) const { return myTriangles[theIndex].GetGroup(); }

    //! Sets the group ID this triangle belongs to
    void SetTriangleGroup(SimMesh_Id theIndex, const int theGroup) { myTriangles[theIndex].SetGroup(theGroup); }

    //! Return the group ID of the given edge
    int GetEdgeGroup(SimMesh_Id theIndex) const { return myEdges[theIndex].GetGroup(); }

    //! Sets the group ID this edge belongs to
    void SetEdgeGroup(SimMesh_Id theIndex, const int theGroup) { myEdges[theIndex].SetGroup(theGroup); }

public:
    //! Add new vertex.
    Standard_EXPORT SimMesh_Id AddVertex(const SimMesh_Vertex& theVert);

    //! Add new triangle.
    Standard_EXPORT SimMesh_Id AddTriangle(const SimMesh_Triangle& theTria, bool theToLink = true);

    //! Add new edge.
    Standard_EXPORT SimMesh_Id AddEdge(const SimMesh_Edge& theEdge, bool theToLink = true);

    //! Make invalid free vertices (those that are not connected with any triangles)
    Standard_EXPORT void InvalidateFreeVertices();

    //! Return TRUE if vertex is valid.
    bool IsVertexValid(SimMesh_Id theVertId) const { return myVertices[theVertId].IsValid(); }

    //! Return TRUE if triangle is valid.
    bool IsTriangleValid(SimMesh_Id theTriaId) const { return myTriangles[theTriaId].IsValid(); }

    //! Return TRUE if edge is valid.
    bool IsEdgeValid(SimMesh_Id theEdgeId) const { return myEdges[theEdgeId].IsValid(); }

public:
    //! Get triangle normal (unified) and area.
    Standard_EXPORT const SimMesh_Vec3d& GetTriangleNormal(SimMesh_Id theTriaId, double& theArea) const;

    //! Returns the bounding box (AABB) of the model
    Standard_EXPORT const Bnd_B3f& GetBndBox() const;

    //! Remove invalidated vertices.
    Standard_EXPORT void CompactVertices();

    //! Split vertices with attributes discontinuity.
    //! It is worth to call after simplification is done.
    Standard_EXPORT void SplitVertices();

    //! Returns maximal error of simplification.
    double GetMaxError() const { return myMaxError; }

    //! Checks model consistency
    Standard_EXPORT bool IsModelConsistent(Standard_OStream* theStr = nullptr) const;

    DEFINE_STANDARD_RTTI_INLINE(SimMesh_Model, Standard_Transient)

protected:
    //! Create contraction object for the given pair of vertices.
    Standard_EXPORT void createContraction(SimMesh_Id                     theVert1,
                                           SimMesh_Id                     theVert2,
                                           SimMesh_VertexPairContraction& theContraction);

    //! Performs contraction according to the given vertex pair contraction object.
    //! @param[in] theContraction The contraction description.
    //! @param[in] theTargetType The definition of the position of the new vertex.
    //! @param[in] theTargetPos The position in the case of optimized target.
    Standard_EXPORT void performContraction(const SimMesh_VertexPairContraction& theContraction,
                                            const SimMesh_TargetType             theTargetType,
                                            const SimMesh_Vec3d&                 theTargetPos);

    //! Merges vertex data from vertex 2 to vertex 1.
    Standard_EXPORT void mergeVertexData(const SimMesh_VertexPairContraction& theContraction,
                                         const SimMesh_TargetType             theTargetType,
                                         const SimMesh_Vec3d&                 theTargetPos);

    //! Removes dead triangles and edges after merging vertices. The vertices that have remained alive
    //! are put to array \p theLeftVertices.
    Standard_EXPORT void removeDeadEntities(const SimMesh_VertexPairContraction& theContraction,
                                            SimMesh_Id                           theLeftVertices[],
                                            int&                                 theNbLeftVert);

    //! Updates changed triangles and edges after merging vertices.
    Standard_EXPORT void updateChangedEntities(const SimMesh_VertexPairContraction& theContraction);

    //! Removes non-beam edges adjacent to the given vertices that are left without face support.
    Standard_EXPORT void removeEdgesLeftWithoutFace(const SimMesh_Id theVertices[], const int theNbVert);

    //! Gathers vertices adjacent to the given one.
    Standard_EXPORT void gatherAdjacentVertices(SimMesh_Id theVertId, SimMesh_IdList& theVerts);

    //! Gathers triangles and edges containing both vertices \p theA and \p theB.
    Standard_EXPORT void gatherEdgeNeighbors(SimMesh_Id      theA,
                                             SimMesh_Id      theB,
                                             SimMesh_IdList& theTriangles,
                                             SimMesh_IdList& theEdges);

    //! Gathers all triangles containing either vertex \p theA or vertex \p theB.
    Standard_EXPORT void gatherNeighboringTriangles(SimMesh_Id theA, SimMesh_Id theB, SimMesh_IdList& theTriangles);

    //! Marks with the given value all triangles containing the given vertex.
    Standard_EXPORT void markNeighborhood(SimMesh_Id theVertId, uint8_t theMark);

    //! Gathers all triangles containing the given vertex and having the mark = 0.
    //! All found triangles are marked again with 1 to avoid duplicates.
    Standard_EXPORT void gatherUnmarkedNeighborTriangles(SimMesh_Id theVertId, SimMesh_IdList& theTriangles);

    //! Gathers all edges containing the given vertex and having the mark = 0.
    //! All found edges are marked again with 1 to avoid duplicates.
    Standard_EXPORT void gatherUnmarkedNeighborEdges(SimMesh_Id theVertId, SimMesh_IdList& theEdges);

    //! Adds the given value to the mark of all triangles and edges containing the given vertex.
    Standard_EXPORT void markNeighborhoodDelta(SimMesh_Id theVertId, uint8_t theDelta);

    //! From the triangles containing the given vertex and having the mark != 0, gathers two groups,
    //! first is for triangles with mark < \p thePivot, and the second is for the others.
    Standard_EXPORT void splitMarkedNeighborTriangles(SimMesh_Id      theVertId,
                                                      uint8_t         thePivot,
                                                      SimMesh_IdList& theTrianglesLower,
                                                      SimMesh_IdList& theTrianglesHigher);

    //! From the edges containing the given vertex and having the mark != 0, gathers two groups,
    //! first is for edges with mark < \p thePivot, and the second is for the others.
    Standard_EXPORT void splitMarkedNeighborEdges(SimMesh_Id      theVertId,
                                                  uint8_t         thePivot,
                                                  SimMesh_IdList& theEdgesLower,
                                                  SimMesh_IdList& theEdgesHigher);

    //! Sets the given boolean value to all vertices of all triangles from the given list.
    Standard_EXPORT void markTrianglesVertices(const SimMesh_IdList& theTriangles, const bool theMark);

    //! Sets the given boolean value to all vertices of edges from the given list.
    Standard_EXPORT void markEdgesVertices(const SimMesh_IdList& theEdges, const bool theMark);

    //! Gathers unmarked vertices of triangles from the given list.
    Standard_EXPORT void gatherUnmarkedTrianglesVertices(const SimMesh_IdList& theTriangles, SimMesh_IdList& theVerts);

    //! Gathers unmarked vertices of edges from the given list.
    Standard_EXPORT void gatherUnmarkedEdgesVertices(const SimMesh_IdList& theEdges, SimMesh_IdList& theVerts);

    //! Removes the given triangle/edge by its invalidation, updates myVertDataLists and invalidates
    //! vertices becoming free.
    template<class TEntity>
    Standard_EXPORT void removeEntity(SimMesh_Id theEntId, std::vector<TEntity>& theContainer);

    //! Sets maximal error of simplification.
    void setMaxError(const double theVal) { myMaxError = theVal; }

    //! Check that valid triangles refer only valid vertices;
    //! check back references from vertices to triangles.
    Standard_EXPORT bool checkTrianglesConsistency(Standard_OStream* theStr) const;

    //! Check that valid entities (triangles or edges) refer only valid vertices;
    //! check back references from vertices to the same entities.
    template<class TEntity>
    Standard_EXPORT bool checkEntitiesConsistency(Standard_OStream*           theStr,
                                                  const std::vector<TEntity>& theContainer) const;

private:
    std::vector<SimMesh_Vertex>   myVertices;  //!< Vertices
    std::vector<SimMesh_Triangle> myTriangles; //!< Triangles
    std::vector<SimMesh_Edge>     myEdges;     //!< Edges

    std::vector<SimMesh_VertexDataList> myVertDataLists; //!< per-vertex data
    std::vector<SimMesh_Color>          myTriaColors;    //!< per-triangle colors

    bool myIsVertNormals = false; //!< flag indicating existence of per-vertex normal data
    bool myIsVertTCoords = false; //!< flag indicating existence of per-vertex texture coordinates data
    bool myIsVertColors  = false; //!< flag indicating existence of per-vertex color data
    bool myIsTriaColors  = false; //!< flag indicating existence of per-triangle color data

    mutable Bnd_B3f myBndBox;         //!< Bounding box of the model
    double          myMaxError = 0.0; //!< Maximal error after simplification

    friend class SimMesh_Simplifier;
};

#endif // SIMMESH_MODEL_HEADERFILE