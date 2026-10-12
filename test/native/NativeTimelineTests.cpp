#include <Viewer/NativeCutTimeline.h>
#include <RWCae/MeshScalarProperty.h>
#include <Draw.hxx>
#include <Draw_Interpretor.hxx>
#include <iostream>
#include <cmath>
#include <stdexcept>

namespace {
void require(bool value, const std::string& message) { if (!value) { throw std::runtime_error(message); } }
void closeTo(double value, double reference) { require(std::isfinite(value) && std::abs(value-reference) < 2e-6,"Timeline numerical reference mismatch"); }
Handle(RWCae::MeshData) snapshot(int step, double shift, bool remesh = false, bool missing = false)
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (auto p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) {
        p.x() += (float)shift; mesh->AddNodePosition(p);
    }
    RWCae::MeshEntityId nodes[] = {0,1,2,3}; mesh->AddElement(RWCae::MeshElementTetrahedron(nodes));
    if (remesh) { RWCae::MeshEntityId other[] = {0,2,1,3}; mesh->AddElement(RWCae::MeshElementTetrahedron(other)); }
    for (const auto& name : {"contour","displacement"}) {
        if (missing && std::string(name) == "contour") { continue; }
        const bool vector = std::string(name) == "displacement";
        Handle(RWCae::MeshNVecProperty) field = new RWCae::MeshNVecProperty(name,vector ? 3 : 1,
            RWCae::MeshPropertyBase::MappingType::Nodal,vector ? RWCae::MeshNVecPropertyType::Vector : RWCae::MeshNVecPropertyType::ScalarSet);
        field->SetTime(step == 0 ? 0 : step == 1 ? 0.5 : 2,step); field->Init<RWCae::MeshScalarProperty>();
        for (int c = 0; c < field->NbComponents(); ++c) {
            auto scalar = Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(c)); scalar->Init(4);
            for (int n = 0; n < 4; ++n) {
                scalar->SetValue(RWCae::MeshPropertyEntityInfo(n),vector ? (c == 0 ? 1 : 0) : 10*step+mesh->Nodes()[n].x()+2*mesh->Nodes()[n].y()+3*mesh->Nodes()[n].z());
            }
        }
        field->UpdateComponents(); mesh->ChangeProperties().Append(field);
    }
    return mesh;
}
Handle(RWCae::MeshData) series(bool missing = false, bool remesh = false)
{
    auto a = snapshot(0,0), b = snapshot(1,0.2,remesh), c = snapshot(2,0.4,false,missing);
    // Published root and independent snapshots must not form a reference cycle.
    auto root = snapshot(0,0);
    root->ChangeSeriesSteps() = {{0,"a.vtu",a},{0.5,"b.vtu",b},{2,"c.vtu",c}}; return root;
}
void tests()
{
    Viewer::NativeCutView native(series()); Viewer::NativeCutTimeline timeline(native);
    require(native.Display()->Cuts().AddPlane("p",Graphic3d_Vec4d(1,0,0,-0.25)),"Timeline plane");
    require(native.BindField({"contour",true,0,0}),native.LastError());
    const auto initial = native.Display();
    auto style = initial->Style(ViewCaeGeom::MeshCutPortion::On); style.Transparency = 0.35f;
    initial->SetStyle("p",ViewCaeGeom::MeshCutPortion::On,style);
    auto contour = initial->ContourOptions(); contour.ShowLegend = true; contour.AutomaticRange = false; contour.Maximum = 30;
    initial->SetContourOptions(contour); initial->SetVectorField(initial->SourceMesh()->Properties()[1],0.2,0.5,true);
    Viewer::CutMaterialFrame updated; updated.Configuration = Viewer::CutFrameConfiguration::Updated;
    updated.Anchors = {{0,{0,1,2}},{1,{0,1,2}},{2,{0,1,2}}};
    require(timeline.SetMaterialFrame("moving",updated),native.LastError());
    Viewer::CutMaterialFrame reference; reference.Anchors = {{-1,{0,1,2}}};
    require(timeline.SetMaterialFrame("fixed",reference),native.LastError());
    require(timeline.BindCutFrame("p","moving"),native.LastError());
    require(timeline.SwitchStep(1),native.LastError());
    require(native.Display() != initial && initial->Cuts().Definitions().empty(),"Old display/handles remain live");
    closeTo(native.Datums().at("moving").Origin.x(),0.2); closeTo(native.Datums().at("fixed").Origin.x(),0);
    closeTo(native.Display()->Cuts().Definitions()[0].Plane.w(),-0.45);
    closeTo(native.Display()->EffectiveStyle("p",ViewCaeGeom::MeshCutPortion::On).Transparency,0.35);
    require(native.Display()->ContourOptions().ShowLegend && native.Display()->VectorOrientation(),"Step lost display options");
    require(native.Display()->VectorField()->TimeStep() == 1,"Stale vector step");
    const auto probe = native.Display()->Probe("p",ViewCaeGeom::MeshCutPortion::On,0,true);
    closeTo(probe.Values[0],10+probe.Position.x()+2*probe.Position.y()+3*probe.Position.z());
    require(probe.SourceElement == 0 && !probe.SourceNodes.empty(),"Remapped probe provenance");
    require(timeline.BindDeformation({"displacement",true,0,1},0.1),native.LastError());
    const double effective = native.Display()->Cuts().EffectiveDeformationScale();
    closeTo(native.Datums().at("moving").Origin.x(),0.2+effective);
    closeTo(native.Display()->Cuts().Definitions()[0].Plane.w(),-0.45-effective);
    closeTo(native.Datums().at("fixed").Origin.x(),0);
    require(native.BeginDrag(),native.LastError());
    auto definition = native.Display()->Cuts().Definitions()[0]; definition.Plane.w() -= 0.05;
    require(native.Edit(definition),native.LastError());
    require(timeline.SwitchStep(2),native.LastError());
    require(!native.IsDragging() && native.EventCounts()[(size_t)Viewer::CutEditEvent::Abort] == 1,"Successful coordinated switch did not cancel drag");
    closeTo(native.Display()->Cuts().Definitions()[0].Plane.w(),-0.65-effective);
    require(timeline.PlayTime(1,false,true),native.LastError());
    require(timeline.Tick(0.49) && timeline.Step() == 0,"Timestamp advanced early");
    require(timeline.Tick(0.02) && timeline.Step() == 1,"Timestamp did not advance");
    timeline.Pause(); require(timeline.Tick(10) && timeline.Step() == 1,"Paused animation advanced");
    require(timeline.Play(0.1,false,true) && timeline.Tick(0.25) && timeline.Step() == 2 && !timeline.IsPlaying(),"Elapsed frames/restart/end contract");
    require(!timeline.SwitchStep(3),"Out of range step accepted");

    Viewer::NativeCutView broken(series(true)); Viewer::NativeCutTimeline failing(broken);
    require(broken.Display()->Cuts().AddPlane("p",Graphic3d_Vec4d(1,0,0,-0.25)) && broken.BindField({"contour",true,0,0}),"Failure fixture");
    require(broken.BeginDrag(),broken.LastError()); auto draft = broken.Display()->Cuts().Definitions()[0]; draft.Plane.w() = -0.3;
    require(broken.Edit(draft),broken.LastError());
    const auto old = broken.Display(); const auto json = old->Cuts().DumpDefinitionsJson(); const auto generation = broken.Generation();
    require(!failing.SwitchStep(2) && failing.Step() == 0 && broken.Display() == old && broken.IsDragging(),"Failed frame replaced live/drag state");
    require(old->Cuts().DumpDefinitionsJson() == json && generation == broken.Generation(),"Failed frame mutated definitions/generation");
    require(broken.EndDrag(false),broken.LastError());
    require(failing.Play(0.1,false,true) && !failing.Tick(0.25) && !failing.IsPlaying() && failing.Step() == 0,"Failed animation did not pause atomically");

    Viewer::NativeCutView remeshed(series(false,true)); Viewer::NativeCutTimeline remesh(remeshed);
    require(remeshed.Display()->Cuts().AddPlane("p",Graphic3d_Vec4d(1,0,0,-0.25)) && remeshed.BindField({"contour",true,0,0}),"Remesh fixture");
    auto recovery = remeshed.Display()->Cuts().RecoveryOptions(); recovery.Regions = {7};
    require(remeshed.Display()->Cuts().SetRecoveryOptions(recovery),"Initial region mapping");
    require(!remesh.SwitchStep(1),"Remesh accepted old region mapping");
    recovery.Regions = {7,8}; remesh.SetRecoveryForStep(1,recovery);
    require(remesh.SwitchStep(1),remeshed.LastError());
    require(remeshed.Display()->SourceMesh()->NbIndexedElems() == 2,"Remesh retained stale topology");
    Viewer::CutMaterialFrame invalid; invalid.Configuration = Viewer::CutFrameConfiguration::Updated; invalid.Anchors = {{1,{0,0,2}}};
    require(!remesh.SetMaterialFrame("invalid",invalid) && !remeshed.Datums().count("invalid"),"Collinear material frame accepted");
    const auto rotating = series();
    auto target = rotating->ChangeSeriesSteps()[1].Mesh;
    for (RWCae::MeshEntityId n = 0; n < target->NbNodes(); ++n) {
        const auto p = target->Nodes()[n]; target->ChangeNodes()[n] = Graphic3d_Vec3(0.2f-p.y(),p.x()-0.2f,p.z());
    }
    Viewer::NativeCutView rotation(rotating); Viewer::NativeCutTimeline rotationTime(rotation);
    require(rotation.Display()->Cuts().AddPlane("p",Graphic3d_Vec4d(1,0,0,-0.25)),"Rotated material cut");
    require(rotationTime.SetMaterialFrame("moving",updated) && rotationTime.BindCutFrame("p","moving") && rotationTime.SwitchStep(1),rotation.LastError());
    const auto rotatedPlane = rotation.Display()->Cuts().Definitions()[0].Plane;
    closeTo(rotatedPlane.x(),0); closeTo(rotatedPlane.y(),1); closeTo(rotatedPlane.w(),-0.25);
    closeTo(rotation.Datums().at("moving").X.y(),1);
    const auto schemaSource = series();
    auto badMesh = schemaSource->ChangeSeriesSteps()[2].Mesh;
    Handle(RWCae::MeshNVecProperty) changed = new RWCae::MeshNVecProperty("contour",2,RWCae::MeshPropertyBase::MappingType::Nodal);
    changed->SetTime(2,2); changed->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 2; ++c) {
        auto value = Handle(RWCae::MeshScalarProperty)::DownCast(changed->GetComponent(c)); value->Init(4);
        for (int n = 0; n < 4; ++n) { value->SetValue(RWCae::MeshPropertyEntityInfo(n),1); }
    }
    changed->UpdateComponents(); badMesh->ChangeProperties()[0] = changed;
    Viewer::NativeCutView schema(schemaSource); Viewer::NativeCutTimeline schemaTime(schema);
    require(schema.BindField({"contour",true,0,0}),schema.LastError());
    const auto schemaOld = schema.Display();
    require(!schemaTime.SwitchStep(2) && schema.Display() == schemaOld && schemaTime.Step() == 0,"Changed schema published a partial frame");
}
}
int main()
{
    Draw::GetInterpretor().Init();
    try { tests(); std::cout << "NATIVE_TIMELINE_PASS\n"; return 0; }
    catch (const Standard_Failure& error) { std::cerr << "NATIVE_TIMELINE_FAIL " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "NATIVE_TIMELINE_FAIL " << error.what() << '\n'; }
    return 1;
}
