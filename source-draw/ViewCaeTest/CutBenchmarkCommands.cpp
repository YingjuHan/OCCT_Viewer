#include <Viewer/NativeCutView.h>
#include <ViewerTest.hxx>
#include <Draw_Interpretor.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <RWCae/MeshScalarProperty.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace {
int memory(Draw_Interpretor& di,int argc,const char**)
{
    if(argc!=1) { return 1; }
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{}; counters.cb=sizeof(counters);
    if(!GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters))) { return 1; }
    di<<"private_bytes "<<(double)counters.PrivateUsage<<" working_set "<<(double)counters.WorkingSetSize;
#else
    di<<"unsupported 1";
#endif
    return 0;
}
int benchmark(Draw_Interpretor& di,int argc,const char** argv)
{
    try {
        if(argc!=5) { throw std::invalid_argument("Usage: scvcutbenchmark grid frames crossing/rotating/deformed outputDirectory"); }
        const int n=std::stoi(argv[1]),frames=std::stoi(argv[2]); const std::string mode(argv[3]);
        if(n<2||n>150||frames<2||frames>1000||(mode!="crossing"&&mode!="rotating"&&mode!="deformed")) { throw std::invalid_argument("Invalid benchmark parameters"); }
        const auto context=ViewerTest::GetAISContext(); const auto view=ViewerTest::CurrentView();
        if(context.IsNull()||view.IsNull()) { throw std::invalid_argument("Benchmark requires an actual native viewport"); }
        const std::filesystem::path out(argv[4]); std::filesystem::create_directories(out);
        using Clock=std::chrono::steady_clock; auto ms=[](Clock::time_point begin) { return std::chrono::duration<double,std::milli>(Clock::now()-begin).count(); };
        const auto setup=Clock::now(); Handle(RWCae::MeshData) mesh=new RWCae::MeshData();
        Handle(RWCae::MeshNVecProperty) field=new RWCae::MeshNVecProperty("temperature",1,RWCae::MeshPropertyBase::MappingType::Nodal); field->Init<RWCae::MeshScalarProperty>();
        auto scalar=Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(0)); scalar->Init((n+1)*(n+1)*(n+1));
        Handle(RWCae::MeshNVecProperty) deformation=new RWCae::MeshNVecProperty("displacement",3,RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector); deformation->Init<RWCae::MeshScalarProperty>();
        for(int c=0;c<3;++c) { Handle(RWCae::MeshScalarProperty)::DownCast(deformation->GetComponent(c))->Init((n+1)*(n+1)*(n+1)); }
        auto node=[n](int x,int y,int z)->RWCae::MeshEntityId { return x+(n+1)*(y+(n+1)*z); };
        for(int z=0;z<=n;++z) { for(int y=0;y<=n;++y) { for(int x=0;x<=n;++x) {
            mesh->AddNodePosition(Graphic3d_Vec3((float)x,(float)y,(float)z)); const auto id=RWCae::MeshPropertyEntityInfo(node(x,y,z)); scalar->SetValue(id,x+2*y+3*z,true);
            deformation->GetComponent(0)->SetValue(id,.1*std::sin(3.141592653589793*y/n),false); deformation->GetComponent(1)->SetValue(id,.05*x/n,false); deformation->GetComponent(2)->SetValue(id,.03*z/n,false);
        } } }
        field->UpdateComponents(); deformation->UpdateComponents(); mesh->ChangeProperties().Append(field); mesh->ChangeProperties().Append(deformation);
        for(int z=0;z<n;++z) { for(int y=0;y<n;++y) { for(int x=0;x<n;++x) { const RWCae::MeshEntityId ids[]={node(x,y,z),node(x+1,y,z),node(x+1,y+1,z),node(x,y+1,z),node(x,y,z+1),node(x+1,y,z+1),node(x+1,y+1,z+1),node(x,y+1,z+1)}; mesh->AddElement(RWCae::MeshElementHexahedron(ids)); } } }
        Viewer::NativeCutView native(mesh); auto display=native.Display(); auto& cuts=display->Cuts(); cuts.AddPlane("benchmark",Graphic3d_Vec4d(1,0,0,-n*.5-.125));
        if(mode=="deformed"&&!cuts.SetDeformation(deformation,.005)) { throw std::runtime_error("Benchmark deformation rejected"); }
        if(!native.BindField({"temperature",true,0,-1})||!native.Attach(context,view)) { throw std::runtime_error(native.LastError()); }
        view->FitAll(); view->Redraw(); glFinish(); const double setupMs=ms(setup);
        const auto renderer=glGetString(GL_RENDERER); if(!renderer) { throw std::runtime_error("Benchmark has no current OpenGL rendering context"); }
        std::ofstream hardware(out/(mode+"-renderer.txt")); hardware<<renderer<<'\n'<<glGetString(GL_VERSION)<<'\n';
        std::ofstream log(out/(mode+".csv")); log<<"frame,edit_ms,source_ms,intersection_ms,slice_ms,trace_ms,FE_ms,primitives_ms,actual_redraw_ms,total_ms,nodes,cells\n";
        std::vector<double> timings; double sum=0;
        for(int i=0;i<frames;++i) {
            const auto start=Clock::now(); const double angle=mode=="rotating" ? .005+.3*i/frames : 0;
            const Graphic3d_Vec4d plane(std::cos(angle),std::sin(angle),0,-n*.5*(std::cos(angle)+std::sin(angle))-.125-3.4*i/frames);
            if(!cuts.SetPlane("benchmark",plane)) { throw std::runtime_error("Benchmark plane rejected"); } const double edit=ms(start);
            const auto prepare=Clock::now(); if(!native.Refresh(false)) { throw std::runtime_error(native.LastError()); } const double prepareMs=ms(prepare);
            const auto phases=cuts.LastTimings();
            const auto redraw=Clock::now(); view->Redraw(); glFinish(); const double redrawMs=ms(redraw),total=ms(start);
            const auto result=cuts.Evaluate("benchmark",field); const auto data=result.Portions[1]->GetMeshData();
            if(data->NbIndexedElems()==0||data->NbNodes()==0) { throw std::runtime_error("Benchmark produced an empty contour"); }
            ViewCaeGeom::MeshInterpolatedNode origin; if(!result.Portions[1]->OriginNode(0,origin)) { throw std::runtime_error("Benchmark lost FE provenance"); }
            double reference=0; for(const auto& w:origin.BaseNodes()) { const auto p=mesh->Nodes()[w.first]; reference+=w.second*(p.x()+2*p.y()+3*p.z()); }
            const double value=result.PhysicalFields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0));
            if(!std::isfinite(value)||std::abs(value-reference)>1e-6) { throw std::runtime_error("Benchmark original FE value mismatch"); }
            log<<i<<','<<edit<<','<<phases.SourceMs<<','<<phases.IntersectionMs<<','<<phases.SliceMs<<','<<phases.TraceMs<<','<<phases.FieldsMs<<','<<std::max(0.0,prepareMs-phases.SourceMs-phases.IntersectionMs-phases.FieldsMs)<<','<<redrawMs<<','<<total<<','<<data->NbNodes()<<','<<data->NbIndexedElems()<<'\n'; timings.push_back(total); sum+=total;
            if(i==0||i==frames-1) { if(!view->Dump((out/(mode+"-"+std::to_string(i)+".png")).string().c_str())) { throw std::runtime_error("Benchmark framebuffer capture failed"); } }
        }
        std::sort(timings.begin(),timings.end()); const double p95=timings[(size_t)std::ceil(.95*frames)-1],fps=1000*frames/sum;
        di<<"cells "<<(int)mesh->NbIndexedElems()<<" frames "<<frames<<" scenario "<<mode.c_str()<<" setup_ms "<<setupMs<<" p50_ms "<<timings[frames/2]<<" p95_ms "<<p95<<" fps "<<fps<<" actual_redraw 1 original_FE 1 target "<<(int)(fps>=15&&p95<=67);
        native.Close(); return 0;
    } catch(const Standard_Failure& e) { di<<"Error: "<<e.GetMessageString(); }
    catch(const std::exception& e) { di<<"Error: "<<e.what(); } return 1;
}
}
namespace ViewCaeTest { void RegisterCutBenchmarkCommands(Draw_Interpretor& di) { di.Add("scvcutbenchmark","Full native contour/primitive/redraw benchmark",__FILE__,benchmark,"SC Viewer CAE Prs"); di.Add("scvcutmemory","Native process resource sample",__FILE__,memory,"SC Viewer CAE Prs"); } }
