
#ifndef SIMMESH_SIMPLIFIER_HEADERFILE
#define SIMMESH_SIMPLIFIER_HEADERFILE

#include <SimMesh_Triangle.h>
#include <SimMesh_Model.h>
#include <SimMesh_VertexPairContraction.h>
#include <SimMesh_VertexPairHeap.h>
#include <SimMesh_VertexPairList.h>

#include <NCollection_CellFilter.hxx>
#include <Standard_TypeDef.hxx>

#include <algorithm>
#include <functional>
#include <list>
#include <vector>

class SimMesh_Quadric;

//! This class implements the simplification algorithm based on the principle
//! of vertex pair contraction.
//!
//! User should prepare a mesh model of type SimMesh_Model.
//! It can be created from a TopoDS_Shape with triangulations using
//! the method SimMesh::ModelFromShape().
//!
//! The algorithm sequentially removes the pairs of vertices from the model,
//! giving priority to the pairs that make the least distortion of the geometry.
//! It goes until one of the following stop conditions take place:<br>
//! - the resulting mesh has the required number of triangles, which is given
//!   by the ratio of target number of triangles to the initial number.<br>
//! - the simplification leads to exceeding the maximal allowed error (deviation).
//!
//! The algorithm takes care of both triangles and edges. For that, the model
//! has the methods to add triangles and edges. An edge can be connected
//! to a triangle by sharing the same vertices, or can be a separate edge.
//! A chain of separate edges form a representation of a 3D beam. So,
//! the algorithm can simplify wire models as well as triangle models, or any
//! combination of them.
//!
//! The algorithm takes care of saving the per-vertex attributes (normals, colors
//! and texture coordinates) and per-triangle colors. Also, the triangles can be assigned
//! to group IDs. At the end, the remaining triangles will preserve these assignments.
//! This allows restoring triangles and edges belongness to topological faces and edges.
//!
//! The algorithm can deal with not connected meshes as well. If two distinct
//! vertices from different triangles have the same 3D position they will be
//! merged into one vertex in the very early stage. At this, the attributes
//! of vertices (normal, texture coords and color) will be preserved on per-triangle basis.
//! After the algorithm is finished the vertices can be split by the method SplitVertices
//! of the model class in order to obtain distinct triangles with vertices as on input.
//!
//! The algorithm can work in one of two modes, aggregation and non-aggregation.
//! Aggregation mode is turned on by default. In this mode the non-connected pairs of
//! vertices can be contracted, in general with no regard to the distance between them.
//! But first there will be contracted real edges (vertices connected by a triangle or
//! an edge) having the length less than the distance between non-connected vertices.
//! This mode allows aggregataion of not-connected regions of meshes in one connected
//! region that then allows getting more quality mesh with greater simplification ratio.
//!
//! At the end of the process, before getting the result, it is recommented to call the
//! methods CompactVertices() and SplitVertices().
//!
//! The algorithm provides the caller with the error obtained in the output result
//! due to simplification. It somehow evaluates the measure of linear deviation
//! of the result from the initial model. There are 3 metrics:<br>
//! - Maximal Error <br>
//! - Maximal Deviation <br>
//! - Standard Deviation (root mean square) <br>
//! See more detailed description of them in corresponding methods.
//!
//! The algorithm is developed inspired by the following works: <br>
//! - M.Garland. Quadric-Based Polygonal Surface Simplification (1999)
//! (http://reports-archive.adm.cs.cmu.edu/anon/1999/CMU-CS-99-105.pdf)<br>
//! - C.Erikson. GAPS: General and Automatic PolygonalSimplification (1999)
//! (http://www.cs.unc.edu/techreports/98-033.pdf)
//! - H.Hoppe. New Quadric Metric for Simplifying Meshes with Appearance Attributes (2004)
//! (http://hhoppe.com/newqem.pdf)
class SimMesh_Simplifier {
public:
    //! Empty constructor.
    Standard_EXPORT SimMesh_Simplifier();

    //! Initializes algorithm with a mesh model.
    Standard_EXPORT void Initialize(const Handle(SimMesh_Model)& theModel);

    //! Sets the aggregation mode, it is when distinct triangle regions
    //! are permitted to join together. By default it is true.
    void SetAggregationMode(const bool theVal) { myAggregation = theVal; }

    //! Returns true if the aggregation mode is active.
    //! In this mode distinct triangle regions are permitted to join together.
    bool IsAggregationMode() const { return myAggregation; }

    //! Sets allow inversion flag. In aggregation mode setting this flag
    //! to true allows contraction of vertex pairs that lead to inversion of triangles.
    //! This relaxes simplification conditions and helps the algorithm
    //! quicker get the target. The default value is true.
    //! It is ignored in non-aggregation mode.
    void SetAllowInversion(const bool theVal) { myAllowInversion = theVal; }

