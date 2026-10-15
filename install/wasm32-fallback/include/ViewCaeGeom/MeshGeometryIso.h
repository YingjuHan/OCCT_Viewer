#ifndef _MESHGEOMETRYISO_HEADERFILE
#define _MESHGEOMETRYISO_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

namespace ViewCaeGeom {
//! Mesh geometry for mesh iso-surfaces.
//! It generates a new mesh data with 2d elements.
class MeshGeometryIso : public MeshGeometry {
public:
    //! Constructor.
    Standard_EXPORT explicit MeshGeometryIso(const Handle(MeshBvhData)& theBvhData);

    //! Return geometry type.
    virtual MeshGeometryType Type() const override { return MeshGeometryType::Iso; }

    //! Return property used for iso-surface calculation.
    const RWCae::MeshPropertyComponent& Property() const { return myProperty; }

    //! Set property used for iso-surface calculation.
    Standard_EXPORT void SetProperty(const RWCae::MeshPropertyComponent& theProp, bool toClearValues = true);

    //! Return defined iso values.
    const std::vector<double>& IsoValues() const { return myIsoValues; }

    //! Set iso values.
    void SetIsoValues(const std::vector<double>& theIsoValues)
    {
        myIsoValues = theIsoValues;
        SetToUpdate();
    }

    //! Generate a new property from base property for mesh data for this mesh geometry.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const override;

    //! Dump geometry parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson) override;

    //! Restore geometry parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged) override;

protected:
    //! Calculate mesh data based on parent mesh geometries.
    Standard_EXPORT virtual void generateMeshData() override;

    //! Called when marked to update from others mesh geometries in the chain.
    virtual void onUpdatedFromOthers() override { clear(); }

    //! Generate iso properties for parent mesh geometries.
    Standard_EXPORT void genBaseProperties();

    //! Convert input elemental property to nodal property.
    Standard_EXPORT void genInputNodalProp(const Handle(MeshGeometry)&                    theMeshGeom,
                                           const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                           const Handle(RWCae::MeshScalarProperty)&     theResProp) const;

    //! Fill generated nodal property with elemental values.
    template<class Elem_T>
    Standard_EXPORT void genInputNodalPropT(const RWCae::MeshAdaptor<Elem_T>&            theAdaptor,
                                            const Handle(RWCae::MeshScalarPropertyBase)& thePropIn,
                                            const Handle(RWCae::MeshScalarPropertyBase)& thePropOut,
                                            NCollection_Array1<int>*                       theNodesNbValues) const;

    //! Convert input elemental property to nodal property for specified element.
    Standard_EXPORT void genInputNodalPropElem(const Handle(MeshGeometry)&                    theMeshGeom,
                                               const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                               const Handle(RWCae::MeshScalarProperty)&     theResProp,
                                               const RWCae::MeshEntityId                    theElemIdx,
                                               RWCae::MeshData::EntityMap&                  theNodesMap) const;

    //! Convert input elemental property to nodal property for specified nodes.
    Standard_EXPORT void genInputNodalPropNodes(const Handle(MeshGeometry)&                    theMeshGeom,
                                                const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                                const Handle(RWCae::MeshScalarProperty)&     theResProp,
                                                const std::vector<MeshInterpolatedNode>&       theNodes,
                                                RWCae::MeshData::EntityMap&                  theNodesMap) const;

    //! Add to property average node value calculated by adjacent elements.
    Standard_EXPORT void calcNodeAverageValue(const Handle(MeshGeometry)&                    theMeshGeom,
                                              const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                              const Handle(RWCae::MeshScalarProperty)&     theResProp,
                                              RWCae::MeshEntityId                          theNodeIdx,
                                              RWCae::MeshData::EntityMap&                  theNodesMap,
                                              RWCae::MeshEntityId&                         theNbAddedNodes) const;

    //! Clear auxiliary data.
    Standard_EXPORT virtual void clear() override;

protected:
    RWCae::MeshPropertyComponent                                                     myProperty;
    NCollection_DataMap<Handle(MeshGeometry), Handle(RWCae::MeshScalarPropertyBase)> myProperties;
    std::vector<double>                                                                myIsoValues;
};

} // namespace

#endif // _MESHGEOMETRYISO_HEADERFILE
 