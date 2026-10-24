#include <Viewer/NativeCutAnalysis.h>
#include <Draw.hxx>
#include <Draw_Interpretor.hxx>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <RWCae/MeshGroupProperty.h>
static void require(bool v,const std::string& m) { if(!v) { throw std::runtime_error(m); } }
int main()
{
    Draw::GetInterpretor().Init();
    try {
        RWMeshConv::MeshLoader loader; const auto mesh=loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),NATIVE_CUT_FIXTURE_DIR "/stress_heat.vtu");
        Viewer::NativeCutView native(mesh); auto& cuts=native.Display()->Cuts(); require(cuts.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.25)),"plane");
        auto& a=native.Analysis(); Viewer::NativeCutAnalysisRequest r; r.Cut="p"; r.Field={"stress",true,0,-1}; r.Components={{0,3,4,3,1,5,4,5,2}};
        require(a.Set("force",r),native.LastError()); require(!a.Overlays().at("force").empty(),"resultant overlays missing");
        const double reference=a.Reports().at("force").Total.Force.x(); const auto old=a.Overlays().at("force");
        auto invalid=r; invalid.Options.Units.LengthToSI=0; require(!a.Set("force",invalid),"invalid units accepted");
        require(a.Overlays().at("force")==old && a.Reports().at("force").Total.Force.x()==reference,"failed request changed reports or overlays");
        require(a.SetJson("roundtrip",Viewer::NativeCutAnalysis::RequestJson(r)),native.LastError());
        require(a.CaptureDefault("force") && a.HasDefault() && a.DefaultRequest().Cut.empty(),native.LastError());
        require(cuts.AddPlane("template",Graphic3d_Vec4d(0,1,0,-.25)),"template target");
        require(a.ApplyDefault("template","inherited",false) && !a.Requests().at("inherited").Enabled,native.LastError());
        require(a.DefaultRequest().Field.Name=="stress" && a.Requests().at("inherited").Cut=="template","Template became a cut override");
        require(!a.ApplyDefault("missing","invalid",false) && !a.Requests().count("invalid"),"Disabled template application bypassed FE validation");
        require(a.Remove("inherited") && cuts.Remove("template"),native.LastError());
        require(a.Reports().at("roundtrip").Total.Force.x()==reference,"JSON changed physical results");
        for(const auto& json:{"{}","[]","{\"cut\":0}","{\"cut\":\"p\",\"field\":{\"name\":\"stress\",\"nodal\":true,\"component\":0,\"step\":-1},\"sections\":{}}"}) { require(!a.SetJson("bad",json),"bad JSON accepted"); }
        require(a.Requests().size()==2,"invalid request publication");
        const auto samples=ViewCaeGeom::MeshCutSlicing::Range(cuts.Definitions().front(),0,.25,3);
        require(cuts.ReplaceSlices("scan","p",samples),"Slice collection creation");
        const auto scene=cuts.DumpDefinitionsJson(); std::vector<ViewCaeGeom::MeshCutPhysicsReport> reports;
        require(a.ScanSlices("scan",r,true,reports),native.LastError());
        require(reports.size()==3 && cuts.DumpDefinitionsJson()==scene && a.Requests().size()==2,"Scan polluted scene/requests");
        require(reports.front().IntervalEnd==.0625 && reports.back().IntervalEnd==.25 && reports.front().IntervalUnits=="model_length","Scan intervals/units");
        require(std::abs(reports.back().MomentOrigin.x()-.5)<1e-8,"Path summation point");
        auto broken=r; broken.Field.Name="missing";
        require(!a.ScanSlices("scan",broken,false,reports) && reports.size()==3 && cuts.DumpDefinitionsJson()==scene,"Failed scan published partial reports");
        require(cuts.EnableSlices("scan",true) && cuts.EvaluateVisible().size()==3,"Slicing did not replace source display");
        ViewCaeGeom::MeshCutViewManager loaded(mesh);
        require(loaded.RestoreDefinitionsJson(cuts.DumpDefinitionsJson()) && loaded.EvaluateVisible().size()==3,"Collection persistence");
        require(!cuts.Remove("scan_0"),"Slice member deleted independently");
        require(cuts.RemoveSlices("scan") && cuts.Definitions().size()==1,"Collection cleanup");
        Handle(RWCae::MeshNVecProperty) labels=new RWCae::MeshNVecProperty("sets",1,RWCae::MeshPropertyBase::MappingType::Elemental);
        labels->Init<RWCae::MeshGroupProperty>();
        auto group=Handle(RWCae::MeshGroupProperty)::DownCast(labels->GetRawComponent(0)); group->Init(mesh->NbIndexedElems()); group->SetGroups(0,std::vector<int>{1,4}); labels->UpdateComponents(); mesh->ChangeProperties().Append(labels);
        auto domain=r; domain.DomainField={"sets",false,0,-1}; domain.DomainMask=true; domain.DomainValue=4; domain.DomainSource="element_set";
        require(a.SetJson("domain",Viewer::NativeCutAnalysis::RequestJson(domain)) && a.Reports().at("domain").Total.Force.x()==reference,native.LastError());
        domain.DomainValue=3; domain.Options.SumPoint=ViewCaeGeom::MeshCutSummationPoint::Centroid;
        require(!a.Set("domain",domain) && a.Reports().at("domain").Total.Force.x()==reference,"Empty element set fell back to all");
        Handle(ViewCae::InteractiveCaeMeshOwner) owner=new ViewCae::InteractiveCaeMeshOwner(mesh);
        Handle(ViewCae::InteractiveCaeMesh) original=new ViewCae::InteractiveCaeMesh(owner);
        const std::string prs=original->CreatePresentation(ViewCae::MeshPresentationType::Scalar).ToCString();
        require(a.CaptureDisplayGroup("domain",original,prs),native.LastError());
        require(a.Requests().at("domain").DomainSource=="display_snapshot" && a.Requests().at("domain").Options.Elements.size()==mesh->NbIndexedElems(),"Actual display visibility was not captured");
        original->Presentation(prs.c_str())->VisProps()->SetVisible(false);
        require(!a.CaptureDisplayGroup("domain",original,prs),"Empty source display group fell back to all");
        require(a.Remove("domain"),native.LastError());
        require(a.Overlays().at("force").size()<=2,"Default overlays scaled with integration primitives");
        require(a.SetEnabled("force",false) && !a.Requests().at("force").Enabled && !a.Reports().count("force"),native.LastError());
        require(native.Refresh(false) && a.Requests().count("force") && !a.Reports().count("force"),"Disabled free body was removed/recomputed");
        require(a.SetEnabled("force",true) && a.Reports().at("force").Total.Force.x()==reference,native.LastError());
        require(cuts.AddPlane("q",Graphic3d_Vec4d(0,1,0,-.25)),"Component test plane");
        auto local=r; local.Cut="q"; local.Options.SumPoint=ViewCaeGeom::MeshCutSummationPoint::Centroid;
        local.Resolution=Viewer::CutComponentResolution::NormalTangential; local.TangentY=Graphic3d_Vec3d(0,0,1); local.ComponentVectors=true;
        require(a.SetJson("basis",Viewer::NativeCutAnalysis::RequestJson(local)),native.LastError());
        require(std::abs(a.Reports().at("basis").MeanNormal.x()-1)<1e-6,"Normal/tangent components did not resolve");
        const auto resolved=a.Reports().at("basis");
        require(std::abs(resolved.Total.Moment.Modulus())<1e-6,"Centroid resultant moment");
        auto parallel=local; parallel.TangentY=Graphic3d_Vec3d(0,1,0);
        require(!a.Set("basis",parallel) && a.Reports().at("basis").Total.Force==resolved.Total.Force,"Invalid tangent replaced report");
        ViewCaeGeom::MeshCutFrame datum; datum.X=Graphic3d_Vec3d(0,1,0); datum.Y=Graphic3d_Vec3d(0,0,1); datum.Z=Graphic3d_Vec3d(1,0,0);
        require(native.SetDatum("components",datum),native.LastError()); local.Resolution=Viewer::CutComponentResolution::Datum; local.Datum="components";
        require(a.Set("basis",local) && (a.Reports().at("basis").Total.Force-resolved.Total.Force).Modulus()<1e-6,native.LastError());
        local.Enabled=false; require(a.SetJson("basis",Viewer::NativeCutAnalysis::RequestJson(local)) && !a.Reports().count("basis"),native.LastError());
        require(a.Remove("basis") && cuts.Remove("q"),native.LastError());
        require(native.BeginDrag(),native.LastError()); auto d=cuts.Definitions()[0]; d.Plane.w()=-.5; require(native.Edit(d),native.LastError());
        require(std::abs(a.Reports().at("force").Total.Area-.125)<1e-6,"drag physics stale");
        require(native.EndDrag(false),native.LastError()); require(std::abs(a.Reports().at("force").Total.Force.x()-reference)<1e-6,"abort physics stale");
        r.Heat=true; r.Field={"heat_flux",true,0,-1}; r.Components={{0,1,2,0,0,0,0,0,0}}; require(a.Set("heat",r),native.LastError());
        require(std::abs(a.Reports().at("heat").Total.HeatFlow-.5625)<1e-6,"physical heat independent reference");
        require(a.Remove("roundtrip"),native.LastError()); require(cuts.Remove("p")&&native.Refresh(false),native.LastError());
        require(a.Requests().empty()&&a.Overlays().empty(),"removed cut retained analysis"); native.Close();
        std::cout<<"NATIVE_ANALYSIS_PASS\n"; return 0;
    } catch(const Standard_Failure& e) { std::cerr<<"NATIVE_ANALYSIS_FAIL "<<e.GetMessageString()<<'\n'; }
    catch(const std::exception& e) { std::cerr<<"NATIVE_ANALYSIS_FAIL "<<e.what()<<'\n'; } return 1;
}
