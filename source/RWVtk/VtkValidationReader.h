#ifndef RWVTK_VALIDATIONREADER_HEADERFILE
#define RWVTK_VALIDATIONREADER_HEADERFILE
#include <RWVtk/VtkBaseXMLReader.h>
namespace RWVtk {
//! Validate original arrays/topology before the native SDK allocates a dataset.
class VtkValidationReader : public VtkBaseXMLReader {
public:
    Standard_EXPORT explicit VtkValidationReader(bool polyData = false);
    Standard_EXPORT virtual ~VtkValidationReader();
protected:
    TCollection_AsciiString dataType() const override { return myPolyData ? "PolyData" : "UnstructuredGrid"; }
    Standard_EXPORT bool perform() override;
private:
    bool myPolyData;
};
}
#endif