    //! Returns true if inversion of triangles is allowed in aggregation mode.
    bool IsAllowInversion() const { return myAllowInversion; }

    //! Sets the max allowed error. The algorithm stops if the next contraction will
    //! lead to exceeding this error.
    //! By default this value is zero that disables this stop condition.
    void SetAllowedError(const double theValue) { myAllowedError = theValue; }

    //! Gets the max allowed error.
    double GetAllowedError() const { return myAllowedError; }

    //! Sets the target ratio. It is the expected ratio of the number of entities in
    //! simplified mesh to the number of entities in initial mesh. If the mesh contains
    //! triangles then only number of triangles is considered. Otherwise it is assumed
    //! that the mesh contains only edges and the number of edges is considered.
    //! By default the target ratio is zero that disables this stop condition.
    //! However, if both AllowedError and TargetRatio are zero the target ratio is set
    //! to 0.5 to facilitate default simplification rate.
    void SetTargetRatio(const double theValue) { myTargetRatio = theValue; }

    //! Gets the target ratio (expected number of triangles in simplified mesh divided
    //! by the number of triangles in initial mesh).
    double GetTargetRatio() const { return myTargetRatio; }

    //! Performs simplification.
    //! This method can be called several times reapeatedly (with the needed
    //! stop conditions set before each call) to generate series of LODs.
    //! @return true in case of success, false otherwise.
    Standard_EXPORT bool Perform();

    //! Returns maximal error of simplification.
    //! For a triangular model it includes maximum between:
    //!   - maximum deviation of vertices from planes of initial triangles;
    //!   - maximum of linear measure of change of area of triangles surrounding a vertex
    //!     (it is square root of change of area divided by M_PI).
    //! For a beam model it is a deviation of the new polyline vertices
    //! from the old polyline.
    double GetMaxError() const { return myMaxError; }

    //! Returns the number of vertex pair contractions led to triangles inversions.
    int NbInversions() const { return myNbInversions; }

public:
    //! Limit the algorithm by the number of simplification iterations. It is for debug purposes.
    //! Default value is 0 means no limit.
    void SetNbIterLimit(const int theValue) { myNbIterLimit = theValue; }

    //! Resume computation broken on iterations limit. It continues the work of
    //! the previous call of Perform().
    bool ResumePerform() { return perform(false); }

    //! Returns true if simplification target has been got. It is useful when
    //! SetNbIterLimit() was called.
    bool IsTargetGot()
    {
        return (myNbTargetEntities > 0 && myNbValidEntities <= myNbTargetEntities)
            || (myAllowedError > 0. && myMaxError >= myAllowedError);
    }

protected:
    //! Performs simplification until the number of mesh entities is reduced to myNbTargetEntities,
    //! deviation exceeds the allowed error or current iteration number is greater than the limit.
    //! If theNewStart is false than resume the previous work.
    Standard_EXPORT bool perform(bool theNewStart);

private:
    //! Class for spatial search of registered vertices
    //! located within a given distance from a given vertex.
    class VertexFinder {
    public:
        VertexFinder(const opencascade::handle<SimMesh_Model>& theModel,
                     const SimMesh_Id                          theCheckedVertId,
                     const SimMesh_IdList&                     theAvoidVertices,
                     const float                               theSqDistThreshold)
            : myModel(theModel),
              myVert(myModel->GetVertex(theCheckedVertId)),
              mySqDistThreshold(theSqDistThreshold),
              myAvoidVertices(theAvoidVertices),
              myGatheredVerts(10)
        {
        }

        //! Points dimension
        static constexpr int Dimension = 3;

        //! Target type
        using Target = SimMesh_Id;

        //! Points type
        using Point = SimMesh_Vertex;

        //! Access to co-ordinate
        static double Coord(int i, const SimMesh_Vertex& thePnt) { return thePnt[i]; }

        //! Auxiliary method to shift point by each coordinate on given value;
        //! useful for preparing a points range for Inspect with tolerance
        static SimMesh_Vertex Shift(const SimMesh_Vertex& thePnt, float theTol)
        {
            return SimMesh_Vertex(thePnt.x() + theTol, thePnt.y() + theTol, thePnt.z() + theTol);
        }

        //! Method called from the CellFilter for each candidate vertex
        NCollection_CellFilter_Action Inspect(const SimMesh_Id theOtherVertId)
        {
            if (std::find(myAvoidVertices.begin(), myAvoidVertices.end(), theOtherVertId) != myAvoidVertices.end()) {
                // this vertex is in avoid list
                return CellFilter_Keep;
            }
            const SimMesh_Vertex& anOtherVert = myModel->GetVertex(theOtherVertId);
            if ((myVert - anOtherVert).SquareModulus() < mySqDistThreshold) {
                myGatheredVerts.push_back(theOtherVertId);
            }
            return CellFilter_Keep;
        }

        //! Returns gathered vertices
        const SimMesh_IdList& GetGatheredVertices() const { return myGatheredVerts; }

