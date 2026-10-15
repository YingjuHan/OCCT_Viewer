#ifndef _INTERACTIVECAEMESH_HEADERFILE
#define _INTERACTIVECAEMESH_HEADERFILE

#include <ViewCae/CaeMeshAnimation.h>
#include <ViewCae/CaeMeshStreamlinesBuilder.h>
#include <ViewCae/InteractiveCaeMeshDrawer.h>
#include <ViewCae/InteractiveCaeMeshOwner.h>
#include <ViewCae/InteractiveCaeMeshPrsMode.h>

#include <ViewBase/IInteractiveObject.h>
#include <ViewBase/InteractiveOwner.h>

#include <AIS_InteractiveObject.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <NCollection_IncAllocator.hxx>

//! @namespace ViewCae
//! `ViewCae` is a C++ library implementing presentations and tools for CAE mesh for FEA.
//!
//! The main presentation classes are `ViewCae::InteractiveCaeMesh` / `ViewCae::InteractiveCaeMeshOwner`,
//! as well as `ViewCae::InteractiveColorScale` / `ViewCae::InteractiveColorScaleOwner` for displaying a color bar.
//!
//! The data source of CAE mesh is defined by `RWBase::MeshData` class from `RWBase` library.
//!
//! Native file formats supported for loading data in 3D Viewer:
//! - **MDB**, optimized internal binary format
//!   implemented by `RWBase::MeshBinWriter` / `RWBase::MeshBinReader` storage tools.
//! - **GMSH 2.2**, ASCII file format implemented by `RWBase::GmshLoader`.
//!   WARNING! Current reader implementation has limited support of GMSH structures,
//!   and intended for testing purposes only - could be removed in future library releases.
namespace ViewCae {
//! Interactive object for CAE mesh data - defined by surface elements (triangles, quads),
//! volumetric elements (pyramids, hexahedrons, etc.) and color-mapped properties.
//!
//! The data source of CAE mesh is defined by `RWBase::MeshData` class from `RWBase` library.
class InteractiveCaeMesh : public AIS_InteractiveObject, public ViewBase::IInteractiveObject {
    InteractiveCaeMesh()                          = delete;
    InteractiveCaeMesh(const InteractiveCaeMesh&) = delete;
    InteractiveCaeMesh& operator=(const InteractiveCaeMesh&) = delete;

public:
    //! Default constructor.
    Standard_EXPORT explicit InteractiveCaeMesh(const Handle(ViewCae::InteractiveCaeMeshOwner)& theNewId);

    //! Destructor.
    Standard_EXPORT virtual ~InteractiveCaeMesh();

    //! Return the mesh owner of current interactive object
    const Handle(ViewCae::InteractiveCaeMeshOwner)& MeshOwner() const { return myMeshOwner; }

    //! Returns assigned mesh data.
    const Handle(RWCae::MeshData)& GetMeshData() const { return myMeshOwner->GetMeshData(); }

    //! Return TRUE for supported display mode - only 0th display mode is accepted.
    virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const Standard_OVERRIDE
    {
        return theMode == 0;
    }

    using AIS_InteractiveObject::BoundingBox;

    //! Return bounding box calculated by mesh data nodes.
    const Bnd_Box& BoundingBox() { return myMeshOwner->BoundingBox(); }

    //! Gather properties that are needed for computation.
    //! Request update for this object if all properties are loaded,
    //! and mark update as deferred otherwise.
    //! Should be called after UpdateAspects().
    Standard_EXPORT void SetDeferredUpdate();

    //! Called when properties needed for computation are not loaded.
    //! Parameters
    //! - properties time step;
    //! - string contains list of requested properties ids, separated by "\n".
    const std::function<void(int, const TCollection_AsciiString&)>& OnPropertiesRequestedCallback() const
    {
        return myOnPropertiesRequested;
    }

    //! Set a callback that is called when properties needed for computation are not loaded.
    void SetOnPropertiesRequestedCallback(const std::function<void(int, const TCollection_AsciiString&)>& theCallback)
    {
        myOnPropertiesRequested = theCallback;
    }

public:
    //! Return mesh data property by id and current used time step.
    const Handle(RWCae::MeshNVecProperty)& GetPropertyByActiveTimeStep(const TCollection_AsciiString& theId) const
    {
        return GetMeshData()->GetPropertyById(theId, ActiveTimeStep());
    }

