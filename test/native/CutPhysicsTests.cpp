#include <ViewCaeGeom/MeshCutPhysics.h>
#include <ViewCaeGeom/MeshCutAutomatic.h>
#include <RWCae/MeshScalarProperty.h>
#include <iostream>
#include <cmath>
#include <stdexcept>

namespace {
using namespace ViewCaeGeom;
void require(bool value,const char* message) { if (!value) { throw std::runtime_error(message); } }
void closeTo(double value,double reference,double tolerance = 2e-6) { if(!std::isfinite(value)||std::abs(value-reference)>=tolerance) { std::cerr<<"actual="<<value<<" reference="<<reference<<" tolerance="<<tolerance<<'\n'; throw std::runtime_error("Physics independent reference mismatch"); } }
template<class F> void rejects(F action) { bool rejected = false; try { action(); } catch (const std::invalid_argument&) { rejected = true; } require(rejected,"Invalid physics mapping accepted"); }
Handle(RWCae::MeshNVecProperty) tensor(const Handle(RWCae::MeshData)& mesh,double xx = 4)
{
    Handle(RWCae::MeshNVecProperty) field = new RWCae::MeshNVecProperty("stress",9,RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Tensor);
    field->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 9; ++c) {
        auto values = Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(c)); values->Init(mesh->NbNodes());
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) { values->SetValue(RWCae::MeshPropertyEntityInfo(n),c == 0 || c == 4 || c == 8 ? xx : 0); }
    }
    field->UpdateComponents(); return field;
}
Handle(RWCae::MeshData) geometry(int dimension)
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (auto p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) { mesh->AddNodePosition(p); }
    RWCae::MeshEntityId nodes[] = {0,1,2,3};
    if (dimension == 3) { mesh->AddElement(RWCae::MeshElementTetrahedron(nodes)); }
    else if (dimension == 2) { mesh->AddElement(RWCae::MeshElementTriangle(nodes)); }
    else { RWCae::MeshData::GenericCell line; line.Type = 3; line.Nodes = {0,1}; mesh->AddGenericCell(line); }
    return mesh;
}
const std::array<int,9> components = {{0,1,2,3,4,5,6,7,8}};
Handle(RWCae::MeshNVecProperty) flux(const Handle(RWCae::MeshData)& mesh,Graphic3d_Vec3d origin,bool radial=true)
{
    Handle(RWCae::MeshNVecProperty) f=new RWCae::MeshNVecProperty("flux",3,RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
    f->Init<RWCae::MeshScalarProperty>();
    for(int c=0;c<3;++c) { auto v=Handle(RWCae::MeshScalarProperty)::DownCast(f->GetComponent(c)); v->Init(mesh->NbNodes()); for(RWCae::MeshEntityId n=0;n<mesh->NbNodes();++n) { v->SetValue(RWCae::MeshPropertyEntityInfo(n),radial ? mesh->Nodes()[n][c]-origin[c] : c==0?2:0); } }
    f->UpdateComponents(); return f;
}
void tests()
{
    auto volume = geometry(3); auto field = tensor(volume);
    MeshCutViewManager manager(volume); require(manager.AddPlane("p",Graphic3d_Vec4d(1,0,0,-0.25)),"Volume cut");
    auto cut = manager.EvaluatePhysical("p",field); MeshCutPhysicsOptions options;
    const auto report = MeshCutPhysics::Stress(cut,components,options);
    closeTo(report.Total.Area,0.28125); closeTo(report.Total.Force.x(),1.125);
    closeTo(report.Total.Moment.y(),0.28125); closeTo(report.Total.Moment.z(),-0.28125);
    require(!report.Distributed.empty() && report.Csv().find("Fx_N") != std::string::npos && report.Csv().find("total") != std::string::npos,"Distributed CSV report");
    options.SumPoint=MeshCutSummationPoint::Centroid;
    const auto centred=MeshCutPhysics::Stress(cut,components,options);
    for(int c=0;c<3;++c) { closeTo(centred.Centroid[c],.25); closeTo(centred.MomentOrigin[c],.25); closeTo(centred.Total.Moment[c],0); }
    closeTo(centred.MeanNormal.x(),1); closeTo(centred.Total.Force.x(),1.125);
    options.NormalSign=-1; const auto reverse=MeshCutPhysics::Stress(cut,components,options);
    closeTo(reverse.Total.Force.x(),-centred.Total.Force.x()); closeTo(reverse.MeanNormal.x(),-1);
    closeTo(MeshCutPhysics::HeatFlux(manager.EvaluatePhysical("p",flux(volume,Graphic3d_Vec3d(0),false)),{{0,1,2}},options).Total.HeatFlow,-.5625);
    options.SumPoint=MeshCutSummationPoint::UserDefined; options.NormalSign=1; options.MomentOrigin=Graphic3d_Vec3d(.1,.2,.3);
    const auto shifted=MeshCutPhysics::Stress(cut,components,options);
    const auto expected=report.Total.Moment-Graphic3d_Vec3d::Cross(options.MomentOrigin,report.Total.Force);
    for(int c=0;c<3;++c) { closeTo(shifted.Total.Moment[c],expected[c]); }
    options.NormalSign=0; rejects([&] { MeshCutPhysics::Stress(cut,components,options); });
    options=MeshCutPhysicsOptions();
    options.GroupName = "part"; options.Elements = {0}; options.Units.Length = "mm"; options.Units.LengthToSI = 0.001;
    options.Units.Stress = "MPa"; options.Units.StressToSI = 1e6;
    options.OutputFrame.X = Graphic3d_Vec3d(0,1,0); options.OutputFrame.Y = Graphic3d_Vec3d(-1,0,0);
    auto converted = MeshCutPhysics::Stress(cut,components,options);
    closeTo(converted.Total.Force.y(),-1.125); closeTo(converted.Total.Moment.x(),0.00028125); closeTo(converted.Total.Area,2.8125e-7);
    options.Summation = MeshCutSummation::AbsoluteContributions; closeTo(MeshCutPhysics::Stress(cut,components,options).Total.Force.y(),1.125);
    options.Elements.clear(); rejects([&] { MeshCutPhysics::Stress(cut,components,options); });
    options = MeshCutPhysicsOptions(); options.Units.StressToSI = 0; rejects([&] { MeshCutPhysics::Stress(cut,components,options); });
    options = MeshCutPhysicsOptions();
    auto anisotropic=tensor(volume); auto yy=Handle(RWCae::MeshScalarProperty)::DownCast(anisotropic->GetComponent(4));
    for(int n=0;n<4;++n) { yy->SetValue(RWCae::MeshPropertyEntityInfo(n),2); } anisotropic->UpdateComponents();
    options.FieldFrame.X=Graphic3d_Vec3d(0,1,0); options.FieldFrame.Y=Graphic3d_Vec3d(-1,0,0);
    closeTo(MeshCutPhysics::Stress(manager.EvaluatePhysical("p",anisotropic),components,options).Total.Force.x(),.5625);
    options=MeshCutPhysicsOptions();
    closeTo(MeshCutPhysics::HeatFlux(manager.EvaluatePhysical("p",flux(volume,Graphic3d_Vec3d(0),false)),{{0,1,2}},options).Total.HeatFlow,0.5625);
    options.Units.LengthToSI=0.001; options.Units.FluxToSI=1e6;
    closeTo(MeshCutPhysics::HeatFlux(manager.EvaluatePhysical("p",flux(volume,Graphic3d_Vec3d(0),false)),{{0,1,2}},options).Total.HeatFlow,0.5625);
    options=MeshCutPhysicsOptions();
    auto shell = geometry(2); auto shellField = tensor(shell); MeshCutViewManager shellManager(shell);
    require(shellManager.AddPlane("p",Graphic3d_Vec4d(1,0,0,-0.25)),"Shell cut"); const auto shellCut = shellManager.EvaluatePhysical("p",shellField);
    rejects([&] { MeshCutPhysics::Stress(shellCut,components,options); });
    MeshCutSectionMapping mapping; mapping.Kind = MeshCutSectionKind::ShellStress; mapping.Thickness = 0.2; options.Sections[0] = mapping;
    auto shellReport = MeshCutPhysics::Stress(shellCut,components,options);
    closeTo(shellReport.Total.Force.x(),0.6); closeTo(shellReport.Total.Area,0.15); closeTo(shellReport.Total.Moment.z(),-0.225);
    options.SumPoint=MeshCutSummationPoint::Centroid;
    const auto shellCentre=MeshCutPhysics::Stress(shellCut,components,options);
    closeTo(shellCentre.MomentOrigin.x(),.25); closeTo(shellCentre.MomentOrigin.y(),.375); closeTo(shellCentre.Total.Moment.z(),0);
    options.SumPoint=MeshCutSummationPoint::UserDefined;
    mapping.Kind = MeshCutSectionKind::CompositeStress;
    MeshCutLayer layer; layer.Field = shellField; layer.Thickness = 0.1; layer.Offset = 0.05;
    MeshCutLayer second = layer; second.Field = tensor(shell,8); second.Offset = -0.05;
    mapping.Layers = {layer,second}; options.Sections[0] = mapping;
    const auto composite = MeshCutPhysics::Stress(shellCut,components,options);
    closeTo(composite.Total.Force.x(),0.9); closeTo(composite.Total.Moment.y(),-0.015); closeTo(composite.Total.Moment.z(),-0.3375);
    mapping.Layers[0].Thickness = 0; options.Sections[0] = mapping; rejects([&] { MeshCutPhysics::Stress(shellCut,components,options); });
    mapping.Kind=MeshCutSectionKind::ShellStress; mapping.Thickness=.2; mapping.Frame.X=Graphic3d_Vec3d(0,0,-1); mapping.Frame.Z=Graphic3d_Vec3d(1,0,0); options.Sections[0]=mapping;
    rejects([&] { MeshCutPhysics::Stress(shellCut,components,options); });
    auto beam = geometry(1); auto beamField = tensor(beam); MeshCutViewManager beamManager(beam);
    MeshCutDefinition beamPlane; beamPlane.Id = "beam"; beamPlane.Plane = Graphic3d_Vec4d(1,0,0,-0.25);
    options = MeshCutPhysicsOptions(); options.SourceGeometry = beamManager.SourceGeometry(); mapping = MeshCutSectionMapping();
    mapping.Kind = MeshCutSectionKind::BeamStress; mapping.Area = 0.5; options.Sections[0] = mapping;
    closeTo(MeshCutPhysics::Beam(beamPlane,beamField,components,options).Total.Force.x(),2);
    mapping.Kind = MeshCutSectionKind::BeamResultants; options.Sections[0] = mapping;
    const auto preintegrated = MeshCutPhysics::Beam(beamPlane,beamField,components,options); closeTo(preintegrated.Total.Force.x(),4); closeTo(preintegrated.Total.Moment.y(),4);
    Handle(RWCae::MeshNVecProperty) displacement=new RWCae::MeshNVecProperty("U",3,RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
    displacement->Init<RWCae::MeshScalarProperty>();
    for(int c=0;c<3;++c) { auto v=Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c)); v->Init(beam->NbNodes()); for(RWCae::MeshEntityId n=0;n<beam->NbNodes();++n) { v->SetValue(RWCae::MeshPropertyEntityInfo(n),c==0?1:0); } }
    displacement->UpdateComponents();
    options.ReferenceGeometry=beamManager.SourceGeometry();
    MeshCutViewManager currentBeam(beam); require(currentBeam.SetDeformation(displacement,1),"Beam deformation"); options.SourceGeometry=currentBeam.SourceGeometry();
    beamPlane.FollowDeformation=true; options.SumPoint=MeshCutSummationPoint::Centroid;
    const auto materialBeam=MeshCutPhysics::Beam(beamPlane,beamField,components,options);
    closeTo(materialBeam.Total.Force.x(),4); closeTo(materialBeam.Centroid.x(),1.25);
    options.ReferenceGeometry.Nullify(); rejects([&] { MeshCutPhysics::Beam(beamPlane,beamField,components,options); });
    options.ReferenceGeometry=beamManager.SourceGeometry(); beamPlane.FollowDeformation=false; options.SourceGeometry=beamManager.SourceGeometry(); options.SumPoint=MeshCutSummationPoint::UserDefined;
    mapping.BeamComponents[5]=99; options.Sections[0]=mapping; rejects([&] { MeshCutPhysics::Beam(beamPlane,beamField,components,options); });
    mapping=MeshCutSectionMapping(); mapping.Kind=MeshCutSectionKind::ShellStress; mapping.Thickness=.1; options.Sections[0]=mapping; rejects([&] { MeshCutPhysics::Beam(beamPlane,beamField,components,options); });
    options = MeshCutPhysicsOptions();
    MeshCutDefinition plane; plane.Id = "slice"; plane.Plane = Graphic3d_Vec4d(1,0,0,0);
    const auto samples = MeshCutSlicing::Range(plane,0.1,0.9,3);
    const auto intervals = MeshCutPhysics::Slices(manager,samples,field,components,options);
    require(intervals.size() == 3 && manager.Definitions().size() == 1,"Distributed slicing changed original cuts");
    for (size_t i = 0; i < intervals.size(); ++i) {
        closeTo(intervals[i].Total.Force.x(),2*(1-samples[i].Coordinate)*(1-samples[i].Coordinate));
        closeTo(intervals[i].IntervalBegin,samples[i].IntervalBegin); closeTo(intervals[i].IntervalEnd,samples[i].IntervalEnd);
    }
    require(manager.AddSphere("sphere",Graphic3d_Vec3d(0.2,0.2,0.2),0.1),"Curved physics cut");
    const auto sphere = MeshCutPhysics::Stress(manager.EvaluatePhysical("sphere",field),components,options);
    closeTo(sphere.Total.Force.x(),0,0.01); closeTo(sphere.Total.Force.y(),0,0.01); closeTo(sphere.Total.Force.z(),0,0.01);
    closeTo(sphere.Total.Area,4*std::acos(-1.0)*0.01,0.004);
    const auto sphereHeat=MeshCutPhysics::HeatFlux(manager.EvaluatePhysical("sphere",flux(volume,Graphic3d_Vec3d(.2,.2,.2))),{{0,1,2}},options);
    closeTo(sphereHeat.Total.HeatFlow,4*std::acos(-1.0)*.001,.0005);
    require(manager.AddCylinder("cylinder",Graphic3d_Vec3d(.2,.2,0),Graphic3d_Vec3d(0,0,1),.05),"Cylinder reference");
    const auto cylinder=MeshCutPhysics::Stress(manager.EvaluatePhysical("cylinder",field),components,options);
    closeTo(cylinder.Total.Area,2*std::acos(-1.0)*.05*.6,.008);
    closeTo(cylinder.Total.Force.x(),-4*std::acos(-1.0)*.0025,.005);
    auto cylinderFlux=flux(volume,Graphic3d_Vec3d(.2,.2,0));
    auto z=Handle(RWCae::MeshScalarProperty)::DownCast(cylinderFlux->GetComponent(2)); for(int i=0;i<4;++i) { z->SetValue(RWCae::MeshPropertyEntityInfo(i),0); } cylinderFlux->UpdateComponents();
    closeTo(MeshCutPhysics::HeatFlux(manager.EvaluatePhysical("cylinder",cylinderFlux),{{0,1,2}},options).Total.HeatFlow,2*std::acos(-1.0)*.0025*.6,.0006);
    Handle(RWCae::MeshNVecProperty) density = new RWCae::MeshNVecProperty("explicit_density",1,RWCae::MeshPropertyBase::MappingType::Nodal);
    density->Init<RWCae::MeshScalarProperty>(); auto values = Handle(RWCae::MeshScalarProperty)::DownCast(density->GetComponent(0)); values->Init(4);
    for (int i = 0; i < 4; ++i) { values->SetValue(RWCae::MeshPropertyEntityInfo(i),volume->Nodes()[i].x()); } density->UpdateComponents();
    MeshCutAutomaticMapping automatic; automatic.Kind = MeshCutAutomaticKind::OptimizationDensity; automatic.Field = density; automatic.Threshold = 0.25;
    require(MeshCutAutomatic::Add(manager,"density",automatic).State == MeshCutAutomaticState::Crossing,"Density state");
    rejects([&] { MeshCutPhysics::Stress(manager.EvaluatePhysical("density",field),components,options); });
    options.ElementNormals={{0,Graphic3d_Vec3d(1,0,0)}};
    closeTo(MeshCutPhysics::Stress(manager.EvaluatePhysical("density",field),components,options).Total.Force.x(),1.125);
    options=MeshCutPhysicsOptions();
    automatic.Maximum = 0.5; rejects([&] { MeshCutAutomatic::Add(manager,"invalid",automatic); }); require(manager.Definitions().size() == 4,"Invalid automatic mapping published");
    automatic.Kind = MeshCutAutomaticKind::CrackDistance; automatic.Sign = -1; automatic.Threshold = 0; automatic.Visibility = 1;
    require(MeshCutAutomatic::Add(manager,"crack",automatic).State == MeshCutAutomaticState::Crossing,"Crack zero state");
    const auto definitions = manager.Definitions();
    const auto crack = std::find_if(definitions.begin(),definitions.end(),[](const auto& d) { return d.Id == "crack"; });
    require(crack != definitions.end() && crack->Visibility == 1 && crack->Origin==MeshCutOrigin::Xfem,"Crack Below semantics");
    require(!manager.Remove("crack") && !manager.Rename("crack","other") && !manager.Duplicate("crack","copy")
        && !manager.SetIsoValue("crack",.1) && !manager.SetVisibility("crack",4),"Automatic protection");
    const auto visible=manager.EvaluateVisible();
    const auto whole=std::find_if(visible.begin(),visible.end(),[](const auto& r) { return r.Definition.Id=="crack"; });
    require(whole!=visible.end() && whole->Portions[0]->GetMeshData()->NbIndexedElems()==volume->NbIndexedElems(),"XFEM Below clipped the source");
    const auto before=manager.DumpDefinitionsJson();
    auto invalidAuto=automatic; invalidAuto.Threshold=.1;
    require(!MeshCutAutomatic::Replace(manager,"explicit",{{"bad",invalidAuto}}) && manager.DumpDefinitionsJson()==before,"Failed snapshot changed live definitions");
    require(MeshCutAutomatic::Replace(manager,"explicit",{}) && manager.Definitions().size()==3,"Provider removal failed");
    auto paired=geometry(3); paired->AddNodePosition(Graphic3d_Vec3(-1,0,0)); RWCae::MeshEntityId ns[]={0,4,3,2}; paired->AddElement(RWCae::MeshElementTetrahedron(ns));
    Handle(RWCae::MeshNVecProperty) jump=new RWCae::MeshNVecProperty("jump",9,RWCae::MeshPropertyBase::MappingType::Elemental,RWCae::MeshNVecPropertyType::Tensor); jump->Init<RWCae::MeshScalarProperty>();
    for(int c=0;c<9;++c) { auto s=Handle(RWCae::MeshScalarProperty)::DownCast(jump->GetComponent(c)); s->Init(2); for(int e=0;e<2;++e) { s->SetValue(RWCae::MeshPropertyEntityInfo(e),c==0?4+4*e:0); } } jump->UpdateComponents();
    MeshCutViewManager interfaceManager(paired); require(interfaceManager.AddPlane("interface",Graphic3d_Vec4d(1,0,0,0)),"physical interface");
    MeshCutTraceOptions trace; trace.Policy=MeshCutTracePolicy::Negative; require(interfaceManager.SetTraceOptions("interface",true,trace),"negative physical trace");
    closeTo(MeshCutPhysics::Stress(interfaceManager.EvaluatePhysical("interface",jump),components,MeshCutPhysicsOptions()).Total.Force.x(),4);
    trace.Policy=MeshCutTracePolicy::Positive; require(interfaceManager.SetTraceOptions("interface",true,trace),"positive physical trace");
    closeTo(MeshCutPhysics::Stress(interfaceManager.EvaluatePhysical("interface",jump),components,MeshCutPhysicsOptions()).Total.Force.x(),2);
}
}
int main()
{
    try { tests(); std::cout << "CUT_PHYSICS_PASS\n"; return 0; }
    catch (const Standard_Failure& error) { std::cerr << "CUT_PHYSICS_FAIL " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "CUT_PHYSICS_FAIL " << error.what() << '\n'; }
    return 1;
}
