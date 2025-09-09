
#ifndef _MESHPROPERTYBASE_HEADERFILE
#define _MESHPROPERTYBASE_HEADERFILE

#include <TCollection_AsciiString.hxx>

namespace RWCae {
//! Base class for property assigned to mesh elements/nodes.
class MeshPropertyBase : public Standard_Transient {
public:
    enum class MappingType { Nodal, Elemental };

    //! Generate a new property id.
    Standard_EXPORT static TCollection_AsciiString generateId();

public:
    //! Return property identifier.
    const TCollection_AsciiString& Id() const { return myId; }

    //! Set property identifier.
    void SetId(const TCollection_AsciiString& theId) { myId = theId; }

    //! Return property name.
    const TCollection_AsciiString& Name() const { return myName; }

    //! Set property name.
    void SetName(const TCollection_AsciiString& theName) { myName = theName; }

    //! Return property description.
    const TCollection_AsciiString& Description() const { return myDescription; }

    //! Set property description.
    void SetDescription(const TCollection_AsciiString& theDescription) { myDescription = theDescription; }

    //! Return type of property (elemental/nodal).
    MappingType MapType() const { return myMapType; }

    //! Return true if this property should be mapped to the mesh elements.
    bool IsElemental() const { return myMapType == MappingType::Elemental; }

    //! Return true if this property should be mapped to the mesh nodes.
    bool IsNodal() const { return myMapType == MappingType::Nodal; }

    //! Return true if property values data is loaded.
    bool IsLoaded() const { return myIsLoaded; }

    //! Define if property values data is loaded.
    void SetLoaded(bool theIsLoaded) { myIsLoaded = theIsLoaded; }

    //! Return true if property values data can be loaded from file.
    bool CanBeLoaded() const { return !myRefFilePath.IsEmpty(); }

    //! Clear values data and mark as not loaded.
    virtual void Unload() {}

    //! Return file that stores values data and can be used as a cache.
    const TCollection_AsciiString& RefFilePath() const { return myRefFilePath; }

    //! Set file that stores values data and can be used as a cache.
    void SetRefFilePath(const TCollection_AsciiString& thePath) { myRefFilePath = thePath; }

public:
    //! Default constructor.
    Standard_EXPORT MeshPropertyBase(const TCollection_AsciiString& theName, const MappingType theMapType);

    //! Reset property id static counter.
    Standard_EXPORT static void ResetStaticCounter();

protected:
    TCollection_AsciiString myId;          //!< property identifier
    TCollection_AsciiString myName;        //!< property name
    TCollection_AsciiString myDescription; //!< property description
    TCollection_AsciiString myRefFilePath;
    MappingType             myMapType; //!< type of property
    bool                    myIsLoaded = false;
};

} // namespace

#endif // _MESHPROPERTYBASE_HEADERFILE
