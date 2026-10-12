#ifndef RWVTK_REFERENCESTAGER_HEADERFILE
#define RWVTK_REFERENCESTAGER_HEADERFILE
#include <RWVtk/VtkResourceLimits.h>
#include <RWVtk/VtkImportReport.h>
#include <TCollection_AsciiString.hxx>
namespace RWVtk {
//! Freeze a local reference tree into a fresh, collision-free directory.
class VtkReferenceStager {
public:
    Standard_EXPORT bool Perform(const TCollection_AsciiString& source,const TCollection_AsciiString& directory,const VtkResourceLimits& limits);
    const TCollection_AsciiString& StagedSource() const {return mySource;}
    const VtkImportReport& Report() const {return myReport;}
private:
    TCollection_AsciiString mySource;
    VtkImportReport myReport;
};
}
#endif
