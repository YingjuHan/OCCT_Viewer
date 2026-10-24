#ifndef OCCTV_TEST_CELLFIXTURE_HEADERFILE
#define OCCTV_TEST_CELLFIXTURE_HEADERFILE
#include <ViewCaeGeom/MeshCellKernel.h>
#include <RWCae/MeshData.h>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <functional>
#include <map>
#include <stdexcept>

namespace CellFixture {
using Fields=std::map<std::string,std::vector<double>>;
inline std::vector<double> Values(const Handle(RWCae::MeshNVecProperty)& field)
{
    std::vector<double> values;for(int i=0;i<field->NbValues();++i) {values.push_back(field->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(i)));}return values;
}
inline std::vector<double> Values(const Handle(RWCae::MeshData)& mesh,const std::function<double(const Graphic3d_Vec3&)>& f)
{
    std::vector<double> values;for(const auto& p:mesh->Nodes()) {values.push_back(f(p));}return values;
}
//! Minimal ASCII fixture encoder; tests do not require an external VTK writer.
inline void Write(const Handle(RWCae::MeshData)& mesh,const std::string& path,Fields nodal,Fields elemental)
{
    std::filesystem::create_directories(std::filesystem::u8path(path).parent_path());
    std::ofstream out(std::filesystem::u8path(path));out<<std::setprecision(17);
    auto array=[&](const char* type,const std::string& name,int components,const std::vector<double>& values) {
        out<<"<DataArray type=\""<<type<<"\" Name=\""<<name<<"\" NumberOfComponents=\""<<components<<"\" format=\"ascii\">";
        for(double v:values) {out<<v<<' ';}out<<"</DataArray>";
    };
    out<<"<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid><Piece NumberOfPoints=\""<<mesh->NbNodes()<<"\" NumberOfCells=\""<<mesh->GenericCells().size()<<"\"><Points>";
    std::vector<double> points;for(const auto& p:mesh->Nodes()) {for(int a=0;a<3;++a) {points.push_back(p[a]);}}array("Float64","Points",3,points);out<<"</Points><Cells>";
    std::vector<double> connectivity,offsets,types,faces,faceOffsets,degrees,weights(mesh->NbNodes(),1);bool hasFaces=false,hasDegrees=false,hasWeights=false;
    for(const auto& cell:mesh->GenericCells()) {
        connectivity.insert(connectivity.end(),cell.Nodes.begin(),cell.Nodes.end());offsets.push_back((double)connectivity.size());types.push_back(cell.Type);
        if(!cell.Faces.empty()) {hasFaces=true;faces.push_back((double)cell.Faces.size());for(const auto& f:cell.Faces) {faces.push_back((double)f.size());faces.insert(faces.end(),f.begin(),f.end());}faceOffsets.push_back((double)faces.size());}else {faceOffsets.push_back(-1);}
        hasDegrees|=!cell.Degrees.empty();for(int a=0;a<3;++a) {degrees.push_back(cell.Degrees.empty()?0:cell.Degrees.at(a));}
        hasWeights|=!cell.RationalWeights.empty();if(!cell.RationalWeights.empty()) {for(size_t i=0;i<cell.Nodes.size();++i) {weights.at(cell.Nodes[i])=cell.RationalWeights.at(i);}}
    }
    array("Int32","connectivity",1,connectivity);array("Int32","offsets",1,offsets);array("UInt8","types",1,types);
    if(hasFaces) {array("Int32","faces",1,faces);array("Int32","faceoffsets",1,faceOffsets);}out<<"</Cells><PointData"<<(hasWeights?" RationalWeights=\"RationalWeights\"":"")<<">";
    for(const auto& f:nodal) {array("Float64",f.first,1,f.second);}if(hasWeights) {array("Float64","RationalWeights",1,weights);}out<<"</PointData><CellData"<<(hasDegrees?" HigherOrderDegrees=\"HigherOrderDegrees\"":"")<<">";
    for(const auto& f:elemental) {array("Float64",f.first,1,f.second);}if(hasDegrees) {array("Int32","HigherOrderDegrees",3,degrees);}out<<"</CellData></Piece></UnstructuredGrid></VTKFile>";
    if(!out.good()) {throw std::runtime_error("Cannot write native cell fixture");}
}
}
#endif
