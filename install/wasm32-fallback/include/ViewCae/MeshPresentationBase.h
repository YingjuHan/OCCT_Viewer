#ifndef _MESHPRESENTATIONBASE_HEADERFILE
#define _MESHPRESENTATIONBASE_HEADERFILE

#include <ViewCae/MeshPart.h>

#include <ViewCaeGeom/MeshGeometry.h>
#include <ViewCaeGeom/MeshGeometryGen.h>

namespace ViewCae {
//! Class for mesh presentation part parameters.
class MeshPrsPartParams : public Standard_Transient {
public:
    //! Return presentation's active property.
    virtual const RWCae::MeshPropertyComponent& Property() const { return myProperty; }

    //! Set presentation's active property.
    virtual void SetProperty(const RWCae::MeshPropertyComponent& theProp) { myProperty = theProp; }

    //! Dump parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson);

    //! Restore parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged);

protected:
    RWCae::MeshPropertyComponent myProperty;
};

//! Base container for mesh presentation.
class MeshPresentationBase : public Standard_Transient {
public:
    //! Constructor.
    Standard_EXPORT MeshPresentationBase(const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                         const TCollection_AsciiString&            theId,
                                         bool                                      theToUpdateGeomId = false);

    //! Destructor.
    Standard_EXPORT virtual ~MeshPresentationBase();

    //! Return id of presentation.
    const TCollection_AsciiString& Id() const { return myId; }

    //! Return active time step.
    int ActiveTimeStep() const { return myActiveTimeStep; }

    //! Set active time step.
    Standard_EXPORT void SetActiveTimeStep(const int theTimeStep);

public:
    //! Return corresponding chain of mesh geometries.
    const Handle(ViewCaeGeom::MeshGeometry)& GetMeshGeometry() const { return myGeometry; }

    //! Return true if defined chain of mesh geometries contains the specified geometry.
    Standard_EXPORT bool HasMeshGeometry(const Handle(ViewCaeGeom::MeshGeometry)& theGeom) const;

    //! Insert a new geometry into children chain based on its order index.
    Standard_EXPORT void AppendMeshGeometry(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                            bool                                      theToUpdateGeomId = false);

    //! Return map of custom geometries for mesh parts.
    const NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>& GetGenGeometries() const
    {
        return myGenGeometry;
    }

    //! Return custom geometry for the specified mesh part.
    Standard_EXPORT Handle(ViewCaeGeom::MeshGeometryGen) GetGenGeometry(int thePart) const;

    //! Return geometry for the specified mesh part.
    Handle(ViewCaeGeom::MeshGeometry) GetMeshGeometry(int thePart) const
    {
        if (!myToUseGenGeom) {
            return myGeometry;
        }
        return GetGenGeometry(thePart);
    }

    //! Init custom geometries for mesh parts.
    Standard_EXPORT void CreateGenGeometry();

    //! Set to update custom geometries for mesh part/parts.
    Standard_EXPORT void SetToUpdateGenGeometry(int theMeshPart = -1);

public:
    //! Return map of mesh parts parameters.
    const NCollection_DataMap<int, Handle(MeshPrsPartParams)>& GetPartsParams() const { return myPartsParams; }

    //! Return mesh part parameters.
    Handle(MeshPrsPartParams) GetPartParams(int thePart) const
    {
        Handle(MeshPrsPartParams) aRes;
        myPartsParams.Find(thePart, aRes);
        return aRes;
    }

    //! Return downcasted mesh part parameters.
    template<typename T>
    Handle(T) GetPartParams(int thePart) const
    {
        return Handle(T)::DownCast(GetPartParams(thePart));
    }

    //! Set mesh part parameters.
    void SetPartParams(int thePart, const Handle(MeshPrsPartParams)& theParams)
    {
        myPartsParams.Bind(thePart, theParams);
    }

    //! Reset presentation id static counter.
    Standard_EXPORT static void ResetStaticCounter();

protected:
    Handle(ViewCaeGeom::MeshGeometry)                              myGeometry;
    NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)> myGenGeometry;
    bool                                                            myToUseGenGeom = false;

    TCollection_AsciiString myId;
    int                     myActiveTimeStep = -1;

    NCollection_DataMap<int, Handle(MeshPrsPartParams)> myPartsParams;
};

typedef NCollection_List<Handle(MeshPresentationBase)> ListOfMeshPresentations;

} // namespace

#endif // _MESHPRESENTATIONBASE_HEADERFILE