#ifndef RWVTK_LEGACYREADER_HEADERFILE
#define RWVTK_LEGACYREADER_HEADERFILE

#include <RWVtk/VtkBaseReader.h>
#include <RWCae/MeshData.h>

namespace RWVtk {
class VtkLegacyReader : public VtkBaseReader {
public:
    void SetValidationOnly(bool enabled) { myValidationOnly = enabled; }
    Standard_EXPORT VtkLegacyReader();
    Standard_EXPORT ~VtkLegacyReader() override;
    Standard_EXPORT bool PerformStream(std::istream& theStream, const TCollection_AsciiString& theFileName) override;
    const Handle(RWCae::MeshData)& GetMeshData() const { return myMeshData; }
private:
    Handle(RWCae::MeshData) myMeshData;
    bool myValidationOnly = false;
};
}
#endif
