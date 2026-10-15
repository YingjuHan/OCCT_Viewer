#ifndef _VTKUNSTRUCTUREDGRIDREADER_HEADERFILE
#define _VTKUNSTRUCTUREDGRIDREADER_HEADERFILE

#include <RWVtk/VtkUnstructuredDataReader.h>

namespace RWVtk {

//! Reader class for unstructured grid VTK data stored in XML format.
class VtkUnstructuredGridReader : public VtkUnstructuredDataReader {
public:
    //! Constructor.
    Standard_EXPORT VtkUnstructuredGridReader();

    //! Destructor.
    Standard_EXPORT virtual ~VtkUnstructuredGridReader();

protected:
    //! Return type of data.
    virtual TCollection_AsciiString dataType() const override { return "UnstructuredGrid"; }

    //! Read number of mesh points and elements.
    Standard_EXPORT virtual bool readMeshInfo(const LDOM_Element& theElem) override;

    //! Read mesh points and elements.
    Standard_EXPORT virtual bool readMeshGeometry(const LDOM_Element& theElem) override;

    //! Read mesh cells.
    Standard_EXPORT bool parseCells(const LDOM_Element& theElem);

    //! Read mesh cells.
    Standard_EXPORT void parseCells(const MeshElements& theElems, NCollection_DataMap<int, int>& theSkippedElements);

    //! Add 0d element.
    Standard_EXPORT void addElem0d(const Handle(VtkAbstractArray)& theConnectivityArray,
                                   int                             theCellBegin,
                                   int                             theElemIndex);

    //! Add tetrahedron element.
    Standard_EXPORT void addTetrahedron(const Handle(VtkAbstractArray)& theConnectivityArray,
                                        int                             theCellBegin,
                                        int                             theElemIndex,
                                        bool                            theIs2nd);

    //! Add pyramid element.
    Standard_EXPORT void addPyramid(const Handle(VtkAbstractArray)& theConnectivityArray,
                                    int                             theCellBegin,
                                    int                             theElemIndex,
                                    bool                            theIs2nd);

    //! Add prism element.
    Standard_EXPORT void addPrism(const Handle(VtkAbstractArray)& theConnectivityArray,
                                  int                             theCellBegin,
                                  int                             theElemIndex,
                                  bool                            theIs2nd);

    //! Add hexahedron element.
    Standard_EXPORT void addHexahedron(const Handle(VtkAbstractArray)& theConnectivityArray,
                                       int                             theCellBegin,
                                       int                             theElemIndex,
                                       bool                            theIs2nd);

    //! Add polyhedron element.
    Standard_EXPORT void addPolyhedron(const Handle(VtkAbstractArray)& theConnectivityArray,
                                       const Handle(VtkAbstractArray)& theFacesArray,
                                       int                             theCellBegin,
                                       int                             theCellEnd,
                                       int                             theFacesBegin,
                                              int                             theFacesEnd,
                                       int                             theElemIndex);

protected:
    int myNbElements = 0;
};

}

#endif // _VTKUNSTRUCTUREDGRIDREADER_HEADERFILE
