#include <Viewer/NativeCutSession.h>
#include <Viewer/NativeCutDigest.h>
#include <Viewer/NativeCutAnalysis.h>
#include <Draw.hxx>
#include <Draw_Interpretor.hxx>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <sstream>
#include <chrono>
namespace {
namespace fs=std::filesystem;
void require(bool v,const std::string& m) { if(!v) { throw std::runtime_error(m); } }
std::string read(const fs::path& p) { std::ifstream in(p,std::ios::binary); return {(std::istreambuf_iterator<char>(in)),{}}; }
void write(const fs::path& p,const std::string& s) { std::ofstream out(p,std::ios::binary); out<<s; require(bool(out),"test fixture write"); }
std::string json(const rapidjson::Document& d) { rapidjson::StringBuffer b; rapidjson::Writer<rapidjson::StringBuffer> w(b); d.Accept(w); return {b.GetString(),b.GetSize()}; }
void digests()
{
    using D=Viewer::NativeCutDigest;
    require(D::Text("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855","SHA empty");
    require(D::Text("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","SHA abc");
    const std::string block="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    require(D::Text(block)=="248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1","SHA multiblock");
    require(D::Text(std::string(1000000,'a'))=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0","SHA million a");
    D chunked; for(char c:block) { chunked.Update(&c,1); } require(chunked.Finish()==D::Text(block),"SHA chunked");
    bool rejected=false; try { chunked.Finish(); } catch(const std::exception&) { rejected=true; } require(rejected,"SHA double finish");
}
std::string dataset(bool remesh)
{
    std::ostringstream body; const int count=remesh?5:4;
    body<<"<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid><Piece NumberOfPoints=\""<<count<<"\" NumberOfCells=\""<<(remesh?2:1)<<"\"><Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">0 0 0 1 0 0 0 1 0 0 0 1"<<(remesh?" 1 1 1":"")<<"</DataArray></Points><Cells><DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 2 3"<<(remesh?" 1 2 3 4":"")<<"</DataArray><DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">4"<<(remesh?" 8":"")<<"</DataArray><DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">10"<<(remesh?" 10":"")<<"</DataArray></Cells><PointData Vectors=\"displacement\">";
    body<<"<DataArray type=\"Float64\" Name=\"displacement\" NumberOfComponents=\"3\" format=\"ascii\">";
    for(int i=0;i<count;++i) { body<<"1 0 0 "; } body<<"</DataArray><DataArray type=\"Float64\" Name=\"density\" format=\"ascii\">0 1 0 0"<<(remesh?" 1":"")<<"</DataArray></PointData></Piece></UnstructuredGrid></VTKFile>"; return body.str();
}
void seriesTests(const fs::path& out,bool remesh)
{
    fs::create_directories(out); write(out/"a.vtu",dataset(false)); write(out/"b.vtu",dataset(remesh));
    const auto source=out/"model.vtm.series";
    write(out/"a.vtm","<VTKFile type=\"vtkMultiBlockDataSet\" byte_order=\"LittleEndian\"><vtkMultiBlockDataSet><DataSet index=\"0\" file=\"a.vtu\"/></vtkMultiBlockDataSet></VTKFile>");
    write(out/"b.vtm","<VTKFile type=\"vtkMultiBlockDataSet\" byte_order=\"LittleEndian\"><vtkMultiBlockDataSet><DataSet index=\"0\" file=\"b.vtu\"/></vtkMultiBlockDataSet></VTKFile>");
    write(source,"{\"files\":[{\"name\":\"a.vtm\",\"time\":0.25},{\"name\":\"b.vtm\",\"time\":1.75}]}");
    RWMeshConv::MeshLoader loader; const auto mesh=loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),source.string().c_str()); require(!mesh.IsNull(),"session series import");
    Viewer::NativeCutView native(mesh); Viewer::NativeCutTimeline timeline(native); Viewer::NativeCutSession session(native,timeline);
    const auto file=out/"series.json";
    require(session.Save(file.string(),source.string()),native.LastError()); // Initial legacy series root is supported.
    require(timeline.SwitchStep(0),native.LastError());
    auto& cuts=native.Display()->Cuts(); require(cuts.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.25)),"series plane");
    require(native.BindField({"density",true,0,0}),native.LastError());
    Viewer::CutMaterialFrame material; material.Configuration=Viewer::CutFrameConfiguration::Updated; material.Anchors={{0,{0,1,2}},{1,{0,1,2}}};
    require(timeline.SetMaterialFrame("material",material)&&timeline.BindCutFrame("p","material"),native.LastError());
    require(timeline.BindDeformation({"displacement",true,0,0},.05),native.LastError());
    auto recovery=native.Display()->Cuts().RecoveryOptions(); recovery.ThresholdPercent=35; recovery.Regions={7};
    require(native.Display()->Cuts().SetRecoveryOptions(recovery),"session recovery"); timeline.SetRecoveryForStep(0,recovery);
    if(remesh) { recovery.Regions={7,8}; } timeline.SetRecoveryForStep(1,recovery);
    ViewCaeGeom::MeshCutTraceOptions trace; trace.Regions={7}; timeline.SetTraceForStep(0,"p",trace,trace);
    if(remesh) { trace.Regions={7,8}; } timeline.SetTraceForStep(1,"p",trace,trace);
    Viewer::NativeCutAnalysisRequest heat; heat.Cut="p"; heat.Heat=true; heat.Field={"displacement",true,0,0}; heat.Components={{0,1,2,0,0,0,0,0,0}}; heat.Options.Elements={0};
    require(native.Analysis().Set("heat",heat),native.LastError()); native.Analysis().SetForStep(0,"heat",heat);
    heat.Field.Step=1; if(remesh) { heat.Options.Elements={0,1}; } native.Analysis().SetForStep(1,"heat",heat);
    ViewCaeGeom::MeshCutAutomaticMapping mapping; mapping.Kind=ViewCaeGeom::MeshCutAutomaticKind::OptimizationDensity; mapping.Threshold=.4;
    require(native.Analysis().Automatic("auto",{"density",true,0,0},mapping),native.LastError());
    require(timeline.SwitchStep(1)&&timeline.PlayTime(2,true),native.LastError());
    const auto definitions=native.Display()->Cuts().DumpDefinitionsJson(); const double reference=native.Analysis().Reports().at("heat").Total.HeatFlow;
    require(session.Save(file.string(),source.string()),native.LastError()); const auto body=read(file);
    require(session.Load(file.string()),native.LastError());
    require(timeline.Step()==1&&timeline.IsPlaying()&&native.Display()->Cuts().DumpDefinitionsJson()==definitions,"series definitions/step/playback roundtrip");
    require(timeline.MaterialFrames().size()==1&&timeline.CutFrames().size()==1&&native.Analysis().AutomaticBindings().size()==1,"material/automatic roundtrip");
    require(std::abs(native.Analysis().Reports().at("heat").Total.HeatFlow-reference)<1e-9,"series physical roundtrip");
    require(timeline.SwitchStep(0)&&timeline.SwitchStep(1),native.LastError());
    for(int test=0;test<9;++test) {
        rapidjson::Document d; d.Parse(body.c_str()); auto& a=d.GetAllocator(); auto& step=d["analysisSteps"][0];
        switch(test) {
        case 0: step["id"].SetString("unknown",a); break;
        case 1: d["analysisSteps"].PushBack(rapidjson::Value(step,a),a); break;
        case 2: step["request"]["elements"][0].SetInt(999); break;
        case 3: step["request"]["units"]["length"][1].SetDouble(-1); break;
        case 4: step["request"]["components"][0].SetInt(999); break;
        case 5: step["request"]["cut"].SetString("unknown",a); break;
        case 6: d["automatic"][0]["threshold"].SetDouble(.6); break;
        case 7: d["materialFrames"][0]["anchors"][0][1].SetUint(999); break;
        case 8: d["following"].PushBack(rapidjson::Value(d["following"][0],a),a); break;
        }
        const auto old=native.Display(); const auto generation=native.Generation(); write(out/"invalid.json",json(d));
        require(!session.Load((out/"invalid.json").string())&&native.Display()==old&&native.Generation()==generation&&timeline.Step()==1,"invalid series mapping changed live session");
    }
    // Even malformed mappings inserted through low-level setters cannot publish an invalid save.
    heat.Field.Step=1; heat.Options.Elements={999}; native.Analysis().SetForStep(1,"heat",heat);
    require(!session.Save(file.string(),source.string())&&read(file)==body,"invalid mapping save replaced document");
}
void tests()
{
    digests(); const fs::path out=NATIVE_SESSION_OUTPUT_DIR; fs::create_directories(out);
    {
        // Captured with HEAD 047207f and VTK 9.5.2, before removal of the SDK.
        // This mixed dataset includes a polyline, quad and Float32 vector/scalars.
        const std::string source=NATIVE_CUT_FIXTURE_DIR "/sdk_session_general.vtu";
        RWMeshConv::MeshLoader loader; const auto mesh=loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),source.c_str());
        require(!mesh.IsNull() && mesh->GenericCells().size()==2,"v3 general source representation");
        Viewer::NativeCutView owner(mesh); Viewer::NativeCutTimeline timeline(owner); Viewer::NativeCutSession session(owner,timeline);
        const auto file=out/"sdk-general.json"; require(session.Save(file.string(),source),owner.LastError());
        rapidjson::Document document; document.Parse(read(file).c_str());
        require(std::string(document["source"]["meshDigest"].GetString())=="231807589d04ac6a8ace2311ad2bccf7576ae65d7fdcba22406f105ffc31b3e2",
            "SDK v3 session fingerprint compatibility");
        require(session.Load(file.string()),owner.LastError());
    }
    RWMeshConv::MeshLoader loader; const std::string source=NATIVE_CUT_FIXTURE_DIR "/stress_heat.vtu";
    const auto mesh=loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),source.c_str()); require(!mesh.IsNull(),"source import");
    // A rejected numeric edit must restore the definition without preparing a
    // second JSON validator while the previous presentation pins the budget.
    {
        Viewer::NativeCutView bounded(mesh); auto& cuts=bounded.Display()->Cuts();
        require(cuts.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.25))&&bounded.BindField({"heat_flux",true,0,-1}),bounded.LastError());
        ViewCaeGeom::MeshCutTraceOptions negative; negative.Policy=ViewCaeGeom::MeshCutTracePolicy::Negative;
        require(cuts.SetTraceOptions("p",false,negative)&&bounded.Refresh(false),"bounded edit trace fixture");
        const auto snapshot=cuts.DumpDefinitionsJson(); const auto used=cuts.CacheStatistics();
        const auto generation=bounded.Generation(); const auto events=bounded.EventCounts();
        const auto synchronized=bounded.Display()->SynchronizationCount();
        require(cuts.SetCachePolicy({used.SourceReserved,used.ResultReserved}),"bounded edit policy");
        cuts.InvalidateGeometry(); // Old published geometry remains charged across a source epoch reset.
        auto changed=cuts.Definitions()[0]; changed.Plane=Graphic3d_Vec4d(0,1,0,-.4);
        require(!bounded.Edit(changed)&&!bounded.LastError().empty(),"over-budget edit accepted");
        require(cuts.DumpDefinitionsJson()==snapshot&&bounded.Generation()==generation&&bounded.EventCounts()==events
            &&bounded.Display()->SynchronizationCount()==synchronized,"failed edit changed live definition/events/presentation");
        require(cuts.CacheStatistics().SourceReserved==used.SourceReserved&&cuts.CacheStatistics().ResultReserved==used.ResultReserved,"failed edit leaked reservations");
        ViewCaeGeom::MeshCutAutomaticMapping mapping; mapping.Threshold=2;
        require(!bounded.Analysis().Automatic("rejected",{"heat_flux",true,0,-1},mapping)
            &&cuts.DumpDefinitionsJson()==snapshot&&bounded.Analysis().AutomaticBindings().empty(),"failed automatic cut left a definition/binding");
    }
    Viewer::NativeCutView native(mesh); Viewer::NativeCutTimeline timeline(native); Viewer::NativeCutSession session(native,timeline);
    auto display=native.Display(); auto& cuts=display->Cuts(); require(cuts.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.25)),"session plane");
    require(native.BindField({"heat_flux",true,1,-1}),native.LastError());
    auto style=display->Style(ViewCaeGeom::MeshCutPortion::On); style.Transparency=.35f; style.LineWidth=2; display->SetStyle("p",ViewCaeGeom::MeshCutPortion::On,style);
    auto contour=display->ContourOptions(); contour.AutomaticRange=false; contour.Minimum=-2; contour.Maximum=10; contour.ShowLegend=true; contour.Intervals=8; display->SetContourOptions(contour);
    display->SetVectorField(mesh->Properties()[1],.2,.5,true);
    ViewCaeGeom::MeshCutFrame datum; datum.Origin=Graphic3d_Vec3d(.1,.2,.3); require(native.SetDatum("datum",datum)&&native.SetReferenceAxes("datum",true,2),native.LastError());
    ViewCae::MeshCutProbe point; point.Position=Graphic3d_Vec3d(.25,.1,.2); require(native.AddAnnotation("note",point,"Physical heat flux"),native.LastError());
    Viewer::NativeCutAnalysisRequest r; r.Cut="p"; r.Heat=true; r.Field={"heat_flux",true,0,-1}; r.Components={{0,1,2,0,0,0,0,0,0}}; require(native.Analysis().Set("heat",r),native.LastError());
    const double heat=native.Analysis().Reports().at("heat").Total.HeatFlow;
    const auto cad=BRepPrimAPI_MakeBox(1,1,1).Shape(); const auto cadfile=out/"source.brep"; BRepTools::Write(cad,cadfile.string().c_str());
    require(native.SetCadSection(cad,"p",true),native.LastError());
    native.GlobalClip()->SetOn(false);
    const auto file=out/"complete.json"; require(session.Save(file.string(),source,cadfile.string()),native.LastError()); const auto body=read(file);
    require(session.Load(file.string()),native.LastError()); require(native.Display()!=display&&display->Cuts().Definitions().empty(),"old session handle still live");
    require(native.FieldBinding().Name=="heat_flux"&&native.FieldBinding().Component==1,"field binding roundtrip");
    require(native.Display()->StyleOverrides().size()==1&&std::abs(native.Display()->EffectiveStyle("p",ViewCaeGeom::MeshCutPortion::On).Transparency-.35)<1e-6,"style roundtrip");
    require(native.Display()->ContourOptions().Intervals==8&&native.Display()->VectorOrientation(),"display options roundtrip");
    require(native.Datums().size()==1&&native.ReferenceDatum()=="datum"&&native.Annotations().size()==1,"frames/annotations roundtrip");
    require(!native.CadSection().IsNull()&&!native.GlobalClip().IsNull()&&!native.GlobalClip()->IsOn(),"CAD/clip roundtrip");
    require(std::abs(native.Analysis().Reports().at("heat").Total.HeatFlow-heat)<1e-9,"physical roundtrip");
    require(session.Save(file.string(),session.Source(),session.CadSource()),native.LastError()); require(session.Load(file.string()),native.LastError());
    const auto current=read(file);
    {
        auto& liveCuts=native.Display()->Cuts(); const auto policy=liveCuts.CachePolicy(); const auto used=liveCuts.CacheStatistics();
        const auto generation=native.Generation(); const auto old=native.Display();
        require(liveCuts.SetCachePolicy({used.SourceReserved,policy.ResultBytes}),"tight session save policy");
        require(!session.Save(file.string(),session.Source(),session.CadSource())&&read(file)==current
            &&native.Display()==old&&native.Generation()==generation,"save validator escaped live source budget");
        require(liveCuts.CacheStatistics().SourceReserved==used.SourceReserved&&liveCuts.SetCachePolicy(policy),"failed save validator leaked/change policy");
    }
    require(native.BeginDrag(),native.LastError());
    auto reject=[&](const std::string& changed) { const auto old=native.Display(); const auto generation=native.Generation(); const auto policy=old->Cuts().CachePolicy(); write(out/"invalid.json",changed); require(!session.Load((out/"invalid.json").string()),"invalid session accepted"); require(native.Display()==old&&native.Generation()==generation&&native.IsDragging()&&old->Cuts().CachePolicy().ResultBytes==policy.ResultBytes,"failed restore changed live session"); };
    reject("{"); reject("[]");
    reject("{\"format\":\"occt-cut-native\","+current.substr(1));
    for(int test=0;test<14;++test) { rapidjson::Document d; d.Parse(current.c_str()); auto& a=d.GetAllocator();
        switch(test) {
        case 0: d["version"].SetInt(99); break;
        case 1: d["source"]["bundle"].SetString("../escape",a); break;
        case 2: d["field"]["name"].SetString("missing",a); break;
        case 3: d["source"]["files"][0]["sha256"].SetString("wrong",a); break;
        case 4: d["cache"][1].SetUint64(1); break;
        case 5: d["styles"][0]["alpha"].SetDouble(2); break;
        case 6: d["playback"]["period"].SetDouble(-1); break;
        case 7: d["geometry"]["absolute"].SetDouble(0); break;
        case 8: d["deformation"]["name"].SetString("missing",a); break;
        case 9: d["analysis"][0]["id"].SetString("",a); break;
        case 10: d["datums"].PushBack(rapidjson::Value(d["datums"][0],a),a); break;
        case 11: d.RemoveMember("vectors"); break;
        case 12: d["clip"]["plane"][0].SetDouble(0); d["clip"]["plane"][1].SetDouble(0); d["clip"]["plane"][2].SetDouble(0); break;
        case 13: d["overrides"].PushBack(rapidjson::Value(d["overrides"][0],a),a); break;
        } reject(json(d));
    }
    require(native.EndDrag(false),native.LastError());
    const auto aborts=native.EventCounts()[(size_t)Viewer::CutEditEvent::Abort];
    require(native.BeginDrag()&&session.Load(file.string())&&!native.IsDragging()&&native.EventCounts()[(size_t)Viewer::CutEditEvent::Abort]==aborts+1,"successful restore did not cancel old drag");
    // Save failures must preserve a previously published document byte-for-byte.
    const auto wrongCad=out/"wrong.brep"; BRepTools::Write(BRepPrimAPI_MakeBox(2,1,1).Shape(),wrongCad.string().c_str());
    require(!session.Save(file.string(),session.Source(),wrongCad.string())&&read(file)==current,"failed CAD save replaced valid session");
    for(int version:{1,2,3}) {
        rapidjson::Document d; d.Parse(current.c_str()); auto& definitions=d["definitions"]; definitions["version"].SetInt(version);
        definitions.RemoveMember("isoOptions"); definitions.RemoveMember("recovery");
        for(auto& cut:definitions["cuts"].GetArray()) { cut.RemoveMember("displayTrace"); cut.RemoveMember("physicalTrace"); if(version==1) { cut.RemoveMember("shape"); } }
        write(out/"definitions-legacy.json",json(d)); require(session.Load((out/"definitions-legacy.json").string()),native.LastError());
        require(native.Display()->Cuts().Definitions().size()==1,"legacy definition migration");
    }
    // Older full-session versions migrate missing additions to native defaults.
    for(int version:{1,2}) { rapidjson::Document d; d.Parse(current.c_str()); d["version"].SetInt(version);
        for(const auto key:{"deformation","deformationScale","styles","overrides","contour","vectors","datums","axes","materialFrames","following","recoverySteps","traceSteps","playback","analysis","analysisSteps","automatic","annotations","cad","clip","camera","cache","geometry"}) { d.RemoveMember(key); }
        write(out/"legacy.json",json(d)); require(session.Load((out/"legacy.json").string()),native.LastError()); require(native.Datums().empty()&&native.Annotations().empty(),"legacy defaults");
    }
    write(file,current); const auto relocated=out/("relocated-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())); fs::create_directories(relocated); fs::copy_file(file,relocated/file.filename());
    fs::copy(out/(file.filename().string()+".assets"),relocated/(file.filename().string()+".assets"),fs::copy_options::recursive|fs::copy_options::overwrite_existing);
    require(session.Load((relocated/file.filename()).string()),native.LastError());
    require(Viewer::NativeCutDigest::File((relocated/file.filename()).string(),32*1024*1024)==Viewer::NativeCutDigest::Text(current),"file SHA");
    RWVtk::VtkResourceLimits limits; limits.MaxFileBytes=1; session.SetLimits(limits); require(!session.Load((relocated/file.filename()).string()),"session source budget accepted");
    native.Close();
    seriesTests(out/"stable",false); seriesTests(out/"remeshed",true);
}
}
int main() { Draw::GetInterpretor().Init(); try { tests(); std::cout<<"NATIVE_SESSION_PASS\n"; return 0; } catch(const Standard_Failure& e) { std::cerr<<"NATIVE_SESSION_FAIL "<<e.GetMessageString()<<'\n'; } catch(const std::exception& e) { std::cerr<<"NATIVE_SESSION_FAIL "<<e.what()<<'\n'; } return 1; }
