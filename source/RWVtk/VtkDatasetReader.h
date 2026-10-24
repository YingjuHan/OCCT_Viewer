#ifndef RWVTK_DATASETREADER_HEADERFILE
#define RWVTK_DATASETREADER_HEADERFILE
#include <RWVtk/VtkBaseReader.h>
#include <RWCae/MeshData.h>
namespace RWVtk {
//! Shared single-dataset policy for composite and series references.
class VtkDatasetReader : public VtkBaseReader {
public:
    Standard_EXPORT bool PerformStream(std::istream& stream,const TCollection_AsciiString& filename) override;
    const Handle(RWCae::MeshData)& GetMeshData() const { return myMesh; }
    void SetPreserveCellTypes(bool enabled) { myPreserveCellTypes=enabled; }
    void SetTimeStep(int step) { myStep=step; }
    void SetTimeStamp(double time) { myTime=time;myExplicitTime=true; }
private:
    bool myPreserveCellTypes=false;
    Handle(RWCae::MeshData) myMesh;
    int myStep=-1;
    double myTime=0.0;
    bool myExplicitTime=false;
};
}
#endif
