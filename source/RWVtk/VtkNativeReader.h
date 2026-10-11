#ifndef _VTKNATIVEREADER_HEADERFILE
#define _VTKNATIVEREADER_HEADERFILE

#include <RWVtk/VtkBaseReader.h>
#include <RWCae/MeshData.h>

namespace RWVtk {
//! Native VTK import preserving concrete cell types, orders and rational weights.
//! This backend is enabled by OCCTV_USE_VTK, independently of the portable readers.
class VtkNativeReader {
public:
    Standard_EXPORT bool Perform(const TCollection_AsciiString& theFileName);
    const Handle(RWCae::MeshData)& GetMeshData() const { return myMesh; }
    const VtkImportReport& Report() const { return myReport; }
private:
    Handle(RWCae::MeshData) myMesh;
    VtkImportReport myReport;
};
}
#endif
