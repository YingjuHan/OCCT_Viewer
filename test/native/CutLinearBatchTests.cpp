#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshGeometrySlice.h>
#include <RWCae/MeshScalarProperty.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string>

using namespace ViewCaeGeom;
static void require(bool value,const char* message) { if(!value) { throw std::runtime_error(message); } }
struct QueryMode {
    bool Present=false; std::string Previous;
    QueryMode() { const auto* value=std::getenv("OCCTV_CUT_PARALLEL_QUERY"); Present=value!=nullptr; if(value) { Previous=value; } Set("1"); }
    static void Set(const char* value) {
#ifdef _WIN32
        _putenv_s("OCCTV_CUT_PARALLEL_QUERY",value?value:"");
#else
        if(value) { setenv("OCCTV_CUT_PARALLEL_QUERY",value,1); } else { unsetenv("OCCTV_CUT_PARALLEL_QUERY"); }
#endif
    }
    ~QueryMode() { Set(Present?Previous.c_str():nullptr); }
};

static void check(RWCae::MeshElemType type,int n)
{
    QueryMode queryMode;
    const auto start=std::chrono::steady_clock::now();
    Handle(RWCae::MeshData) mesh=new RWCae::MeshData();
    const auto id=[n](int x,int y,int z) { return RWCae::MeshEntityId(x+(n+1)*(y+(n+1)*z)); };
    for(int z=0;z<=n;++z) { for(int y=0;y<=n;++y) { for(int x=0;x<=n;++x) { mesh->AddNodePosition(Graphic3d_Vec3(x,y,z)); } } }
    for(int z=0;z<n;++z) { for(int y=0;y<n;++y) { for(int x=0;x<n;++x) {
        const RWCae::MeshEntityId h[]={id(x,y,z),id(x+1,y,z),id(x+1,y+1,z),id(x,y+1,z),id(x,y,z+1),id(x+1,y,z+1),id(x+1,y+1,z+1),id(x,y+1,z+1)};
        if(type==RWCae::MeshElemType::Tetrahedron) {
            const int cells[6][4]={{0,1,2,6},{0,2,3,6},{0,3,7,6},{0,7,4,6},{0,4,5,6},{0,5,1,6}};
            for(const auto& c:cells) { const RWCae::MeshEntityId nodes[]={h[c[0]],h[c[1]],h[c[2]],h[c[3]]}; mesh->AddElement(RWCae::MeshElementTetrahedron(nodes)); }
        } else if(type==RWCae::MeshElemType::Prism) {
            const RWCae::MeshEntityId a[]={h[0],h[1],h[2],h[4],h[5],h[6]},b[]={h[0],h[2],h[3],h[4],h[6],h[7]};
            mesh->AddElement(RWCae::MeshElementPrism(a)); mesh->AddElement(RWCae::MeshElementPrism(b));
        } else {
            const auto center=mesh->AddNodePosition(Graphic3d_Vec3(x+.5f,y+.5f,z+.5f));
            const int faces[6][4]={{0,1,2,3},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}};
            for(const auto& f:faces) { const RWCae::MeshEntityId nodes[]={h[f[0]],h[f[1]],h[f[2]],h[f[3]],center}; mesh->AddElement(RWCae::MeshElementPyramid(nodes)); }
        }
    } } }
    require(mesh->NbIndexedElems()>=1000000,"Linear batch reference must contain a million cells");
    Handle(RWCae::MeshNVecProperty) field=new RWCae::MeshNVecProperty("temperature",1,RWCae::MeshPropertyBase::MappingType::Nodal);
    field->Init<RWCae::MeshScalarProperty>(); const auto scalar=Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(0)); scalar->Init(mesh->NbNodes());
    for(RWCae::MeshEntityId node=0;node<mesh->NbNodes();++node) { const auto& p=mesh->Nodes()[node]; scalar->SetValue(RWCae::MeshPropertyEntityInfo(node),p.x()+2*p.y()+3*p.z()); }
    field->UpdateComponents();
    MeshCutViewManager cuts(mesh); const double initialOffset=n*.5+.371;
    for(double movement:{0.0,.01,-.02,1.37}) {
    const double slope=std::sqrt(.03)+(movement==.01?1e-4:0);
    const double offset=initialOffset+movement;
    if(movement==0) { require(cuts.AddPlane("p",Graphic3d_Vec4d(1,slope,0,-offset)),"Linear batch plane rejected"); }
    else { require(cuts.SetPlane("p",Graphic3d_Vec4d(1,slope,0,-offset)),"Incremental linear plane rejected"); }
    const auto result=cuts.EvaluateSectionForDisplay("p",field); const auto section=result.Portions[1]; const auto output=section->GetMeshData();
    const auto timings=cuts.LastTimings();
    if(movement==0) {
        QueryMode::Set("0"); MeshCutViewManager serial(mesh); serial.AddPlane("p",Graphic3d_Vec4d(1,slope,0,-offset));
        const auto reference=serial.EvaluateSectionForDisplay("p",field); const auto data=reference.Portions[1]->GetMeshData(); QueryMode::Set("1");
        require(data->NbNodes()==output->NbNodes() && data->NbIndexedElems()==output->NbIndexedElems(),"Parallel query changed section counts");
        for(RWCae::MeshEntityId node=0;node<output->NbNodes();++node) {
            MeshInterpolatedNode a(2),b(2); section->OriginNode(node,a); reference.Portions[1]->OriginNode(node,b);
            require(output->Nodes()[node]==data->Nodes()[node] && a.BaseNodes()==b.BaseNodes(),"Parallel query changed node order or FE weights");
        }
        for(RWCae::MeshEntityId e=0;e<output->NbIndexedElems();++e) {
            std::vector<RWCae::MeshEntityId> a,b; output->ElementNodes(e,a); data->ElementNodes(e,b);
            const auto& ai=output->IndexedElements()[e]; const auto& bi=data->IndexedElements()[e]; RWCae::MeshEntityId ao,bo;
            require(a==b && section->OriginElement(MeshGeomElem(ai.Index,ai.Type,false),ao)
                && reference.Portions[1]->OriginElement(MeshGeomElem(bi.Index,bi.Type,false),bo) && ao==bo,"Parallel query changed connectivity or original cell identity");
        }
    }
    require(timings.Candidates>4096 && timings.Fallbacks<timings.Candidates,"Parallel linear batch path was not exercised");
    if(movement==.01 && MeshGeometrySlice::CellTopologyReuseEnabled()) { require(timings.TopologyCellHits>0,"Small plane translation did not reuse cell topology"); }
    require(timings.TopologyCellHits+timings.TopologyCellMisses==timings.Candidates,"Cell topology counters do not partition candidates");
    require(timings.Unsupported==0 && timings.Warped==0 && timings.Degenerate==0,"Regular linear cells unexpectedly used unsupported/warped fallback");
    std::vector<Graphic3d_Vec3d> positions(output->NbNodes());
    for(RWCae::MeshEntityId node=0;node<output->NbNodes();++node) {
        MeshInterpolatedNode origin; require(section->OriginNode(node,origin),"Missing linear FE provenance");
        double sum=0; positions[node]=Graphic3d_Vec3d(0);
        for(const auto& w:origin.BaseNodes()) { positions[node]+=Graphic3d_Vec3d(mesh->Nodes()[w.first])*w.second; sum+=w.second; }
        const auto p=positions[node]; require(std::abs(sum-1)<1e-10 && std::abs(p.x()+slope*p.y()-offset)<1e-8,"Linear FE reconstruction is off plane");
        require(std::abs(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(node))-(p.x()+2*p.y()+3*p.z()))<1e-7,"Linear scalar interpolation mismatch");
    }
    double area=0;
    for(RWCae::MeshEntityId e=0;e<output->NbIndexedElems();++e) {
        std::vector<RWCae::MeshEntityId> nodes; output->ElementNodes(e,nodes); require(nodes.size()>=3,"Invalid section polygon");
        for(size_t j=1;j+1<nodes.size();++j) { area+=.5*Graphic3d_Vec3d::Cross(positions[nodes[j]]-positions[nodes[0]],positions[nodes[j+1]]-positions[nodes[0]]).Modulus(); }
    }
    require(std::abs(area-n*n*std::sqrt(1+slope*slope))<1e-5,"Million-cell linear section has missing or duplicate area");
    std::cout<<"LINEAR_BATCH type "<<(int)type<<" cells "<<mesh->NbIndexedElems()<<" candidates "<<timings.Candidates
             <<" topology_hits "<<timings.TopologyCellHits<<" fallbacks "<<timings.Fallbacks<<" elapsed_ms "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<'\n';
    }
}
int main()
{
    try { check(RWCae::MeshElemType::Tetrahedron,60); check(RWCae::MeshElemType::Prism,80); check(RWCae::MeshElemType::Pyramid,56); std::cout<<"CUT_LINEAR_BATCH_PASS\n"; return 0; }
    catch(const std::exception& e) { std::cerr<<"CUT_LINEAR_BATCH_FAIL "<<e.what()<<'\n'; return 1; }
}