    //! Return active mesh property for specified mesh presentation.
    RWCae::MeshPropertyComponent Property(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->Property(thePrsId);
    }

    //! Set active mesh property for specified mesh presentation.
    Standard_EXPORT void SetProperty(const TCollection_AsciiString&           thePrsId,
                                     const Handle(RWCae::MeshNVecProperty)& theProperty);

    //! Set index of active mesh property component for specified mesh presentation.
    Standard_EXPORT void SetPropertyComponentIndex(const TCollection_AsciiString& thePrsId, const int theIndex);

public:
    //! Return active time step used to retrieve properties from mesh data.
    //! -1 means not defined time step, the first available will be used.
    int ActiveTimeStep() const { return myMeshOwner->ActiveTimeStep(); }

    //! Set active time step used to retrieve properties from mesh data.
    //! Update all used properties/geometries/presentations.
    Standard_EXPORT void SetActiveTimeStep(const int theTimeStep);

    //! Return animation object.
    const Handle(CaeMeshAnimation)& Animation() const { return myAnimation; }

    //! Toggle caching for computed per time step primitives.
    Standard_EXPORT void SetCacheTimeSteps(bool theToCache, bool theToClearCached);

    //! Called when animation state (running/stopped) is changed.
    Standard_EXPORT void OnAnimationToggled(bool theIsRunning);

    //! Updates internal state after some property has had it's dependencies updated.
    //! Triggers loading of cached dependency properties.
    //! Should be called after changing dependencies in a mesh data that corresponds to this mesh.
    Standard_EXPORT void OnPropertyDependencyUpdate();

public:
    //! Return all used mesh color mappers.
    std::vector<Handle(MeshColorMapper)> ColorMappers() const { return myMeshOwner->PropertyData()->ColorMappers(); }

    //! Returns an object for mapping mesh values to colors.
    Handle(MeshColorMapper) ColorMapper(const RWCae::MeshPropertyComponent& theProp) const
    {
        return myMeshOwner->PropertyData()->ColorMapper(theProp);
    }

    //! Set an object for mapping mesh values to colors.
    Standard_EXPORT void SetColorMapper(const Handle(ViewCae::MeshColorMapper)& theColorMapper,
                                        const RWCae::MeshPropertyComponent&    theProp);

    //! Set a callback that is called when color mapper is created or its attached property is changed.
    void SetOnColorMapperChangeCallback(
        const std::function<void(const RWCae::MeshPropertyComponent&, const Handle(MeshColorMapper)&)>& theCallback)
    {
        myMeshOwner->PropertyData()->SetOnColorMapperChangeCallback(theCallback);
    }

public:
    //! Create new mesh presentation of specified type with specified id (generates a new one if empty).
    Standard_EXPORT const TCollection_AsciiString& CreatePresentation(MeshPresentationType           theType,
                                                                      const TCollection_AsciiString& theId = "");

    //! Remove mesh presentation by its id.
    Standard_EXPORT void RemovePresentation(const TCollection_AsciiString& thePrsId);

    //! Return list of mesh presentations.
    const ListOfMeshPresentations& Presentations() const { return myMeshOwner->Presentations(); }

    //! Return mesh presentation by its id.
    Handle(MeshPresentation) Presentation(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->Presentation(thePrsId);
    }

    //! Return mesh presentation by its index inside list.
    Handle(MeshPresentation) PresentationByIndex(int theIndex) const
    {
        return myMeshOwner->PresentationByIndex(theIndex);
    }

    //! Return mesh presentation by its id.
    template<class Prs_T>
    Handle(Prs_T) Presentation(const TCollection_AsciiString& thePrsId) const
    {
        return Handle(Prs_T)::DownCast(myMeshOwner->Presentation(thePrsId));
    }

    //! Set custom geometry for specified presentation.
    Standard_EXPORT void SetPartGenMeshData(const TCollection_AsciiString&   thePrsId,
                                            int                              thePartId,
                                            const Handle(RWCae::MeshData)& theMeshData);

    //! Called when custom presentation geometry needed for computation is not loaded.
    //! Parameters
    //! - presentation id;
    //! - part id;
    //! - Json contains presentation parameters.
    const std::function<void(const TCollection_AsciiString&, int, const std::string&)>& OnGenGeomRequestedCallback()
        const
    {
        return myOnGenGeomRequested;
    }

