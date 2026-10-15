#include <RWVtk/VtkDatasetReader.h>
#include <RWVtk/VtkLegacyReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>
#include <RWVtk/VtkPolyDataReader.h>
#include <algorithm>

namespace RWVtk {
bool VtkDatasetReader::PerformStream(std::istream& stream,const TCollection_AsciiString& filename)
{
    myMesh.Nullify();
    if(!VtkBaseReader::PerformStream(stream,filename)) {return false;}
    TCollection_AsciiString lower=filename;lower.LowerCase();
    VtkImportReport portable;
    auto read=[&](auto& reader) {
        reader.SetPreserveCellTypes(myPreserveCellTypes);reader.SetResourceBudget(myBudget);reader.SetMappingInfo(myMappingInfo);reader.SetToReportMessages(false);
        const bool ok=reader.PerformStream(stream,filename);portable=reader.Report();if(ok) {myMesh=reader.GetMeshData();}
    };
    try {
        if(lower.EndsWith(".vtu")) {
            VtkUnstructuredGridReader reader;reader.SetTimeStep(myStep);if(myExplicitTime) {reader.SetTimeStamp(myTime);}read(reader);
        } else if(lower.EndsWith(".vtp")) {
            VtkPolyDataReader reader;reader.SetTimeStep(myStep);if(myExplicitTime) {reader.SetTimeStamp(myTime);}read(reader);
        } else if(lower.EndsWith(".vtk")) {VtkLegacyReader reader;read(reader);}
        else {return fail(std::string("Unsupported VTK dataset reference: ")+filename.ToCString());}
        myReport=portable;myReport.backend="portable";
        myReport.attempts.push_back({"portable",!myMesh.IsNull(),portable.errors,portable.warnings});

        if(myMesh.IsNull()) {return myReport.errors.empty()?fail("VTK reference contains no mesh"):false;}
        if(myExplicitTime || myStep!=-1) {
            for(const auto& field:myMesh->Properties()) {field->SetTime(myExplicitTime?myTime:field->Time(),myStep==-1?0:myStep);}
        }
        myReport.succeeded=true;myReport.errors.clear();myReport.source=filename.ToCString();
        myReport.points=myMesh->NbNodes();myReport.cells=myMesh->NbIndexedElems();myReport.fields=myMesh->Properties().Size();
        return true;
    } catch(const Standard_Failure& error) {myMesh.Nullify();return fail(error.GetMessageString());}
    catch(const std::exception& error) {myMesh.Nullify();return fail(error.what());}
}
}
