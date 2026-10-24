
#ifndef _MESHPRESENTATIONSCALAR_HEADERFILE
#define _MESHPRESENTATIONSCALAR_HEADERFILE

#include <ViewCae/InteractiveCaeMeshPrsMode.h>
#include <ViewCae/MeshPresentation.h>
#include <ViewCae/MeshPropertyData.h>

#include <RWCae/MeshGroupProperty.h>

namespace ViewCae {
class InteractiveCaeMeshOwner; // forward declared class due to cyclic dependency

//! Class for mesh scalar presentation visual properties.
class MeshPrsVisScalarProps : public MeshPrsVisProps {
public:
    //! Return current element types to display.
    unsigned int ElementTypes() const { return myElementTypes; }

    //! Set current element types to display.
    void SetElementTypes(unsigned int theElementTypes) { myElementTypes = theElementTypes; }

    //! Return TRUE if 0d element are visible.
    bool ToShow0d() const { return (myElementTypes & (unsigned int)InteractiveCaeMeshElementType::Element0d) != 0; }

    //! Return TRUE if 1d elements are visible.
    bool ToShow1d() const { return (myElementTypes & (unsigned int)InteractiveCaeMeshElementType::Element1d) != 0; }

    //! Return TRUE if 2d element are visible.
    bool ToShow2d() const { return (myElementTypes & (unsigned int)InteractiveCaeMeshElementType::Face) != 0; }

    //! Return TRUE if 3d element are visible.
    bool ToShow3d() const { return (myElementTypes & (unsigned int)InteractiveCaeMeshElementType::Volume) != 0; }

    //! Return TRUE if nodes are visible.
    bool ToShowNodes() const { return (myElementTypes & (unsigned int)InteractiveCaeMeshElementType::Node) != 0; }

    //! Return TRUE if centroids are visible.
    bool ToShowCentroids() const
    {
        return (myElementTypes & (unsigned int)InteractiveCaeMeshElementType::Centroid) != 0;
    }

    //! Return TRUE if sharp edges are visible.
    bool ToShowSharpEdges() const
    {
        return (myElementTypes & (unsigned int)InteractiveCaeMeshElementType::SharpEdges) != 0
            || myPrsMode == InteractiveCaeMeshPrsMode::ShadedSharpEdges;
    }

    //! Return TRUE if free edges are visible.
    bool ToShowFreeEdges() const
    {
        return (myElementTypes & (unsigned int)InteractiveCaeMeshElementType::FreeEdges) != 0
            || myPrsMode == InteractiveCaeMeshPrsMode::ShadedFreeEdges;
    }

    //! Return mesh presentation mode.
    InteractiveCaeMeshPrsMode PresentationMode() const { return myPrsMode; }

    //! Set mesh presentation mode.
    void SetPresentationMode(InteractiveCaeMeshPrsMode theMode) { myPrsMode = theMode; }

    //! Return true if it is needed to render/select 3d mesh only by its surface
    //! and to not include internal parts, True by default.
    bool IsVolumeAsSurface() const { return myIsVolumeAsSurface; }

    //! Set if it is needed to render/select 3d mesh only by its surface
    //! and to not include internal parts.
    void SetVolumeAsSurface(bool theIsVolumeAsSurface) { myIsVolumeAsSurface = theIsVolumeAsSurface; }

    //! Return true if it is needed to show annotations for min/max values
    //! for current active property, False by default.
    bool ToShowMinMax() const { return myToShowMinMax; }

    //! Set if it is needed to show annotations for min/max values
    //! for current active property.
    void SetShowMinMax(bool theToShow) { myToShowMinMax = theToShow; }

    //! Return true if it is needed to show annotations for current active group property, False by default.
    bool ToShowGroupsInfo() const { return myToShowGroupsInfo; }

