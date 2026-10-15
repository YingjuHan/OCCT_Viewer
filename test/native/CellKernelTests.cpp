#include <ViewCaeGeom/MeshCellKernel.h>
#include <rapidjson/document.h>
#include <fstream>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <utility>
#include <cmath>
#include <stdexcept>

using ViewCaeGeom::MeshCellKernel;
namespace {
void check(bool value,const char* message) {if(!value) {throw std::runtime_error(message);}}
void reference()
{
    std::ifstream input(CELL_SHAPE_REFERENCE);std::string text((std::istreambuf_iterator<char>(input)),{});
    rapidjson::Document data;data.Parse(text.c_str());check(!data.HasParseError() && data.IsArray(),"Invalid FE reference data");
    for(const auto& record:data.GetArray()) {
        const int type=record["type"].GetInt(),n=(int)record["points"].Size();MeshCellKernel cell(type,n);
        const double* coords=cell.GetParametricCoords();
        for(int i=0;i<n;++i) {double p[3];for(int a=0;a<3;++a) {p[a]=record["points"][i][a].GetDouble();check(std::abs(coords[3*i+a]-p[a])<1e-14,"Reference node order changed");}cell.GetPoints()->SetPoint(i,p);}
        double maximum=0;
        for(const auto& sample:record["samples"].GetArray()) {
            double q[3];for(int a=0;a<3;++a) {q[a]=sample["parameter"][a].GetDouble();}
            std::vector<double> weights(n);cell.InterpolateFunctions(q,weights.data());
            for(int i=0;i<n;++i) {maximum=std::max(maximum,std::abs(weights[i]-sample["weights"][i].GetDouble()));}
        }
        std::cout<<"CELL_SHAPE_REFERENCE "<<type<<" maximum "<<maximum<<'\n';
        check(maximum<(type==15 ? 1e-5 : 1e-10),"Finite-element shape function regression");
    }
}
void highOrder()
{
    for(int type:{68,69,70,71,72,73,75,76,77,78,79,80}) {
        const int family=type>=75 ? type-75 : type-68;
        const int sizes[]={5,15,20,35,100,75};const std::vector<int> degrees={4,family==2||family==4 ? 3 : 4,4};
        MeshCellKernel cell(type,sizes[family],degrees);const double* coords=cell.GetParametricCoords();const int n=cell.GetNumberOfPoints();
        for(int i=0;i<n;++i) {cell.GetPoints()->SetPoint(i,coords+3*i);}
        double q[]={.21,.23,.27},p[3];int sub=0;std::vector<double> w(n);cell.EvaluateLocation(sub,q,p,w.data());
        check(std::abs(std::accumulate(w.begin(),w.end(),0.)-1)<1e-10,"High-order partition of unity");
        for(int a=0;a<cell.GetCellDimension();++a) {check(std::abs(p[a]-q[a])<1e-10,"High-order affine reproduction");}
        if(type<75) {for(int i=0;i<n;++i) {cell.InterpolateFunctions(coords+3*i,w.data());for(int j=0;j<n;++j) {check(std::abs(w[j]-(i==j?1:0))<1e-10,"Lagrange nodal cardinality");}}}
        for(int e=0;e<cell.GetNumberOfEdges();++e) {check(cell.GetEdge(e)->GetParametricCoords()!=nullptr,"Missing high-order edge chart");}
        for(int f=0;f<cell.GetNumberOfFaces();++f) {check(cell.GetFace(f)->GetParametricCoords()!=nullptr,"Missing high-order face chart");}
    }
}
void extendedOrders()
{
    // Enriched Lagrange elements retain their extra nodal DOFs.
    for (const auto& item : {std::pair<int,int>{69,7},{71,15},{73,21}}) {
        MeshCellKernel cell(item.first,item.second,{2,2,2});
        const double* coordinates = cell.GetParametricCoords();
        std::vector<double> weights(item.second);
        for (int i = 0; i < item.second; ++i) {
            cell.InterpolateFunctions(coordinates+3*i,weights.data());
            for (int j = 0; j < item.second; ++j) { check(std::abs(weights[j]-(i==j)) < 1e-10,"Enriched Lagrange cardinality"); }
        }
    }
    MeshCellKernel curve(75,1025,{1024,0,0});
    double q[] = {.5,0,0}; std::vector<double> weights(1025);
    curve.InterpolateFunctions(q,weights.data());
    check(std::abs(std::accumulate(weights.begin(),weights.end(),0.)-1) < 3e-12,"High-degree Bernstein partition");
    const auto coordinates = curve.GetParametricCoords(); double mean = 0;
    for (int i = 0; i < 1025; ++i) { check(std::isfinite(weights[i]) && weights[i] >= 0,"High-degree Bernstein finite coefficient"); mean += coordinates[3*i]*weights[i]; }
    check(std::abs(mean-.5) < 2e-12,"High-degree Bernstein affine reproduction");
    // The middle coefficient is C(1024,512)/2^1024, evaluated independently.
    auto middle = std::find_if(weights.begin(),weights.end(),[&](double value) { return std::abs(value-0.024927805892979545) < 1e-12; });
    check(middle != weights.end(),"High-degree Bernstein central coefficient");
}
void polyhedronAffine()
{
    // A translated skew cage probes the near-face limit independently of FE tables.
    const std::array<std::array<double,3>,8> corners = {{{0,0,0},{1,0,0},{1,1,0},{0,1,0},
        {0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
    MeshCellKernel cell(42,8);
    cell.SetFaces({{0,3,2,1},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}});
    auto transform = [](const std::array<double,3>& p) {
        return std::array<double,3>{{-20+40*p[0]+3*p[1]+2*p[2],
            600+2*p[0]+48*p[1]+4*p[2],150+p[0]+2*p[1]+35*p[2]}};
    };
    for (int i = 0; i < 8; ++i) { const auto p = transform(corners[i]); cell.GetPoints()->SetPoint(i,p.data()); }
    double maximum = 0;
    for (double offset : {-1e-5,-1e-7,0.,1e-7,1e-5,.3}) {
        for (const auto& reference : {std::array<double,3>{{.28,.33,offset}},
            std::array<double,3>{{offset,.33,.28}},std::array<double,3>{{.28,1-offset,.33}}}) {
            const auto query = transform(reference); double closest[3],parameter[3],distance; int sub;
            std::vector<double> weights(8);
            check(cell.EvaluatePosition(query.data(),closest,sub,parameter,distance,weights.data()) == 1,
                "Polyhedron inverse map failed");
            check(std::abs(std::accumulate(weights.begin(),weights.end(),0.)-1) < 1e-12,
                "Polyhedron partition of unity");
            for (int a = 0; a < 3; ++a) { maximum = std::max(maximum,std::abs(closest[a]-query[a])); }
        }
    }
    std::cout << "POLYHEDRON_AFFINE maximum " << maximum << '\n';
    check(maximum < 1e-7,"Near-face polyhedron affine reproduction");
    cell.SetFaces({{0,3,2,1},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6}});
    bool rejected = false;
    try { cell.GetNumberOfFaces(); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected,"Open polyhedron cage was accepted");
}
}
int main()
{
    try {reference();highOrder();extendedOrders();polyhedronAffine();std::cout<<"CELL_KERNEL_PASS\n";return 0;}
    catch(const std::exception& error) {std::cerr<<"CELL_KERNEL_FAIL: "<<error.what()<<'\n';return 1;}
}
