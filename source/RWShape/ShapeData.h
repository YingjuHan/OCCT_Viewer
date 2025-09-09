#ifndef RWShape_SHAPEDATA_HEADERFILE
#define RWShape_SHAPEDATA_HEADERFILE

#include <RWShape/ShapeDataExplorer.h>

#include <OSD_FileSystem.hxx>
#include <RWMesh_NodeAttributes.hxx>
#include <SelectMgr_ViewerSelector.hxx>
#include <TColStd_HPackedMapOfInteger.hxx>
#include <TopTools_SequenceOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

//! @namespace RWShape
//! `RWShape` is a C++ library implementing basic data structures
//! and file storage formats for CAD (B-Rep) geometry.
//!
//! The data source of B-Rep definition is defined by `TopoDS_Shape` class from OCCT.
//! Native file formats supported for loading data in 3D Viewer:
//! - **glTF**, [open ISO-standardized file format](https://www.khronos.org/gltf/)
//!   supporting triangulated surfaces, assembly structures and PBR materials;
//! - **BREP**, [open file format](https://dev.opencascade.org/doc/overview/html/specification__brep_format.html)
//!   supporting OCCT B-Rep geometry+topology definition.
//!   Binary and text versions of file format are supported for reading.
namespace RWShape
{

    //! RWShape::ShapeData object state.
    enum DataLoadState
    {
        DataLoadState_Initial = 0, //!< initial state
        DataLoadState_InProcess,
        DataLoadState_Loaded,
        DataLoadState_Removed
    };

    //! Structure holding shape + index.
    struct EnumeratedShape
    {
        TopoDS_Shape Shape;
        int Index = -1;
    };

    //! Class defines presentable shape data.
    class ShapeData : public Standard_Transient
    {
    public:
        //! Constructor for loading external shape.
        Standard_EXPORT explicit ShapeData(const TCollection_AsciiString &theId,
                                           const Handle(OSD_FileSystem) &theFileSystem = Handle(OSD_FileSystem)());

        //! Constructor from existing shape.
        Standard_EXPORT ShapeData(const TCollection_AsciiString &theId,
                                  const TopoDS_Shape &theShape,
                                  const DataLoadState theState = DataLoadState::DataLoadState_Loaded);

        //! Copy constructor.
        Standard_EXPORT ShapeData(const TCollection_AsciiString &theNewId, const Handle(ShapeData) & theOther);

    public:
        //! Return id.
        const TCollection_AsciiString &Id() const { return myId; }

        //! Set new id. Should be done only before usage of this ShapeData as Owner id!
        void SetId(const TCollection_AsciiString &theId) { myId = theId; }

        //! Get all shapes as a single shape or compound of root shapes.
        Standard_EXPORT TopoDS_Shape GetSingleShape(bool theToKeepList = false) const;

        //! Return sequence of root shapes.
        const TopTools_SequenceOfShape &Shapes() const { return myRootShapes; }

        //! Return sequence of root shapes.
        TopTools_SequenceOfShape &ChangeShapes() { return myRootShapes; }

        //! Return shape attributes.
        const RWMesh_NodeAttributeMap &AttributeMap() const { return myAttribMap; }

        //! Return shape attributes.
        RWMesh_NodeAttributeMap &ChangeAttributeMap() { return myAttribMap; }

        //! Return metadata.
        const TColStd_IndexedDataMapOfStringString &Metadata() const { return myMetadata; }

        //! Return metadata.
        TColStd_IndexedDataMapOfStringString &ChangeMetadata() { return myMetadata; }

        //! Set a single shape.
        Standard_EXPORT void SetShape(const TopoDS_Shape &theShape);

        //! Add shapes to the provided map
        //! @param[out] theShapeMap map to fill in
        //! @return number of deferred faces
        Standard_EXPORT int GetFaces(TopTools_IndexedMapOfShape &theShapeMap) const;

        //! Calculate and return map of shapes with subshapes indices ranges.
        Standard_EXPORT const SubShapesBoundsMap &GetSubShapesBoundsMap(bool theToUpdate = false);

    public:
        //! Reset file system.
        void ResetFileSystem() { myFileSystem.Nullify(); }

        //! FileSystem.
        const Handle(OSD_FileSystem) & FileSystem() const { return myFileSystem; }

    public:
        //! Check if triangulation is loaded.
        bool IsLoaded() const { return myState != DataLoadState_InProcess && myState != DataLoadState_Initial; }

