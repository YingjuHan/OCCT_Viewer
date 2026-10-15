
#ifndef _MESHPROPERTYDATA_HEADERFILE
#define _MESHPROPERTYDATA_HEADERFILE

#include <ViewCae/MeshColorMapper.h>

#include <RWCae/MeshData.h>
#include <RWCae/MeshNVecProperty.h>

#include <functional>
#include <vector>

namespace ViewCae {
//! Class stores mesh property-related data.
class MeshPropertyData : public Standard_Transient {
public:
    //! Constructor.
    Standard_EXPORT MeshPropertyData();

    //! Destructor.
    Standard_EXPORT virtual ~MeshPropertyData();

public:
    //! Return mesh property component by its index.
    Standard_EXPORT RWCae::MeshPropertyComponent Property(const int thePropIndex) const;

    //! Set mesh property by its index.
    Standard_EXPORT void SetProperty(const Handle(RWCae::MeshNVecProperty)& theProperty, const int thePropIndex);

    //! Add new mesh property.
    Standard_EXPORT int AddProperty(const Handle(RWCae::MeshNVecProperty)& theProperty);

    //! Remove mesh property by its index.
    Standard_EXPORT void RemoveProperty(const int thePropIndex);

    //! Set index of mesh property component.
    Standard_EXPORT void SetPropertyComponentIndex(const int thePropertyComponentIndex, const int thePropIndex);

    //! Fill map of used indices for the specified mesh property.
    Standard_EXPORT void PropertyUsedIndices(const RWCae::MeshPropertyComponent& theProp,
                                             TColStd_PackedMapOfInteger&           theIndices);

    //! Update all stored properties with values of specified time step.
    Standard_EXPORT bool UpdatePropertiesTimeStep(const Handle(RWCae::MeshData)& theMeshData, int theTimeStep);

public:
    typedef NCollection_DataMap<RWCae::MeshPropertyComponent, Handle(MeshColorMapper), RWCae::MeshPropertyComponent>
        MapOfColorMappers;

    //! Returns an object for mapping mesh values to colors.
    Standard_EXPORT Handle(MeshColorMapper) ColorMapper(const RWCae::MeshPropertyComponent& theProp) const;

    //! Sets an object for mapping mesh values to colors.
    Standard_EXPORT void SetColorMapper(const Handle(MeshColorMapper)&        theColorMapper,
                                        const RWCae::MeshPropertyComponent& theProp);

    //! Update color mapper by related property.
    Standard_EXPORT void UpdateColorMapper(const RWCae::MeshPropertyComponent& theProp);

    //! Return all created color mappers.
    Standard_EXPORT std::vector<Handle(MeshColorMapper)> ColorMappers() const;

    //! Return all used color mappers.
    const MapOfColorMappers& UsedColorMappers() const { return myAttachedColorMappers; }

    //! Check if unused color mappers are saved for further reuse.
    bool ToKeepFreeColorMappers() const { return myToKeepFreeColorMappers; }

    //! Define if unused color mappers should be saved for further reuse.
    void SetKeepFreeColorMappers(const bool theToKeep) { myToKeepFreeColorMappers = theToKeep; }

    //! Set a callback that is called when color mapper is created or its attached property is changed.
    void SetOnColorMapperChangeCallback(
        const std::function<void(const RWCae::MeshPropertyComponent&, const Handle(MeshColorMapper)&)>& theCallback)
    {
        myOnColorMapperChangeCallback = theCallback;
    }

    //! Reset property id static counter.
    Standard_EXPORT static void ResetStaticCounter();

protected:
    //! Called when color mapper of specified property should be updated.
    Standard_EXPORT void onColorMapperUpdated(const RWCae::MeshPropertyComponent& theOldProp, int theNewPropIndex);

    //! Called when color mapper of specified property should be updated.
    Standard_EXPORT void attachColorMapper(const RWCae::MeshPropertyComponent& theOldProp,
                                           const RWCae::MeshPropertyComponent& theNewProp,
                                           bool                                  toKeepOldProp,
                                           bool                                  toKeepNewProp);

    //! Release resources.
    Standard_EXPORT void release() noexcept;

protected:
    NCollection_DataMap<int, RWCae::MeshPropertyComponent> myProperties;
    MapOfColorMappers                                        myAttachedColorMappers;
    NCollection_List<Handle(MeshColorMapper)>                myFreeColorMappers;
    bool                                                     myToKeepFreeColorMappers = true;

    std::function<void(const RWCae::MeshPropertyComponent&, const Handle(MeshColorMapper)&)>
        myOnColorMapperChangeCallback;
};

} // namespace

#endif // _MESHPROPERTYDATA_HEADERFILE
