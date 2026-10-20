#include <ViewCaeGeom/MeshCutMaterial.h>
#include <RWCae/MeshScalarProperty.h>
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace ViewCaeGeom;
namespace {
void require(bool value,const char* message) { if (!value) { throw std::runtime_error(message); } }
void closeTo(double a,double b) { require(std::isfinite(a) && std::abs(a-b)<2e-6,"Material coordinate/field mismatch"); }
Handle(RWCae::MeshData) mesh()
{
    Handle(RWCae::MeshData) m = new RWCae::MeshData();
    for (int z=0;z<2;++z) { for (int y=0;y<2;++y) { for (int x=0;x<4;++x) { m->AddNodePosition(Graphic3d_Vec3((float)x,(float)y,(float)z)); } } }
    for (unsigned x=0;x<3;++x) { const RWCae::MeshEntityId n[]={x,x+1,x+5,x+4,x+8,x+9,x+13,x+12}; m->AddElement(RWCae::MeshElementHexahedron(n)); }
    return m;
}
Handle(RWCae::MeshNVecProperty) scalar(const Handle(RWCae::MeshData)& m)
{
    Handle(RWCae::MeshNVecProperty) f = new RWCae::MeshNVecProperty("T",1,RWCae::MeshPropertyBase::MappingType::Nodal);
    f->Init<RWCae::MeshScalarProperty>(); const auto c=Handle(RWCae::MeshScalarProperty)::DownCast(f->GetComponent(0)); c->Init(m->NbNodes());
    for (RWCae::MeshEntityId n=0;n<m->NbNodes();++n) { const auto p=m->Nodes()[n]; c->SetValue(RWCae::MeshPropertyEntityInfo(n),p.x()+2*p.y()+3*p.z()); }
    f->UpdateComponents(); return f;
}
Handle(RWCae::MeshNVecProperty) displacement(const Handle(RWCae::MeshData)& m)
{
    Handle(RWCae::MeshNVecProperty) f = new RWCae::MeshNVecProperty("U",3,RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
    f->Init<RWCae::MeshScalarProperty>();
    for (int c=0;c<3;++c) { const auto v=Handle(RWCae::MeshScalarProperty)::DownCast(f->GetComponent(c)); v->Init(m->NbNodes());
        for (RWCae::MeshEntityId n=0;n<m->NbNodes();++n) { const auto p=m->Nodes()[n]; v->SetValue(RWCae::MeshPropertyEntityInfo(n),c==2 ? .2*p.x()*p.x() : 0); }
    }
    f->UpdateComponents(); return f;
}
void tests()
{
    const auto m=mesh(); const auto u=displacement(m), t=scalar(m);
    MeshCutViewManager cuts(m); require(cuts.AddPlane("p",Graphic3d_Vec4d(0,0,1,-.5)),"Plane");
    require(cuts.SetFollowDeformation("p",true),"Material reference");
    const auto base=cuts.Evaluate("p",t); const auto count=base.Portions[1]->GetMeshData()->NbNodes(); require(count>0,"Missing material cap");
    const auto json=cuts.DumpDefinitionsJson();
    require(cuts.SetDeformation(u,.4),"Deform material"); const double scale=cuts.EffectiveDeformationScale();
    auto result=cuts.Evaluate("p",t); const auto cap=result.Portions[1]->GetMeshData(); require(cap->NbNodes()==count,"Reference topology changed");
    for (RWCae::MeshEntityId n=0;n<cap->NbNodes();++n) {
        MeshInterpolatedNode w; require(result.Portions[1]->OriginNode(n,w),"Missing original weights");
        Graphic3d_Vec3d expected(0); double temp=0;
        for (const auto& a:w.BaseNodes()) { Graphic3d_Vec3d d; u->GetVector(RWCae::MeshPropertyEntityInfo(a.first),d); expected+=(Graphic3d_Vec3d(m->Nodes()[a.first])+d*scale)*a.second; temp+=a.second*t->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(a.first)); }
        for(int a=0;a<3;++a) { closeTo(cap->Nodes()[n][a],expected[a]); }
        closeTo(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),temp);
    }
    require(cuts.MaterialStatistics("p")[0]==1,"Reintersected during deformation");
    for(double s:{-.2,0.,.2,.6}) { require(cuts.SetDeformation(u,s),"Scale"); cuts.Evaluate("p",t); }
    require(cuts.MaterialStatistics("p")[0]==1,"Reference cache lost during scale animation");
    require(cuts.MaterialStatistics("p")[1]==6,"Mapping counter");
    require(cuts.RestoreDefinitionsJson(json) && cuts.DumpDefinitionsJson()==json,"Material serialization");
    require(cuts.Duplicate("p","copy"),"Material duplicate");
    for(unsigned mask=0;mask<8;++mask) { cuts.SetVisibility("p",mask); result=cuts.Evaluate("p",t); for(unsigned part=0;part<3;++part) { if(mask&(1u<<part)) { require(!result.Portions[part].IsNull(),"Missing material portion"); } } }
    const auto old=cuts.DumpDefinitionsJson(); require(!cuts.SetFollowDeformation("p",true,8),"Invalid reference accepted"); require(cuts.DumpDefinitionsJson()==old,"Invalid reference changed state");
    require(cuts.SetPlane("p",Graphic3d_Vec4d(0,0,1,-.25)),"Move material plane"); cuts.EvaluateSectionForDisplay("p",t);
    require(cuts.MaterialStatistics("p")[0]==1,"Moved cut did not rebuild its own reference");
    require(cuts.SetFollowDeformation("p",false),"Spatial mode"); require(!cuts.Definitions().back().FollowDeformation,"Follow remained enabled");
    // Frozen reference field/scale is independent of the current deformation scale.
    m->ChangeProperties().Append(u);
    require(cuts.SetFollowDeformation("p",true,-1,"U",.1),"Reference displacement"); cuts.EvaluateSectionForDisplay("p",t);
    require(cuts.SetDeformation(u,.7),"Current scale"); cuts.EvaluateSectionForDisplay("p",t); require(cuts.MaterialStatistics("p")[0]==1,"Captured reference scale drifted");
    // A source with different correspondence must be rejected before publication.
    const auto remesh=mesh(); remesh->AddNodePosition(Graphic3d_Vec3(9,9,9));
    MeshCutViewManager target(remesh); target.SetMaterialSource(m); target.AddPlane("p",Graphic3d_Vec4d(0,0,1,-.5));
    require(!target.SetFollowDeformation("p",true),"Remesh material mapping guessed");
    // Shell lines, beam crossings and quadratic FE provenance use the same mapping.
    for (int kind=0;kind<3;++kind) {
        Handle(RWCae::MeshData) other = new RWCae::MeshData();
        if (kind==0) {
            for (auto p:{Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(1,1,0),Graphic3d_Vec3(0,1,0)}) { other->AddNodePosition(p); }
            const RWCae::MeshEntityId ids[]={0,1,2,3}; other->AddElement(RWCae::MeshElementQuad(ids));
        } else if (kind==1) {
            other->AddNodePosition(Graphic3d_Vec3(0,0,0)); other->AddNodePosition(Graphic3d_Vec3(1,0,0));
            RWCae::MeshData::GenericCell c; c.Type=3; c.Nodes={0,1}; other->AddGenericCell(c);
        } else {
            for (auto p:{Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1),
                Graphic3d_Vec3(.5,0,0),Graphic3d_Vec3(.5,.5,0),Graphic3d_Vec3(0,.5,0),Graphic3d_Vec3(0,0,.5),Graphic3d_Vec3(.5,0,.5),Graphic3d_Vec3(0,.5,.5)}) { other->AddNodePosition(p); }
            const RWCae::MeshEntityId ids[]={0,1,2,3,4,5,6,7,8,9}; other->AddElement(RWCae::MeshElementTetrahedron2nd(ids));
        }
        MeshCutViewManager material(other); material.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.3));
        require(material.SetFollowDeformation("p",true),"Mixed dimension reference");
        const auto field=displacement(other); require(material.SetDeformation(field,.2),"Mixed dimension deformation");
        const auto mapped=material.Evaluate("p",scalar(other)); const auto data=mapped.Portions[1]->GetMeshData();
        require(data->NbNodes()>0,"Missing shell/beam/high-order intersection");
        for (RWCae::MeshEntityId n=0;n<data->NbNodes();++n) {
            MeshInterpolatedNode weights; require(mapped.Portions[1]->OriginNode(n,weights),"Lost mixed-dimension FE weights");
            double referenceX=0,expectedZ=0;
            for (const auto& w:weights.BaseNodes()) { const auto p=other->Nodes()[w.first]; referenceX+=w.second*p.x(); expectedZ+=w.second*(p.z()+material.EffectiveDeformationScale()*.2*p.x()*p.x()); }
            closeTo(referenceX,.3); closeTo(data->Nodes()[n].z(),expectedZ);
        }
    }
}
}
int main() {
    try { tests(); std::cout<<"CUT_MATERIAL_PASS\n"; return 0; }
    catch(const Standard_Failure& e) { std::cerr<<"CUT_MATERIAL_FAIL "<<e.GetMessageString()<<'\n'; }
    catch(const std::exception& e) { std::cerr<<"CUT_MATERIAL_FAIL "<<e.what()<<'\n'; }
    return 1;
}
