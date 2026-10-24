#include <ViewCaeGeom/MeshCutMaterial.h>
#include <ViewCaeGeom/MeshCutPhysics.h>
#include <RWCae/MeshTools.h>
#include <RWCae/MeshScalarProperty.h>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <map>
#include <RWCae/MeshAdaptorsIterator.h>
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
Handle(RWCae::MeshNVecProperty) vectorField(const Handle(RWCae::MeshData)& m,
    const std::function<Graphic3d_Vec3d(const Graphic3d_Vec3d&)>& value)
{
    auto f = displacement(m);
    for (RWCae::MeshEntityId n=0;n<m->NbNodes();++n) {
        const auto v=value(Graphic3d_Vec3d(m->Nodes()[n]));
        for (int c=0;c<3;++c) { f->GetComponent(c)->SetValue(RWCae::MeshPropertyEntityInfo(n),v[c]); }
    }
    f->UpdateComponents(); return f;
}
Handle(RWCae::MeshData) quadraticTetra()
{
    Handle(RWCae::MeshData) m=new RWCae::MeshData();
    for (auto p:{Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1),
        Graphic3d_Vec3(.5,0,0),Graphic3d_Vec3(.5,.5,0),Graphic3d_Vec3(0,.5,0),Graphic3d_Vec3(0,0,.5),Graphic3d_Vec3(.5,0,.5),Graphic3d_Vec3(0,.5,.5)}) { m->AddNodePosition(p); }
    const RWCae::MeshEntityId ids[]={0,1,2,3,4,5,6,7,8,9}; m->AddElement(RWCae::MeshElementTetrahedron2nd(ids)); return m;
}
double distanceToTriangle(const Graphic3d_Vec3d& p,const std::array<Graphic3d_Vec3d,3>& t)
{
    using V=Graphic3d_Vec3d; const auto a=t[1]-t[0],b=t[2]-t[0],v=p-t[0]; const auto n=V::Cross(a,b);
    const double aa=a.Dot(a),ab=a.Dot(b),bb=b.Dot(b),det=aa*bb-ab*ab;
    if(det>0) {
        const double u=(bb*v.Dot(a)-ab*v.Dot(b))/det,w=(aa*v.Dot(b)-ab*v.Dot(a))/det;
        if(u>=0&&w>=0&&u+w<=1) { return std::abs(v.Dot(n))/n.Modulus(); }
    }
    double d=std::numeric_limits<double>::infinity();
    for(int i=0;i<3;++i) {
        const auto direction=t[(i+1)%3]-t[i]; const double length=direction.Dot(direction);
        const double q=length>0?std::max(0.,std::min(1.,(p-t[i]).Dot(direction)/length)):0;
        d=std::min(d,(p-t[i]-direction*q).Modulus());
    }
    return d;
}
void gapBound(const MeshCutResult& cut)
{
    if(cut.Portions[1].IsNull()) { return; }
    const auto cap=cut.Portions[1]->GetMeshData();
    std::map<std::pair<RWCae::MeshEntityId,RWCae::MeshEntityId>,int> edges;
    for(RWCae::MeshEntityId e=0;e<cap->NbIndexedElems();++e) {
        std::vector<RWCae::MeshEntityId> ids; cap->ElementNodes(e,ids);
        for(size_t i=0;i<ids.size();++i) { ++edges[{std::min(ids[i],ids[(i+1)%ids.size()]),std::max(ids[i],ids[(i+1)%ids.size()])}]; }
    }
    for(unsigned side:{0u,2u}) {
        if(cut.Portions[side].IsNull()) { continue; }
        const auto data=cut.Portions[side]->GetMeshData(); std::vector<std::array<Graphic3d_Vec3d,3>> faces;
        auto collect=[&](const auto& a) {
            for(RWCae::MeshEntityId e=0;e<a.NbElements();++e) {
                const auto& f=a.GetElement(e);
                for(RWCae::MeshElemEntityId n=1;n+1<f.NbNodes();++n) { faces.push_back({{Graphic3d_Vec3d(data->Nodes()[f[0]]),Graphic3d_Vec3d(data->Nodes()[f[n]]),Graphic3d_Vec3d(data->Nodes()[f[n+1]])}}); }
            }
        };
        RWCae::MeshAdaptorsIterator2dElems::For(collect,data); RWCae::MeshAdaptorsIterator2dElems::For(collect,data,true);
        require(!faces.empty(),"Missing material boundary faces for gap check");
        for(const auto& edge:edges) {
            if(edge.second!=1) { continue; }
            const auto a=Graphic3d_Vec3d(cap->Nodes()[edge.first.first]),b=Graphic3d_Vec3d(cap->Nodes()[edge.first.second]);
            for(double t:{.25,.5,.75}) {
                const auto p=a*(1-t)+b*t; double distance=std::numeric_limits<double>::infinity();
                for(const auto& face:faces) { distance=std::min(distance,distanceToTriangle(p,face)); }
                require(distance<=2*cut.MaterialTolerance+2e-6,"Material cap/body sampled gap exceeds combined tolerance");
            }
        }
    }
}
void curvedMaterial()
{
    const auto m=quadraticTetra();
    const auto u=vectorField(m,[](const auto& p) { return Graphic3d_Vec3d(0,0,.6*p.x()*p.x()); });
    const auto flux=vectorField(m,[](const auto&) { return Graphic3d_Vec3d(0,0,1); });
    MeshCutViewManager cuts(m); auto options=cuts.GeometryOptions(); options.RelativeTolerance=0; options.AbsoluteTolerance=2e-4;
    require(cuts.SetGeometryOptions(options) && cuts.AddPlane("p",Graphic3d_Vec4d(0,0,1,-.3)),"Curved reference");
    require(cuts.SetFollowDeformation("p",true),"Curved material");
    auto base=cuts.Evaluate("p",scalar(m)); const auto count=base.Portions[1]->GetMeshData()->NbNodes();
    require(cuts.SetDeformation(u,.8),"Curved deformation"); const double k=.6*cuts.EffectiveDeformationScale();
    auto result=cuts.Evaluate("p",scalar(m)); auto cap=result.Portions[1]->GetMeshData();
    require(cap->NbNodes()>count && result.MaterialDepth>0 && result.MaterialResidual<=result.MaterialTolerance,"Curved interior was not sampled to tolerance");
    std::map<std::pair<RWCae::MeshEntityId,RWCae::MeshEntityId>,int> incidence;
    for(RWCae::MeshEntityId e=0;e<cap->NbIndexedElems();++e) {
        std::vector<RWCae::MeshEntityId> ids; cap->ElementNodes(e,ids); require(ids.size()==3,"Refined surface is not triangular");
        for(int a=0;a<3;++a) { ++incidence[{std::min(ids[a],ids[(a+1)%3]),std::max(ids[a],ids[(a+1)%3])}]; }
        for(const auto& b:{Graphic3d_Vec3d(1./3,1./3,1./3),Graphic3d_Vec3d(.2,.3,.5),Graphic3d_Vec3d(.6,.2,.2)}) {
            Graphic3d_Vec3d p(0); for(int a=0;a<3;++a) { p+=Graphic3d_Vec3d(cap->Nodes()[ids[a]])*b[a]; }
            require(std::abs(p.z()-(.3+k*p.x()*p.x()))<=result.MaterialTolerance*1.1,"Independent current curvature residual");
        }
    }
    for(const auto& e:incidence) {
        require(e.second==1||e.second==2,"Nonmanifold refinement");
        if(e.second==1) {
            const auto a=cap->Nodes()[e.first.first],b=cap->Nodes()[e.first.second];
            require((std::abs(a.x())<2e-6&&std::abs(b.x())<2e-6)||(std::abs(a.y())<2e-6&&std::abs(b.y())<2e-6)
                ||(std::abs(a.x()+a.y()-.7)<2e-6&&std::abs(b.x()+b.y()-.7)<2e-6),"Hanging material edge inside cap");
        }
    }
    auto physical=cuts.EvaluatePhysical("p",flux);
    closeTo(MeshCutPhysics::HeatFlux(physical,{{0,1,2}},{}).Total.HeatFlow,.245);
    closeTo(MeshCutIntegration::HeatFlux(physical,{{0,1,2}}).HeatFlow,.245);
    require(cuts.MaterialStatistics("p")[0]==1,"Adaptive refinement reintersected reference");
    base=MeshCutResult(); result=MeshCutResult(); physical=MeshCutResult(); cap.Nullify();
    // Curved retained boundaries use the same FE map/tolerance as the cap.
    for(unsigned mask:{3u,6u,1u,4u,5u,7u}) {
        cuts.SetVisibility("p",mask); auto all=cuts.Evaluate("p",scalar(m));
        require(all.MaterialResidual<=all.MaterialTolerance,"Retained boundary did not satisfy sampled tolerance");
        gapBound(all);
        for(unsigned part=0;part<3;++part) {
            if (!(mask&(1u<<part))) { continue; }
            const auto data=all.Portions[part]->GetMeshData();
            for(RWCae::MeshEntityId n=0;n<data->NbNodes();++n) {
                MeshInterpolatedNode weights; require(all.Portions[part]->OriginNode(n,weights),"Retained boundary lost FE provenance");
                double refZ=0;
                for(const auto& w:weights.BaseNodes()) { refZ+=m->Nodes()[w.first].z()*w.second; }
                closeTo(data->Nodes()[n].z(),refZ+k*data->Nodes()[n].x()*data->Nodes()[n].x());
            }
        }
    }
    cuts.SetVisibility("p",2);
    // Strict limits fail explicitly, rather than silently accepting a coarse curved section.
    options.FaceSubdivisionLimit=0; require(cuts.SetGeometryOptions(options),"Zero depth setting");
    bool rejected=false; try { cuts.Evaluate("p"); } catch(const std::runtime_error&) { rejected=true; }
    require(rejected,"Unresolved curvature accepted at depth limit");
    const auto retained=cuts.CacheStatistics().ResultReserved;
    for(int retry=0;retry<4;++retry) {
        rejected=false; try { cuts.Evaluate("p"); } catch(const std::runtime_error&) { rejected=true; }
        require(rejected && cuts.CacheStatistics().ResultReserved==retained,"Failed sampling retained another allocation");
    }
    options.FaceSubdivisionLimit=7; options.MaxSourcePrimitives=2; cuts.SetGeometryOptions(options);
    rejected=false; try { cuts.Evaluate("p"); } catch(const std::runtime_error&) { rejected=true; }
    require(rejected,"Unbounded material refinement");
    // Nonfinite displacement and volume reversal are never treated as undeformed nodes.
    options.MaxSourcePrimitives=2000000; cuts.SetGeometryOptions(options);
    auto bad=vectorField(m,[](const auto&) { return Graphic3d_Vec3d(0); });
    bad->GetComponent(2)->SetValue(RWCae::MeshPropertyEntityInfo(4),std::numeric_limits<double>::quiet_NaN()); bad->UpdateComponents();
    cuts.SetDeformation(bad,.5); rejected=false;
    try { cuts.Evaluate("p"); } catch(const std::invalid_argument&) { rejected=true; }
    require(rejected,"Missing material displacement was silently skipped");
    bad=vectorField(m,[](const auto& p) { return Graphic3d_Vec3d(0,0,-2*p.z()); });
    cuts.SetDeformation(bad,1/RWCae::MeshTools::ComputeVectorTrsfRatio(m,bad)); rejected=false;
    try { cuts.Evaluate("p"); } catch(const std::invalid_argument&) { rejected=true; }
    require(rejected,"Inverted material volume accepted");
}
void warpedHexBoundary()
{
    const auto m=mesh(); const auto u=vectorField(m,[](const auto& p) { return Graphic3d_Vec3d(0,0,.3*p.x()*p.y()); }), t=scalar(m);
    MeshCutViewManager cuts(m); auto options=cuts.GeometryOptions(); options.AbsoluteTolerance=1e-3; options.RelativeTolerance=0;
    cuts.SetGeometryOptions(options); cuts.AddPlane("p",Graphic3d_Vec4d(0,0,1,-.5)); cuts.SetFollowDeformation("p",true);
    cuts.SetDeformation(u,.4);
    for(unsigned mask:{3u,6u,7u}) {
        cuts.SetVisibility("p",mask); const auto cut=cuts.Evaluate("p",t);
        require(!cut.BoundaryMesh && cut.MaterialResidual<=cut.MaterialTolerance,"Native retained volume boundary metadata");
        gapBound(cut);
        for(unsigned part=0;part<3;++part) {
            if(!(mask&(1u<<part))) { continue; }
            const auto data=cut.Portions[part]->GetMeshData();
            for(RWCae::MeshEntityId n=0;n<data->NbNodes();++n) {
                MeshInterpolatedNode weights; require(cut.Portions[part]->OriginNode(n,weights),"Lost native FE boundary map");
                double value=0; for(const auto& w:weights.BaseNodes()) { value+=w.second*t->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(w.first)); }
                closeTo(cut.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),value);
            }
        }
    }
    cuts.SetPlane("p",Graphic3d_Vec4d(0,0,1,-20));
    const auto empty=cuts.Evaluate("p",t);
    require(empty.Portions[1]->GetMeshData()->NbNodes()==0,"Empty material cap retained old facets");
}
void curvedShell()
{
    Handle(RWCae::MeshData) m=new RWCae::MeshData();
    for(auto p:{Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(1,1,0),Graphic3d_Vec3(0,1,0),
        Graphic3d_Vec3(.5,0,0),Graphic3d_Vec3(1,.5,0),Graphic3d_Vec3(.5,1,0),Graphic3d_Vec3(0,.5,0)}) { m->AddNodePosition(p); }
    const RWCae::MeshEntityId ids[]={0,1,2,3,4,5,6,7}; m->AddElement(RWCae::MeshElementQuad2nd(ids));
    const auto u=vectorField(m,[](const auto& p) { return Graphic3d_Vec3d(0,0,.4*p.y()*p.y()); });
    MeshCutViewManager cuts(m); auto options=cuts.GeometryOptions(); options.AbsoluteTolerance=2e-4; options.RelativeTolerance=0;
    cuts.SetGeometryOptions(options); cuts.AddPlane("p",Graphic3d_Vec4d(1,0,0,-.3)); cuts.SetFollowDeformation("p",true); cuts.SetDeformation(u,.8);
    const auto result=cuts.Evaluate("p",scalar(m)); const auto slice=Handle(MeshGeometrySlice)::DownCast(result.Portions[1]);
    const auto data=slice->GetMeshData(); require(slice->Lines().size()>1,"Quadratic material shell line was not refined");
    for(const auto& p:data->Nodes()) { closeTo(p.x(),.3); closeTo(p.z(),.4*cuts.EffectiveDeformationScale()*p.y()*p.y()); }
    for(size_t l=0;l<slice->Lines().size();++l) { const auto normal=result.MaterialNormal(l,true); closeTo(normal.x(),1); closeTo(normal.y(),0); }
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
    try { tests(); curvedMaterial(); warpedHexBoundary(); curvedShell(); std::cout<<"CUT_MATERIAL_PASS\n"; return 0; }
    catch(const Standard_Failure& e) { std::cerr<<"CUT_MATERIAL_FAIL "<<e.GetMessageString()<<'\n'; }
    catch(const std::exception& e) { std::cerr<<"CUT_MATERIAL_FAIL "<<e.what()<<'\n'; }
    return 1;
}
