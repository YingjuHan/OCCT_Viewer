#include <ViewCaeGeom/MeshCutSlicing.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <ViewCaeGeom/MeshGeometrySlice.h>
#include <ViewCaeGeom/MeshCutTrace.h>
#include <RWCae/MeshScalarProperty.h>
#include <ViewCaeGeom/MeshGeometryDeformed.h>

// Exceeds the native staging threshold and the compact trace threshold.
// Verify analytic geometry and original FE identities, not thread scheduling.
static void largePlaneSection()
{
    using namespace ViewCaeGeom;
    const int n=100; // One million cells; several output blocks and mixed fast/fallback paths.
    Handle(RWCae::MeshData) source=new RWCae::MeshData();
    const auto id=[&](int x,int y,int z) { return (RWCae::MeshEntityId)(x+(n+1)*(y+(n+1)*z)); };
    for(int z=0;z<=n;++z) { for(int y=0;y<=n;++y) { for(int x=0;x<=n;++x) { source->AddNodePosition(Graphic3d_Vec3(x,y,z)); } } }
    for(int z=0;z<n;++z) { for(int y=0;y<n;++y) { for(int x=0;x<n;++x) {
        const RWCae::MeshEntityId nodes[]={id(x,y,z),id(x+1,y,z),id(x+1,y+1,z),id(x,y+1,z),id(x,y,z+1),id(x+1,y,z+1),id(x+1,y+1,z+1),id(x,y+1,z+1)};
        source->AddElement(RWCae::MeshElementHexahedron(nodes));
    } } }
    MeshCutViewManager manager(source); manager.AddPlane("large",Graphic3d_Vec4d(1,0,0,-24.25));
    Handle(RWCae::MeshNVecProperty) field=new RWCae::MeshNVecProperty("linear",1,RWCae::MeshPropertyBase::MappingType::Nodal);
    field->Init<RWCae::MeshScalarProperty>();
    const auto component=Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(0)); component->Init(source->NbNodes());
    for(RWCae::MeshEntityId node=0;node<source->NbNodes();++node) { const auto& p=source->Nodes()[node]; component->SetValue(RWCae::MeshPropertyEntityInfo(node),p.x()+2*p.y()+3*p.z()); }
    field->UpdateComponents();
    for(double offset:{24.25,24.75,25.25,25.0}) {
        manager.SetPlane("large",Graphic3d_Vec4d(1,0,0,-offset));
        const auto result=manager.EvaluateSectionForDisplay("large",field);
        const auto section=result.Portions[1]; const auto mesh=section->GetMeshData();
        const auto values=result.Fields[1]->GetComponent(0);
        double minimum=0,maximum=0;
        if(!values->ValueRange().GetBounds(minimum,maximum)) { throw std::runtime_error("Parallel nodal field range is empty"); }
        // Generated components retain the source range for a stable color scale.
        if(std::abs(minimum)>1e-9 || std::abs(maximum-6*n)>1e-9) { throw std::runtime_error("Parallel nodal field source range mismatch"); }
        if(mesh->NbIndexedElems()!=n*n || mesh->NbNodes()!=(n+1)*(n+1)) { throw std::runtime_error("Large section has duplicate/missing faces or shared nodes"); }
        double area=0;
        for(RWCae::MeshEntityId e=0;e<mesh->NbIndexedElems();++e) {
            RWCae::MeshEntityId original; const auto& entry=mesh->IndexedElements()[e];
            if(!section->OriginElement(MeshGeomElem(entry.Index,entry.Type,false),original) || original>=source->NbIndexedElems()) { throw std::runtime_error("Large section missing source element"); }
            std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(e,nodes);
            for(size_t i=1;i+1<nodes.size();++i) {
                const Graphic3d_Vec3d a(mesh->Nodes()[nodes[0]]),b(mesh->Nodes()[nodes[i]]),c(mesh->Nodes()[nodes[i+1]]);
                area+=Graphic3d_Vec3d::Cross(b-a,c-a).Modulus()*.5;
            }
        }
        if(std::abs(area-n*n)>1e-8) { throw std::runtime_error("Large section area mismatch"); }
        for(RWCae::MeshEntityId node=0;node<mesh->NbNodes();++node) {
            MeshInterpolatedNode origin; if(!section->OriginNode(node,origin)) { throw std::runtime_error("Large section missing FE weights"); }
            double sum=0; Graphic3d_Vec3d position(0);
            for(const auto& base:origin.BaseNodes()) { sum+=base.second; position+=Graphic3d_Vec3d(source->Nodes()[base.first])*base.second; }
            if(std::abs(sum-1)>1e-12 || std::abs(position.x()-offset)>1e-9 || (position-Graphic3d_Vec3d(mesh->Nodes()[node])).Modulus()>1e-8) { throw std::runtime_error("Large section FE reconstruction mismatch"); }
            if(std::abs(values->Value(RWCae::MeshPropertyEntityInfo(node))-(position.x()+2*position.y()+3*position.z()))>1e-9) { throw std::runtime_error("Parallel nodal field interpolation mismatch"); }
        }
    }
    // Oblique cuts contain hex corner triangles with only four original
    // supports. They are interior traces, while the axis-aligned face above
    // must retain interface pairing/deduplication.
    const double slope=.2137,offset=32.25;
    manager.SetPlane("large",Graphic3d_Vec4d(1,slope,0,-offset));
    const auto result=manager.EvaluateSectionForDisplay("large",field);
    if(manager.LastTimings().Fallbacks==0 || manager.LastTimings().Fallbacks>=manager.LastTimings().Candidates) {
        throw std::runtime_error("Million-cell oblique reference did not exercise mixed fast/fallback output");
    }
    const auto section=Handle(MeshGeometryTraceSection)::DownCast(result.Portions[1]);
    const auto mesh=section->GetMeshData();
    std::vector<Graphic3d_Vec3d> positions(mesh->NbNodes());
    for(RWCae::MeshEntityId node=0;node<mesh->NbNodes();++node) {
        MeshInterpolatedNode origin; section->OriginNode(node,origin); positions[node]=Graphic3d_Vec3d(0);
        for(const auto& base:origin.BaseNodes()) { positions[node]+=Graphic3d_Vec3d(source->Nodes()[base.first])*base.second; }
        if(std::abs(positions[node].x()+slope*positions[node].y()-offset)>1e-9) { throw std::runtime_error("Oblique FE node is off plane"); }
        const double value=result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(node));
        if(std::abs(value-(positions[node].x()+2*positions[node].y()+3*positions[node].z()))>1e-9) { throw std::runtime_error("Oblique parallel FE interpolation mismatch"); }
    }
    double area=0;
    for(RWCae::MeshEntityId e=0;e<mesh->NbIndexedElems();++e) {
        if(section->ElementTraces()[e].size()!=1 || section->ElementTraces()[e][0].Ambiguous) { throw std::runtime_error("Hex corner misclassified as interface"); }
        std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(e,nodes);
        for(size_t i=1;i+1<nodes.size();++i) { area+=Graphic3d_Vec3d::Cross(positions[nodes[i]]-positions[nodes[0]],positions[nodes[i+1]]-positions[nodes[0]]).Modulus()*.5; }
    }
    if(std::abs(area-n*n*std::sqrt(1+slope*slope))>1e-7) { throw std::runtime_error("Oblique section has missing/duplicate area"); }

    // A source snapshot is tied to its BVH, not to the last plane. Keeping an
    // old shared snapshot alive also checks atomic replacement after invalidation.
    Handle(RWCae::MeshNVecProperty) displacement=new RWCae::MeshNVecProperty("displacement",3,
        RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
    displacement->Init<RWCae::MeshScalarProperty>();
    for(int c=0;c<3;++c) {
        const auto component=Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c)); component->Init(source->NbNodes());
        for(RWCae::MeshEntityId node=0;node<source->NbNodes();++node) { component->SetValue(RWCae::MeshPropertyEntityInfo(node),c==0?1:0); }
    }
    displacement->UpdateComponents();
    Handle(MeshBvhData) bvh=new MeshBvhData(); Handle(MeshGeometry) root=new MeshGeometryBase(bvh,source);
    Handle(MeshGeometryDeformed) deformed=new MeshGeometryDeformed(bvh); root->Append(deformed); deformed->SetDeformedProperty(displacement);
    std::shared_ptr<const std::vector<Graphic3d_Vec3>> previous;
    for(double scale:{.1,.2,0.0}) {
        deformed->SetDeformedScaleCoeff(scale);
        if(bvh->PreparedNodePositions(root)) { throw std::runtime_error("Deformation update retained stale node snapshot"); }
        bvh->BvhBoxSet(root); const auto snapshot=bvh->PreparedNodePositions(root);
        if(scale==0) { if(snapshot) { throw std::runtime_error("Zero deformation allocated a node snapshot"); } continue; }
        if(!snapshot || snapshot==previous || snapshot!=bvh->PreparedNodePositions(root) || snapshot->size()!=source->NbNodes()) {
            throw std::runtime_error("Modified source snapshot lifetime mismatch");
        }
        for(RWCae::MeshEntityId node=0;node<source->NbNodes();node+=997) {
            auto expected=source->Nodes()[node]; root->ModifyNode(node,expected);
            if((*snapshot)[node]!=expected) { throw std::runtime_error("Cached deformation differs from modifier"); }
        }
        previous=snapshot;
    }
    root->RemoveChildren(); bvh->Clear();
}

