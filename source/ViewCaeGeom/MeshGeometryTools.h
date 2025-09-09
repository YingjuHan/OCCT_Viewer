

#ifndef _MESHGEOMETRYTOOLS_HEADERFILE
#define _MESHGEOMETRYTOOLS_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

#include <NCollection_Map.hxx>

#include <functional>

namespace ViewCaeGeom {
//! Class for storing set of bitmasks.
template<int N>
class BitMaskSet {
public:
    //! Constructor.
    explicit BitMaskSet(int theNbValues = 0) { Init(theNbValues); }

    //! Init.
    void Init(int theNbValues)
    {
        myNbValues = theNbValues;
        for (int i = 0; i < N; i++) {
            myMaskSet[i].clear();
            myMaskSet[i].resize((size_t)(Ceiling((double)myNbValues / myNbBits)), 0);
        }
    }

    //! Check if bit is set.
    bool IsSet(int theIdx, int theMaskId = 0) const
    {
        return (getByte(theIdx, theMaskId) & (1 << (theIdx % myNbBits))) != 0;
    }

    //! Set bit.
    void Set(int theIdx, int theMaskId = 0) { getByte(theIdx, theMaskId) |= (1 << (theIdx % myNbBits)); }

    //! Set all bits.
    void SetAll(int theMaskId = 0)
    {
        for (uint8_t& aVal : myMaskSet[theMaskId]) {
            aVal = std::numeric_limits<uint8_t>::max();
        }
    }

    //! Unset bit.
    void Unset(int theIdx, int theMaskId = 0) { getByte(theIdx, theMaskId) &= ~(1 << (theIdx % myNbBits)); }

    //! Unset all bits.
    void UnsetAll(int theMaskId = 0)
    {
        for (uint8_t& aVal : myMaskSet[theMaskId]) {
            aVal = 0;
        }
    }

    //! Perform logical AND with other set.
    void Intersected(const BitMaskSet& theOther)
    {
        if (myNbValues != theOther.myNbValues) {
            return;
        }

        for (int i = 0; i < N; i++) {
            for (size_t j = 0; j < myMaskSet[i].size(); j++) {
                myMaskSet[i][j] &= theOther.myMaskSet[i][j];
            }
        }
    }

    //! Check if all stored values are zero.
    bool IsZero() const
    {
        for (int i = 0; i < N; i++) {
            for (uint8_t aVal : myMaskSet[i]) {
                if (aVal != 0) {
                    return false;
                }
            }
        }

        return true;
    }

    //! Return number of values.
    int NbValues() const { return myNbValues; }

    //! Hash value, for Map interface.
    static int HashCode(const BitMaskSet& theElem, const int theUpper)
    {
        uint32_t aHashCode = 0;
        for (int i = 0; i < N; i++) {
            for (uint8_t aByte : theElem.myMaskSet[i]) {
                aHashCode += ::HashCode(aByte, theUpper);
            }
        }
        return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
    }

    //! Matching two instances, for Map interface.
    static bool IsEqual(const BitMaskSet& theElem1, const BitMaskSet& theElem2)
    {
        if (theElem1.myNbValues != theElem2.myNbValues) {
            return false;
        }
        for (int i = 0; i < N; i++) {
            if (theElem1.myMaskSet[i] != theElem2.myMaskSet[i]) {
                return false;
            }
        }
        return true;
    }

protected:
    //! Return corresponding byte for specified bit.
    uint8_t getByte(int theIdx, int theMaskId) const { return myMaskSet[theMaskId][theIdx / myNbBits]; }

    //! Return corresponding byte for specified bit.
    uint8_t& getByte(int theIdx, int theMaskId) { return myMaskSet[theMaskId][theIdx / myNbBits]; }

protected:
    static constexpr int myNbBits = std::numeric_limits<uint8_t>::digits;

    std::vector<uint8_t> myMaskSet[N];
    int                  myNbValues = 0;
};

//! Auxiliary methods for mesh geometry.
class MeshGeometryTools {
public:
    static constexpr int m_two = 2;

    using BitMaskSignedSet   = BitMaskSet<m_two>;
    using ListOfContours     = NCollection_List<NCollection_List<Graphic3d_Vec2ub>>;
    using MapOfContours      = NCollection_DataMap<BitMaskSignedSet, ListOfContours, BitMaskSignedSet>;
    using TypedMapOfContours = NCollection_DataMap<int, MapOfContours>;
    using AddedNodeMap       = NCollection_DataMap<MeshInterpolatedNode, RWCae::MeshEntityId, MeshInterpolatedNode>;
    using NodesAdjElems      = std::pair<std::vector<BitMaskSet<1>>, std::vector<RWCae::MeshEntityId>>;

public:
    //! Dump geometry chain parameters to Json string.
    Standard_EXPORT static bool DumpToJson(const Handle(MeshGeometry)& theGeom, std::string& theJsonStr);

