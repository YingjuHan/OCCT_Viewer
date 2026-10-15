#ifndef _INTERACTIVECAEMESHOWNER_HEADERFILE
#define _INTERACTIVECAEMESHOWNER_HEADERFILE

#include <ViewCae/InteractiveCaeMeshSelMode.h>
#include <ViewCae/MeshAdaptorSensitive.h>
#include <ViewCae/MeshPresentations.h>
#include <ViewCae/MeshPropertyData.h>
#include <ViewCae/MeshSubEntity.h>

#include <ViewCaeGeom/MeshBvhData.h>

#include <ViewBase/InteractiveOwner.h>

#include <RWCae/MeshData.h>

#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Select3D_SensitivePrimitiveArray.hxx>
#include <Select3D_SensitiveSet.hxx>
#include <TColStd_DataMapOfIntegerInteger.hxx>

namespace ViewCae {
//! Enum for mesh sub entity type.
//! Warning! Mesh elements should be in the same order as in MeshElemType enum.
enum class MeshEntityType {
    UNKNOWN = -1,
    NODE,
    ELEMENT_0D,
    TRIANGLE,
    TRIANGLE_2ND,
    QUADRANGLE,
    QUADRANGLE_2ND,
    POLYGON,
    TETRAHEDRON,
    TETRAHEDRON_2ND,
    PYRAMID,
    PYRAMID_2ND,
    PRISM,
    PRISM_2ND,
    HEXAHEDRON,
    HEXAHEDRON_2ND,
    POLYHEDRON,
    LABEL_GROUP
};

constexpr int MeshEntityTypeElemLower = (int)MeshEntityType::ELEMENT_0D;

//! Class represents an owner for ViewCae::InteractiveCaeMesh presentation.
class InteractiveCaeMeshOwner : public ViewBase::InteractiveOwner {
public:
    //! Constructor.
    Standard_EXPORT explicit InteractiveCaeMeshOwner(const Handle(RWCae::MeshData)& theMeshData);

    //! Destructor.
    Standard_EXPORT virtual ~InteractiveCaeMeshOwner();

    //! Return copy of this presentation; default implementation returns NULL.
    Standard_EXPORT virtual Handle(ViewBase::InteractiveOwner) Clone(const TCollection_AsciiString& theNewId) override;

    //! Highlights selectable object's presentation.
    Standard_EXPORT virtual void HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                                  const Handle(Prs3d_Drawer)&               theStyle,
                                                  const Standard_Integer                    theMode) override;

    //! Removes highlighting from the owner.
    Standard_EXPORT virtual void Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                           const Standard_Integer                    theMode) override;

    //! Return true if owner dynamic hilighting presentation should be re-computed on mouse move.
    virtual bool IsForcedHilight() const override
    {
        return (mySelMode & (int)InteractiveCaeMeshSelMode::Box) == 0 && myToHighlightEntities;
    }

public:
    //! Returns assigned mesh data.
    const Handle(RWCae::MeshData)& GetMeshData() const { return myPresentations->GetMeshData(); }

    //! Returns auxiliary BVH data.
    const Handle(ViewCaeGeom::MeshBvhData)& GetMeshBvhData() const { return myPresentations->GetMeshBvhData(); }

    //! Returns list of used mesh geometry chains.
    ViewCaeGeom::MapOfMeshGeometries& Geometries() { return myPresentations->Geometries(); }

    //! Returns list of used mesh geometry chains.
    const ViewCaeGeom::MapOfMeshGeometries& Geometries() const { return myPresentations->Geometries(); }

    //! Calculate (if needed) and return bounding box.
    Standard_EXPORT const Bnd_Box& BoundingBox();

    //! Request update of bounding box.
    void SetBoundingBoxToUpdate() { myBoundingBoxUpdated = false; }

