



#ifndef _VTKUNSTRUCTUREDDATAREADER_HEADERFILE
#define _VTKUNSTRUCTUREDDATAREADER_HEADERFILE

#include <RWCae/MeshData.h>
#include <RWCae/MeshScalarProperty.h>

#include <RWVtk/VtkBaseXMLReader.h>

#include <LDOMParser.hxx>

#include <vector>

namespace RWVtk {
//! Base reader class for unstructured VTK data stored in XML format.
class VtkUnstructuredDataReader : public VtkBaseXMLReader {
public:
    //! Constructor.
    Standard_EXPORT VtkUnstructuredDataReader();

    //! Destructor.
    Standard_EXPORT virtual ~VtkUnstructuredDataReader();

    //! Return mesh data.
    const Handle(RWCae::MeshData)& GetMeshData() const { return myMeshData; }

    //! Return vector of loaded elements.
    const std::shared_ptr<std::vector<std::vector<int>>>& GetStoredElements() const { return myStoredElements; }

    //! Set already loaded mesh data.
    Standard_EXPORT void SetLoadedMeshData(const Handle(RWCae::MeshData)&                      theMeshData,
                                           const std::shared_ptr<std::vector<std::vector<int>>>& theStoredElements);

    //! Set time step for all properties.
    void SetTimeStep(int theTimeStep)
    {
        myTimeStep = theTimeStep;
        if (myTimeStep != -1) {
            myTimeStamp = (double)myTimeStep;
        }
    }

public:
    //! DataArrays that is used to define array of VTK elements.
    struct MeshElements {
        DataArray Connectivity;
        DataArray Offsets;
        DataArray Types;
        DataArray Faces;
        DataArray FaceOffsets;
    };

    //! Struct for mapped property.
    struct MappedProp {
        TCollection_AsciiString              Name;
        TCollection_AsciiString              Id;
        std::vector<int>                     Components;
        std::vector<TCollection_AsciiString> DisplayUnits;
        RWCae::MeshNVecPropertyType        Type = RWCae::MeshNVecPropertyType::ScalarSet;
        TCollection_AsciiString              TimeUnits;
        TCollection_AsciiString              TimeDesc;

        MappedProp() {}
    };

protected:
    //! Perform reading of element data.
    Standard_EXPORT virtual bool perform() override;

    //! Read number of mesh points and elements.
    Standard_EXPORT virtual bool readMeshInfo(const LDOM_Element& theElem);

    //! Read mesh points and elements.
    Standard_EXPORT virtual bool readMeshGeometry(const LDOM_Element& theElem);

    //! Read mesh properties.
    Standard_EXPORT bool readMeshProperties(const LDOM_Element& theElem);

    //! Read and store mesh data nodes.
    Standard_EXPORT bool parsePoints(const LDOM_Element& theElem);

    //! Read 'connectivity', 'offsets' and 'types' data arrays.
    Standard_EXPORT void parseElements(const LDOM_Element& theElem, MeshElements& theMeshElems);

    //! Read and store mesh data properties.
    Standard_EXPORT bool parseProperties(const LDOM_Element& theElem, bool theIsNodal);

    //! Parse properties according to the mapping info.
    Standard_EXPORT void parseMappedProps(const VtkBaseXMLReader::DataArray&              theProp,
                                          const std::vector<MappedProp>&                  theMappedProps,
                                          RWCae::MeshPropertyBase::MappingType          theMappingType,
                                          std::vector<Handle(RWCae::MeshNVecProperty)>& theProperties);

    //! Map properties according to the mapping info.
    Standard_EXPORT std::vector<MappedProp> extractMappedProps(const VtkBaseXMLReader::DataArray& theProp,
                                                               bool                               theIsNodal) const;

    //! Map property according to the mapping info.
    Standard_EXPORT bool extractMappedProp(const VtkBaseXMLReader::DataArray&   theProp,
                                           const RWCae::MeshFieldMappingInfo& theInfo,
                                           MappedProp&                          theMappedProp) const;

    //! Store mesh data property values.
    Standard_EXPORT void storePropValues(RWCae::MeshScalarProperty*    theComp,
                                         const Handle(VtkAbstractArray)& theData,
                                         int                             theCompIndex);

    //! Add triangle element.
    Standard_EXPORT void addTriangle(const Handle(VtkAbstractArray)& theConnectivityArray,
                                     int                             theCellBegin,
                                     int                             theElemIndex,
                                     bool                            theIs2nd);

    //! Add triangle element.
    Standard_EXPORT void addElemTriangle(int theVert1, int theVert2, int theVert3, int theElemIndex);

    //! Add quad element.
    Standard_EXPORT void addQuad(const Handle(VtkAbstractArray)& theConnectivityArray,
                                 int                             theCellBegin,
                                 int                             theElemIndex,
                                 bool                            theIs2nd);

    //! Add quad element.
    Standard_EXPORT void addElemQuad(int theVert1, int theVert2, int theVert3, int theVert4, int theElemIndex);

    //! Add polygon element.
    Standard_EXPORT void addPolygon(const Handle(VtkAbstractArray)& theConnectivityArray,
                                    int                             theCellBegin,
                                    int                             theCellEnd,
                                    int                             theElemIndex);

protected:
    Handle(RWCae::MeshData) myMeshData;
    int                       myNbNodes            = 0;
    int                       myNodesOffset        = 0;
    int                       myNbProcessed        = 0;
    int                       myCurPiece           = 0;
    double                    myTimeStamp          = 0.0;
    int                       myTimeStep           = -1;
    bool                      myToReadMeshGeometry = true;

    std::shared_ptr<std::vector<std::vector<int>>> myStoredElements;
};

}

#endif // _VTKUNSTRUCTUREDDATAREADER_HEADERFILE 