    //! Set a callback that is called when custom presentation geometry needed for computation is not loaded.
    void SetOnGenGeomRequestedCallback(
        const std::function<void(const TCollection_AsciiString&, int, const std::string&)>& theCallback)
    {
        myOnGenGeomRequested = theCallback;
    }

public:
    //! Return active vector property for specified mesh presentation.
    Handle(RWCae::MeshNVecProperty) DeformedProperty(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->DeformedProperty(thePrsId);
    }

    //! Set active vector property for specified mesh presentation.
    Standard_EXPORT void SetDeformedProperty(const TCollection_AsciiString&           thePrsId,
                                             const Handle(RWCae::MeshNVecProperty)& theDeformedProperty);

    //! Return scale coefficient for specified mesh presentation.
    double DeformedScaleCoeff(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->DeformedScaleCoeff(thePrsId);
    }

    //! Set scale coefficient for specified mesh presentation.
    Standard_EXPORT void SetDeformedScaleCoeff(const TCollection_AsciiString& thePrsId, double theDeformedScaleCoeff);

    //! Return clip plane used to filter mesh elements for specified mesh presentation.
    Handle(Graphic3d_ClipPlane) ClipPlane(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->ClipPlane(thePrsId);
    }

    //! Set clip plane used to filter mesh elements for specified mesh presentation.
    Standard_EXPORT void SetClipPlane(const TCollection_AsciiString&     thePrsId,
                                      const Handle(Graphic3d_ClipPlane)& theClipPlane);

    //! Check whether mesh clipping cuts or filters elements.
    bool ToCutOnClipPlane(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->ToCutOnClipPlane(thePrsId);
    }

    //! Define whether mesh clipping cuts or filters elements.
    Standard_EXPORT void SetCutOnClipPlane(const TCollection_AsciiString& thePrsId, bool theToCut);

    //! Return slice planes used to cut mesh elements for specified mesh presentation.
    Graphic3d_SequenceOfHClipPlane SlicePlanes(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->SlicePlanes(thePrsId);
    }

    //! Set slice planes used to cut mesh elements for specified mesh presentation.
    Standard_EXPORT void SetSlicePlanes(const TCollection_AsciiString&        thePrsId,
                                        const Graphic3d_SequenceOfHClipPlane& theSlicePlanes);

    //! Return property used to calculate iso surfaces for specified mesh presentation.
    RWCae::MeshPropertyComponent IsoProperty(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->IsoProperty(thePrsId);
    }

    //! Set property used to calculate iso surfaces for specified mesh presentation.
    Standard_EXPORT void SetIsoProperty(const TCollection_AsciiString&        thePrsId,
                                        const RWCae::MeshPropertyComponent& theIsoProperty);

    //! Return values used to calculate iso surfaces for specified mesh presentation.
    std::vector<double> IsoValues(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->IsoValues(thePrsId);
    }

    //! Set values used to calculate iso surfaces for specified mesh presentation.
    Standard_EXPORT void SetIsoValues(const TCollection_AsciiString& thePrsId, const std::vector<double>& theIsoValues);

    //! Return visible MeshData blocks for specified mesh presentation.
    Handle(RWCae::MeshDataBlock) VisibleBlocks(const TCollection_AsciiString& thePrsId) const
    {
        return myMeshOwner->VisibleBlocks(thePrsId);
    }

    //! Set visibility of MeshData blocks for specified mesh presentation.
    Standard_EXPORT void SetBlocksVisible(const TCollection_AsciiString&        thePrsId,
                                          const Handle(RWCae::MeshDataBlock)& theBlocks,
                                          bool                                  theVisible);

public:
    //! Return ids of vec3 properties.
    Standard_EXPORT std::vector<TCollection_AsciiString> GetAvailableVec3Properties() const;

    //! Return active property for vector mesh presentation.
    Standard_EXPORT Handle(RWCae::MeshNVecProperty) VectorProperty(const TCollection_AsciiString& thePrsId) const;

    //! Set active property for vector mesh presentation.
    Standard_EXPORT void SetVectorProperty(const TCollection_AsciiString&           thePrsId,
                                           const Handle(RWCae::MeshNVecProperty)& theProperty);

    //! Return vector scale coefficient for vector mesh presentation.
    Standard_EXPORT double VectorScaleCoeff(const TCollection_AsciiString& thePrsId) const;

