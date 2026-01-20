#ifndef _VTKPOLYDATAREADER_HEADERFILE
#define _VTKPOLYDATAREADER_HEADERFILE

#include <RWVtk/VtkUnstructuredDataReader.h>

namespace RWVtk {

//! Reader class for poly data VTK data stored in XML format.
class VtkPolyDataReader : public VtkUnstructuredDataReader {
public:
    //! Constructor.
    Standard_EXPORT VtkPolyDataReader();

    //! Destructor.
    Standard_EXPORT virtual ~VtkPolyDataReader();

protected:
    //! Return type of data.
    virtual TCollection_AsciiString dataType() const override { return "PolyData"; }

    //! Read number of mesh points and elements.
    Standard_EXPORT virtual bool readMeshInfo(const LDOM_Element& theElem) override;

    //! Read mesh points and elements.
    Standard_EXPORT virtual bool readMeshGeometry(const LDOM_Element& theElem) override;

    //! Read 0d elements.
    Standard_EXPORT bool parseVerts(const LDOM_Element& theElem);

    //! Read triangle elements.
    Standard_EXPORT bool parseStrips(const LDOM_Element& theElem);

    //! Read polygon elements.
    Standard_EXPORT bool parsePolys(const LDOM_Element& theElem);

protected:
    int myNbVerts  = 0;
    int myNbLines  = 0;
    int myNbStrips = 0;
    int myNbPolys  = 0;
};

}

#endif // _VTKPOLYDATAREADER_HEADERFILE