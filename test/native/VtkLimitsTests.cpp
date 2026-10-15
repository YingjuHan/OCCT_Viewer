#include <RWVtk/VtkBaseXMLReader.h>
#include <RWVtk/VtkLegacyReader.h>
#include <RWVtk/VtkSeriesReader.h>
#include <RWMeshConv/MeshLoader.h>
#include <RWVtk/VtkNativeReader.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace {
const std::string root=VTK_LIMIT_FIXTURE_DIR;
void require(bool ok,const char* why) {if(!ok) {throw std::runtime_error(why);}}
class Probe : public RWVtk::VtkBaseXMLReader {
public:
    int Count=0;
    TCollection_AsciiString dataType() const override {return "UnstructuredGrid";}
    bool perform() override {
        Count=0;
        for(const auto& e:getChildrenElementsByTagName(myDataElement,"DataArray")) {
            auto a=readDataArray(e); if(a.Data.IsNull()) {return false;}
            require(a.Data->GetInt(0,0)==Count,"Appended array offset/value corrupted"); ++Count;
        }
        return true;
    }
};
void write(const std::string& name,const std::string& text) {std::ofstream file(root+"/"+name,std::ios::binary);file<<text;require(file.good(),"Cannot write limits fixture");}
void memory(int arrays)
{
    const size_t size=512*1024; const auto name=root+"/memory-"+std::to_string(arrays)+".vtu";
    {
        std::ofstream file(name,std::ios::binary);file<<"<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>";
        for(int i=0;i<arrays;++i) {file<<"<DataArray type=\"Int32\" format=\"appended\" offset=\""<<(size+4)*i<<"\"/>";}
        file<<"</UnstructuredGrid><AppendedData encoding=\"raw\">_";
        std::string bytes(size,'\0');const uint32_t length=(uint32_t)size;
        for(int i=0;i<arrays;++i) {std::memcpy(bytes.data(),&i,sizeof(i));file.write((const char*)&length,4);file.write(bytes.data(),bytes.size());}
        file<<"</AppendedData></VTKFile>";require(file.good(),"Memory fixture failed");
    }
    Probe reader;reader.SetToReportMessages(false);
    require(reader.Perform(name.c_str())&&reader.Count==arrays,"Bounded multi-offset import failed");
    std::cout<<"MEMORY_PASS arrays="<<arrays<<" payload_bytes="<<size*arrays<<'\n';
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters={};
    require(GetProcessMemoryInfo(GetCurrentProcess(),&counters,sizeof(counters))!=0,"Cannot measure process peak memory");
    std::cout<<"MEMORY_PEAK_BYTES "<<counters.PeakWorkingSetSize<<'\n';
#endif
}
void tests()
{
    RWMeshConv::MeshLoader loader;loader.SetToReportMessages(false);
    const std::string tetra=std::string(VTK_FIXTURE_DIR)+"/tetra.vtu";
    for(int kind=0;kind<7;++kind) {
        RWVtk::VtkResourceLimits limit;
        if(kind==0) {limit.MaxFileBytes=64;} if(kind==1) {limit.MaxNodes=3;} if(kind==2) {limit.MaxCells=0;}
        if(kind==3) {limit.MaxArrays=1;} if(kind==4) {limit.MaxComponents=2;} if(kind==5) {limit.MaxArrayBytes=8;} if(kind==6) {limit.MaxDecodedBytes=96;}
        loader.SetVtkResourceLimits(limit);
        require(loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),tetra.c_str()).IsNull()&&!loader.VtkReport().errors.empty(),"Configured limit bypassed");
        require(loader.VtkReport().attempts.size()==1,"Limit failure triggered fallback");
    }
    loader.SetVtkResourceLimits(RWVtk::VtkResourceLimits());
    require(!loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),tetra.c_str()).IsNull(),"Limits leaked into next import");
    std::ifstream source(tetra,std::ios::binary);write("tetra.vtu",std::string(std::istreambuf_iterator<char>(source),{}));
    write("tetra.vtm","<VTKFile type=\"vtkMultiBlockDataSet\" byte_order=\"LittleEndian\"><vtkMultiBlockDataSet><DataSet file=\"tetra.vtu\"/></vtkMultiBlockDataSet></VTKFile>");
    write("two.vtm.series","{\"files\":[{\"name\":\"tetra.vtm\",\"time\":0},{\"name\":\"tetra.vtm\",\"time\":1}]}");
    RWVtk::VtkSeriesReader series;series.SetToReportMessages(false);RWVtk::VtkResourceLimits limit;limit.MaxNodes=4;series.SetResourceLimits(limit);
    require(!series.Perform((root+"/two.vtm.series").c_str())&&series.GetMergedMeshData().IsNull(),"Series bypassed aggregate node budget");
    write("dense.vtm","<VTKFile type=\"vtkMultiBlockDataSet\" byte_order=\"LittleEndian\"><vtkMultiBlockDataSet>"
        "<DataSet index=\"0\" file=\"tetra.vtu\"/><DataSet index=\"1\" file=\"tetra.vtu\"/>"
        "<DataSet index=\"2\" file=\"tetra.vtu\"/><DataSet index=\"3\" file=\"tetra.vtu\"/></vtkMultiBlockDataSet></VTKFile>");
    limit=RWVtk::VtkResourceLimits();limit.MaxArrayBytes=96;loader.SetVtkResourceLimits(limit);
    require(loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),(root+"/dense.vtm").c_str()).IsNull(),"Dense merged fields bypassed pre-allocation budget");
    RWVtk::VtkLegacyReader legacy;legacy.SetToReportMessages(false);limit=RWVtk::VtkResourceLimits();limit.MaxArrayBytes=1024;legacy.SetResourceLimits(limit);
    std::istringstream huge("# vtk DataFile Version 3.0\nhuge\nASCII\nDATASET UNSTRUCTURED_GRID\nPOINTS 2147483647 double\n");
    require(!legacy.PerformStream(huge,"huge.vtk"),"Legacy declaration allocated beyond limit");
    Probe probe;probe.SetToReportMessages(false);
    std::string header;for(uint64_t value:{uint64_t(1),uint64_t(1)<<40,uint64_t(1)<<40,uint64_t(1)}) {header.append((const char*)&value,8);}
    std::istringstream bomb("<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\" header_type=\"UInt64\" compressor=\"vtkZLibDataCompressor\">"
        "<UnstructuredGrid><DataArray type=\"Int32\" format=\"appended\" offset=\"0\"/></UnstructuredGrid><AppendedData encoding=\"raw\">_"+header+"x</AppendedData></VTKFile>");
    require(!probe.PerformStream(bomb,"bomb.vtu")&&!probe.Report().errors.empty(),"Compression declaration allocated beyond budget");
    RWVtk::VtkNativeReader native;limit=RWVtk::VtkResourceLimits();limit.MaxNodes=3;native.SetResourceLimits(limit);
    require(!native.Perform(tetra.c_str())&&native.GetMeshData().IsNull(),"Native SDK bypassed preflight node budget");
    memory(4);
}
}
int main(int argc,char** argv)
{
    try {std::filesystem::create_directories(root);if(argc==3&&std::string(argv[1])=="--memory") {memory(std::stoi(argv[2]));} else {tests();}std::cout<<"VTK_LIMITS_PASS\n";return 0;}
    catch(const Standard_Failure& e) {std::cerr<<e.GetMessageString()<<'\n';}
    catch(const std::exception& e) {std::cerr<<e.what()<<'\n';}
    return 1;
}