    //! Set vector scale coefficient for vector mesh presentation.
    Standard_EXPORT void SetVectorScaleCoeff(const TCollection_AsciiString& thePrsId, double theCoeff);

    //! Return distribution type for vector mesh presentation.
    Standard_EXPORT MeshVectorDistrType VectorDistrType(const TCollection_AsciiString& thePrsId) const;

    //! Set distribution type for vector mesh presentation.
    Standard_EXPORT void SetVectorDistrType(const TCollection_AsciiString& thePrsId, MeshVectorDistrType theType);

    //! Return percentage of displayed vectors for vector mesh presentation, from 0.0 to 1.0.
    Standard_EXPORT double VectorSamplingRatio(const TCollection_AsciiString& thePrsId) const;

    //! Set percentage of displayed vectors for vector mesh presentation, from 0.0 to 1.0.
    Standard_EXPORT void SetVectorSamplingRatio(const TCollection_AsciiString& thePrsId, double theRatio);

public:
    //! Return active property for streamlines mesh presentation.
    Standard_EXPORT Handle(RWCae::MeshNVecProperty) StreamlinesProperty(
        const TCollection_AsciiString& thePrsId) const;

    //! Set active property for streamlines mesh presentation.
    Standard_EXPORT void SetStreamlinesProperty(const TCollection_AsciiString&           thePrsId,
                                                const Handle(RWCae::MeshNVecProperty)& theProperty);

    //! Return true if the initial points should be distributed regularly, otherwise they are placed
    //! randomly in the seed plane.
    Standard_EXPORT bool IsRegularStreamlines(const TCollection_AsciiString& thePrsId) const;

    //! Switch between regular (true) and random (false) distribution of the initial points in the seed plane.
    Standard_EXPORT void SetIsRegularStreamlines(const TCollection_AsciiString& thePrsId, bool theIsRegular);

    //! Return number of streamlines for streamlines mesh presentation.
    Standard_EXPORT int NbStreamlines(const TCollection_AsciiString& thePrsId) const;

    //! Set number of streamlines for streamlines mesh presentation.
    Standard_EXPORT void SetNbStreamlines(const TCollection_AsciiString& thePrsId, int theNbStreamlines);

    //! Return "U x V" streamlines layout for regular distribution of initial points.
    Standard_EXPORT Graphic3d_Vec2i UVStreamlines(const TCollection_AsciiString& thePrsId) const;

    //! Set "U x V" streamlines layout for regular distribution of initial points.
    Standard_EXPORT void SetUVStreamlines(const TCollection_AsciiString& thePrsId,
                                          const Graphic3d_Vec2i&         theUVStreamlines);

    //! Return bounding plane for streamlines mesh presentation.
    Standard_EXPORT MeshStreamlinesPlane StreamlinesPlane(const TCollection_AsciiString& thePrsId) const;

    //! Set bounding plane for streamlines mesh presentation.
    Standard_EXPORT void SetStreamlinesPlane(const TCollection_AsciiString& thePrsId,
                                             const MeshStreamlinesPlane&    thePlane);

    //! Return integration step coefficient for streamlines presentation,
    //! lower value leads to more accurate streamlines.
    Standard_EXPORT double StreamlinesStepCoeff(const TCollection_AsciiString& thePrsId) const;

    //! Set integration step coefficient for streamlines presentation,
    //! lower value leads to more accurate streamlines.
    Standard_EXPORT void SetStreamlinesStepCoeff(const TCollection_AsciiString& thePrsId, double theStepCoeff);

    //! Return max length coefficient for streamlines presentation, higher value leads to longer streamlines.
    Standard_EXPORT double StreamlinesLengthCoeff(const TCollection_AsciiString& thePrsId) const;

    //! Set max length coefficient for streamlines presentation, higher value leads to longer streamlines.
    Standard_EXPORT void SetStreamlinesLengthCoeff(const TCollection_AsciiString& thePrsId, double theLengthCoeff);

    //! Return thickness for streamlines presentation as a coefficient multiplied by bounding box size.
    Standard_EXPORT double StreamlinesThickness(const TCollection_AsciiString& thePrsId) const;

    //! Set thickness for streamlines presentation as a coefficient multiplied by bounding box size.
    Standard_EXPORT void SetStreamlinesThickness(const TCollection_AsciiString& thePrsId, double theThickness);

    //! Return particles animation speed for streamlines presentation, in [0.0, 1.0] range.
    Standard_EXPORT float StreamlinesAnimationSpeed(const TCollection_AsciiString& thePrsId) const;