    //! Set if it is needed to show annotations for current active group property.
    void SetShowGroupsInfo(bool theToShow) { myToShowGroupsInfo = theToShow; }

protected:
    unsigned int              myElementTypes      = (unsigned int)InteractiveCaeMeshElementType::AllElemental;
    InteractiveCaeMeshPrsMode myPrsMode           = InteractiveCaeMeshPrsMode::FlatShaded;
    bool                      myIsVolumeAsSurface = true;
    bool                      myToShowMinMax      = false;
    bool                      myToShowGroupsInfo  = false;
};

//! Class for mesh minmax part parameters.
class MeshPrsMinMaxPartParams : public MeshPrsPartParams {
public:
    //! Return current property minmax range.
    const Graphic3d_Vec2d& MinMaxRange() const { return myMinMaxRange; }

    //! Set current property minmax range.
    void SetMinMaxRange(const Graphic3d_Vec2d& theRange) { myMinMaxRange = theRange; }

    //! Dump parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson) override;

    //! Restore parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged) override;

protected:
    Graphic3d_Vec2d myMinMaxRange;
};

//! Presentation for colored mesh data.
class MeshPresentationScalar : public MeshPresentation {
public:
    //! Constructor.
    Standard_EXPORT MeshPresentationScalar(const Handle(MeshPropertyData)&           thePropData,
                                           int                                       thePropIndex,
                                           const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                           const TCollection_AsciiString&            theId,
                                           bool                                      theToUpdateGeomId = false);

    //! Destructor.
    Standard_EXPORT virtual ~MeshPresentationScalar();

    //! Update current state.
    Standard_EXPORT virtual void UpdateAspects(const Handle(InteractiveCaeMeshOwner)& theOwner) override;

    //! Return type of presentation.
    virtual MeshPresentationType Type() const override { return MeshPresentationType::Scalar; }

    //! Return visual properties of scalar presentation.
    Handle(MeshPrsVisScalarProps) ScalarVisProps() const
    {
        return Handle(MeshPrsVisScalarProps)::DownCast(myPrsVisProps);
    }

    //! Calculate min/max values for current active property.
    Standard_EXPORT static bool ComputeMinMaxPoints(const Handle(ViewCaeGeom::MeshGeometry)&     theGeom,
                                                    const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                                    const Graphic3d_Vec2d&                        theRange,
                                                    NCollection_Vec2<Graphic3d_Vec3>&             theMinMaxPnts,
                                                    NCollection_Vec2<double>&                     theMinMaxValues);

    //! Calculate min/max values for current active property.
    Standard_EXPORT bool ComputeMinMaxPoints(NCollection_Vec2<Graphic3d_Vec3>& theMinMaxPnts,
                                             NCollection_Vec2<double>&         theMinMaxValues) const;

    //! Create group annotations in default position.
    Standard_EXPORT void FillGroupsInfo();

    //! Calculate positions for group annotations.
    Standard_EXPORT static void ComputeGroupsAreas(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                   const RWCae::MeshPropertyComponent&     theProp,
                                                   MapOfGroupAreas&                          theAreas);

    //! Calculate positions for group annotations.
    Standard_EXPORT void ComputeGroupsAreas(MapOfGroupAreas& theAreas) const;

    //! Return group's node that is closer to the center.
    Standard_EXPORT static int GetGroupCenterNode(const TColStd_PackedMapOfInteger& theGroup,
                                                  const Handle(RWCae::MeshData)&  theMeshData);

    //! Update presentation when corresponding properties were changed.
    Standard_EXPORT virtual void OnPropertiesUpdated(const NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps,
                                                     bool theCachedProps) override;

protected:
    //! Generate mesh parts.
    Standard_EXPORT virtual void updateParts(MeshParts& theParts) override;

    //! Check if any part should be updated.
    Standard_EXPORT virtual bool needToUpdateParts(const MeshParts& theParts) const override;

    //! Generate mesh sensitive parts.
    Standard_EXPORT virtual void updateSensParts(const Handle(InteractiveCaeMeshOwner)& theOwner) override;

