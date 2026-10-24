#include <ViewCaeGeom/MeshCutViewManager.h>
#include <iostream>
#include <stdexcept>
static void require(bool v,const char* m) { if(!v) { throw std::runtime_error(m); } }
template<class F> static void rejects(F f) { bool rejected=false; try { f(); } catch(const std::exception&) { rejected=true; } require(rejected,"cache resource/request limit accepted"); }
int main()
{
    try {
        Handle(RWCae::MeshData) mesh=new RWCae::MeshData(); for(auto p:{Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) { mesh->AddNodePosition(p); } RWCae::MeshEntityId ns[]={0,1,2,3}; mesh->AddElement(RWCae::MeshElementTetrahedron(ns));
        ViewCaeGeom::MeshCutViewManager m(mesh); m.AddPlane("a",Graphic3d_Vec4d(1,0,0,-.25));
        const auto source=m.EstimatedSourceBytes(),result=m.EstimatedResultBytes();
        require(m.SetCachePolicy({source-1,result*2}),"source policy"); rejects([&] { m.Evaluate("a"); }); require(m.CacheStatistics().SourceReserved==0&&m.CacheStatistics().ResultReserved==0,"source rejection allocated caches");
        require(m.SetCachePolicy({source,result-1}),"result policy"); rejects([&] { m.Evaluate("a"); }); require(m.CacheStatistics().ResultReserved==0,"result rejection allocated caches");
        require(m.SetCachePolicy({source,result}),"exact reservations"); m.Evaluate("a"); m.AddPlane("b",Graphic3d_Vec4d(0,1,0,-.25));
        auto held=m.Evaluate("b"); require(m.CacheStatistics().Evictions==1,"LRU did not evict old result");
        rejects([&] { m.Evaluate("a"); }); require(m.CacheStatistics().ResultReserved==result,"exported result escaped cache charge");
        held={}; auto live=m.Evaluate("a"); require(m.CacheStatistics().SourceReserved<=source&&m.CacheStatistics().ResultReserved<=result,"hard reservation limit exceeded");
        require(!m.SetCachePolicy({source-1,result}),"shrinking below pinned resources accepted");
        auto token=std::make_shared<ViewCaeGeom::MeshCutCancellation>(); const auto generation=m.BeginRequest(token); token->Cancel(); rejects([&] { m.Evaluate("a"); });
        auto next=std::make_shared<ViewCaeGeom::MeshCutCancellation>(); require(m.BeginRequest(next)>generation&&token->Cancelled.load(),"stale generation remained live"); m.Evaluate("a");
        for(int i=0;i<200;++i) { require(m.SetPlane("a",Graphic3d_Vec4d(1,0,0,-.2-.001*(i%100))),"long session edit"); m.Evaluate("a"); require(m.CacheStatistics().ResultReserved<=result,"long session cache growth"); }
        m.Clear(); require(next->Cancelled.load(),"close did not cancel pending request"); require(m.CacheStatistics().ResultReserved>0,"held result released budget early"); live={};
        require(m.CacheStatistics().ResultReserved==0&&m.CacheStatistics().SourceReserved==0,"old geometry reservation leaked after release");
        ViewCaeGeom::MeshCutViewManager previous(mesh),candidate(mesh); previous.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.25)); previous.SetCachePolicy({source*2,result*2}); auto pinned=previous.Evaluate("p");
        require(candidate.ShareCacheFrom(previous),"candidate shared accounting"); candidate.AddPlane("q",Graphic3d_Vec4d(0,1,0,-.25));
        require(candidate.SetCachePolicy({source*2,result}),"candidate local policy"); rejects([&] { candidate.Evaluate("q"); });
        require(previous.CachePolicy().ResultBytes==result*2,"failed candidate changed live policy");
        require(candidate.SetCachePolicy({source*2,result*2}),"candidate combined policy"); auto other=candidate.Evaluate("q");
        require(candidate.CacheStatistics().ResultReserved==result*2,"candidate omitted pinned previous epoch"); previous.Clear(); candidate.Clear(); pinned={}; other={};
        require(candidate.CacheStatistics().ResultReserved==0 && candidate.CacheStatistics().SourceReserved==0,"shared accounting leaked");
        ViewCaeGeom::MeshCutViewManager restored(mesh); restored.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.2));
        ViewCaeGeom::MeshCutTraceOptions negative; negative.Policy=ViewCaeGeom::MeshCutTracePolicy::Negative;
        require(restored.SetTraceOptions("p",false,negative),"restore trace fixture");
        const auto snapshot=restored.DumpDefinitionsJson(); const auto used=restored.CacheStatistics();
        require(restored.SetCachePolicy({used.SourceReserved,used.ResultReserved*3}),"tight restore policy");
        require(!restored.RestoreDefinitionsJson(snapshot)&&restored.DumpDefinitionsJson()==snapshot,"definition validator escaped shared source budget");
        require(restored.CacheStatistics().SourceReserved==used.SourceReserved&&restored.CacheStatistics().ResultReserved==used.ResultReserved,"failed validation leaked reservations");
        ViewCaeGeom::MeshInterpolatedNode ab,ba,nearA,a; ab.AddBaseNode(100,.4); ab.AddBaseNode(101,.6); ba.AddBaseNode(101,.6); ba.AddBaseNode(100,.4); nearA.AddBaseNode(100,.9999); nearA.AddBaseNode(101,.0001); a.AddBaseNode(100,1);
        require(ViewCaeGeom::MeshInterpolatedNode::IsEqual(ab,ba)&&ViewCaeGeom::MeshInterpolatedNode::HashCode(ab,1009)==ViewCaeGeom::MeshInterpolatedNode::HashCode(ba,1009),"edge orientation changed hashing");
        require(ViewCaeGeom::MeshInterpolatedNode::IsEqual(nearA,a)&&ViewCaeGeom::MeshInterpolatedNode::HashCode(nearA,1009)==ViewCaeGeom::MeshInterpolatedNode::HashCode(a,1009),"near-node equality/hash disagreement");
        require(!ViewCaeGeom::MeshInterpolatedNode::IsEqual(ab,nearA),"different active supports merged");
        std::cout<<"CUT_CACHE_PASS\n"; return 0;
    } catch(const Standard_Failure& e) { std::cerr<<"CUT_CACHE_FAIL "<<e.GetMessageString()<<'\n'; }
    catch(const std::exception& e) { std::cerr<<"CUT_CACHE_FAIL "<<e.what()<<'\n'; } return 1;
}
