#ifndef _MESHNVECPROPERTY_HEADERFILE
#define _MESHNVECPROPERTY_HEADERFILE

#include <RWCae/MeshScalarPropertyBase.h>

#include <NCollection_IndexedMap.hxx>
#include <NCollection_Vector.hxx>

#include <vector>

namespace RWCae {
//! Type specifies meaning of property components.
enum class MeshNVecPropertyType {
    ScalarSet, //!< set of independent values
    Vector,    //!< vector of values with auto-calculated magnitude
    Tensor     //!< 6 or 9 components set with 3x3 matrix values
};
enum { MeshNVecPropertyType_NB = (int)MeshNVecPropertyType::Tensor + 1 };

class MeshNVecProperty;
typedef NCollection_Vector<Handle(MeshNVecProperty)> MeshNVecProperties;

//! N-component property containing scalar properties.
class MeshNVecProperty : public MeshPropertyBase {
public:
    //! Default constructor.
    Standard_EXPORT MeshNVecProperty(const TCollection_AsciiString&      theName,
                                     const int                           theNbComponents,
                                     const MeshPropertyBase::MappingType theMapType,
                                     const MeshNVecPropertyType          theType = MeshNVecPropertyType::ScalarSet);

public:
    //! Init components of specified type.
    template<class Property_T>
    inline void Init()
    {
        static_assert(opencascade::is_base_but_not_same<MeshScalarPropertyBase, Property_T>::value,
                      "MeshScalarPropertyBase is not base class of Property_T");
        Standard_ASSERT_RAISE(myNbComponents > 0, "Error! Incorrect number of property components.");
        for (int i = NbRawComponents() - 1; i >= 0; i--) {
            myComponents.SetValue(i, Property_T::NewInstance(MapType()));
        }

        if (IsVector()) {
            myComponents[0]->SetName("Magnitude");
            myComponents[0]->SetDescription("Magnitude");
        }
    }

    //! Check if property was initialized.
    bool IsInitialized() const { return !myComponents.IsEmpty(); }

    //! Return number of components (excluding auto-generated components).
    int NbComponents() const { return myNbComponents; }

    //! Return number of components (including auto-generated components).
    int NbRawComponents() const { return myNbComponents + getNbAuxComponents(); }

    //! Return specified component @sa NbComponents().
    Standard_EXPORT const Handle(MeshScalarPropertyBase)& GetComponent(int theIndex);

    //! Return specified component @sa NbRawComponents().
    Standard_EXPORT const Handle(MeshScalarPropertyBase)& GetRawComponent(int theIndex);

    //! Return property components.
    NCollection_Vector<Handle(MeshScalarPropertyBase)>& ChangeComponents() { return myComponents; }

    //! Return number of values for single scalar property component.
    int NbValues() const { return myNbValues; }

    //! Return type of property components.
    MeshNVecPropertyType Type() const { return myType; }

    //! Return true if components form vector.
    bool IsVector() const { return myType == MeshNVecPropertyType::Vector; }

    //! Return true if components form 3d vector.
    bool IsVector3() const { return IsVector() && myNbComponents == 3; }

    //! If this is a 3D vector property, normalize its components to have magnitude in [0; 1] range.
    //! Does nothing otherwise.
    Standard_EXPORT void NormalizeVector();

    //! Gather values of all component (excluding auto-generated) for specified index.
    template<class Vector_T>
    inline void GetVector(const MeshPropertyEntityInfo theInfo, Vector_T& theVector) const
    {
        int anOffset = getNbAuxComponents();
        for (int i = 0; i < myNbComponents; i++) {
            theVector[i] = myComponents[i + anOffset]->Value(theInfo);
        }
    }

    //! Gather values of all component (including auto-generated) for specified index.
    template<class Vector_T>
    inline void GetRawVector(const MeshPropertyEntityInfo theInfo, Vector_T& theVector) const
    {
        int aNbComponents = NbRawComponents();
        for (int i = 0; i < aNbComponents; i++) {
            theVector[i] = myComponents[i]->Value(theInfo);
        }
    }

    //! Set values of all component (excluding auto-generated) for specified index.
    template<class Vector_T>
    inline void SetVector(const MeshPropertyEntityInfo theInfo, const Vector_T& theVector)
    {
        int anOffset = getNbAuxComponents();
        for (int i = 0; i < myNbComponents; i++) {
            myComponents[i + anOffset]->SetValue(theInfo, theVector[i]);
        }
    }

    //! Set values of all component (including auto-generated) for specified index.
    template<class Vector_T>
    inline void SetRawVector(const MeshPropertyEntityInfo theInfo, const Vector_T& theVector)
    {
        int aNbComponents = NbRawComponents();
        for (int i = 0; i < aNbComponents; i++) {
            myComponents[i]->SetValue(theInfo, theVector[i]);
        }
    }

    //! Return TRUE if time step is defined.
    bool HasTimeStep() const { return myTimeStep >= 0; }

    //! Return timestamp; NaN by default, which means UNDEFINED.
    double Time() const { return myTimestamp; }

    //! Return time step index; -1 by default, which means UNDEFINED.
    int TimeStep() const { return myTimeStep; }

    //! Set timestep.
    void SetTime(double theTime, int theStep)
    {
        myTimestamp = theTime;
        myTimeStep  = theStep;
    }

    //! Return the string describing the meaning of "time step".
    //! It is "Time step" by default and can be overridden for specific analysis time
    //! (e.g. it can be "Eigen mode" for modal analysis or "Frequency" for harmonic etc.)
    const TCollection_AsciiString& TimeStepDescription() const { return myTimeStepDescr; }

