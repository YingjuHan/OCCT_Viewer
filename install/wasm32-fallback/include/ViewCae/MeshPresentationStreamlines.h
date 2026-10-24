#ifndef _MESHPRESENTATIONSTREAMLINES_HEADERFILE
#define _MESHPRESENTATIONSTREAMLINES_HEADERFILE

#include <ViewCae/MeshPresentation.h>

namespace ViewCae {
class InteractiveCaeMeshOwner; // forward declared class due to cyclic dependency

//! Class for mesh streamlines presentation properties.
class MeshPrsStreamlinesVisProps : public MeshPrsVisProps {
public:
    //! Return streamlines thickness as a coefficient multiplied by bounding box size; 0.0 by default.
    double Thickness() const { return myThickness; }

    //! Set streamlines thickness as a coefficient multiplied by bounding box size.
    void SetThickness(double theThickness) { myThickness = theThickness; }

    //! Return particles animation speed, in [0.0, 1.0] range, 0.25f by default.
    float AnimationSpeed() const { return myAnimData.Speed; }

    //! Set particles animation speed, in [0.0, 1.0] range.
    void SetAnimationSpeed(float theSpeed) { myAnimData.Speed = theSpeed; }

    //! Return animation particles trail length, in [0.0, 1.0] range, 0.5f by default.
    float AnimationTrail() const { return myAnimData.Trail; }

    //! Set animation particles trail length, in [0.0, 1.0] range.
    void SetAnimationTrail(float theTrail) { myAnimData.Trail = theTrail; }

    //! Return animation particles frequency, in [1, 100] range, 10 by default.
    int AnimationFrequency() const { return myAnimData.Frequency; }

    //! Set animation particles frequency, in [1, 100] range.
    void SetAnimationFrequency(int theFreq) { myAnimData.Frequency = theFreq; }

    //! Check if streamlines are shown as animated particles.
    bool IsAnimated() const { return myAnimData.IsAnimated; }

    //! Define if streamlines should be shown as animated particles.
    void SetAnimated(bool theIsAnimated) { myAnimData.IsAnimated = theIsAnimated; }

    //! Check if streamlines animation is paused.
    bool IsAnimationPaused() const { return myAnimData.IsPaused; }

    //! Define if streamlines animation is paused.
    void SetAnimationPaused(bool theIsPaused) { myAnimData.IsPaused = theIsPaused; }

protected:
    double myThickness = 0.0;

    MeshStreamlinesAnimationData myAnimData;
};

//! Class for mesh streamlines part parameters.
class MeshPrsStreamlinesPartParams : public MeshPrsPartParams {
public:
    //! Return streamlines property.
    const Handle(RWCae::MeshNVecProperty)& StreamlinesProperty() const { return myStreamlinesProperty; }

    //! Set streamlines property.
    void SetStreamlinesProperty(const Handle(RWCae::MeshNVecProperty)& theProp) { myStreamlinesProperty = theProp; }

    //! Return bounding plane used to compute initial streamlines points.
    const MeshStreamlinesPlane& Plane() const { return myPlane; }

    //! Set bounding plane used to compute initial streamlines points.
    void SetPlane(const MeshStreamlinesPlane& thePlane) { myPlane = thePlane; }

    //! Return true if the initial points should be distributed regularly, otherwise they are placed
    //! randomly in the seed plane.
    bool IsRegular() const { return myIsRegular; }

    //! Switch between regular (true) and random (false) distribution of the initial points in the seed plane.
    void SetIsRegular(bool theIsRegular) { myIsRegular = theIsRegular; }

    //! Return number of streamlines; 100 by default.
    int NbStreamlines() const { return myNbStreamlines; }

    //! Set number of streamlines.
    void SetNbStreamlines(int theNbStreamlines) { myNbStreamlines = theNbStreamlines; }

    //! Return "U x V" streamlines layout for regular distribution of initial points.
    Graphic3d_Vec2i UVStreamlines() const { return myUVStreamlines; }