        //! Return current object state
        DataLoadState State() const { return myState; }

        //! Set state
        void SetState(DataLoadState theState) { myState = theState; }

        //! Return cached bounding box.
        const Bnd_Box &BoundingBox() const { return myBndBox; }

        //! Return cached count of triangles.
        int NbTriangles() const { return myNbTris; }

        //! Return cached count of triangle nodes.
        int NbTriangleNodes() const { return myNbTriNodes; }

        //! Calculate shape stats.
        Standard_EXPORT void UpdateStats();

    public:
        //! Return the index map of subshapes of specified type.
        //! @param[in] theType subshape type to find
        //! @param[in] theIsFree if TRUE, only free subshapes will be considered
        //! @param[in] theParent parent shape for exploring subshapes or NULL to explore all roots
        //! @param[in] theVisibility include only visible (1), hidden (0) or any subshape
        Standard_EXPORT Handle(TColStd_HPackedMapOfInteger) GetSubshapesIndexes(
            const TopAbs_ShapeEnum theType,
            const bool theIsFree,
            const TopoDS_Shape &theParent = TopoDS_Shape(),
            const int theVisibility = -1) const;

        //! Find subshapes defined by index map of subshapes of specified type.
        Standard_EXPORT bool GetSubshapes(std::vector<EnumeratedShape> &theShapes,
                                          const TopAbs_ShapeEnum theType,
                                          const Handle(TColStd_HPackedMapOfInteger) & theIndices) const;

        //! Find subshape of specified type and index.
        Standard_EXPORT TopoDS_Shape GetSubshape(const TopAbs_ShapeEnum theType, const int theIndex) const;

        //! Return TRUE if subshape should be hidden.
        Standard_EXPORT bool IsSubshapeHidden(const TopoDS_Shape &theShape,
                                              const float theMinOpacity = ShortRealSmall()) const;

        //! Return TRUE if subshape should be hidden.
        Standard_EXPORT bool IsSubshapeHidden(const TopAbs_ShapeEnum theType,
                                              const int theIndex,
                                              const float theMinOpacity = ShortRealSmall()) const;

        //! Set subshape hidden flag.
        //! @param[in] theType subshapes type to find
        //! @param[in] theIndices subshape indices to modify or NULL (not empty!) to apply to all subshapes
        //! @param[in] theIsHidden hidden (TRUE) or visible (FALSE) state to set to specified subshapes
        //! @param[in] theToInvertOthers if TRUE, apply inverted visibility flag to all other subshapes
        Standard_EXPORT bool SetSubshapeHidden(const TopAbs_ShapeEnum theType,
                                               const Handle(TColStd_HPackedMapOfInteger) & theIndices,
                                               const bool theIsHidden,
                                               const bool theToInvertOthers);

        //! Set subshape hidden flag.
        Standard_EXPORT bool SetSubshapeHidden(const TopoDS_Shape &theSubshape, const bool theIsHidden);

        //! Return subshape color.
        Standard_EXPORT bool GetSubshapeColor(Quantity_ColorRGBA &theColor,
                                              const TopAbs_ShapeEnum theType,
                                              const int theIndex) const;

        //! Return subshape color.
        Standard_EXPORT bool GetSubshapeColor(Quantity_ColorRGBA &theColor, const TopoDS_Shape &theShape) const;

        //! Set subshape color. If map is NULL (not empty!), then color will be applied to all subshapes of specified type.
        Standard_EXPORT bool SetSubshapeColor(const Quantity_ColorRGBA *theColor,
                                              const TopAbs_ShapeEnum theType,
                                              const Handle(TColStd_HPackedMapOfInteger) & theIndices);

        //! Set subshape color.
        Standard_EXPORT bool SetSubshapeColor(const TopoDS_Shape &theSubshape, const Quantity_ColorRGBA *theColor);

    private:
        TCollection_AsciiString myId;
        TopTools_SequenceOfShape myRootShapes; //!< sequence of result root shapes
        RWMesh_NodeAttributeMap myAttribMap;   //!< shape attributes
        TColStd_IndexedDataMapOfStringString myMetadata;
        Handle(OSD_FileSystem) myFileSystem; //!< stored filesystem

        Bnd_Box myBndBox;
        DataLoadState myState = DataLoadState_Initial;
        int myNbTris = 0;
        int myNbTriNodes = 0;

        SubShapesBoundsMap mySubshapesBounds;
    };

} // namespace

#endif // RWShape_SHAPEDATA_HEADERFILE