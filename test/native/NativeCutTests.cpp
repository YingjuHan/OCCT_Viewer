#include <Viewer/NativeCutView.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBndLib.hxx>
#include <TopExp_Explorer.hxx>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <Draw.hxx>
#include <Draw_Interpretor.hxx>

static void require(bool value, const std::string& message) { if (!value) { throw std::runtime_error(message); } }
int main()
{
    Draw::GetInterpretor().Init();
    try {
        RWMeshConv::MeshLoader loader;
        const auto mesh = loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),NATIVE_CUT_FIXTURE_DIR "/stress_heat.vtu");
        require(!mesh.IsNull(),"Native fixture import");
        std::vector<Viewer::CutEditEvent> events;
        Viewer::NativeCutView native(mesh);
        auto display = native.Display(); auto& cuts = display->Cuts();
        require(cuts.AddPlane("p",Graphic3d_Vec4d(1,0,0,-0.25)),"Native plane");
        require(native.BindField({"heat_flux",true,0,-1}),native.LastError());
        const auto generation = native.Generation(); const auto field = display->Field();
        require(!native.BindField({"missing",true,0,-1}) && !native.LastError().empty(),"Missing binding accepted");
        require(native.Generation() == generation && display->Field() == field,"Failed binding changed live state");
        native.SetOnEdit([&](auto event,uint64_t) { events.push_back(event); });
        require(native.BeginDrag(),native.LastError());
        auto definition = cuts.Definitions()[0]; definition.Plane.w() = -0.5;
        require(native.Edit(definition),native.LastError());
        require(native.EndDrag(false),native.LastError());
        require(std::abs(cuts.Definitions()[0].Plane.w()+0.25) < 1e-12,"Drag abort did not restore cut");
        require(events.front() == Viewer::CutEditEvent::Begin && events.back() == Viewer::CutEditEvent::Abort,"Drag event contract");
        auto datum = ViewCaeGeom::MeshCutFrame(); datum.Origin = Graphic3d_Vec3d(0.25,0,0);
        require(native.SetDatum("local",datum),native.LastError()); definition.Plane.w() = 0;
        require(native.Edit(definition,"local"),native.LastError());
        require(std::abs(cuts.Definitions()[0].Plane.w()+0.25) < 1e-12,"Local edit translation");
        require(native.SetCadSection(BRepPrimAPI_MakeBox(1,1,1).Shape(),"p",true),native.LastError());
        TopExp_Explorer edges(native.CadSection()->Shape(),TopAbs_EDGE); require(edges.More(),"CAD section edges absent");
        Bnd_Box bounds; BRepBndLib::Add(native.CadSection()->Shape(),bounds);
        require(std::abs(bounds.CornerMin().X()-0.25) < 1e-6 && std::abs(bounds.CornerMax().X()-0.25) < 1e-6,"CAD section not on plane");
        const auto clip = native.GlobalClip(); require(!clip.IsNull() && clip->IsOn(),"CAD synchronized clip absent");
        const auto independentSection = native.CadSection()->Shape();
        require(native.SetGlobalClip(Graphic3d_Vec4d(0,1,0,-0.3),true),native.LastError());
        require(native.CadSection()->Shape().IsSame(independentSection),"Independent clip changed section");
        require(native.Refresh(false),native.LastError());
        require(std::abs(native.GlobalClip()->GetEquation().w()+0.3) < 1e-12
            && native.GlobalClip()->GetEquation().y() == 1,"CAD refresh overwrote independent viewport clip");
        require(!native.SetGlobalClip(Graphic3d_Vec4d(0,0,0,0),true),"Degenerate global clip accepted");
        const auto probe = display->Probe("p",ViewCaeGeom::MeshCutPortion::On,0,true);
        require(native.AddAnnotation("a",probe,"Original heat flux"),native.LastError());
        require(native.Annotations().size() == 1,"Annotation missing"); native.ClearAnnotations(); require(native.Annotations().empty(),"Annotation removal");
        display->SetVectorField(field,0.1,1,true); display->Synchronize();
        require(display->VectorPresentations().size() == 1,"Orientation presentation missing");
        const auto directions = display->VectorPresentations()[0]->VectorPartParams()->VectorProperty();
        for (int n = 0; n < directions->NbValues(); ++n) {
            Graphic3d_Vec3d direction; directions->GetVector(RWCae::MeshPropertyEntityInfo(n),direction);
            require(std::abs(direction.Modulus()-1) < 1e-12 && direction.x() == 1,"Orientation value/direction reference");
        }
        require(field->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(2)) == 5,"Orientation mutated original field");
        bool rejected = false; try { display->SetVectorField(field,-1); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected,"Negative vector scale accepted");
        display->SetVectorField(nullptr); native.ClearCadSection();
        require(cuts.AddCylinder("cylinder",Graphic3d_Vec3d(0.5,0.5,0),Graphic3d_Vec3d(0,0,1),0.25),"CAD cylinder cut");
        require(native.SetCadSection(BRepPrimAPI_MakeBox(gp_Pnt(-5,-5,-5),10,10,10).Shape(),"cylinder",false),native.LastError());
        Bnd_Box cylinderBounds; BRepBndLib::Add(native.CadSection()->Shape(),cylinderBounds);
        require(!cylinderBounds.IsVoid() && std::abs(cylinderBounds.CornerMin().Z()+5) < 1e-6
            && std::abs(cylinderBounds.CornerMax().Z()-5) < 1e-6,"CAD infinite cylinder incorrectly truncated to FE bounds");
        require(!native.SetCadSection(BRepPrimAPI_MakeBox(1,1,1).Shape(),"cylinder",true),"Curved viewport synchronization accepted");
        require(cuts.Remove("cylinder") && native.Refresh(false),"Removed CAD cut did not clean section state");
        require(native.CadSection().IsNull(),"Removed cut left a CAD section");
        native.Close(); require(native.Display().IsNull() && cuts.Definitions().empty(),"Close did not release native state");
        require(!native.Refresh() && !native.LastError().empty(),"Closed-model refresh accepted");
        require(events.back() == Viewer::CutEditEvent::Close,"Close notification absent");
        std::cout << "NATIVE_CUT_PASS" << std::endl; return 0;
    } catch (const std::exception& error) { std::cerr << "NATIVE_CUT_FAIL " << error.what() << '\n'; return 1; }
}
