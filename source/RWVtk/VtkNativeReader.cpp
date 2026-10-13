#include <RWVtk/VtkNativeReader.h>
#include <RWVtk/VtkDatasetReader.h>
namespace RWVtk {
bool VtkNativeReader::Perform(const TCollection_AsciiString& filename)
{
    myMesh.Nullify(); VtkDatasetReader reader;
    reader.SetPreserveCellTypes(true); reader.SetToReportMessages(false);
    if (myBudget) { reader.SetResourceBudget(myBudget); } else { reader.SetResourceLimits(myLimits); }
    const bool ok=reader.Perform(filename); myReport=reader.Report();
    if(ok) { myMesh=reader.GetMeshData(); } return ok;
}
}
