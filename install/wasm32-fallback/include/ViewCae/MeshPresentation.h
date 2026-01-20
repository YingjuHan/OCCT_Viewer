#ifndef _MESHPRESENTATION_HEADERFILE
#define _MESHPRESENTATION_HEADERFILE

#include <array>

#include <ViewCae/CaeMeshArrowsTools.h>
#include <ViewCae/InteractiveCaeMeshDrawer.h>
#include <ViewCae/InteractiveCaeMeshSelMode.h>
#include <ViewCae/MeshAnnotations.h>
#include <ViewCae/MeshPart.h>
#include <ViewCae/MeshPresentationBase.h>
#include <ViewCae/MeshPropertyData.h>
#include <ViewCae/MeshSensPart.h>

#include <ViewCaeGeom/MeshGeometry.h>
#include <ViewCaeGeom/MeshGeometryGen.h>

#include <Graphic3d_AspectFillArea3d.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Graphic3d_AspectMarker3d.hxx>

namespace ViewCae {
class InteractiveCaeMeshOwner; // forward declared class due to cyclic dependency

//! Class for mesh presentation visual properties.
class MeshPrsVisProps : public Standard_Transient {
public:
    //! Return visibility of mesh presentation.
    bool IsVisible() const { return myIsVisible; }

    //! Set visibility of mesh presentation.
    void SetVisible(bool theIsVisible) { myIsVisible = theIsVisible; }

protected:
    bool myIsVisible = true;
};

//! Type of mesh presentation.
enum class MeshPresentationType { Scalar, Vector, Streamlines };

//! Type of presentation highlighting.
enum class MeshHighlightingType { None, Highlight, Selection };

//! Base class for generating of primitive arrays from mesh geometry chain.
class MeshPresentation : public MeshPresentationBase {
public:
    //! Constructor.
    Standard_EXPORT MeshPresentation(const Handle(MeshPropertyData)&           thePropData,
                                     int                                       thePropIndex,
                                     const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                     const TCollection_AsciiString&            theId,
                                     bool                                      theToUpdateGeomId = false);

    //! Destructor.
    Standard_EXPORT virtual ~MeshPresentation();

    //! Return current drawer.
    const Handle(InteractiveCaeMeshDrawer)& Drawer() const { return myDrawer; }

    //! Return drawer by highlighting type.
    const Handle(InteractiveCaeMeshDrawer)& HighlightDrawer(MeshHighlightingType theType) const
    {
        return myHighlightDrawers[(int)theType];
    }

    //! Return active highlighting type.
    MeshHighlightingType HighlightType() const { return myHiType; }

    //! Set active highlighting type.
    void SetHighlightType(MeshHighlightingType theType) { myHiType = theType; }

    //! Update current state.
    Standard_EXPORT virtual void UpdateAspects(const Handle(InteractiveCaeMeshOwner)& theOwner);

    //! Mark computed primitive sets to update.
    Standard_EXPORT virtual void SetToUpdate(bool theToUpdatePrims, bool theToUpdateProps);

    //! Return map of mesh properties for each mesh geometry.
    const ViewCaeGeom::MapOfMeshGeomScalarProps& GetGeomScalarProps() const { return myGeomScalarProps; }

    //! Generate (if needed) and return mesh parts.
    Standard_EXPORT const MeshParts& GetParts();

    //! Return mesh parts without its computing.
    Standard_EXPORT MeshParts& ChangeParts();

    //! Clear mesh part.
    Standard_EXPORT void ClearPart(MeshPartType thePartType,
                                   int          theTimeStep    = -1,
                                   bool         theWithAuxData = true,
                                   bool         theWithGeom    = true);

    //! Generate (if needed) and return mesh sensitive parts.
    Standard_EXPORT const MeshSensParts& GetSensParts(const Handle(InteractiveCaeMeshOwner)& theOwner);

    //! Return mesh sensitive parts without its computing.
    Standard_EXPORT MeshSensParts& ChangeSensParts() { return mySensitives; }

