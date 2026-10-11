#ifndef RWVTK_LEGACYREADER_HEADERFILE
#define RWVTK_LEGACYREADER_HEADERFILE

#include <RWVtk/VtkBaseReader.h>
#include <RWCae/MeshData.h>

namespace RWVtk {
class VtkLegacyReader : public VtkBaseReader {
public:
    Standard_EXPORT bool PerformStream(std::istream& theStream, const TCollection_AsciiString& theFileName) override;
    const Handle(RWCae::MeshData)& GetMeshData() const { return myMeshData; }
private:
    Handle(RWCae::MeshData) myMeshData;
};
}
#endif