public:
    //! Create new mesh presentation of specified type with specified id (generates a new one if empty).
    Standard_EXPORT const TCollection_AsciiString& CreatePrs(MeshPresentationType           theType,
                                                             const TCollection_AsciiString& theId = "");

    //! Remove mesh presentation by its id.
    Standard_EXPORT void RemovePresentation(const TCollection_AsciiString& thePrsId);

    //! Return mesh presentation by its id.
    Standard_EXPORT Handle(MeshPresentation) Presentation(const TCollection_AsciiString& thePrsId) const;

    //! Return mesh presentation by its index inside list.
    Standard_EXPORT Handle(MeshPresentation) PresentationByIndex(int theIndex) const;

    //! Return list of mesh presentations.
    ListOfMeshPresentations& Presentations() { return myPresentations->Presentations(); }

    //! Return list of mesh presentations.
    const ListOfMeshPresentations& Presentations() const { return myPresentations->Presentations(); }

    //! Return presentation parameters in Json form.
    std::string GetPrsParamsJson(const TCollection_AsciiString& thePrsId, MeshPartType thePart) const;

    //! Return true if custom geometry is used for mesh presentations.
    bool ToUseGenGeom() const { return myToUseGenGeom; }

    //! Define if custom geometry is used for mesh presentations.
    void SetUseGenGeom(bool theToUse) { myToUseGenGeom = theToUse; }

    //! Return true if a separate presentation object that generates custom geometry is used (for testing).
    bool ToUseTestCachePrs() const { return !myTestCachePrs.IsNull(); }

    //! Define if a separate presentation object that generates custom geometry is used (for testing).
    Standard_EXPORT void SetUseTestCachePrs(bool theToUse);

    //! Return testing separate presentation object that generates custom geometry.
    const Handle(MeshPresentations)& TestCachePrs() { return myTestCachePrs; }

    //! Set custom geometry for specified presentation.
    void SetPartGenMeshData(const TCollection_AsciiString&   thePrsId,
                            int                              thePartId,
                            const Handle(RWCae::MeshData)& theMeshData)
    {
        myPresentations->SetPartGenMeshData(thePrsId, thePartId, theMeshData);
    }

public:
    //! Return mesh property data.
    const Handle(MeshPropertyData)& PropertyData() const { return myPropertyData; }

    //! Return active mesh property for specified mesh presentation.
    Standard_EXPORT RWCae::MeshPropertyComponent Property(const TCollection_AsciiString& thePrsId) const;

    //! Set active mesh property for specified mesh presentation.
    Standard_EXPORT void SetProperty(const TCollection_AsciiString&           thePrsId,
                                     const Handle(RWCae::MeshNVecProperty)& theProperty);

    //! Set index of active mesh property component for specified mesh presentation.
    Standard_EXPORT void SetPropertyComponentIndex(const TCollection_AsciiString& thePrsId, const int theIndex);

    //! Update presentations when corresponding properties were changed.
    Standard_EXPORT void OnPropertiesUpdated(const NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps,
                                             bool theCachedProps = false);

    //! Return true if properties data is stored in MDB file and can be loaded/unloaded on demand.
    bool ToCacheProperties() const { return myPresentations->ToCacheProperties(); }

    //! Define if properties data is stored in MDB file and can be loaded/unloaded on demand.
    void SetCacheProperties(bool theToCache) { myPresentations->SetCacheProperties(theToCache); }

public:
    //! Return active time step used to retrieve properties from mesh data.
    //! -1 means not defined time step, the first available will be used.
    int ActiveTimeStep() const { return myActiveTimeStep; }

    //! Set active time step used to retrieve properties from mesh data.
    //! Update all used properties/geometries/presentations.
    Standard_EXPORT bool SetActiveTimeStep(const int theTimeStep);

    //! Return available time steps calculated from properties stored in the mesh data.
    Standard_EXPORT const std::vector<std::pair<int, double>>& AvailableTimeSteps();

    //! Set custom available time steps (or auto-calculate it if theTimeSteps is empty).
    Standard_EXPORT void SetOwnTimeSteps(const std::vector<std::pair<int, double>>& theTimeSteps);