    //! Set the string describing the meaning of "time step".
    void SetTimeStepDescription(const TCollection_AsciiString& theDescription) { myTimeStepDescr = theDescription; }

    //! Return the measurement units used to display the "time stamp" value.
    const TCollection_AsciiString& TimeDisplayUnits() const { return myTimeDispUnits; }

    //! Set the measurement units used to display the "time stamp" value.
    void SetTimeDisplayUnits(const TCollection_AsciiString& theUnits) { myTimeDispUnits = theUnits; }

    //! Return indices of enabled components (including auto-generated).
    Standard_EXPORT NCollection_IndexedMap<int> EnabledComponents();

    //! Should be called after components changed.
    Standard_EXPORT void UpdateComponents(bool toUpdateAuxComponents = true);

    //! Create a copy without filling of internal components values.
    Standard_EXPORT Handle(MeshNVecProperty) CopyRaw(int theNbValues, int theCompIdx = -1) const;

    //! Return components type.
    MeshScalarPropertyBaseType ComponentsType() const { return myComponents[0]->BaseType(); }

    //! Check if components have scalar type.
    bool HasScalarComponents() const { return ComponentsType() == MeshScalarPropertyBaseType::Scalar; }

    //! Check if components have nodevec type.
    bool HasNodeVecComponents() const { return ComponentsType() == MeshScalarPropertyBaseType::NodeVec; }

    //! Check if components have group type.
    bool HasGroupComponents() const { return ComponentsType() == MeshScalarPropertyBaseType::Group; }

    //! Clear values data and mark as not loaded.
    Standard_EXPORT virtual void Unload() override;

    //! Get array of property references that are dependencies of this property.
    //! Dependency properties are loaded in addition to the dependent property when it is set
    //! as an active property for some mesh presentation.
    //! Restrictions on dependencies:
    //! 1) Dependencies are set between NVec properties.
    //! 2) Dependencies between properties residing in different MeshData objects are unsupported.
    //! 3) More than one level of dependencies is unsupported (e.g. autoloading dependencies of dependencies).
    //! 4) Cyclic dependencies are discouraged, but wouldn't leak memory if owned by MeshData.
    //! Elements in returned array may be null.
    virtual const MeshNVecProperties& DependencyProps() const { return myDependencyProps; }

    //! Change array of property references.
    virtual MeshNVecProperties& ChangeDependencyProps() { return myDependencyProps; }

public:
    //! Sorting by timestamp.
    struct SortByTimestampLesser {
        bool operator()(const Handle(MeshNVecProperty)& theA, const Handle(MeshNVecProperty)& theB) const
        {
            if (theA->HasTimeStep() != theB->HasTimeStep()) {
                return !theA->HasTimeStep(); // properties without timestamp to be placed first
            }
            const int aStrComp = ::strncmp(theA->Name().ToCString(),
                                           theB->Name().ToCString(),
                                           theA->Name().Length() + 1);
            if (aStrComp == 0) {
                // order by timestamp
                return theA->TimeStep() < theB->TimeStep();
            }
            // order by name
            return aStrComp == 0 ? theA->TimeStep() < theB->TimeStep() : aStrComp < 0;
        }
    };

protected:
    //! Return number of auto-generated components.
    int getNbAuxComponents() const { return IsVector() ? 1 : 0; }

    //! Calculate magnitude component for vector of MeshScalarProperty components.
    void fillScalarVector();

    //! Calculate magnitude component for vector of MeshNodeVecProperty components.
    void fillNodeVecVector();

    //! Try to replace MeshNodeVecProperty components with single scalar components.
    void replaceNodeVecWithScalar();

protected:
    NCollection_Vector<Handle(MeshScalarPropertyBase)> myComponents;
    int                                                myNbComponents;
    int                                                myNbValues;
    MeshNVecPropertyType                               myType;
    double                                             myTimestamp;
    int                                                myTimeStep;
    TCollection_AsciiString                            myTimeStepDescr;
    TCollection_AsciiString                            myTimeDispUnits;
    //! Array of references to dependency properties.
    MeshNVecProperties myDependencyProps;
};

//! Struct stores mesh property component info.
struct MeshPropertyComponent {
    Handle(MeshNVecProperty) Property;
    int                      ComponentIndex;

    //! Constructor.
    explicit MeshPropertyComponent(const Handle(MeshNVecProperty)& theProp = nullptr, int theIndex = -1)
        : Property(theProp),
          ComponentIndex(theIndex)
    {
    }

    //! Return mesh scalar component.
    Handle(MeshScalarPropertyBase) GetScalarComp() const
    {
        return !Property.IsNull() ? Property->GetRawComponent(ComponentIndex) : nullptr;
    }

    //! Check if the component is defined.
    bool IsDefined() const { return !Property.IsNull() && ComponentIndex != -1; }

    //! Returns TRUE if theOther is equal to <*this>.
    bool operator==(const MeshPropertyComponent& theOther) const
    {
        return Property == theOther.Property && ComponentIndex == theOther.ComponentIndex;
    }

    //! Hash value, for Map interface.
    static int HashCode(const MeshPropertyComponent& theProp, const int theUpper)
    {
        unsigned int aHashCode = 0;

        aHashCode = aHashCode ^ (unsigned int)::HashCode(theProp.Property, theUpper);
        aHashCode = aHashCode ^ (unsigned int)::HashCode(theProp.ComponentIndex, theUpper);
        return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
    }

    //! Matching two instances, for Map interface.
    static bool IsEqual(const MeshPropertyComponent& theProp1, const MeshPropertyComponent& theProp2)
    {
        return theProp1 == theProp2;
    }
};

} // namespace

#endif // _MESHNVECPROPERTY_HEADERFILE