    //! Set particles animation speed for streamlines presentation, in [0.0, 1.0] range.
    Standard_EXPORT void SetStreamlinesAnimationSpeed(const TCollection_AsciiString& thePrsId, float theSpeed);

    //! Return animation particles trail length for streamlines presentation, in [0.0, 1.0] range.
    Standard_EXPORT float StreamlinesAnimationTrail(const TCollection_AsciiString& thePrsId) const;

    //! Set animation particles trail length for streamlines presentation, in [0.0, 1.0] range.
    Standard_EXPORT void SetStreamlinesAnimationTrail(const TCollection_AsciiString& thePrsId, float theTrail);

    //! Return animation particles frequency for streamlines presentation, in [1, 100] range.
    Standard_EXPORT int StreamlinesAnimationFrequency(const TCollection_AsciiString& thePrsId) const;

    //! Set animation particles frequency for streamlines presentation, in [1, 100] range.
    Standard_EXPORT void SetStreamlinesAnimationFrequency(const TCollection_AsciiString& thePrsId, int theFreq);

    //! Check if streamlines presentation is shown as animated particles.
    Standard_EXPORT bool IsStreamlinesAnimated(const TCollection_AsciiString& thePrsId) const;

    //! Define if streamlines presentation should be shown as animated particles.
    Standard_EXPORT void SetStreamlinesAnimated(const TCollection_AsciiString& thePrsId, bool theIsAnimated);

public:
    //! Return presentation mode for specified mesh presentation.
    Standard_EXPORT ViewCae::InteractiveCaeMeshPrsMode PresentationMode(const TCollection_AsciiString& thePrsId) const;

    //! Set mesh presentation mode for specified mesh presentation.
    Standard_EXPORT void SetPresentationMode(const TCollection_AsciiString&      thePrsId,
                                             ViewCae::InteractiveCaeMeshPrsMode theMode);

    //! Return current element types to display for specified mesh presentation.
    Standard_EXPORT unsigned int ElementTypes(const TCollection_AsciiString& thePrsId) const;

    //! Set current element types to display for specified mesh presentation.
    Standard_EXPORT void SetElementTypes(const TCollection_AsciiString& thePrsId, unsigned int theElementTypes);

    //! Return true if it is needed to render/select 3d mesh only by its surface
    //! and to not include internal parts, True by default.
    Standard_EXPORT bool IsVolumeAsSurface(const TCollection_AsciiString& thePrsId) const;

    //! Set if it is needed to render/select 3d mesh only by its surface
    //! and to not include internal parts.
    Standard_EXPORT void SetVolumeAsSurface(const TCollection_AsciiString& thePrsId, bool theIsVolumeAsSurface);

    //! Return visibility flag for specified mesh presentation.
    Standard_EXPORT bool IsPrsVisible(const TCollection_AsciiString& thePrsId) const;

    //! Set visibility flag for specified mesh presentation.
    Standard_EXPORT void SetPrsVisible(const TCollection_AsciiString& thePrsId, bool theIsVisible);

    //! Return visibility of annotations for min/max values of the current active mesh property.
    Standard_EXPORT bool ToShowPrsMinMax(const TCollection_AsciiString& thePrsId) const;

    //! Set visibility of annotations for min/max values of the current active mesh property.
    Standard_EXPORT void SetShowPrsMinMax(const TCollection_AsciiString& thePrsId, bool theToShow);

    //! Clear and set to update min/max annotations for mesh presentations
    //! that use property from the specified color mapper.
    Standard_EXPORT void UpdateMinMaxValues(const Handle(MeshColorMapper)& theColorMapper);

    //! Return visibility of annotations for the current active group mesh property.
    Standard_EXPORT bool ToShowGroupsInfo(const TCollection_AsciiString& thePrsId) const;

    //! Set visibility of annotations for the current active group mesh property.
    Standard_EXPORT void SetShowGroupsInfo(const TCollection_AsciiString& thePrsId, bool theToShow);

public:
    //! Remove properties from mesh data (or clear their values data) and recalculate presentation if needed.
    Standard_EXPORT void RemoveProperties(bool theWithMetadata);

    //! Update current state.
    Standard_EXPORT virtual void UpdateAspects() override;

    //! Recompute selection, clear highlighted/selected sub entities.
    Standard_EXPORT void RecomputeSelection();