public:
    //! Return active vector property for specified mesh presentation.
    Standard_EXPORT Handle(RWCae::MeshNVecProperty) DeformedProperty(const TCollection_AsciiString& thePrsId) const;

    //! Set active vector property for specified mesh presentation.
    Standard_EXPORT void SetDeformedProperty(const TCollection_AsciiString&           thePrsId,
                                             const Handle(RWCae::MeshNVecProperty)& theProperty);

    //! Return scale coefficient for specified mesh presentation.
    Standard_EXPORT double DeformedScaleCoeff(const TCollection_AsciiString& thePrsId) const;

    //! Set scale coefficient for specified mesh presentation.
    Standard_EXPORT void SetDeformedScaleCoeff(const TCollection_AsciiString& thePrsId, double theCoeff);

    //! Return clip plane used to filter mesh elements for specified mesh presentation.
    Standard_EXPORT Handle(Graphic3d_ClipPlane) ClipPlane(const TCollection_AsciiString& thePrsId) const;

    //! Set clip plane used to filter mesh elements for specified mesh presentation.
    Standard_EXPORT void SetClipPlane(const TCollection_AsciiString&     thePrsId,
                                      const Handle(Graphic3d_ClipPlane)& theClipPlane);

    //! Check whether mesh clipping cuts or filters elements.
    Standard_EXPORT bool ToCutOnClipPlane(const TCollection_AsciiString& thePrsId) const;

    //! Define whether mesh clipping cuts or filters elements.
    Standard_EXPORT void SetCutOnClipPlane(const TCollection_AsciiString& thePrsId, bool theToCut);

    //! Return slice planes used to cut mesh elements for specified mesh presentation.
    Standard_EXPORT Graphic3d_SequenceOfHClipPlane SlicePlanes(const TCollection_AsciiString& thePrsId) const;

    //! Set slice planes used to cut mesh elements for specified mesh presentation.
    Standard_EXPORT void SetSlicePlanes(const TCollection_AsciiString&        thePrsId,
                                        const Graphic3d_SequenceOfHClipPlane& theSlicePlanes);

    //! Return property used to calculate iso surfaces for specified mesh presentation.
    Standard_EXPORT RWCae::MeshPropertyComponent IsoProperty(const TCollection_AsciiString& thePrsId) const;

    //! Set property used to calculate iso surfaces for specified mesh presentation.
    Standard_EXPORT void SetIsoProperty(const TCollection_AsciiString&        thePrsId,
                                        const RWCae::MeshPropertyComponent& theIsoProperty);

    //! Return values used to calculate iso surfaces for specified mesh presentation.
    Standard_EXPORT std::vector<double> IsoValues(const TCollection_AsciiString& thePrsId) const;

    //! Set values used to calculate iso surfaces for specified mesh presentation.
    Standard_EXPORT void SetIsoValues(const TCollection_AsciiString& thePrsId, const std::vector<double>& theIsoValues);

    //! Return visible MeshData blocks for specified mesh presentation.
    Standard_EXPORT Handle(RWCae::MeshDataBlock) VisibleBlocks(const TCollection_AsciiString& thePrsId) const;

    //! Set visibility of MeshData blocks for specified mesh presentation.
    Standard_EXPORT bool SetBlocksVisible(const TCollection_AsciiString&        thePrsId,
                                          const Handle(RWCae::MeshDataBlock)& theBlocks,
                                          bool                                  theVisible);

public:
    //! Set new selection mode.
    Standard_EXPORT virtual void SetSelectionMode(const int theMode) override;

    //! Enable/disable exclusive box selection mode.
    Standard_EXPORT void EnableBoxSelectionMode(bool theToEnable);

    //! Return selection mode for specified presentation.
    Standard_EXPORT int GetPrsSelectionMode(const TCollection_AsciiString& thePrsId);

    //! Set selection mode for specified presentation.
    Standard_EXPORT void SetPrsSelectionMode(const TCollection_AsciiString& thePrsId, const int theMode);

    //! Check if selection mode is acceptable.
    virtual bool AcceptSelectionMode(const int theMode) const override
    {
        return ((unsigned int)theMode & ~(unsigned int)InteractiveCaeMeshSelModeMask) == 0;
    }

    typedef NCollection_Map<MeshSubEntity, MeshSubEntity> MapOfSelectedEntities;

    //! Return detected entity (node or element).
    const MeshSubEntity& DetectedSubEntity() const { return myDetectedEntity; }

    //! Return map of selected entities.
    const MapOfSelectedEntities& SelectedSubEntities() const { return mySelectedEntities; }

    //! Return map of selected entities.
    MapOfSelectedEntities& SelectedSubEntities() { return mySelectedEntities; }

    //! Highlight picked sub entities.
    Standard_EXPORT void HighlightSubEntities(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                              const bool                                theIsSelection);

    //! Check if mesh entities should be detected/selected.
    bool ToHighlightEntities() const { return myToHighlightEntities; }

    //! Define if mesh entities should be detected/selected.
    void SetHighlightEntities(bool theToHighlight) { myToHighlightEntities = theToHighlight; }

public:
    //! Detect entity's elements.
    Standard_EXPORT void DetectEntity(const Handle(Select3D_SensitiveEntity)& theEnt,
                                      const Graphic3d_Vec3&                   thePnt,
                                      MeshSubEntity*                          theDet,
                                      MapOfSelectedEntities*                  theSel) const;

    //! Struct for storing temporary maps that will be converted to graphic3d primitive arrays.
    struct SubEntityPrimitiveArrays {
        NCollection_List<Graphic3d_Vec3>  ElementNodes;
        NCollection_List<Graphic3d_Vec3i> ElementTrgs;
        NCollection_List<Graphic3d_Vec2i> ElementEdges;
        NCollection_List<Graphic3d_Vec3>  Nodes;
        bool                              IsWholePrs = false;
    };

    typedef NCollection_DataMap<TCollection_AsciiString, SubEntityPrimitiveArrays> PrsSubEntityPrimitiveArrays;

    //! Fill maps of primitives for detected mesh elements.
    Standard_EXPORT void FillElementPrimitives(const Handle(RWCae::MeshData)& theMeshData,
                                               SubEntityPrimitiveArrays&        thePrimitives,
                                               RWCae::MeshEntityId            theElemIndex);

    //! Highlight picked sub entities.
    Standard_EXPORT void HighlightPrimitives(const TCollection_AsciiString&    thePrsId,
                                             const SubEntityPrimitiveArrays&   thePrimitives,
                                             const Handle(Prs3d_Presentation)& thePrs,
                                             bool                              theIsSelection);

