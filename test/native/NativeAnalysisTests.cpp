#include <Viewer/NativeCutAnalysis.h>
#include <Draw.hxx>
#include <Draw_Interpretor.hxx>
#include <iostream>
#include <stdexcept>
#include <cmath>
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
        require(a.Reports().at("roundtrip").Total.Force.x()==reference,"JSON changed physical results");
        for(const auto& json:{"{}","[]","{\"cut\":0}","{\"cut\":\"p\",\"field\":{\"name\":\"stress\",\"nodal\":true,\"component\":0,\"step\":-1},\"sections\":{}}"}) { require(!a.SetJson("bad",json),"bad JSON accepted"); }
        require(a.Requests().size()==2,"invalid request publication");
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