    //! Restore geometry chain from Json string.
    Standard_EXPORT static bool RestoreFromJson(const std::string&               theJsonStr,
                                                const Handle(RWCae::MeshData)& theMeshData,
                                                const Handle(MeshBvhData)&       theBvhData,
                                                int                              theTime,
                                                MapOfMeshGeometries&             theGeoms,
                                                Handle(MeshGeometry)&            theGeom,
                                                bool&                            theIsChanged);

    //! Collect mesh properties that are used in any geometry.
    Standard_EXPORT static void GatherUsedProps(const MapOfMeshGeometries&                          theGeoms,
                                                NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps);

    //! Tool for mesh element cutting by nodes values and a reference value.
    template<class Elem_T>
    class MeshElementCutter {
    public:
        //! Constructor.
        MeshElementCutter(const RWCae::MeshAdaptor<Elem_T>&                     theAdaptor,
                          const Elem_T&                                           theElem,
                          const std::function<double(RWCae::MeshElemEntityId)>& theNodeValueFunc,
                          TypedMapOfContours*                                     theCachedContours,
                          double                                                  theCutValue,
                          double                                                  theConfusion)
            : myAdaptor(&theAdaptor),
              myElem(&theElem),
              myNodeValueFunc(theNodeValueFunc),
              myCachedContours(theCachedContours),
              myCutValue(theCutValue),
              myConfusion(theConfusion)
        {
        }

        //! Perform cutting extraction as set of contours.
        Standard_EXPORT bool Perform();

        //! Add to mesh data cutting contours as 2d elements.
        Standard_EXPORT void AddCutSurf(const Handle(MeshGeometry)&      theGeom,
                                        const Handle(RWCae::MeshData)& theOutMeshData,
                                        MeshGeometry::NodesMap*          theGenNodes,
                                        MeshGeometry::ElementsMap*       theGenElems,
                                        AddedNodeMap*                    theAddedNodes,
                                        RWCae::MeshEntityId            theLocalIndex) const;

        //! Add to mesh data cut element as polygon/polyhedron.
        Standard_EXPORT bool AddCutElem(const Handle(MeshGeometry)&      theGeom,
                                        const Handle(RWCae::MeshData)& theOutMeshData,
                                        const NodesAdjElems&             theNodesAdjElems,
                                        MeshGeometry::NodesMap*          theGenNodes,
                                        MeshGeometry::ElementsMap*       theGenElems,
                                        AddedNodeMap*                    theAddedNodes,
                                        RWCae::MeshEntityId            theLocalIndex) const;

        //! Return cutting contours.
        const std::vector<std::vector<MeshInterpolatedNode>>& CutContourNodes() const { return myCutNodes; }

        //! Return adjacent elements in case of failed element cutting.
        const std::vector<RWCae::MeshEntityId>& AdjElemToCheck() const { return myAdjElemToCheck; }

    protected:
        using EdgeElemMap =
            NCollection_IndexedDataMap<MeshElemEdge, std::pair<MeshElemEdge, Graphic3d_Vec2ub>, MeshElemEdge>;
        using NodePositionMap = NCollection_DataMap<RWCae::MeshEntityId, Graphic3d_Vec3>;
        using ElemFaces       = std::vector<std::pair<std::vector<MeshInterpolatedNode>, bool>>;
        using EntityToElemId  = NCollection_DataMap<RWCae::MeshEntityId, RWCae::MeshElemEntityId>;
        using EdgeToLerpPoint = NCollection_DataMap<Graphic3d_Vec2u, MeshInterpolatedNode, EdgeHasher>;
        using NodeToLerpPoint = NCollection_DataMap<RWCae::MeshEntityId, MeshInterpolatedNode>;

    protected:
        //! Calculate values in the element's nodes.
        Standard_EXPORT bool generateElementValues();

        //! Generate cutting contours for element with the same values in all nodes.
        Standard_EXPORT void generateWholeElementContours(ListOfContours* theListOfContours);

        //! Generate list of cutting contours.
        Standard_EXPORT bool generateContours(ListOfContours* theListOfContours);

        //! Gather edges with opposite values.
        Standard_EXPORT void gatherIntersectedEdges(EdgeElemMap*                      theEdges,
                                                    std::vector<std::vector<double>>* theFaceNodesValues);

