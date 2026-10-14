#include <Viewer/NativeCutView.h>
#include <ViewerTest.hxx>
#include <Draw_Interpretor.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <RWCae/MeshScalarProperty.h>
#include <RWMeshConv/MeshLoader.h>
#include <OSD_FileSystem.hxx>
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
int frameWait(Draw_Interpretor& di,int argc,const char**)
{
    if (argc != 1 || ViewerTest::CurrentView().IsNull() || glGetString(GL_VERSION) == nullptr) {
        di << "Error: scvcutframewait requires a current native OpenGL viewport";
        return 1;
    }
    glFinish();
    return 0;
}
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
        if(argc<5 || argc>7) { throw std::invalid_argument("Usage: scvcutbenchmark grid frames scenario outputDirectory [mask [auto/cpu]]; scvcutfilebenchmark file frames scenario outputDirectory [mask [auto/cpu]]"); }
        const bool fromFile = std::string(argv[0]) == "scvcutfilebenchmark";
        int n=fromFile ? 0 : std::stoi(argv[1]); const int frames=std::stoi(argv[2]); const std::string mode(argv[3]);
        const unsigned mask = argc > 5 ? (unsigned)std::stoul(argv[5]) : 2;
        const std::string backend = argc > 6 ? argv[6] : "auto";
        if (mask > 7 || (backend != "auto" && backend != "cpu")) { throw std::invalid_argument("Invalid mask/backend"); }
        if((!fromFile && (n<2||n>200))||frames<2||frames>1000||(mode!="crossing"&&mode!="rotating"&&mode!="deformed")) { throw std::invalid_argument("Invalid benchmark parameters"); }
        const auto context=ViewerTest::GetAISContext(); const auto view=ViewerTest::CurrentView();
        if(context.IsNull()||view.IsNull()) { throw std::invalid_argument("Benchmark requires an actual native viewport"); }
        const std::filesystem::path out(argv[4]); std::filesystem::create_directories(out);
        using Clock=std::chrono::steady_clock; auto ms=[](Clock::time_point begin) { return std::chrono::duration<double,std::milli>(Clock::now()-begin).count(); };
        const auto setup=Clock::now(); Handle(RWCae::MeshData) mesh;
        Handle(RWCae::MeshNVecProperty) field, deformation;
        double importMs = 0;
        if (fromFile) {
            RWMeshConv::MeshLoader loader;
            mesh = loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),argv[1]);
            importMs = ms(setup);
            if (mesh.IsNull()) {
                const auto& errors = loader.VtkReport().errors;
                throw std::runtime_error(errors.empty() ? "File benchmark import failed" : errors.front());
            }
            for (const auto& property : mesh->Properties()) {
                if (property->Name() == "temperature") { field = property; }
                if (property->Name() == "displacement") { deformation = property; }
            }
            Bnd_Box bounds;
            for (const auto& p : mesh->Nodes()) { bounds.Add(gp_Pnt(p.x(),p.y(),p.z())); }
            n = (int)std::lround(bounds.CornerMax().X());
            if (n < 2 || n > 200 || mesh->NbIndexedElems() != (uint64_t)n*n*n
                || mesh->NbNodes() != (uint64_t)(n+1)*(n+1)*(n+1) || field.IsNull()
                || !field->IsNodal() || field->NbRawComponents() != 1
                || (mode == "deformed" && deformation.IsNull())) {
                throw std::invalid_argument("File benchmark requires generate-large-vtk.py regular grid and fields");
            }
            const auto& report = loader.VtkReport();
            std::ofstream imported(out/(mode+"-import.txt"));
            imported << "file " << argv[1] << "\nbackend " << report.backend << "\npoints " << report.points
                     << "\ncells " << report.cells << "\nimport_ms " << importMs << '\n';
        } else {
        mesh=new RWCae::MeshData();
        field=new RWCae::MeshNVecProperty("temperature",1,RWCae::MeshPropertyBase::MappingType::Nodal); field->Init<RWCae::MeshScalarProperty>();
        auto scalar=Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(0)); scalar->Init((n+1)*(n+1)*(n+1));
        deformation=new RWCae::MeshNVecProperty("displacement",3,RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector); deformation->Init<RWCae::MeshScalarProperty>();
        for(int c=0;c<3;++c) { Handle(RWCae::MeshScalarProperty)::DownCast(deformation->GetComponent(c))->Init((n+1)*(n+1)*(n+1)); }
        auto node=[n](int x,int y,int z)->RWCae::MeshEntityId { return x+(n+1)*(y+(n+1)*z); };
        for(int z=0;z<=n;++z) { for(int y=0;y<=n;++y) { for(int x=0;x<=n;++x) {
            mesh->AddNodePosition(Graphic3d_Vec3((float)x,(float)y,(float)z)); const auto id=RWCae::MeshPropertyEntityInfo(node(x,y,z)); scalar->SetValue(id,x+2*y+3*z,true);
            deformation->GetComponent(0)->SetValue(id,.1*std::sin(3.141592653589793*y/n),false); deformation->GetComponent(1)->SetValue(id,.05*x/n,false); deformation->GetComponent(2)->SetValue(id,.03*z/n,false);
        } } }
        field->UpdateComponents(); deformation->UpdateComponents(); mesh->ChangeProperties().Append(field); mesh->ChangeProperties().Append(deformation);
        for(int z=0;z<n;++z) { for(int y=0;y<n;++y) { for(int x=0;x<n;++x) { const RWCae::MeshEntityId ids[]={node(x,y,z),node(x+1,y,z),node(x+1,y+1,z),node(x,y+1,z),node(x,y,z+1),node(x+1,y,z+1),node(x+1,y+1,z+1),node(x,y+1,z+1)}; mesh->AddElement(RWCae::MeshElementHexahedron(ids)); } } }
        }
        Viewer::NativeCutView native(mesh); auto display=native.Display(); auto& cuts=display->Cuts(); cuts.AddPlane("benchmark",Graphic3d_Vec4d(1,0,0,-n*.5-.125));
        cuts.SetVisibility("benchmark",mask);
        display->SetRenderBackend(backend == "auto" ? ViewCae::MeshCutRenderBackend::Auto : ViewCae::MeshCutRenderBackend::Cpu);
        if(mode=="deformed"&&!cuts.SetDeformation(deformation,.005)) { throw std::runtime_error("Benchmark deformation rejected"); }
        if(!native.BindField({"temperature",true,0,-1})||!native.Attach(context,view)) { throw std::runtime_error(native.LastError()); }
        view->FitAll(); view->Redraw(); glFinish(); const double setupMs=ms(setup);
        const auto renderer=glGetString(GL_RENDERER); if(!renderer) { throw std::runtime_error("Benchmark has no current OpenGL rendering context"); }
        std::ofstream hardware(out/(mode+"-renderer.txt")); hardware<<renderer<<'\n'<<glGetString(GL_VERSION)<<'\n';
        std::ofstream log(out/(mode+".csv")); log<<"frame,edit_ms,source_ms,intersection_ms,slice_ms,trace_ms,FE_ms,primitives_ms,actual_redraw_ms,total_ms,nodes,cells,body_builds,submitted_body_bytes,submitted_slice_bytes,bvh_ms,node_prepare_ms,cell_cut_ms,merge_ms,candidates,fallbacks,trace_origins_ms,trace_classify_ms,trace_prepare_ms,trace_maps_ms,trace_fill_ms,trace_general_ms,interior_hits_total,interior_misses_total,interior_evictions_total\n";
        const auto initial = display->RenderStatistics();
        auto validateSection = [&]() {
            const auto result=cuts.EvaluateSectionForDisplay("benchmark",field); const auto data=result.Portions[1]->GetMeshData();
            if(data->NbIndexedElems()==0||data->NbNodes()==0) { throw std::runtime_error("Benchmark produced an empty contour"); }
            ViewCaeGeom::MeshInterpolatedNode origin; if(!result.Portions[1]->OriginNode(0,origin)) { throw std::runtime_error("Benchmark lost FE provenance"); }
            double reference=0; for(const auto& w:origin.BaseNodes()) { const auto p=mesh->Nodes()[w.first]; reference+=w.second*(p.x()+2*p.y()+3*p.z()); }
            const double value=result.PhysicalFields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0));
            if(!std::isfinite(value)||std::abs(value-reference)>1e-6) { throw std::runtime_error("Benchmark original FE value mismatch"); }
            return std::pair<RWCae::MeshEntityId,RWCae::MeshEntityId>(data->NbNodes(),data->NbIndexedElems());
        };
        std::vector<double> timings; double sum=0;
        for(int i=0;i<frames;++i) {
            const auto start=Clock::now(); const double angle=mode=="rotating" ? .005+.3*i/frames : 0;
            const Graphic3d_Vec4d plane(std::cos(angle),std::sin(angle),0,-n*.5*(std::cos(angle)+std::sin(angle))-.125-3.4*i/frames);
            if(!cuts.SetPlane("benchmark",plane)) { throw std::runtime_error("Benchmark plane rejected"); } const double edit=ms(start);
            const auto prepare=Clock::now(); if(!native.Refresh(false)) { throw std::runtime_error(native.LastError()); } const double prepareMs=ms(prepare);
            const auto phases=cuts.LastTimings(); const auto cache=cuts.TraceCacheStatistics();
            const auto redraw=Clock::now(); view->Redraw(); glFinish(); const double redrawMs=ms(redraw),total=ms(start);
            const auto stats = display->RenderStatistics();
            if (backend == "auto" && (!stats.GpuPlane || stats.BodyBuilds != initial.BodyBuilds || stats.SubmittedBodyBytes != 0)) {
                throw std::runtime_error("Warm plane frame rebuilt/submitted body or lost GPU backend");
            }
            if (mask == 5 && stats.SliceEvaluations != initial.SliceEvaluations) { throw std::runtime_error("Both-only frame evaluated a display slice"); }
            // Both-only has no display section. Validate its final numerical
            // section after timing all frames, so the next edit cannot include
            // cleanup of a section created solely by the benchmark assertion.
            const auto counts = mask == 5 ? std::pair<RWCae::MeshEntityId,RWCae::MeshEntityId>(0,0) : validateSection();
            log<<i<<','<<edit<<','<<phases.SourceMs<<','<<phases.IntersectionMs<<','<<phases.SliceMs<<','<<phases.TraceMs<<','<<phases.FieldsMs<<','<<std::max(0.0,prepareMs-phases.SourceMs-phases.IntersectionMs-phases.FieldsMs)<<','<<redrawMs<<','<<total<<','<<counts.first<<','<<counts.second<<','<<stats.BodyBuilds<<','<<stats.SubmittedBodyBytes<<','<<stats.SubmittedSliceBytes<<','<<phases.BvhMs<<','<<phases.NodePrepareMs<<','<<phases.CellCutMs<<','<<phases.MergeMs<<','<<phases.Candidates<<','<<phases.Fallbacks<<','<<phases.TraceOriginsMs<<','<<phases.TraceClassifyMs<<','<<phases.TracePrepareMs<<','<<phases.TraceMapsMs<<','<<phases.TraceFillMs<<','<<phases.TraceGeneralMs<<','<<cache.Hits<<','<<cache.Misses<<','<<cache.Evictions<<'\n'; timings.push_back(total); sum+=total;
            if(i==0||i==frames-1) { if(!view->Dump((out/(mode+"-"+std::to_string(i)+".png")).string().c_str())) { throw std::runtime_error("Benchmark framebuffer capture failed"); } }
        }
        if (mask == 5) { validateSection(); }
        std::sort(timings.begin(),timings.end()); const double p95=timings[(size_t)std::ceil(.95*frames)-1],fps=1000*frames/sum;
        di<<"cells "<<(int)mesh->NbIndexedElems()<<" frames "<<frames<<" scenario "<<mode.c_str()<<" mask "<<(int)mask<<" backend "<<backend.c_str()<<" body_builds "<<(double)display->RenderStatistics().BodyBuilds<<" import_ms "<<importMs<<" setup_ms "<<setupMs<<" p50_ms "<<timings[frames/2]<<" p95_ms "<<p95<<" fps "<<fps<<" actual_redraw 1 original_FE 1 target "<<(int)(fps>=15&&p95<=67);
        native.Close(); return 0;
    } catch(const Standard_Failure& e) { di<<"Error: "<<e.GetMessageString(); }
    catch(const std::exception& e) { di<<"Error: "<<e.what(); } return 1;
}
}
namespace ViewCaeTest { void RegisterCutBenchmarkCommands(Draw_Interpretor& di) { di.Add("scvcutbenchmark","Full native contour/primitive/redraw benchmark",__FILE__,benchmark,"SC Viewer CAE Prs"); di.Add("scvcutfilebenchmark","File import and full-frame benchmark for generate-large-vtk.py grids",__FILE__,benchmark,"SC Viewer CAE Prs"); di.Add("scvcutmemory","Native process resource sample",__FILE__,memory,"SC Viewer CAE Prs"); di.Add("scvcutframewait","Wait for current viewport GPU work without another redraw (benchmarking)",__FILE__,frameWait,"SC Viewer CAE Prs"); } }
