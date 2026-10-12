#include <RWVtk/VtkSeriesReader.h>
#include <RWCae/MeshTools.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace {
const std::string root = VTK_SERIES_FIXTURE_DIR;
void require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
void write(const std::string& name, const std::string& text)
{
    std::ofstream file(root + "/" + name, std::ios::binary); file << text;
    require(file.good(), "Cannot write series fixture");
}
std::string dataset(int shift, bool remesh = false, bool vector = true, bool shortField = false)
{
    return "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>"
        "<FieldData><DataArray type=\"Float64\" Name=\"TimeValue\" format=\"ascii\">2.5</DataArray></FieldData>"
        "<Piece NumberOfPoints=\"4\" NumberOfCells=\"1\"><Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">"
        + std::to_string(shift) + " 0 0 " + std::to_string(shift+1) + " 0 0 " + std::to_string(shift) + " 1 0 "
        + std::to_string(shift) + " 0 1</DataArray></Points><Cells>"
        "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">" + (remesh ? "0 1 3" : "0 1 2 3") + "</DataArray>"
        "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" + (remesh ? "3" : "4") + "</DataArray>"
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" + (remesh ? "5" : "10") + "</DataArray></Cells>"
        "<PointData" + (vector ? std::string(" Vectors=\"displacement\"") : "") + "><DataArray type=\"Float64\" Name=\"displacement\" "
        "NumberOfComponents=\"3\" format=\"ascii\">0 0 1 0 0 2 0 0 3" + (shortField ? std::string("") : " 0 0 4")
        + "</DataArray></PointData><CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">"
        + std::to_string(shift+7) + "</DataArray></CellData></Piece></UnstructuredGrid></VTKFile>";
}
std::string manifest(const std::string& second, const std::string& time = "1.75")
{
    return "{\"files\":[{\"name\":\"first.vtm\",\"time\":0.25},{\"name\":\"" + second + "\",\"time\":" + time + "}]}";
}
void tests()
{
    for (const auto& name : {"first", "moved", "remesh", "role", "short"}) {
        write(std::string(name)+".vtu", dataset(std::string(name)=="moved" ? 10 : 0,
            std::string(name)=="remesh", std::string(name)!="role", std::string(name)=="short"));
        write(std::string(name)+".vtm", "<VTKFile type=\"vtkMultiBlockDataSet\" byte_order=\"LittleEndian\"><vtkMultiBlockDataSet><DataSet index=\"0\" file=\""
            +std::string(name)+".vtu\"/></vtkMultiBlockDataSet></VTKFile>");
    }
    RWVtk::VtkSeriesReader reader; reader.SetToReportMessages(false);
    auto read = [&](const std::string& text) { std::istringstream stream(text); return reader.PerformStream(stream, (root+"/test.vtm.series").c_str()); };
    require(read(manifest("first.vtm")), "Stable series failed");
    auto stable = reader.GetMergedMeshData();
    require(stable->SeriesSteps().size() == 2 && stable->Properties().Size() == 4, "Stable series compatibility lost");
    require(stable->SeriesSteps()[0].Mesh != stable && stable->SeriesSteps()[0].Mesh != stable->SeriesSteps()[1].Mesh, "Series snapshot alias");
    require(read(manifest("moved.vtm")), "Moving series failed");
    auto moving = reader.GetMergedMeshData();
    require(moving->SeriesSteps()[1].Mesh->Nodes()[0].x() == 10 && moving->Nodes()[0].x() == 0, "Moving geometry lost");
    require(moving->Properties().Size() == 2 && !reader.Report().warnings.empty(), "Later fields bound to old geometry");
    require(moving->SeriesSteps()[1].Mesh->Properties()[0]->Time() == 1.75, "Manifest time not authoritative");
    require(moving->SeriesSteps()[1].Mesh->Properties()[0]->Id() == moving->SeriesSteps()[0].Mesh->Properties()[0]->Id(), "Series field IDs not stable");
    require(read(manifest("remesh.vtm")), "Remeshed series failed");
    auto remeshed = reader.GetMergedMeshData();
    require(remeshed->SeriesSteps()[1].Mesh->IndexedElements()[0].Type == RWCae::MeshElemType::Triangle, "Remeshed topology lost");
    for (const auto& invalid : {manifest("first.vtm", "0.25"), manifest("first.vtm", "-1"),
        manifest("role.vtm"), manifest("short.vtm"), manifest("missing.vtm"), manifest("first.vtm")+" trailing"}) {
        require(!read(invalid) && !reader.Report().errors.empty() && reader.GetMergedMeshData().IsNull(), "Invalid staged series published");
        require(stable->Properties().Size() == 4 && moving->SeriesSteps()[1].Mesh->Nodes()[0].x() == 10, "Failed stage mutated old mesh");
    }
    Handle(RWCae::MeshData) a = new RWCae::MeshData();
    a->AddNodePosition(Graphic3d_Vec3(0)); a->AddNodePosition(Graphic3d_Vec3(1));
    RWCae::MeshData::GenericCell cell; cell.Type=3; cell.Nodes={0,1}; cell.Faces={{0,1}};
    a->AddGenericCell(cell);
    Handle(RWCae::MeshData) empty = new RWCae::MeshData(); empty->AddGenericCell(RWCae::MeshData::GenericCell());
    auto merged = RWCae::MeshTools::MergeMeshData({{a,{}},{a,{}},{empty,{}}});
    require(merged->GenericCells().size()==3 && merged->NbIndexedElems()==3 && merged->GenericCells()[1].Nodes[1]==3
        && merged->GenericCells()[1].Faces[0][1]==3 && merged->GenericCells()[2].Type==0, "Composite original topology/empty cell lost");
}
}
int main()
{
    try { std::filesystem::create_directories(root); tests(); std::cout << "VTK_SERIES_PASS\n"; return 0; }
    catch (const Standard_Failure& error) { std::cerr << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; }
    return 1;
}