protected:
    //! Detect sub entities of mesh sensitive sets.
    Standard_EXPORT void detectSubEntities(const Handle(AIS_InteractiveContext)& theCtx,
                                           MeshSubEntity*                        theDet,
                                           MapOfSelectedEntities*                theSel) const;

    //! Detect sub elements of mesh sensitive set.
    template<class Elem_T>
    Standard_EXPORT void detectSubElementsT(const opencascade::handle<MeshAdaptorSensitiveElement<Elem_T>>& theSens,
                                            const Graphic3d_Vec3&                                           thePnt,
                                            MeshSubEntity*                                                  theDet,
                                            MapOfSelectedEntities* theSel) const;

    //! Process selection of the mesh element.
    template<class Elem_T>
    Standard_EXPORT void detectSubElementT(const opencascade::handle<MeshAdaptorSensitiveElement<Elem_T>>& theSens,
                                           const Graphic3d_Vec3&                                           thePnt,
                                           RWCae::MeshEntityId                                           theElemIdx,
                                           MeshSubEntity*                                                  theDet,
                                           MapOfSelectedEntities* theSel) const;

    //! Detect sub nodes of mesh sensitive set.
    Standard_EXPORT void detectSubNodes(const Handle(MeshAdaptorSensitiveNode)& theSens,
                                        const Graphic3d_Vec3&                   thePnt,
                                        MeshSubEntity*                          theDet,
                                        MapOfSelectedEntities*                  theSel) const;

    //! Process selection of the mesh node.
    Standard_EXPORT void detectSubNode(const Handle(MeshAdaptorSensitiveNode)& theSens,
                                       const Graphic3d_Vec3&                   thePnt,
                                       const int                               theNodeIndex,
                                       MeshSubEntity*                          theDet,
                                       MapOfSelectedEntities*                  theSel) const;

    //! Try to get original entity from local test cache
    //! if custom geometry is used for mesh presentations.
    Standard_EXPORT void retrieveEntityFromTestCachePrs(const MeshPresentation* thePrs, MeshSubEntity& theEntity) const;

    //! Fill maps of primitives for detected sub entities.
    Standard_EXPORT void fillSubEntityPrimitives(const MeshSubEntity&         theSubEntity,
                                                 PrsSubEntityPrimitiveArrays& thePrimitives,
                                                 const bool                   theIsSelection);

    //! Fill maps of primitives for detected mesh elements.
    template<class Elem_T>
    Standard_EXPORT void fillElementPrimitives(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                                               SubEntityPrimitiveArrays&           thePrimitives,
                                               RWCae::MeshEntityId               theElemIndex);

    //! Combine old and current selection using the selection scheme.
    Standard_EXPORT void applySelectionScheme(const MapOfSelectedEntities& thePrevSelected,
                                              AIS_SelectionScheme          theSelScheme);

    //! Update mesh owner selection part on detection changed.
    Standard_EXPORT void updateSelectionPart(const MeshSubEntity& theSubEntity);

    //! Update mesh presentations that use specified mesh geometry.
    Standard_EXPORT void updatePresentations(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom);

protected:
    //! Local picking info holder.
    class MeshLocalPickInfo;

protected:
    Handle(MeshPresentations) myPresentations;
    Handle(MeshPropertyData)  myPropertyData;

    Bnd_Box myBoundingBox;
    bool    myBoundingBoxUpdated = false;

    MeshSubEntity         myDetectedEntity;
    MapOfSelectedEntities mySelectedEntities;
    bool                  myToHighlightEntities = true;

    int                                 myActiveTimeStep = -1;
    std::vector<std::pair<int, double>> myTimeSteps;
    bool                                myIsOwnTimeSteps = false;

    Handle(MeshPresentations) myTestCachePrs;
    bool                      myToUseGenGeom = false;
};

} // namespace

#endif // _INTERACTIVECAEMESHDRAWER_HEADERFILE