    //! Return visual properties of presentation.
    const Handle(MeshPrsVisProps)& VisProps() const { return myPrsVisProps; }

    //! Return mesh property data.
    const Handle(MeshPropertyData)& PropertyData() const { return myPropData; }

    //! Return index of corresponding property in MeshPropertyData.
    int ActiveProperty() const { return myPropIndex; }

    //! Return type of presentation.
    virtual MeshPresentationType Type() const = 0;

    //! Toggle caching for computed per time step primitives.
    Standard_EXPORT void SetCachePrimitives(bool theToCache, bool theToClearCached);

    //! Gather ids of properties that are/aren't needed in primitives calculation.
    Standard_EXPORT void UsedProps(int                                       theTimeStep,
                                   NCollection_Map<TCollection_AsciiString>& theComputedProps,
                                   NCollection_Map<TCollection_AsciiString>& theNotComputedProps) const;

    //! Update presentation when corresponding properties were changed.
    Standard_EXPORT virtual void OnPropertiesUpdated(const NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps,
                                                     bool theCachedProps);

    //! Return selection mode.
    int SelectionMode() const { return mySelMode; }

    //! Set selection mode.
    void SetSelectionMode(int theMode) { mySelMode = theMode; }

protected:
    //! Retrieve or create parts for active time step.
    Standard_EXPORT MeshParts& getParts();

    //! Generate mesh parts.
    Standard_EXPORT virtual void updateParts(MeshParts& theParts) = 0;

    //! Check if any part should be updated.
    Standard_EXPORT virtual bool needToUpdateParts(const MeshParts& theParts) const = 0;

    //! Generate mesh sensitive parts.
    Standard_EXPORT virtual void updateSensParts(const Handle(InteractiveCaeMeshOwner)& theOwner) = 0;

    //! Add property dependency IDs to array.
    Standard_EXPORT static void addDependencyPropIds(std::vector<TCollection_AsciiString>& thePropIdArray,
                                                     const std::vector<Handle(RWCae::MeshNVecProperty)>& theProps);

    //! Add property ID to the set of used props and mark the corresponding part as a part that had it's used properties updated.
    Standard_EXPORT virtual void addUsedPerPartProp(MeshPartType theType, const TCollection_AsciiString& thePropId);

    //! Generate (if needed) properties for all mesh geometries in the chain.
    Standard_EXPORT virtual void fillProperties();

    //! Called when mesh geometry is updated.
    Standard_EXPORT virtual void onGeomUpdated(const TCollection_AsciiString& theGeomId = "");

protected:
    ViewCaeGeom::MapOfMeshGeomScalarProps myGeomScalarProps;
    bool                                   myGeomPropsUpdated = false;

    Handle(InteractiveCaeMeshDrawer)         myDrawer;
    Handle(InteractiveCaeMeshDrawer)         myHighlightDrawers[int(MeshHighlightingType::Selection) + 1];
    NCollection_DataMap<int, MeshParts>      myPrimitives;
    MeshSensParts                            mySensitives;
    Handle(MeshPrsVisProps)                  myPrsVisProps;
    Handle(MeshPropertyData)                 myPropData;
    int                                      myPropIndex;
    NCollection_Map<TCollection_AsciiString> myUsedPerPartProps[MeshNbPartTypes];
    //! Signifies that the props for a specific MeshPart need to be examined for loading if needed.
    std::array<bool, MeshNbPartTypes> myHasUpdatedPropsPerPart;
    bool                              myToCachePrimitives = false;

    MeshHighlightingType myHiType  = MeshHighlightingType::None;
    int                  mySelMode = (int)InteractiveCaeMeshSelMode::None;
};

//! Mesh part parameters factory.
class MeshPrsPartParamsFactory {
public:
    //! Create parameters object.
    Standard_EXPORT static Handle(MeshPrsPartParams) Create(const Handle(RWCae::MeshData)& theMeshData,
                                                            MeshPartType                     theType);
};

} // namespace

#endif // _MESHPRESENTATION_HEADERFILE
