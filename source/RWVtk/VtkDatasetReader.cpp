#include <RWVtk/VtkDatasetReader.h>
#include <RWVtk/VtkLegacyReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>
#include <RWVtk/VtkPolyDataReader.h>
#ifdef OCCTV_USE_VTK
#include <RWVtk/VtkNativeReader.h>
#endif
#include <algorithm>

namespace RWVtk {
bool CanUseNativeTopology(const VtkImportReport& report)
{
    return !report.errors.empty() && std::all_of(report.errors.begin(),report.errors.end(),[](const std::string& error) {
        return error.find("Unsupported VTK cell type ")==0 || error=="VTK PolyData Lines require line-element support"
            || error=="VTK XML 2.3 polyhedron face encoding requires native cells" || error=="Invalid number of nodes '0'";
    });
}
bool VtkDatasetReader::PerformStream(std::istream& stream,const TCollection_AsciiString& filename)
{
    myMesh.Nullify();
    if(!VtkBaseReader::PerformStream(stream,filename)) {return false;}
    TCollection_AsciiString lower=filename;lower.LowerCase();
    const auto checkpoint=*myBudget;
    VtkImportReport portable;
    auto read=[&](auto& reader) {
        reader.SetResourceBudget(myBudget);reader.SetMappingInfo(myMappingInfo);reader.SetToReportMessages(false);
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
#ifdef OCCTV_USE_VTK
        if(myMesh.IsNull() && myMappingInfo.empty() && CanUseNativeTopology(portable)) {
            *myBudget=checkpoint;
            VtkNativeReader native;native.SetResourceBudget(myBudget);
            const bool ok=native.Perform(filename);myReport=native.Report();myReport.backend="native-vtk";
            myReport.attempts.push_back({"portable",false,portable.errors,portable.warnings});
            myReport.attempts.push_back({"native-vtk",ok,native.Report().errors,native.Report().warnings});
            for(const auto& warning:portable.warnings) {
                if(std::find(myReport.warnings.begin(),myReport.warnings.end(),warning)==myReport.warnings.end()) {myReport.warnings.push_back(warning);}
            }
            if(ok) {myMesh=native.GetMeshData();}
        }
#endif
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
