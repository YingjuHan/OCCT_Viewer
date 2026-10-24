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
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace {
struct ProcessMemory
{
    size_t PrivateBytes=0, WorkingSetBytes=0, PeakWorkingSetBytes=0;
};
ProcessMemory processMemory()
{
    ProcessMemory result;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{}; counters.cb=sizeof(counters);
    if(!GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters))) {
        throw std::runtime_error("Benchmark process memory sampling failed");
    }
    result={counters.PrivateUsage,counters.WorkingSetSize,counters.PeakWorkingSetSize};
#endif
    return result;
}
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
        if(argc<5 || argc>9) { throw std::invalid_argument("Usage: scvcut[file]benchmark grid/file frames scenario outputDirectory [mask [auto/cpu [deformationScale [cloud/wire/cloud_wire]]]]"); }
        const bool fromFile = std::string(argv[0]) == "scvcutfilebenchmark";
        int n=fromFile ? 0 : std::stoi(argv[1]); const int frames=std::stoi(argv[2]); const std::string mode(argv[3]);
        const unsigned mask = argc > 5 ? (unsigned)std::stoul(argv[5]) : 2;
        const std::string backend = argc > 6 ? argv[6] : "auto";
        const double deformationScale = argc > 7 ? std::stod(argv[7]) : .005;
        const std::string displayMode=argc>8?argv[8]:"cloud";
        const auto* validationOption=std::getenv("CUTVIEW_BENCHMARK_FULL_VALIDATION");
        const bool fullValidation=validationOption && validationOption[0]=='1';
        if(fullValidation && !fromFile) { throw std::invalid_argument("Full validation requires a generated file"); }
        if(displayMode!="cloud" && displayMode!="wire" && displayMode!="cloud_wire") { throw std::invalid_argument("Invalid benchmark display mode"); }
        if (!std::isfinite(deformationScale)) { throw std::invalid_argument("Deformation scale must be finite"); }
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
            if (n < 2 || n > 200 || mesh->NbIndexedElems() < (uint64_t)n*n*n
                || mesh->NbNodes() < (uint64_t)(n+1)*(n+1)*(n+1) || field.IsNull()
                || std::abs(bounds.CornerMin().X())>1e-5 || std::abs(bounds.CornerMin().Y())>1e-5 || std::abs(bounds.CornerMin().Z())>1e-5
                || std::abs(bounds.CornerMax().X()-n)>1e-5 || std::abs(bounds.CornerMax().Y()-n)>1e-5 || std::abs(bounds.CornerMax().Z()-n)>1e-5
                || !field->IsNodal() || field->NbRawComponents() != 1
                || (mode == "deformed" && deformation.IsNull())) {
                throw std::invalid_argument("File benchmark requires a generated cubic linear grid and temperature/displacement fields");
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
        display->SetMeshDisplayMode(displayMode=="wire"?ViewCae::InteractiveCaeMeshPrsMode::MeshEdges:
            displayMode=="cloud_wire"?ViewCae::InteractiveCaeMeshPrsMode::ShadedMeshEdges:ViewCae::InteractiveCaeMeshPrsMode::FlatShaded);
        display->SetRenderBackend(backend == "auto" ? ViewCae::MeshCutRenderBackend::Auto : ViewCae::MeshCutRenderBackend::Cpu);
        if(mode=="deformed"&&!cuts.SetDeformation(deformation,deformationScale)) { throw std::runtime_error("Benchmark deformation rejected"); }
        if(!native.BindField({"temperature",true,0,-1})||!native.Attach(context,view)) { throw std::runtime_error(native.LastError()); }
        view->FitAll(); view->Redraw(); glFinish(); const double setupMs=ms(setup);
        const auto renderer=glGetString(GL_RENDERER); if(!renderer) { throw std::runtime_error("Benchmark has no current OpenGL rendering context"); }
        std::ofstream hardware(out/(mode+"-renderer.txt")); hardware<<renderer<<'\n'<<glGetString(GL_VERSION)<<'\n';
        std::ofstream log(out/(mode+".csv")); log<<"frame,edit_ms,source_ms,intersection_ms,slice_ms,trace_ms,FE_ms,primitives_ms,actual_redraw_ms,total_ms,nodes,cells,body_builds,submitted_body_bytes,submitted_slice_bytes,bvh_ms,node_prepare_ms,cell_cut_ms,merge_ms,candidates,fallbacks,trace_origins_ms,trace_classify_ms,trace_prepare_ms,trace_maps_ms,trace_fill_ms,trace_general_ms,interior_hits_total,interior_misses_total,interior_evictions_total,shared_point_ms,append_ms,fallback_ms,fallback_unsupported,fallback_near_plane,fallback_warped,fallback_degenerate,fallback_near_endpoint,topology_hits,topology_cell_hits,topology_cell_misses,process_private_bytes,process_working_set_bytes,process_peak_working_set_bytes\n";
        const auto initial = display->RenderStatistics();
        size_t fullChecks=0; double maximumAreaError=0;
        auto validateSection = [&](bool full) {
            const auto result=cuts.EvaluateSectionForDisplay("benchmark",field); const auto data=result.Portions[1]->GetMeshData();
            if(data->NbIndexedElems()==0||data->NbNodes()==0) { throw std::runtime_error("Benchmark produced an empty contour"); }
            ViewCaeGeom::MeshInterpolatedNode origin; if(!result.Portions[1]->OriginNode(0,origin)) { throw std::runtime_error("Benchmark lost FE provenance"); }
            double reference=0; for(const auto& w:origin.BaseNodes()) { const Graphic3d_Vec3d p(mesh->Nodes()[w.first]); reference+=w.second*(p.x()+2*p.y()+3*p.z()); }
            const double value=result.PhysicalFields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0));
            if(!std::isfinite(value)||std::abs(value-reference)>1e-6) { throw std::runtime_error("Benchmark original FE value mismatch"); }
            if(full) {
                const auto source=cuts.SourceGeometry(); const auto plane=result.Definition.Plane;
                const auto signedValue=[&](const Graphic3d_Vec3d& p) { return plane.x()*p.x()+plane.y()*p.y()+plane.z()*p.z()+plane.w(); };
                std::vector<Graphic3d_Vec3d> positions(data->NbNodes());
                for(RWCae::MeshEntityId node=0;node<data->NbNodes();++node) {
                    ViewCaeGeom::MeshInterpolatedNode weights(2); if(!result.Portions[1]->OriginNode(node,weights)) { throw std::runtime_error("Full reference lost FE origin"); }
                    double temperature=0,sum=0; positions[node]=Graphic3d_Vec3d(0);
                    for(const auto& w:weights.BaseNodes()) {
                        auto point=mesh->Nodes()[w.first]; temperature+=w.second*(double(point.x())+2*double(point.y())+3*double(point.z())); sum+=w.second;
                        source->ModifyNode(w.first,point); positions[node]+=Graphic3d_Vec3d(point)*w.second;
                    }
                    const auto actual=result.PhysicalFields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(node));
                    const double residual=signedValue(positions[node]);
                    if(!std::isfinite(sum) || !std::isfinite(residual) || !std::isfinite(temperature) || !std::isfinite(actual)
                        || std::abs(sum-1)>1e-9 || std::abs(residual)>2e-5 || std::abs(temperature-actual)>1e-6) { throw std::runtime_error("Full reference plane/FE mismatch"); }
                }
                double area=0;
                for(RWCae::MeshEntityId e=0;e<data->NbIndexedElems();++e) {
                    std::vector<RWCae::MeshEntityId> nodes; data->ElementNodes(e,nodes); Graphic3d_Vec3d cross(0);
                    for(size_t j=1;j+1<nodes.size();++j) { cross+=Graphic3d_Vec3d::Cross(positions[nodes[j]]-positions[nodes[0]],positions[nodes[j+1]]-positions[nodes[0]]); }
                    area+=.5*cross.Modulus();
                }
                // Independent whole-volume oracle: generated grids have a cube
                // boundary, transformed affinely by their displacement field.
                const int xyz[8][3]={{0,0,0},{n,0,0},{n,n,0},{0,n,0},{0,0,n},{n,0,n},{n,n,n},{0,n,n}};
                Graphic3d_Vec3d corners[8]; for(int c=0;c<8;++c) { const auto id=xyz[c][0]+(n+1)*(xyz[c][1]+(n+1)*xyz[c][2]); auto p=mesh->Nodes()[id]; source->ModifyNode(id,p); corners[c]=Graphic3d_Vec3d(p); }
                const int edges[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
                std::vector<Graphic3d_Vec3d> boundary;
                for(const auto& edge:edges) {
                    const auto a=corners[edge[0]],b=corners[edge[1]]; const double da=signedValue(a),db=signedValue(b);
                    if(std::abs(da)<1e-9) { boundary.push_back(a); }
                    if(da*db<0) { boundary.push_back(a+(b-a)*(da/(da-db))); }
                }
                if(boundary.size()<3) { throw std::runtime_error("Empty whole-volume area reference"); }
                Graphic3d_Vec3d center(0); for(const auto& p:boundary) { center+=p; } center/=(double)boundary.size();
                const Graphic3d_Vec3d normal(plane.x(),plane.y(),plane.z()); auto u=Graphic3d_Vec3d::Cross(normal,Graphic3d_Vec3d(0,0,1));
                if(u.Modulus()<1e-12) { u=Graphic3d_Vec3d::Cross(normal,Graphic3d_Vec3d(0,1,0)); } u/=u.Modulus(); const auto v=Graphic3d_Vec3d::Cross(normal,u);
                std::sort(boundary.begin(),boundary.end(),[&](const auto& a,const auto& b) { return std::atan2((a-center).Dot(v),(a-center).Dot(u))<std::atan2((b-center).Dot(v),(b-center).Dot(u)); });
                double expected=0; for(size_t j=1;j+1<boundary.size();++j) { expected+=.5*Graphic3d_Vec3d::Cross(boundary[j]-boundary[0],boundary[j+1]-boundary[0]).Modulus(); }
                const double error=std::abs(area-expected); maximumAreaError=std::max(maximumAreaError,error);
                if(!std::isfinite(area) || !std::isfinite(expected) || error>std::max(1.0,expected)*2e-7) { throw std::runtime_error("Full reference found missing/duplicate section area"); }
                ++fullChecks;
            }
            return std::pair<RWCae::MeshEntityId,RWCae::MeshEntityId>(data->NbNodes(),data->NbIndexedElems());
        };
        std::vector<double> timings; double sum=0;
        for(int i=0;i<frames;++i) {
            const auto start=Clock::now(); const double angle=mode=="rotating" ? .005+.3*i/frames : 0;
            const Graphic3d_Vec4d plane(std::cos(angle),std::sin(angle),0,-n*.5*(std::cos(angle)+std::sin(angle))-.125-3.4*i/frames);
            if(!cuts.SetPlane("benchmark",plane)) { throw std::runtime_error("Benchmark plane rejected"); } const double edit=ms(start);
            const auto prepare=Clock::now(); if(!native.Refresh(false)) { throw std::runtime_error(native.LastError()); } const double prepareMs=ms(prepare);
            const auto phases=cuts.LastTimings(); const auto cache=cuts.TraceCacheStatistics();
            if(phases.TopologyCellHits+phases.TopologyCellMisses!=phases.Candidates || phases.Fallbacks!=phases.Unsupported+phases.NearPlane+phases.Warped+phases.Degenerate+phases.NearEndpoint
                || phases.SharedPointMs+phases.AppendMs+phases.FallbackMs>phases.MergeMs+1e-6) {
                throw std::runtime_error("Section profiling counters do not partition fallback/merge work");
            }
            const auto redraw=Clock::now(); view->Redraw(); glFinish(); const double redrawMs=ms(redraw),total=ms(start);
            // Sample after stopping the frame timer, before numerical checks or
            // framebuffer captures. PeakWorkingSet is the process lifetime peak.
            const auto memorySample=processMemory();
            const auto stats = display->RenderStatistics();
            if (backend == "auto" && (!stats.GpuPlane || stats.BodyBuilds != initial.BodyBuilds || stats.SubmittedBodyBytes != 0)) {
                throw std::runtime_error("Warm plane frame rebuilt/submitted body or lost GPU backend");
            }
            if (mask == 5 && stats.SliceEvaluations != initial.SliceEvaluations) { throw std::runtime_error("Both-only frame evaluated a display slice"); }
            // Both-only has no display section. Validate its final numerical
            // section after timing all frames, so the next edit cannot include
            // cleanup of a section created solely by the benchmark assertion.
            const auto counts = mask == 5 ? std::pair<RWCae::MeshEntityId,RWCae::MeshEntityId>(0,0) : validateSection(fullValidation && (i==0 || i==frames-1));
            log<<i<<','<<edit<<','<<phases.SourceMs<<','<<phases.IntersectionMs<<','<<phases.SliceMs<<','<<phases.TraceMs<<','<<phases.FieldsMs<<','<<std::max(0.0,prepareMs-phases.SourceMs-phases.IntersectionMs-phases.FieldsMs)<<','<<redrawMs<<','<<total<<','<<counts.first<<','<<counts.second<<','<<stats.BodyBuilds<<','<<stats.SubmittedBodyBytes<<','<<stats.SubmittedSliceBytes<<','<<phases.BvhMs<<','<<phases.NodePrepareMs<<','<<phases.CellCutMs<<','<<phases.MergeMs<<','<<phases.Candidates<<','<<phases.Fallbacks<<','<<phases.TraceOriginsMs<<','<<phases.TraceClassifyMs<<','<<phases.TracePrepareMs<<','<<phases.TraceMapsMs<<','<<phases.TraceFillMs<<','<<phases.TraceGeneralMs<<','<<cache.Hits<<','<<cache.Misses<<','<<cache.Evictions<<','<<phases.SharedPointMs<<','<<phases.AppendMs<<','<<phases.FallbackMs<<','<<phases.Unsupported<<','<<phases.NearPlane<<','<<phases.Warped<<','<<phases.Degenerate<<','<<phases.NearEndpoint<<','<<phases.TopologyHits<<','<<phases.TopologyCellHits<<','<<phases.TopologyCellMisses<<','<<memorySample.PrivateBytes<<','<<memorySample.WorkingSetBytes<<','<<memorySample.PeakWorkingSetBytes<<'\n'; timings.push_back(total); sum+=total;
            if(i==0||i==frames-1) { if(!view->Dump((out/(mode+"-"+std::to_string(i)+".png")).string().c_str())) { throw std::runtime_error("Benchmark framebuffer capture failed"); } }
        }
        if (mask == 5) { validateSection(fullValidation); }
        std::sort(timings.begin(),timings.end()); const double p95=timings[(size_t)std::ceil(.95*frames)-1],fps=1000*frames/sum;
        di<<"cells "<<(int)mesh->NbIndexedElems()<<" frames "<<frames<<" scenario "<<mode.c_str()<<" mask "<<(int)mask<<" backend "<<backend.c_str()<<" display_mode "<<displayMode.c_str()<<" full_checks "<<(double)fullChecks<<" maximum_area_error "<<maximumAreaError<<" deformation_scale "<<(mode=="deformed"?deformationScale:0)<<" body_builds "<<(double)display->RenderStatistics().BodyBuilds<<" import_ms "<<importMs<<" setup_ms "<<setupMs<<" p50_ms "<<timings[frames/2]<<" p95_ms "<<p95<<" fps "<<fps<<" actual_redraw 1 original_FE 1 target "<<(int)(fps>=15&&p95<=67);
        native.Close(); return 0;
    } catch(const Standard_Failure& e) { di<<"Error: "<<e.GetMessageString(); }
    catch(const std::exception& e) { di<<"Error: "<<e.what(); } return 1;
}
}
namespace ViewCaeTest { void RegisterCutBenchmarkCommands(Draw_Interpretor& di) { di.Add("scvcutbenchmark","Full native contour/primitive/redraw benchmark",__FILE__,benchmark,"SC Viewer CAE Prs"); di.Add("scvcutfilebenchmark","File import and full-frame benchmark for generated cubic linear grids",__FILE__,benchmark,"SC Viewer CAE Prs"); di.Add("scvcutmemory","Native process resource sample",__FILE__,memory,"SC Viewer CAE Prs"); di.Add("scvcutframewait","Wait for current viewport GPU work without another redraw (benchmarking)",__FILE__,frameWait,"SC Viewer CAE Prs"); } }
