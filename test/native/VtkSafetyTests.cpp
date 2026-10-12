#include <RWVtk/VtkBaseXMLReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>
#include <RWVtk/VtkLegacyReader.h>
#include <RWVtk/VtkValidationReader.h>
#include <zlib.h>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace {
void require(bool ok, const char* why) { if (!ok) { throw std::runtime_error(why); } }
std::string b64(const std::string& bytes)
{
    const char* digits="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; std::string out;
    for (size_t i=0; i<bytes.size(); i+=3) {
        unsigned v=(unsigned char)bytes[i]<<16;
        if (i+1<bytes.size()) { v|=(unsigned char)bytes[i+1]<<8; }
        if (i+2<bytes.size()) { v|=(unsigned char)bytes[i+2]; }
        out+=digits[(v>>18)&63]; out+=digits[(v>>12)&63];
        out+=i+1<bytes.size()?digits[(v>>6)&63]:'='; out+=i+2<bytes.size()?digits[v&63]:'=';
    }
    return out;
}
template<class T> void append(std::string& out,T value,bool big=false)
{
    char bytes[sizeof(T)]; std::memcpy(bytes,&value,sizeof(T));
    if(big) { std::reverse(bytes,bytes+sizeof(T)); } out.append(bytes,sizeof(T));
}
class Probe : public RWVtk::VtkBaseXMLReader {
public:
    DataArray Array;
    TCollection_AsciiString dataType() const override { return "UnstructuredGrid"; }
    bool perform() override { Array=readDataArray(getFirstElement(myDataElement,"DataArray")); return !Array.Data.IsNull(); }
};
std::string xml(const std::string& array,const std::string& extra="",const std::string& attrs="")
{
    return "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\" "+attrs+"><UnstructuredGrid>"+array+"</UnstructuredGrid>"+extra+"</VTKFile>";
}
std::string array(const std::string& text,const std::string& type="Int32",const std::string& attrs="",const std::string& format="ascii")
{
    return "<DataArray type=\""+type+"\" format=\""+format+"\" "+attrs+">"+text+"</DataArray>";
}
bool read(Probe& probe,const std::string& text) { std::istringstream stream(text); return probe.PerformStream(stream,"matrix.vtu"); }
void matrix()
{
    Probe p; p.SetToReportMessages(false);
    for(const auto& type: {"Int8","UInt8","Int16","UInt16","Int32","UInt32","Int64","UInt64","Float32","Float64"}) {
        require(read(p,xml(array("0 1 2",type))),"Supported ASCII type rejected");
        require(p.Array.Data->GetDouble(2,0)==2,"ASCII type value corrupted");
    }
    for (const auto& invalid: {xml(array("-1","UInt64")),xml(array("256","UInt8")),xml(array("128","Int8")),
        xml(array("1.5","Int32")),xml(array("1e100","Float32")),xml(array("1e999","Float64")),
        xml(array("0","Unknown")),xml(array("0","Int32","NumberOfComponents=\"bad\"")),
        xml(array("1 2 3","Float64","NumberOfComponents=\"2\""))}) {
        require(!read(p,invalid)&&!p.Report().errors.empty(),"Invalid typed ASCII array accepted");
    }
    for(bool big:{false,true}) { for(bool wide:{false,true}) { for(bool compressed:{false,true}) { for(int encoding:{0,1,2}) {
        std::string values,header,packed; for(int32_t v:{-3,7,42}) { append(values,v,big); }
        auto word=[&](uint64_t v) { if(wide) {append(header,v,big);} else {append(header,(uint32_t)v,big);} };
        if(compressed) {
            for(size_t offset=0;offset<values.size();offset+=8) {
                const size_t size=std::min(size_t(8),values.size()-offset); uLongf length=compressBound(size); std::string block(length,'\0');
                require(compress2((Bytef*)block.data(),&length,(const Bytef*)values.data()+offset,size,Z_BEST_SPEED)==Z_OK,"Compression fixture failed");
                block.resize(length); packed+=block;
                if(offset==0) {word(2);word(8);word(4);} word(length);
            }
        } else {word(values.size());packed=values;}
        const std::string bytes=encoding==2?header+packed:b64(header)+b64(packed);
        const std::string appended=encoding?"<AppendedData encoding=\""+std::string(encoding==2?"raw":"base64")+"\">_"+bytes+"</AppendedData>":"";
        std::string document=xml(array(encoding?"":bytes,"Int32","offset=\"0\"",encoding?"appended":"binary"),appended,
            std::string("header_type=\"")+(wide?"UInt64":"UInt32")+"\" "+(compressed?"compressor=\"vtkZLibDataCompressor\"":""));
        if(big) {document.replace(document.find("LittleEndian"),12,"BigEndian");}
        require(read(p,document)&&p.Array.Data->GetInt(0,0)==-3&&p.Array.Data->GetInt(2,0)==42,"Binary/endian/block matrix corrupted");
        for(size_t length:{size_t(0),size_t(1),header.size()-1,header.size()+packed.size()-1}) {
            const std::string cut=(header+packed).substr(0,length);
            std::string bad=xml(array(b64(cut),"Int32","","binary"),"",std::string("header_type=\"")+(wide?"UInt64":"UInt32")+"\" "+(compressed?"compressor=\"vtkZLibDataCompressor\"":""));
            if(big) {bad.replace(bad.find("LittleEndian"),12,"BigEndian");}
            require(!read(p,bad),"Truncated binary/compressed array accepted");
        }
    } } } }
    for(const auto& bad:{"A","A===","!!!!","AB==","AAAAA","=AAA"}) { require(!read(p,xml(array(bad,"Int32","","binary"))),"Malformed base64 accepted"); }
    std::string bytes; append(bytes,uint32_t(4));append(bytes,int32_t(7));
    require(!read(p,xml(array(b64(bytes+"junk"),"Int32","","binary"))),"Inline trailing bytes accepted");
    for(const auto& offset:{"-1","99999","2x"}) {
        require(!read(p,xml(array("","Int32",std::string("offset=\"")+offset+"\"","appended"),"<AppendedData encoding=\"raw\">_"+bytes+"</AppendedData>")),"Invalid appended offset accepted");
    }
    require(read(p,xml(array("7")))&&p.Report().errors.empty(),"Failed-array diagnostics leaked");
    const std::string points="<Points>"+array("0 0 0 1 0 0 0 1 0 0 0 1","Float64","NumberOfComponents=\"3\"")+"</Points>";
    auto topology=[&](const std::string& conn,const std::string& off,const std::string& type) {
        return "<Cells>"+array(conn,"Int32","Name=\"connectivity\"")+array(off,"Int32","Name=\"offsets\"")
            +array(type,"UInt8","Name=\"types\"")+"</Cells>";
    };
    auto piece=[&](const std::string& cells,const std::string& extra="") {return xml("<Piece NumberOfPoints=\"4\" NumberOfCells=\"1\">"+points+cells+extra+"</Piece>");};
    for (const auto& document:{piece(topology("0 1 2 99","4","10")),piece(topology("0 1 2 3","5","10")),
        piece(topology("0 1 2 3","-1","10")),piece(topology("0 1 2 3 0","4","10")),piece(topology("0 1 2 3","4","0")),
        piece(topology("0 1 2 3","4","42")),piece(topology("0 1 2 3","4","10"),"<PointData>"+array("1 2","Float64","Name=\"short\"")+"</PointData>")}) {
        RWVtk::VtkValidationReader validation; validation.SetToReportMessages(false); std::istringstream stream(document);
        require(!validation.PerformStream(stream,"invalid.vtu")&&!validation.Report().errors.empty(),"Malformed original topology/tuple accepted");
    }
    std::string modern="<Cells>"+array("0 1 2 3","Int64","Name=\"connectivity\"")+array("4","Int64","Name=\"offsets\"")
        +array("42","UInt8","Name=\"types\"")+array("0 1 2 0 1 3 0 2 3 1 2 99","Int64","Name=\"face_connectivity\"")
        +array("3 6 9 12","Int64","Name=\"face_offsets\"")+array("0 1 2 3","Int64","Name=\"polyhedron_to_faces\"")
        +array("4","Int64","Name=\"polyhedron_offsets\"")+"</Cells>";
    RWVtk::VtkValidationReader validation; validation.SetToReportMessages(false); std::istringstream invalidModern(piece(modern));
    require(!validation.PerformStream(invalidModern,"invalid.vtu"),"Modern polyhedron invalid face point accepted");
}
}
int main()
{
    try {matrix();std::cout<<"VTK_SAFETY_PASS\n";return 0;}
    catch(const Standard_Failure& e) {std::cerr<<e.GetMessageString()<<'\n';}
    catch(const std::exception& e) {std::cerr<<e.what()<<'\n';}
    return 1;
}
