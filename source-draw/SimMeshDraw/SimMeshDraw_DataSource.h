
#ifndef _SimMeshDraw_DataSource_HeaderFile
#define _SimMeshDraw_DataSource_HeaderFile

#include <MeshVS_DataSource.hxx>
#include <Standard_TypeDef.hxx>

class SimMesh_Model;

//! The sample DataSource for working with SimMesh_Model
class SimMeshDraw_DataSource : public MeshVS_DataSource {
public:
    //! Constructor
    Standard_EXPORT SimMeshDraw_DataSource(const Handle(SimMesh_Model)& theModel);

    Standard_EXPORT bool GetGeom(const int             theID,
                                 const bool            theIsElement,
                                 TColStd_Array1OfReal& theCoords,
                                 int&                  theNbNodes,
                                 MeshVS_EntityType&    theType) const Standard_OVERRIDE;

    Standard_EXPORT bool GetGeomType(const int          theID,
                                     const bool         theIsElement,
                                     MeshVS_EntityType& theType) const Standard_OVERRIDE;

    Standard_EXPORT Standard_Address GetAddr(const int theID, const bool theIsElement) const Standard_OVERRIDE;

    Standard_EXPORT bool GetNodesByElement(const int                theID,
                                           TColStd_Array1OfInteger& theNodeIDs,
                                           int&                     theNbNodes) const Standard_OVERRIDE;

    Standard_EXPORT const TColStd_PackedMapOfInteger& GetAllNodes() const Standard_OVERRIDE;

    Standard_EXPORT const TColStd_PackedMapOfInteger& GetAllElements() const Standard_OVERRIDE;

    Standard_EXPORT bool GetNormal(const int theId, const int theMax, double& theNx, double& theNy, double& theNz) const
        Standard_OVERRIDE;

    Standard_EXPORT bool GetNodeNormal(const int theRanknode,
                                       const int theElementId,
                                       double&   theNx,
                                       double&   theNy,
                                       double&   theNz) const Standard_OVERRIDE;

    DEFINE_STANDARD_RTTI_INLINE(SimMeshDraw_DataSource, MeshVS_DataSource)

private:
    Handle(SimMesh_Model)      myModel;
    TColStd_PackedMapOfInteger myNodes;
    TColStd_PackedMapOfInteger myTriangles;
};

#endif // _SimMeshDraw_DataSource_HeaderFile
