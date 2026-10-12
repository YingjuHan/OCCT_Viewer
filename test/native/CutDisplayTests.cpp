#include <ViewCae/InteractiveCaeCutView.h>
#include <ViewCaeGeom/MeshCutFrame.h>
#include <RWCae/MeshScalarProperty.h>
#include <SelectMgr_Selection.hxx>
#include <Image_SupportedFormats.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value, const char* message) { if (!value) { throw std::runtime_error(message); } }
static void closeTo(double value, double reference) {
    if (!std::isfinite(value) || std::abs(value-reference) >= 1e-6) {
        std::cerr << "actual " << value << " expected " << reference << '\n'; throw std::runtime_error("Reference mismatch");
    }
}
class SelectableCut : public ViewCae::InteractiveCaeCutView {
public:
    explicit SelectableCut(const Handle(RWCae::MeshData)& mesh) : InteractiveCaeCutView(mesh) {}
    using InteractiveCaeCutView::ComputeSelection;
};
int main()
{
    try {
        Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
        for (auto p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) { mesh->AddNodePosition(p); }
        RWCae::MeshEntityId nodes[] = {0,1,2,3}; mesh->AddElement(RWCae::MeshElementTetrahedron(nodes));
        Handle(RWCae::MeshNVecProperty) field = new RWCae::MeshNVecProperty("temperature",1,RWCae::MeshPropertyBase::MappingType::Nodal);
        field->Init<RWCae::MeshScalarProperty>();
        auto scalar = Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(0)); scalar->Init(4);
        for (int n = 0; n < 4; ++n) { scalar->SetValue(RWCae::MeshPropertyEntityInfo(n),n); }
        field->UpdateComponents();
        Handle(SelectableCut) view = new SelectableCut(mesh);
        require(view->Cuts().AddPlane("p",Graphic3d_Vec4d(1,0,0,-0.25)),"Plane add");
        ViewCaeGeom::MeshCutFrame frame;
        frame.Origin = Graphic3d_Vec3d(0.25,0,0); frame.X = Graphic3d_Vec3d(0,1,0); frame.Y = Graphic3d_Vec3d(-1,0,0);
        require(frame.IsValid(),"Rotated datum");
        const auto local = frame.PlaneToLocal(Graphic3d_Vec4d(1,0,0,-0.25));
        const auto global = frame.PlaneToGlobal(local);
        closeTo(global.x(),1); closeTo(global.w(),-0.25);
        auto definition = view->Cuts().Definitions()[0]; definition.Plane = local;
        require(frame.Edit(view->Cuts(),definition),"Local edit");
        frame.Z = Graphic3d_Vec3d(0,0,-1); require(!frame.Edit(view->Cuts(),definition),"Left-handed datum accepted");
        ViewCae::MeshCutDisplayStyle style; style.Transparency = 0.4f; style.LineWidth = 4;
        view->SetStyle("p",ViewCaeGeom::MeshCutPortion::On,style);
        closeTo(view->Style(ViewCaeGeom::MeshCutPortion::On).Transparency,0);
        closeTo(view->EffectiveStyle("p",ViewCaeGeom::MeshCutPortion::On).Transparency,0.4);
        view->SetField(field); view->Synchronize();
        require(view->ContourState() == ViewCae::MeshCutContourState::Variable,"Variable state");
        closeTo(view->Legend()->GetMin(),0); closeTo(view->Legend()->GetMax(),3);
        auto options = view->ContourOptions(); options.AutomaticRange = false; options.Minimum = -1; options.Maximum = 5; options.ShowLegend = true;
        view->SetContourOptions(options); view->Synchronize();
        closeTo(view->Legend()->GetMin(),-1); closeTo(view->Legend()->GetMax(),5);
        options.Minimum = 10;
        bool rejected = false; try { view->SetContourOptions(options); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected,"Invalid range accepted"); closeTo(view->ContourOptions().Minimum,-1);
        const auto probe = view->Probe("p",ViewCaeGeom::MeshCutPortion::On,0,true);
        closeTo(probe.Values[0],probe.Position.x()+2*probe.Position.y()+3*probe.Position.z());
        require(probe.SourceElement == 0 && !probe.SourceNodes.empty(),"Probe provenance");
        closeTo(view->ProbeSource(3,true)[0],3);
        Handle(SelectMgr_Selection) selection = new SelectMgr_Selection(1); view->ComputeSelection(selection,1);
        require(!selection->Entities().IsEmpty(),"Missing selectable nodes");
        Handle(ViewCae::MeshCutPickOwner) owner = new ViewCae::MeshCutPickOwner(view,"p",1,0,true,view->SynchronizationCount());
        const auto picked = view->Probe(owner);
        closeTo(picked.Values[0],picked.Position.x()+2*picked.Position.y()+3*picked.Position.z());
        view->Synchronize(); rejected = false;
        try { view->Probe(owner); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected,"Stale owner accepted");
        scalar->SetValueRange(Bnd_Range());
        for (int n = 0; n < 4; ++n) { scalar->SetValue(RWCae::MeshPropertyEntityInfo(n),7); }
        field->UpdateComponents(); view->Cuts().InvalidateFields(); view->Synchronize();
        require(view->ContourState() == ViewCae::MeshCutContourState::Constant,"Constant state");
        auto fixed = view->ContourOptions(); fixed.Minimum = 0; fixed.Maximum = 20;
        view->SetContourOptions(fixed); view->Synchronize();
        const auto presentation = view->Presentations().front();
        const auto properties = presentation->PropertyData();
        const auto mapper = properties->ColorMapper(properties->Property(presentation->ActiveProperty()));
        const auto texture = mapper->ColorTexture()->GetImage(new Image_SupportedFormats());
        Quantity_Color expected; AIS_ColorScale::FindColor(7,0,20,(int)texture->SizeX(),expected);
        const auto actual = texture->PixelColor(0,0).GetRGB();
        require(std::abs(actual.Red()-expected.Red()) < 1.0/255 && std::abs(actual.Green()-expected.Green()) < 1.0/255
            && std::abs(actual.Blue()-expected.Blue()) < 1.0/255,"Constant field color does not match fixed legend range");
        double originalMin = 0, originalMax = 0; scalar->ValueRange().GetBounds(originalMin,originalMax);
        closeTo(originalMin,7); closeTo(originalMax,7);
        view->Cuts().SetPlane("p",Graphic3d_Vec4d(1,0,0,-2)); view->Synchronize();
        require(view->ContourState() == ViewCae::MeshCutContourState::Empty,"Empty section state");
        view->SetField(nullptr); view->Synchronize(); require(view->ContourState() == ViewCae::MeshCutContourState::NoField,"No-field state");
        view->ClearStyle("p",ViewCaeGeom::MeshCutPortion::On); closeTo(view->EffectiveStyle("p",ViewCaeGeom::MeshCutPortion::On).Transparency,0);
        Handle(RWCae::MeshData) joined = new RWCae::MeshData();
        for (auto p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(-1,0,0)}) { joined->AddNodePosition(p); }
        RWCae::MeshEntityId positive[] = {0,1,2,3}, negative[] = {0,2,1,4};
        joined->AddElement(RWCae::MeshElementTetrahedron(positive)); joined->AddElement(RWCae::MeshElementTetrahedron(negative));
        Handle(RWCae::MeshNVecProperty) jump = new RWCae::MeshNVecProperty("jump",1,RWCae::MeshPropertyBase::MappingType::Elemental);
        jump->Init<RWCae::MeshScalarProperty>();
        auto cells = Handle(RWCae::MeshScalarProperty)::DownCast(jump->GetComponent(0)); cells->Init(2);
        cells->SetValue(RWCae::MeshPropertyEntityInfo(0),10); cells->SetValue(RWCae::MeshPropertyEntityInfo(1),20); jump->UpdateComponents();
        Handle(ViewCae::InteractiveCaeCutView) interface = new ViewCae::InteractiveCaeCutView(joined);
        require(interface->Cuts().AddPlane("interface",Graphic3d_Vec4d(1,0,0,0)),"Interface probe plane"); interface->SetField(jump);
        ViewCaeGeom::MeshCutTraceOptions physical; physical.Policy = ViewCaeGeom::MeshCutTracePolicy::Negative;
        require(interface->Cuts().SetTraceOptions("interface",true,physical),"Physical trace policy");
        const auto negativeProbe = interface->Probe("interface",ViewCaeGeom::MeshCutPortion::On,0,true);
        require(negativeProbe.SourceElement == 1,"Physical probe owner incorrectly uses display owner"); closeTo(negativeProbe.Values[0],20);
        Handle(RWCae::MeshData) shell = new RWCae::MeshData();
        shell->AddNodePosition(Graphic3d_Vec3(0,0,0)); shell->AddNodePosition(Graphic3d_Vec3(1,0,0)); shell->AddNodePosition(Graphic3d_Vec3(0,1,0));
        RWCae::MeshEntityId triangle[] = {0,1,2}; shell->AddElement(RWCae::MeshElementTriangle(triangle));
        Handle(RWCae::MeshNVecProperty) shellField = new RWCae::MeshNVecProperty("shell",1,RWCae::MeshPropertyBase::MappingType::Nodal);
        shellField->Init<RWCae::MeshScalarProperty>(); auto values = Handle(RWCae::MeshScalarProperty)::DownCast(shellField->GetComponent(0)); values->Init(3);
        values->SetValue(RWCae::MeshPropertyEntityInfo(0),0); values->SetValue(RWCae::MeshPropertyEntityInfo(1),1); values->SetValue(RWCae::MeshPropertyEntityInfo(2),2); shellField->UpdateComponents();
        Handle(ViewCae::InteractiveCaeCutView) shellView = new ViewCae::InteractiveCaeCutView(shell);
        shellView->Cuts().AddPlane("line",Graphic3d_Vec4d(1,0,0,-0.25)); shellView->SetField(shellField);
        const auto lineProbe = shellView->ProbeLine("line",0,0);
        closeTo(lineProbe.Values[0],lineProbe.Position.x()+2*lineProbe.Position.y());
        const auto shellNode = shellView->Probe("line",ViewCaeGeom::MeshCutPortion::On,0,true);
        closeTo(shellNode.Values[0],shellNode.Position.x()+2*shellNode.Position.y());
        std::cout << "CUT_DISPLAY_PASS\n"; return 0;
    } catch (const std::exception& error) { std::cerr << "CUT_DISPLAY_FAIL " << error.what() << '\n'; return 1; }
}