using namespace ViewCaeGeom;
static void require(bool value, const char* message) { if (!value) { throw std::runtime_error(message); } }
static void closeTo(double value, double reference, double tolerance = 1e-9) {
    if (!std::isfinite(value) || std::abs(value-reference) >= tolerance) {
        std::cerr << "actual=" << value << " reference=" << reference << " tolerance=" << tolerance << '\n';
        throw std::runtime_error("Slice reference mismatch");
    }
}
template<class F> static void rejects(F action) { bool rejected = false; try { action(); } catch (const std::invalid_argument&) { rejected = true; } require(rejected,"Invalid slicing input accepted"); }
int main()
{
    try {
        largePlaneSection();
        MeshCutDefinition plane; plane.Id = "slice"; plane.Plane = Graphic3d_Vec4d(2,0,0,-1);
        const auto regular = MeshCutSlicing::Regular(plane,0,0.25,5);
        require(regular.size() == 5,"Regular count");
        for (size_t i = 0; i < regular.size(); ++i) {
            closeTo(regular[i].Definition.Plane.x(),1); closeTo(regular[i].Definition.Plane.w(),-0.5-0.25*i);
            require(regular[i].Frame.IsValid(),"Regular frame"); closeTo(regular[i].Frame.Origin.x(),0.5+0.25*i);
        }
        closeTo(regular[0].IntervalBegin,0); closeTo(regular[0].IntervalEnd,0.125); closeTo(regular[4].IntervalEnd,1);
        const auto range = MeshCutSlicing::Range(plane,-1,1,3); closeTo(range[1].Definition.Plane.w(),-0.5);
        const double pi = std::acos(-1.0);
        const auto rotated = MeshCutSlicing::Rotational(plane,Graphic3d_Vec3d(0,0,2),Graphic3d_Vec3d(0,0,0),0,pi/2,3);
        closeTo(rotated.back().Definition.Plane.x(),0); closeTo(rotated.back().Definition.Plane.y(),1); closeTo(rotated.back().Definition.Plane.w(),-0.5);
        const auto pivot = MeshCutSlicing::Rotational(plane,Graphic3d_Vec3d(0,0,1),Graphic3d_Vec3d(1,0,0),pi/2,pi/2,1);
        closeTo(pivot[0].Frame.Origin.x(),1); closeTo(pivot[0].Frame.Origin.y(),-0.5); closeTo(pivot[0].Definition.Plane.w(),0.5);
        const auto path = MeshCutSlicing::Path("path",{Graphic3d_Vec3d(0,0,0),Graphic3d_Vec3d(1,0,0),Graphic3d_Vec3d(1,1,0),Graphic3d_Vec3d(1,1,2)},Graphic3d_Vec3d(0,0,1));
        for (size_t i = 0; i < path.size(); ++i) {
            require(path[i].Frame.IsValid(),"Transport frame");
            const auto p = path[i].Frame.Origin;
            closeTo(path[i].Definition.Plane.x()*p.x()+path[i].Definition.Plane.y()*p.y()+path[i].Definition.Plane.z()*p.z()+path[i].Definition.Plane.w(),0);
        }
        closeTo(path.back().Coordinate,4); closeTo(path[1].IntervalBegin,0.5); closeTo(path[2].IntervalEnd,3);
        const auto resampled=MeshCutSlicing::ResampledPath("equal",{Graphic3d_Vec3d(0,0,0),Graphic3d_Vec3d(1,0,0),Graphic3d_Vec3d(1,3,0)},Graphic3d_Vec3d(0,0,1),4);
        for(size_t i=0;i<resampled.size();++i) { closeTo(resampled[i].Coordinate,4.0*i/3); require(resampled[i].Frame.IsValid(),"Resampled frame"); }
        closeTo(resampled[1].Frame.Origin.y(),1.0/3);
        Handle(RWCae::MeshData) tetra = new RWCae::MeshData();
        for (auto p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) { tetra->AddNodePosition(p); }
        RWCae::MeshEntityId ids[] = {0,1,2,3}; tetra->AddElement(RWCae::MeshElementTetrahedron(ids));
        MeshCutViewManager manager(tetra); plane.Plane = Graphic3d_Vec4d(1,0,0,0);
        for (const auto& sample : MeshCutSlicing::Range(plane,0.1,0.9,5)) {
            require(manager.AddPlane(sample.Definition.Id,sample.Definition.Plane),"Slice branch creation");
            const auto result = manager.Evaluate(sample.Definition.Id);
            const auto mesh = result.Portions[1]->GetMeshData();
            require(mesh->NbIndexedElems() == 1 && mesh->NbElems(RWCae::MeshElemType::Triangle) == 1,"Slice section topology");
            std::vector<RWCae::MeshEntityId> triangle; mesh->ElementNodes(0,triangle);
            const Graphic3d_Vec3d a(mesh->Nodes()[triangle[0]]), b(mesh->Nodes()[triangle[1]]), c(mesh->Nodes()[triangle[2]]);
            // Generated MeshData positions use float storage; the exact frame checks
            // above retain their double tolerance.
            closeTo(Graphic3d_Vec3d::Cross(b-a,c-a).Modulus()/2,(1-sample.Coordinate)*(1-sample.Coordinate)/2,2e-7);
        }
        // Transport preserves the initial X through a reversal; it does not invent an arbitrary roll.
        const auto reversal = MeshCutSlicing::Path("reverse",{Graphic3d_Vec3d(0,0,0),Graphic3d_Vec3d(1,0,0),Graphic3d_Vec3d(0,0,0)},Graphic3d_Vec3d(0,0,1));
        closeTo(reversal[0].Frame.X.Dot(reversal[1].Frame.X),1);
        rejects([&] { MeshCutSlicing::Regular(plane,0,0,10); });
        rejects([&] { MeshCutSlicing::Range(plane,1,0,10); });
        rejects([&] { MeshCutSlicing::Range(plane,0,1,1); });
        rejects([&] { MeshCutSlicing::Range(plane,0,1,10001); });
        rejects([&] { MeshCutSlicing::Rotational(plane,Graphic3d_Vec3d(0),Graphic3d_Vec3d(0),0,1,3); });
        rejects([&] { MeshCutSlicing::Path("bad",{Graphic3d_Vec3d(0),Graphic3d_Vec3d(0)},Graphic3d_Vec3d(0,0,1)); });
        rejects([&] { MeshCutSlicing::Path("bad",{Graphic3d_Vec3d(0),Graphic3d_Vec3d(1,0,0)},Graphic3d_Vec3d(1,0,0)); });
        std::cout << "CUT_SLICING_PASS\n"; return 0;
    } catch (const std::exception& error) { std::cerr << "CUT_SLICING_FAIL " << error.what() << '\n'; return 1; }
}