        //! Get adjacent edge for the specified face-edge.
        Standard_EXPORT int getFaceAdjEdge(const EdgeElemMap&                      theEdges,
                                           const std::vector<std::vector<double>>& theFaceNodesValues,
                                           RWCae::MeshElemEntityId               theNbFaceNodes,
                                           MeshElemEdge*                           theEdge);

        //! Generate cutting contours for 2d element.
        Standard_EXPORT bool generateContours2d(ListOfContours* theListOfContours);

        //! Gather and process cutting contours.
        Standard_EXPORT void generateCutPoints(const ListOfContours& theListOfContours);

    protected:
        //! Gather positions for all element's nodes.
        Standard_EXPORT void fillNodesPositionMap(const Handle(MeshGeometry)& theGeom, NodePositionMap* theNodes) const;

        //! Add to mesh data nodes of cut element.
        Standard_EXPORT void addCutElemNodes(const Handle(MeshGeometry)&         theGeom,
                                             const Handle(RWCae::MeshData)&    theOutMeshData,
                                             const NodePositionMap&              theNodes,
                                             MeshGeometry::NodesMap*             theGenNodes,
                                             AddedNodeMap*                       theAddedNodes,
                                             std::vector<RWCae::MeshEntityId>* theElemNodes,
                                             AddedNodeMap*                       theLerpToId,
                                             EntityToElemId*                     theIdToIdx) const;

        //! Return adjacent element for specified element's face in bit mask form.
        Standard_EXPORT void getAdjElemsMask(const NodesAdjElems&      theNodesAdjElems,
                                             RWCae::MeshElemEntityId theFaceIter,
                                             RWCae::MeshElemEntityId theNbFaceNodes,
                                             BitMaskSet<1>*            theMask) const;

        //! Extract adjacent element id from bit mask.
        Standard_EXPORT bool getFirstAdjElem(const NodesAdjElems&   theNodesAdjElems,
                                             const BitMaskSet<1>&   theMask,
                                             RWCae::MeshEntityId* theIdx) const;

        //! Cut element's face and return cutting result as nodes contour.
        Standard_EXPORT void extractCutElemFace(const NodesAdjElems&      theNodesAdjElems,
                                                const EdgeToLerpPoint&    theContourMapEdges,
                                                const NodeToLerpPoint&    theContourMapNodes,
                                                RWCae::MeshElemEntityId theFaceIter,
                                                bool                      theIs2d,
                                                ElemFaces*                theFaces) const;

        //! Add cutting contours as separate element faces.
        Standard_EXPORT void extractCutElemCutFace(ElemFaces& theFaces) const;

        //! Add to mesh data cutting result of 2d element as polygon.
        Standard_EXPORT void addCutElem2d(const Handle(MeshGeometry)&      theGeom,
                                          const Handle(RWCae::MeshData)& theOutMeshData,
                                          const ElemFaces&                 theFaces,
                                          const AddedNodeMap&              theLerpToId,
                                          RWCae::MeshEntityId            theLocalIndex,
                                          MeshGeometry::ElementsMap*       theGenElems) const;

        //! Add to mesh data cutting result of 3d element as polyhedron + its outer surface.
        Standard_EXPORT void addCutElem3d(const Handle(MeshGeometry)&               theGeom,
                                          const Handle(RWCae::MeshData)&          theOutMeshData,
                                          const std::vector<RWCae::MeshEntityId>& theElemNodes,
                                          const ElemFaces&                          theFaces,
                                          const AddedNodeMap&                       theLerpToId,
                                          const EntityToElemId&                     theIdToIdx,
                                          RWCae::MeshEntityId                     theLocalIndex,
                                          MeshGeometry::ElementsMap*                theGenElems) const;

    protected:
        const RWCae::MeshAdaptor<Elem_T>* myAdaptor;
        const Elem_T*                       myElem;

        std::function<double(RWCae::MeshElemEntityId)> myNodeValueFunc;

        std::vector<double> myNodeValues;
        TypedMapOfContours* myCachedContours;
        BitMaskSignedSet    myValueMask;

        std::vector<std::vector<MeshInterpolatedNode>> myCutNodes;

        mutable std::vector<RWCae::MeshEntityId> myAdjElemToCheck;

        double myCutValue  = 0.0;
        double myAvValue   = 0.0;
        double myConfusion = 0.0;
    };
};

#ifdef Standard_UNUSED
    // suppress CodeCheck warnings considering template exports as 'forward declarations'
    #define ViewCaeGeom_ExportTemplate
#endif
} // namespace

#endif // _MESHGEOMETRYTOOLS_HEADERFILE