    //! Update visibility of mesh elements.
    Standard_EXPORT void updateElementsVisibility();

    //! Update selectability of mesh elements.
    Standard_EXPORT void updateElementsSelectability();

    //! Update mesh elements polygon offsets.
    Standard_EXPORT void updatePolygonOffsets();

    //! Update used properties for each mesh part.
    Standard_EXPORT void updateUsedProps();

    //! Calculate min/max values for current active nodal property.
    Standard_EXPORT static void computeMinMaxPointsNodal(const Handle(RWCae::MeshData)&               theMeshData,
                                                         const Handle(ViewCaeGeom::MeshGeometry)&      theGeom,
                                                         const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                                         NCollection_Vec2<Graphic3d_Vec3>&              theMinMaxPnts,
                                                         NCollection_Vec2<double>&                      theMinMaxValues,
                                                         const Graphic3d_Vec2d&                         theRangeMinMax);

    //! Calculate min/max values for current active elemental property.
    Standard_EXPORT static void computeMinMaxPointsElemental(const Handle(RWCae::MeshData)&               theMeshData,
                                                             const Handle(ViewCaeGeom::MeshGeometry)&      theGeom,
                                                             const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                                             NCollection_Vec2<Graphic3d_Vec3>& theMinMaxPnts,
                                                             NCollection_Vec2<double>&         theMinMaxValues,
                                                             const Graphic3d_Vec2d&            theRangeMinMax);

    //! Calculate min/max values for current active elemental property.
    template<class Elem_T>
    Standard_EXPORT static void computeMinMaxPointsElementalT(const RWCae::MeshAdaptor<Elem_T>&            theMesh,
                                                              const Handle(ViewCaeGeom::MeshGeometry)&      theGeom,
                                                              const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                                              NCollection_Vec2<Graphic3d_Vec3>& theMinMaxPnts,
                                                              NCollection_Vec2<double>&         theMinMaxValues,
                                                              const Graphic3d_Vec2d&            theRangeMinMax);

    //! Calculate min/max values for current active elemental property.
    template<class Elem_T>
    Standard_EXPORT static void generateMinMaxPointsElementalT(const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                                                               const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                               NCollection_Vec2<Graphic3d_Vec3>&         theMinMaxPnts,
                                                               const NCollection_Vec2<int>& theResMinMaxIdx,
                                                               bool                         isNodeVec);

    //! Fill vector of nodes with connected nodes.
    Standard_EXPORT static void generateConnectedPoints(
        const Handle(RWCae::MeshData)&                 theMeshData,
        const Handle(RWCae::MeshGroupProperty)&        theProp,
        Handle(RWCae::MeshGroupProperty)&              theNodalProp,
        std::vector<std::vector<RWCae::MeshEntityId>>& theConnectedPoints);

    //! Fill vector of nodes with connected nodes.
    template<class Elem_T>
    Standard_EXPORT static void generateConnectedPointsT(
        const RWCae::MeshAdaptor<Elem_T>&              theMesh,
        const Handle(RWCae::MeshGroupProperty)&        theProp,
        const Handle(RWCae::MeshGroupProperty)&        theNodalProp,
        std::vector<std::vector<RWCae::MeshEntityId>>& theConnectedPoints);

    //! Calculate a group of connected nodes.
    Standard_EXPORT static void processGroupNodes(
        int                                                    theGroupId,
        const Handle(RWCae::MeshGroupProperty)&              theProp,
        NCollection_List<RWCae::MeshEntityId>&               theQueuedNodes,
        TColStd_PackedMapOfInteger&                            theGroupOfNodes,
        const std::vector<std::vector<RWCae::MeshEntityId>>& theConnectedPoints,
        std::vector<bool>&                                     theProcessedPoints);
};

} // namespace

#endif // _MESHPRESENTATIONSCALAR_HEADERFILE