    //! Gather ids of properties that are/aren't needed in primitives calculation for all presentations.
    Standard_EXPORT void UsedProps(int                                       theTimeStep,
                                   NCollection_Map<TCollection_AsciiString>& theComputedProps,
                                   NCollection_Map<TCollection_AsciiString>& theNotComputedProps) const;

    //! Gather ids of properties that are needed in primitives calculation but not loaded /
    //! can be loaded from cache files.
    Standard_EXPORT void GetNotLoadedProps(std::vector<Handle(RWCae::MeshNVecProperty)>& theProps,
                                           std::vector<Handle(RWCae::MeshNVecProperty)>& theCachedProps,
                                           std::vector<TCollection_AsciiString>&           theNotExistedProps) const;

    //! Check if any presentation has not loaded custom geometry.
    Standard_EXPORT bool HasNotLoadedGeom() const;

public:
    //! Returns object's transparency.
    virtual double Transparency() const override { return ShadingTransparency("", MeshHighlightingType::None); }

    //! Sets global transparency overrider, replacing custom materials.
    virtual void SetTransparency(const double theValue) override
    {
        SetShadingTransparency("", (float)theValue, MeshHighlightingType::None);
    }

    //! Removes global transparency overrider.
    virtual void UnsetTransparency() override { SetShadingTransparency("", 0.0f, MeshHighlightingType::None); }

    //! Returns global color.
    virtual void Color(Quantity_Color& theColor) const override
    {
        theColor = ShadingColor("", MeshHighlightingType::None);
    }

    //! Sets global color overrider, replacing custom materials.
    virtual void SetColor(const Quantity_Color& theColor) override
    {
        SetShadingColor("", theColor, MeshHighlightingType::None);
    }

    //! Removes global color overrider.
    virtual void UnsetColor() override { SetShadingColor("", Quantity_NOC_WHITE, MeshHighlightingType::None); }

    //! Sets global width overrider.
    virtual void SetWidth(const double theLineWidth) override
    {
        SetEdgesWidth("", (float)theLineWidth, MeshHighlightingType::None);
    }

    //! Sets global width overrider.
    virtual void UnsetWidth() override { SetEdgesWidth("", 1.0f, MeshHighlightingType::None); }

public:
    //! Return shading color for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT Quantity_Color ShadingColor(const TCollection_AsciiString& thePrsId,
                                                MeshHighlightingType           theHiType) const;

    //! Set shading color for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT void SetShadingColor(const TCollection_AsciiString& thePrsId,
                                         const Quantity_Color&          theColor,
                                         MeshHighlightingType           theHiType);

    //! Return edges color for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT Quantity_Color EdgesColor(const TCollection_AsciiString& thePrsId,
                                              MeshHighlightingType           theHiType) const;

    //! Set edges color for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT void SetEdgesColor(const TCollection_AsciiString& thePrsId,
                                       const Quantity_Color&          theColor,
                                       MeshHighlightingType           theHiType);

    //! Return points color for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT Quantity_Color PointsColor(const TCollection_AsciiString& thePrsId,
                                               MeshHighlightingType           theHiType) const;

    //! Set points color for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT void SetPointsColor(const TCollection_AsciiString& thePrsId,
                                        const Quantity_Color&          theColor,
                                        MeshHighlightingType           theHiType);

    //! Return edges width for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT float EdgesWidth(const TCollection_AsciiString& thePrsId, MeshHighlightingType theHiType) const;

    //! Set edges width for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT void SetEdgesWidth(const TCollection_AsciiString& thePrsId,
                                       float                          theWidth,
                                       MeshHighlightingType           theHiType);

    //! Return points size for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT float PointsSize(const TCollection_AsciiString& thePrsId, MeshHighlightingType theHiType) const;

    //! Set points size for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT void SetPointsSize(const TCollection_AsciiString& thePrsId,
                                       float                          theSize,
                                       MeshHighlightingType           theHiType);

    //! Return shading transparency for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT float ShadingTransparency(const TCollection_AsciiString& thePrsId,
                                              MeshHighlightingType           theHiType) const;

    //! Set shading transparency for specified mesh presentation (or whole mesh if id is empty).
    Standard_EXPORT void SetShadingTransparency(const TCollection_AsciiString& thePrsId,
                                                float                          theValue,
                                                MeshHighlightingType           theHiType);