    //! Set "U x V" streamlines layout for regular distribution of initial points.
    void SetUVStreamlines(const Graphic3d_Vec2i& theUVStreamlines) { myUVStreamlines = theUVStreamlines; }

    //! Return integration step coefficient for streamlines presentation,
    //! lower value leads to more accurate streamlines; 0.2 by default.
    double StepCoeff() const { return myStepCoeff; }

    //! Set integration step coefficient for streamlines presentation,
    //! lower value leads to more accurate streamlines.
    void SetStepCoeff(double theStepCoeff) { myStepCoeff = theStepCoeff; }

    //! Return max length coefficient for streamlines presentation,
    //! higher value leads to longer streamlines; 1.0 by default.
    double LengthCoeff() const { return myLengthCoeff; }

    //! Set max length coefficient for streamlines presentation,
    //! higher value leads to longer streamlines.
    void SetLengthCoeff(double theLengthCoeff) { myLengthCoeff = theLengthCoeff; }

    //! Dump parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson) override;

    //! Restore parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged) override;

protected:
    //! Restore vector values from Json.
    template<typename T>
    void vecFromJson(const RWBase::JsonHelper::Value* theVal, T& theVec)
    {
        int i = 1;
        for (rapidjson::Value::ConstValueIterator anIter = theVal->Begin(); anIter != theVal->End(); ++anIter, ++i) {
            theVec.SetCoord(i, anIter->GetDouble());
        }
    };

    //! Restore Graphic3d_Vec2i values from Json.
    void vec2iFromJson(const RWBase::JsonHelper::Value* theVal, Graphic3d_Vec2i& theVec)
    {
        int i = 0;
        for (rapidjson::Value::ConstValueIterator anIter = theVal->Begin(); anIter != theVal->End(); ++anIter, ++i) {
            theVec.ChangeData()[i] = anIter->GetInt();
        }
    }

    //! Restore seed plane from Json
    bool planeFromJson(const RWBase::JsonHelper::Value& theJson, bool& theIsChanged);

    //! Restore numeric parameters from Json
    bool paramsFromJson(const RWBase::JsonHelper::Value& theJson, bool& theIsChanged);

protected:
    Handle(RWCae::MeshNVecProperty) myStreamlinesProperty;
    MeshStreamlinesPlane              myPlane;
    double                            myStepCoeff     = 0.2;
    double                            myLengthCoeff   = 1.0;
    bool                              myIsRegular     = false;
    int                               myNbStreamlines = 100;
    Graphic3d_Vec2i                   myUVStreamlines = Graphic3d_Vec2i(10);
};

//! Presentation for displaying streamlines.
class MeshPresentationStreamlines : public MeshPresentation {
public:
    //! Constructor.
    Standard_EXPORT MeshPresentationStreamlines(const Handle(MeshPropertyData)&           thePropData,
                                                int                                       thePropIndex,
                                                const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                                const TCollection_AsciiString&            theId,
                                                bool                                      theToUpdateGeomId = false);

    //! Destructor.
    Standard_EXPORT virtual ~MeshPresentationStreamlines();

    //! Update current state.
    Standard_EXPORT virtual void UpdateAspects(const Handle(InteractiveCaeMeshOwner)& theOwner) override;

    //! Return type of presentation.
    virtual MeshPresentationType Type() const override { return MeshPresentationType::Streamlines; }

    //! Return visual properties of streamlines presentation.
    Handle(MeshPrsStreamlinesVisProps) StreamlinesVisProps() const
    {
        return Handle(MeshPrsStreamlinesVisProps)::DownCast(myPrsVisProps);
    }

    //! Return streamlines part parameters.
    Handle(MeshPrsStreamlinesPartParams) StreamlinesPartParams() const
    {
        return GetPartParams<MeshPrsStreamlinesPartParams>((int)ViewCae::MeshPartType::Streamlines);
    }

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

protected:
};

} // namespace

#endif // _MESHPRESENTATIONSTREAMLINES_HEADERFILE