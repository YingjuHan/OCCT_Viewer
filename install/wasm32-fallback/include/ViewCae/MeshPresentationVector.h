#ifndef _MESHPRESENTATIONVECTOR_HEADERFILE
#define _MESHPRESENTATIONVECTOR_HEADERFILE

#include <ViewCae/InteractiveCaeMeshPrsMode.h>
#include <ViewCae/MeshPresentation.h>
#include <ViewCae/MeshPropertyData.h>

namespace ViewCae {
class InteractiveCaeMeshOwner; // forward declared class due to cyclic dependency

//! Class for mesh vector presentation visual properties.
class MeshPrsVectorVisProps : public MeshPrsVisProps {
public:
    //! Return vector scale coefficient.
    double VectorScaleCoeff() const { return myVectorScaleCoeff; }

    //! Set vector scale coefficient.
    void SetVectorScaleCoeff(double theCoeff) { myVectorScaleCoeff = theCoeff; }

protected:
    double myVectorScaleCoeff = 1.0;
};

//! Class for mesh vector part parameters.
class MeshPrsVectorPartParams : public MeshPrsPartParams {
public:
    //! Return vector property.
    const Handle(RWCae::MeshNVecProperty)& VectorProperty() const { return myVectorProperty; }

    //! Set vector property.
    void SetVectorProperty(const Handle(RWCae::MeshNVecProperty)& theProp) { myVectorProperty = theProp; }

    //! Return distribution type for vector mesh presentation.
    MeshVectorDistrType VectorDistrType() const { return myVectorDistrType; }

    //! Set distribution type for vector mesh presentation.
    void SetVectorDistrType(MeshVectorDistrType theType) { myVectorDistrType = theType; }

    //! Return percentage of displayed vectors for vector mesh presentation, from 0.0 to 1.0.
    double SamplingRatio() const { return mySamplingRatio; }

    //! Set percentage of displayed vectors for vector mesh presentation, from 0.0 to 1.0.
    void SetSamplingRatio(double theRatio) { mySamplingRatio = theRatio; }

    //! Dump parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson) override;

    //! Restore parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged) override;

protected:
    Handle(RWCae::MeshNVecProperty) myVectorProperty;
    MeshVectorDistrType               myVectorDistrType = MeshVectorDistrType::Volume;
    double                            mySamplingRatio   = 1.0;
};

//! Presentation for displaying vectors in mesh data nodes.
class MeshPresentationVector : public MeshPresentation {
public:
    //! Constructor.
    Standard_EXPORT MeshPresentationVector(const Handle(MeshPropertyData)&           thePropData,
                                           int                                       thePropIndex,
                                           const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                           const TCollection_AsciiString&            theId,
                                           bool                                      theToUpdateGeomId = false);

    //! Destructor.
    Standard_EXPORT virtual ~MeshPresentationVector();

    //! Returns the effective vector scale.
    double EffectiveScale() const { return myEffectiveScale; }

    //! Update current state.
    Standard_EXPORT virtual void UpdateAspects(const Handle(InteractiveCaeMeshOwner)& theOwner) override;

    //! Mark computed primitive sets to update.
    Standard_EXPORT virtual void SetToUpdate(bool theToUpdatePrims, bool theToUpdateProps) override
    {
        MeshPresentation::SetToUpdate(theToUpdatePrims, theToUpdateProps);
        if (theToUpdateProps) {
            myGeomVecProps.Clear();
        }
    }

    //! Return type of presentation.
    virtual MeshPresentationType Type() const override { return MeshPresentationType::Vector; }

    //! Return visual properties of vector presentation.
    Handle(MeshPrsVectorVisProps) VectorVisProps() const
    {
        return Handle(MeshPrsVectorVisProps)::DownCast(myPrsVisProps);
    }

    //! Return vector part parameters.
    Handle(MeshPrsVectorPartParams) VectorPartParams() const
    {
        return GetPartParams<MeshPrsVectorPartParams>((int)ViewCae::MeshPartType::Vectors);
    }

    //! Return map of vector properties for each mesh geometry.
    const ViewCaeGeom::MapOfMeshGeomNVecProps& GetGeomVecProps() const { return myGeomVecProps; }

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

    //! Called when mesh geometry is updated.
    Standard_EXPORT virtual void onGeomUpdated(const TCollection_AsciiString& theGeomId) override;

    //! Generate (if needed) properties for all mesh geometries in the chain.
    Standard_EXPORT virtual void fillProperties() override;

    //! Calculate the effective vector scale from the scale factor, mesh AABB and field value range.
    Standard_EXPORT void updateEffectiveScale(const Handle(InteractiveCaeMeshOwner)& theOwner);

    //! Internal setter for the effective vector scale, shouldn't be called from the outside.
    void setEffectiveScale(double theScale) { myEffectiveScale = theScale; }

    //! Update used properties for each mesh part.
    Standard_EXPORT void updateUsedProps();

protected:
    double                               myEffectiveScale = 1.0;
    ViewCaeGeom::MapOfMeshGeomNVecProps myGeomVecProps;
};

} // namespace

#endif // _MESHPRESENTATIONVECTOR_HEADERFILE