        VertexFinder(const VertexFinder&) = delete;
        VertexFinder& operator=(const VertexFinder&) = delete;

    private:
        Handle(SimMesh_Model) myModel;
        const SimMesh_Vertex& myVert;
        float                 mySqDistThreshold;
        const SimMesh_IdList& myAvoidVertices;
        SimMesh_IdList        myGatheredVerts;
    };

    //! Initializes vertex pairs as the first thing the algorithm does.
    //! @return True in the case of success.
    bool initVertexPairs(NCollection_CellFilter<VertexFinder>& theCells);

    //! Increase distance theshold and reinitializes vertex pairs.
    //! @return True in the case of success.
    bool increaseThersholdAndReinit(NCollection_CellFilter<VertexFinder>& theCells);

    //! Checks a vertex pair if we can contract it. Invalidates it and updates the heap in case of invalidity.
    //! @return true in case of check pass.
    bool checkVertexPair(SimMesh_VertexPair* thePair, bool& theIsInversion);

    //! Computes quadric for a triangle with the given Id.
    void calcTriangleQuadric(SimMesh_Id theTriaId, SimMesh_Quadric& theQuadric);

    //! Computes per-vertex quadrics (as summ of per-triangle quadrics).
    void createQuadrics();

    //! Iterates on all pairs of connected vertices in the model and performs a given function on them.
    void iterateConnectedVertexPairs(std::function<void(SimMesh_Id, SimMesh_Id)> theFunc);

    //! Adds real edges as vertex pair contractions.
    //! Returns the minimal edge length.
    float addContractionsFromEdges();

    //! Adds virtual edges (non-adjacent vertex pairs for contraction).
    void addContractionsFromVirtualEdges(NCollection_CellFilter<VertexFinder>& theCells, const float theDistThreshold);

    //! Gathers vertices adjacent to the given one according to existing vertex pairs.
    void gatherAdjacentVerticesFromVertexPairs(SimMesh_Id theVertId, SimMesh_IdList& theVerts);

    //! Adds boundary constraints.
    void addBoundaryConstraints();

    //! Defines boundary constraint for a pair of vertices.
    void defineBoundaryConstraint(SimMesh_Id theVert1, SimMesh_Id theVert2, const SimMesh_IdList& theTriangles);

    //! Restores infos of all pairs that have a backup and update the heap.
    void restoreVertexPairInfos();

    //! Computes (update) vertex pair information and put it (update) in the heap.
    void computeVertexPairInfo(SimMesh_VertexPair* thePair);

    //! Computes the error of the given vertex pair using quadrics.
    double computeErrorByQuadrics(SimMesh_VertexPair* thePair);

    //! Computes the error of the given vertex pair based on distortion of edges geometry.
    double computeErrorByEdges(SimMesh_VertexPair* thePair);

    //! Computes the error caused by distortion of edges geometry due to moving
    //! the vertex theVertId from old position to new position.
    double computeErrorByMovement(const SimMesh_Id     theVertId,
                                  const SimMesh_Vec3d& theOldPos,
                                  const SimMesh_Vec3d& theNewPos,
                                  const SimMesh_Id     theVertIdExclude);

    //! Returns true if this pair contraction will cause some triangles inversion.
    //! Checking for inversion prevents normal flips and folding triangles
    //! one with another.
    bool checkInversion(SimMesh_VertexPair* thePair, double& theDeltaArea);

    //! Organizes contraction of a vertex pair.
    void contractVertexPair(const SimMesh_VertexPair* thePair, const float theError, const bool theIsInversion);

    //! Performs contraction operation.
    void doContraction(const SimMesh_VertexPair* thePair);

    //! Prepares structures before contraction operation.
    void updateBeforeContraction();

private:
    Handle(SimMesh_Model) myModel; //!< mesh model for simplification

    double myTargetRatio  = 0.;
    double myAllowedError = 0.;
    double myMaxError     = 0.;

    int64_t myNbTargetEntities = 0;
    int64_t myNbValidEntities  = 0;

    int myNbInversions = 0;
    int myNbIterLimit  = 0;
    int myIterNum      = 0;

    float myDistThreshold = 0.f;
    float myBndExtent     = 0.f;

    bool myAggregation    = true;
    bool myAllowInversion = true;
    bool myWasContraction = false;

    std::list<SimMesh_VertexPair>       myVertexPairs;     //!< storage of vertex pairs
    std::vector<SimMesh_VertexPairList> myVertexPairLinks; //!< per-vertex pairs
    SimMesh_VertexPairHeap              myVertexPairHeap;  //!< heap of pairs to contract sorted in by quadric error

    SimMesh_VertexPairContraction myContraction;
    SimMesh_IdList                myTmpTriangles;
    SimMesh_IdList                myTmpVertices;
};

#endif // SIMMESH_SIMPLIFIER_HEADERFILE