public:
    //! Get whether the mesh renders a 2D grid.
    bool ToShow2DGrid() { return myToShow2DGrid; }

    //! Set parameter to show a 2D grid for meshes that are part of a graph.
    void SetShow2DGrid(bool theArg)
    {
        if (myToShow2DGrid == theArg)
            return;

        myToShow2DGrid = theArg;
        SetToUpdate();
    }

    //! Structure for naming parameters of 2D grid axes.
    struct Axes2DNamingParams {
        std::string              NameX;
        std::string              NameY;
        std::vector<std::string> TicksX;
        std::vector<std::string> TicksY;
    };

    //! Get axes names and tick mark labels.
    const Axes2DNamingParams& Get2DAxesNaming() const { return my2DAxesNames; }

    //! Set axes names and tick mark labels.
    void Set2DAxesNaming(const Axes2DNamingParams& theParams)
    {
        my2DAxesNames = theParams;
        SetToUpdate();
    }

public:
    //! Callback called by Viewer before redrawing.
    Standard_EXPORT virtual AIS_RedrawProgressResult ProcessRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                                   const Handle(V3d_View)&               theView,
                                                                   const AIS_RedrawProgress theStage) override;

    //! Clears or invalidates dynamic highlight presentation.
    Standard_EXPORT virtual void ClearDynamicHighlight(const Handle(PrsMgr_PresentationManager)& theMgr) override;

    //! Return name of this type.
    virtual const char* TypeName() const override { return "InteractiveCaeMesh"; }

protected:
    //! Redefined method to compute presentation.
    Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                         const Handle(Prs3d_Presentation)&         thePrs,
                                         const Standard_Integer                    theMode) Standard_OVERRIDE;

    //! Compute selection.
    Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
                                                  const Standard_Integer             theMode) Standard_OVERRIDE;

protected:
    //! Initialize default attributes.
    Standard_EXPORT void initDefaultAttributes();

    //! Perform actions when active property (or property component) for specified mesh presentation is updated.
    Standard_EXPORT bool onPropertyUpdate(const TCollection_AsciiString& thePrsId,
                                          bool                           wasTriangleTexturesEnabled,
                                          bool                           wasNodesTexturesEnabled);

    //! Update all created color mappers.
    Standard_EXPORT void updateColorMappers();

    //! Try to load properties needed for computation.
    Standard_EXPORT void processDeferredProps();

    //! Try to load custom presentation geometry needed for computation.
    Standard_EXPORT void processDeferredGeoms();

    //! Computes presentation primitives for a 2d coordinate grid.
    //! Grid is situated on -Z side of the bounding box with the axes aligned with OX and OY.
    Standard_EXPORT void compute2DGrid(const Handle(Prs3d_Presentation)& thePrs);

    //! Return object's drawer depending on highlighting type.
    Standard_EXPORT Handle(InteractiveCaeMeshDrawer) getDrawer(MeshHighlightingType theHiType) const;

protected:
    //! Computes text for drawing 2D Grid.
    void drawGridText(const Handle(Prs3d_Presentation)& thePrs,
                      const Handle(Prs3d_TextAspect)&   theAspect,
                      const TCollection_AsciiString&    theString,
                      const gp_Pnt&                     thePos,
                      Graphic3d_HorizontalTextAlignment theHAlign,
                      Graphic3d_VerticalTextAlignment   theVAlign,
                      const float                       theHeight);

protected:
    Handle(ViewCae::InteractiveCaeMeshOwner) myMeshOwner;
    Handle(CaeMeshAnimation)                  myAnimation;
    bool                                      myIsDeferredUpdate = false;

    bool               myToShow2DGrid = false; //!< Controls whether to show 2D grid for a plot-like presentation
    Axes2DNamingParams my2DAxesNames;          //!< Names of the axes and tick mark strings.
    Handle(Graphic3d_AspectLine3d) my2DGridLineAspect;  //!< Aspect for grid lines.
    Handle(Prs3d_TextAspect)       my2DGridTextAspect;  //!< Aspect for most grid text.
    Handle(Prs3d_TextAspect)       my2DYAxisTextAspect; //!< Aspect for Y axis label.

    std::function<void(int, const TCollection_AsciiString&)> myOnPropertiesRequested;

    std::function<void(const TCollection_AsciiString&, int, const std::string&)> myOnGenGeomRequested;
};

} // namespace

#endif // _INTERACTIVECAEMESH_HEADERFILE