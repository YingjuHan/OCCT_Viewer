#include <ViewCaeGeom/MeshGeometry.h>
#include <RWMeshConv/MeshLoader.h>
#include <OSD_FileSystem.hxx>
#include <OSD_ThreadPool.hxx>
#include <BVH_LinearBuilder.hxx>
#include <BVH_BinnedBuilder.hxx>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>

using Set=ViewCaeGeom::MeshBvhData::MeshBvhBoxSet;
using Builder=opencascade::handle<BVH_Builder<float,3>>;
using Clock=std::chrono::steady_clock;
struct Result { std::vector<RWCae::MeshEntityId> Ids; size_t Visited=0; };
struct Predicate {
    Graphic3d_Vec4d Plane; bool Center=false;
    template<class Vector> bool Accept(const Vector& minimum,const Vector& maximum) const {
        if(Center) {
            double distance=Plane.w(),radius=0;
            for(int a=0;a<3;++a) { distance+=Plane[a]*(double(minimum[a])+maximum[a])*.5; radius+=std::abs(Plane[a])*(double(maximum[a])-minimum[a])*.5; }
            return std::abs(distance)<=radius+1e-5;
        }
        double low=Plane.w(),high=low;
        for(int a=0;a<3;++a) { const double x=Plane[a]*minimum[a],y=Plane[a]*maximum[a]; low+=std::min(x,y); high+=std::max(x,y); }
        return low<=1e-5 && high>=-1e-5;
    }
};
struct Collector : BVH_Traverse<float,3,Set::SubBoxSet,bool> {
    Predicate Test; Result* Output;
    bool RejectNode(const BVH_Vec3f& minimum,const BVH_Vec3f& maximum,bool& inside) const override {
        inside=false; ++Output->Visited; return !Test.Accept(minimum,maximum);
    }
    bool Accept(int index,const bool&) override {
        const auto box=myBVHSet->Box(index);
        if(!Test.Accept(box.CornerMin(),box.CornerMax())) { return false; }
        Output->Ids.push_back(myBVHSet->Element(index)); return true;
    }
};
static Result query(const Handle(Set)& tree,const Predicate& test,bool parallel)
{
    std::vector<Result> pieces(tree->Size());
    const auto work=[&](int,int i) {
        const auto& subset=tree->SubSet(tree->Element(i)); const auto box=subset->Box();
        if(!test.Accept(box.CornerMin(),box.CornerMax())) { return; }
        Collector collector; collector.Test=test; collector.Output=&pieces[i]; collector.SetBVHSet(subset.get()); collector.Select();
    };
    if(parallel && tree->Size()>1) { OSD_ThreadPool::Launcher launcher(*OSD_ThreadPool::DefaultPool(),4); launcher.Perform(0,tree->Size(),work); }
    else { for(int i=0;i<tree->Size();++i) { work(0,i); } }
    Result result; size_t count=0; for(const auto& part:pieces) { count+=part.Ids.size(); result.Visited+=part.Visited; }
    result.Ids.reserve(count); for(const auto& part:pieces) { result.Ids.insert(result.Ids.end(),part.Ids.begin(),part.Ids.end()); } return result;
}
static Handle(Set) rebuild(const Handle(Set)& source,const std::function<Builder()>& factory)
{
    int count=0,capacity=0;
    for(int i=0;i<source->Size();++i) { const int s=source->Element(i); count=std::max(count,s+1); capacity=std::max(capacity,source->SubSet(s)->Size()); }
    Handle(Set) result=new Set(factory()); result->Init(count,capacity);
    for(int s=0;s<count;++s) { result->SubSet(s)->SetBuilder(factory()); }
    for(int i=0;i<source->Size();++i) { const int s=source->Element(i); const auto& part=source->SubSet(s);
        for(int e=0;e<part->Size();++e) { result->AddElement(s,part->Element(e),part->Box(e)); }
    }
    result->Update(); return result;
}
int main(int argc,char** argv)
{
    try {
        if(argc!=3) { throw std::invalid_argument("Usage: cut_bvh_benchmark generated.vtu output.csv"); }
        RWMeshConv::MeshLoader loader; const auto mesh=loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),argv[1]);
        if(mesh.IsNull() || mesh->NbIndexedElems()<1000000) { throw std::invalid_argument("BVH benchmark requires at least a million cells"); }
        Handle(ViewCaeGeom::MeshBvhData) bvh=new ViewCaeGeom::MeshBvhData();
        Handle(ViewCaeGeom::MeshGeometry) root=new ViewCaeGeom::MeshGeometryBase(bvh,mesh);
        const auto source=bvh->BvhBoxSet(root); const auto bounds=mesh->BoundingBox();
        const auto midpoint=(bounds.CornerMin().XYZ()+bounds.CornerMax().XYZ())*.5;
        const Graphic3d_Vec3d center(midpoint.X(),midpoint.Y(),midpoint.Z()); const double scale=bounds.CornerMax().X()-bounds.CornerMin().X();
        const std::pair<const char*,std::function<Builder()>> factories[]={
            {"morton",[] { return Builder(new BVH_LinearBuilder<float,3>()); }},
            {"sah16",[] { return Builder(new BVH_BinnedBuilder<float,3,16>()); }},
            {"sah32",[] { return Builder(new BVH_BinnedBuilder<float,3,32>()); }}
        };
        std::vector<Handle(Set)> trees; std::vector<double> builds;
        for(const auto& factory:factories) { const auto start=Clock::now(); trees.push_back(rebuild(source,factory.second)); builds.push_back(std::chrono::duration<double,std::milli>(Clock::now()-start).count()); }
        std::ofstream csv(argv[2]); if(!csv) { throw std::runtime_error("Cannot write BVH benchmark"); }
        csv<<"builder,query,frame,cells,subsets,build_ms,query_ms,candidates,visited\n";
        for(int frame=0;frame<66;++frame) {
            Graphic3d_Vec3d normal(std::cos(.005+.3*frame/60),std::sin(.005+.3*frame/60),.31*std::sin(.17*frame)); normal/=normal.Modulus();
            Graphic3d_Vec4d plane(normal.x(),normal.y(),normal.z(),-center.Dot(normal)-.125-3.4*frame/60);
            if(frame>=60) { plane=Graphic3d_Vec4d(1,0,0,-scale*.5+(frame-62)*1e-7); }
            std::vector<RWCae::MeshEntityId> reference;
            for(size_t b=0;b<trees.size();++b) { for(int mode=0;mode<4;++mode) {
                const bool centerTest=(mode&1)!=0,parallel=mode>=2;
                const auto start=Clock::now(); auto selected=query(trees[b],Predicate{plane,centerTest},parallel);
                const double time=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
                std::sort(selected.Ids.begin(),selected.Ids.end());
                if(b==0 && mode==0) { reference=selected.Ids; }
                else if(reference!=selected.Ids) { throw std::runtime_error("BVH query strategy changed exact candidate IDs"); }
                csv<<factories[b].first<<','<<(parallel?"parallel_":"serial_")<<(centerTest?"center":"low_high")<<','<<frame<<','<<mesh->NbIndexedElems()<<','<<trees[b]->Size()<<','<<builds[b]<<','<<time<<','<<selected.Ids.size()<<','<<selected.Visited<<'\n';
            } }
        }
        bvh->Clear(); root->RemoveChildren();
        std::cout<<"CUT_BVH_BENCHMARK_PASS cells "<<mesh->NbIndexedElems()<<" subsets "<<source->Size()<<" planes 66 comparisons 792\n"; return 0;
    } catch(const std::exception& e) { std::cerr<<"CUT_BVH_BENCHMARK_FAIL "<<e.what()<<'\n'; return 1; }
}
