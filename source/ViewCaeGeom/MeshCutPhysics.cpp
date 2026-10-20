#include <ViewCaeGeom/MeshCutPhysics.h>
#include <ViewCaeGeom/MeshCutTrace.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <ViewCaeGeom/MeshCellKernel.h>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace {
using namespace ViewCaeGeom;
using Vec = Graphic3d_Vec3d;
using Id = RWCae::MeshEntityId;
Vec unit(const Vec& v)
{
    const double length = std::hypot(v.x(),v.y(),v.z());
    if (!std::isfinite(length) || length <= 1e-12) { throw std::invalid_argument("Invalid physical normal/direction"); }
    return v/length;
}
int dimension(const Handle(RWCae::MeshData)& source,Id e)
{
    const auto& entry=source->IndexedElements()[e];
    if(entry.Type!=RWCae::MeshElemType::Generic) {
        const int type=(int)entry.Type;
        return type>=RWCae::MeshElem3dLowerType && type<=RWCae::MeshElem3dUpperType ? 3 : type>=RWCae::MeshSurfElemLowerType && type<=RWCae::MeshSurfElemUpperType ? 2 : 0;
    }
    const int type=source->GenericCells().at(entry.Index).Type;
    if(type==3) { return 1; }
    auto cell=MeshCellKernel::InstantiateCell(type);
    if(cell) { return cell->GetCellDimension(); }
    throw std::invalid_argument("Unsupported physical source dimension");
}
void validate(const MeshCutResult& cut, const MeshCutPhysicsOptions& options)
{
    if (cut.PresentationOnly || cut.Portions[1].IsNull() || cut.Roots[1].IsNull()
        || !options.Units.IsValid() || !options.OutputFrame.IsValid() || !options.FieldFrame.IsValid()
        || options.GroupName.empty() || (options.GroupName != "all" && options.Elements.empty() && !options.DomainContains)
        || (options.NormalSign != -1 && options.NormalSign != 1)
        || (options.SumPoint != MeshCutSummationPoint::UserDefined && options.SumPoint != MeshCutSummationPoint::Centroid)
        || (options.Summation != MeshCutSummation::Signed && options.Summation != MeshCutSummation::AbsoluteContributions)) {
        throw std::invalid_argument("Physics requires a numerical section, explicit group, units and frames");
    }
    const auto source = cut.Roots[1]->GetMeshData();
    if(!cut.SourceField.IsNull() && cut.SourceField->NbValues()!=(int)(cut.SourceField->IsNodal()?source->NbNodes():source->NbIndexedElems())) { throw std::invalid_argument("Physical source tuple count mismatch"); }
    for (int c = 0; c < 3; ++c) { if (!std::isfinite(options.MomentOrigin[c])) { throw std::invalid_argument("Invalid moment origin"); } }
    for (Id e : options.Elements) { if (e >= source->NbIndexedElems()) { throw std::invalid_argument("Group refers to an unknown source element"); } }
    for (const auto& entry : options.ElementNormals) {
        if (entry.first >= source->NbIndexedElems()) { throw std::invalid_argument("Normal refers to an unknown source element"); }
        unit(entry.second);
    }
    for (const auto& entry : options.Sections) {
        const auto& section = entry.second;
        if (entry.first >= source->NbIndexedElems() || !section.Frame.IsValid()
            || (int)section.Kind < 0 || (int)section.Kind > 4) { throw std::invalid_argument("Invalid section owner/frame/kind"); }
        const int dim=dimension(source,entry.first);
        if((section.Kind==MeshCutSectionKind::Volume && dim!=3)
            || ((section.Kind==MeshCutSectionKind::ShellStress || section.Kind==MeshCutSectionKind::CompositeStress) && dim!=2)
            || ((section.Kind==MeshCutSectionKind::BeamStress || section.Kind==MeshCutSectionKind::BeamResultants) && dim!=1)) { throw std::invalid_argument("Section adapter and source dimension disagree"); }
        if (section.Kind == MeshCutSectionKind::ShellStress && (!std::isfinite(section.Thickness) || section.Thickness <= 0)) {
            throw std::invalid_argument("Shell stress requires explicit positive thickness");
        }
        if (section.Kind == MeshCutSectionKind::BeamStress && (!std::isfinite(section.Area) || section.Area <= 0)) {
            throw std::invalid_argument("Beam stress requires explicit positive section area");
        }
        if (section.Kind == MeshCutSectionKind::CompositeStress) {
            if (section.Layers.empty()) { throw std::invalid_argument("Composite stress requires explicit layers"); }
            for (const auto& layer : section.Layers) {
                if (layer.Field.IsNull() || !layer.Field->IsInitialized() || !layer.FieldFrame.IsValid()
                    || layer.Field->NbValues() != (int)(layer.Field->IsNodal() ? source->NbNodes() : source->NbIndexedElems())
                    || !std::isfinite(layer.Thickness) || layer.Thickness <= 0 || !std::isfinite(layer.Offset)) {
                    throw std::invalid_argument("Invalid composite field/thickness/offset/frame");
                }
                for (int c : layer.Components) { if (c < 0 || c >= layer.Field->NbComponents()) { throw std::invalid_argument("Invalid composite component mapping"); } }
            }
        }
    }
}
bool included(const MeshCutPhysicsOptions& options, Id e) { return (!options.DomainContains || options.DomainContains(e)) && (options.Elements.empty() || options.Elements.count(e)); }
Vec normal(const MeshCutDefinition& definition, Id e, const Vec& p, const MeshCutPhysicsOptions& options)
{
    if (options.Normal) { return unit(options.Normal(e,p))*options.NormalSign; }
    if (!options.ElementNormals.empty()) {
        const auto found = options.ElementNormals.find(e);
        if (found == options.ElementNormals.end()) { throw std::invalid_argument("Missing explicit physical normal for source element"); }
        return unit(found->second)*options.NormalSign;
    }
    if (definition.Shape == MeshCutShape::Plane) { return unit(Vec(definition.Plane.x(),definition.Plane.y(),definition.Plane.z()))*options.NormalSign; }
    auto n = p-definition.Center;
    if (definition.Shape == MeshCutShape::Cylinder) { n -= definition.Axis*n.Dot(definition.Axis); }
    else if (definition.Shape != MeshCutShape::Sphere) { throw std::invalid_argument("Isosurface physics requires explicit local normal mapping"); }
    return unit(n)*options.NormalSign;
}
template<size_t N> std::array<double,N> values(const Handle(RWCae::MeshNVecProperty)& field,
    const std::array<int,N>& components, Id primitive, const std::vector<std::pair<Id,double>>& weights)
{
    if (field.IsNull() || !field->IsInitialized()) { throw std::invalid_argument("Missing physical field mapping"); }
    std::array<double,N> result{};
    for (size_t c = 0; c < N; ++c) {
        if (components[c] < 0 || components[c] >= field->NbComponents()) { throw std::invalid_argument("Invalid physical component mapping"); }
        for (const auto& w : weights) {
            if (w.second == 0) { continue; }
            const auto entity = RWCae::MeshPropertyEntityInfo(field->IsNodal() ? w.first : primitive,field->HasNodeVecComponents() ? (int)w.first : -1);
            if(entity.Index<0 || entity.Index>=field->NbValues()) { throw std::invalid_argument("Physical tuple index outside mapped field"); }
            const double value = field->GetComponent(components[c])->Value(entity);
            if (!std::isfinite(value)) { throw std::invalid_argument("Missing or nonfinite physical component"); }
            result[c] += value*w.second;
        }
    }
    return result;
}
Vec traction(const std::array<double,9>& tensor, const Vec& globalNormal, const MeshCutFrame& frame)
{
    const auto n = frame.VectorToLocal(globalNormal); Vec t(0);
    for (int i = 0; i < 3; ++i) { for (int j = 0; j < 3; ++j) { t[i] += tensor[3*i+j]*n[j]; } }
    return frame.VectorToGlobal(t);
}
void add(MeshCutPhysicsReport& report, MeshCutDistributedResult item, const MeshCutPhysicsOptions& options)
{
    item.Position = options.OutputFrame.PointToLocal(item.Position)*options.Units.LengthToSI;
    item.Result.Force = options.OutputFrame.VectorToLocal(item.Result.Force);
    item.Result.Moment = options.OutputFrame.VectorToLocal(item.Result.Moment);
    item.Normal = options.OutputFrame.VectorToLocal(item.Normal);
    report.Total.Force += item.Result.Force; report.Total.Moment += item.Result.Moment;
    report.Total.Area += item.Result.Area; report.Total.HeatFlow += item.Result.HeatFlow;
    for (int c = 0; c < 3; ++c) {
        if (!std::isfinite(report.Total.Force[c]) || !std::isfinite(report.Total.Moment[c])) { throw std::invalid_argument("Physical resultant overflow"); }
    }
    if (!std::isfinite(report.Total.Area) || !std::isfinite(report.Total.HeatFlow)) { throw std::invalid_argument("Physical scalar resultant overflow"); }
    report.Distributed.push_back(item);
}
void finish(MeshCutPhysicsReport& report,const MeshCutPhysicsOptions& options)
{
    report.OutputFrame = options.OutputFrame;
    const auto oldOrigin = options.OutputFrame.PointToLocal(options.MomentOrigin)*options.Units.LengthToSI;
    double area = 0; Vec weighted(0), normal(0);
    for (const auto& item : report.Distributed) {
        area += item.Result.Area; weighted += item.Position*item.Result.Area; normal += item.Normal*item.Result.Area;
    }
    if (area > 0) { report.Centroid = weighted/area; }
    else if (!report.Distributed.empty()) {
        // Beam crossings have no surface area; their connectivity-line locations define the centre.
        for (const auto& item : report.Distributed) { report.Centroid += item.Position; normal += item.Normal; }
        report.Centroid /= (double)report.Distributed.size();
    } else if (options.SumPoint == MeshCutSummationPoint::Centroid) {
        throw std::invalid_argument("Empty cut/domain has no section centroid");
    }
    const double normalLength = normal.Modulus();
    const double normalWeight = area > 0 ? area : (double)report.Distributed.size();
    report.MeanNormal = normalLength > normalWeight*1e-12 ? normal/normalLength : Vec(0);
    report.MomentOrigin = options.SumPoint == MeshCutSummationPoint::Centroid ? report.Centroid : oldOrigin;
    report.Total = MeshCutResultant();
    for (auto& item : report.Distributed) {
        item.Result.Moment += Vec::Cross(oldOrigin-report.MomentOrigin,item.Result.Force);
        if (options.Summation == MeshCutSummation::AbsoluteContributions) {
            for (int c = 0; c < 3; ++c) { item.Result.Force[c] = std::abs(item.Result.Force[c]); item.Result.Moment[c] = std::abs(item.Result.Moment[c]); }
            item.Result.HeatFlow = std::abs(item.Result.HeatFlow);
        }
        report.Total.Area += item.Result.Area; report.Total.Force += item.Result.Force;
        report.Total.Moment += item.Result.Moment; report.Total.HeatFlow += item.Result.HeatFlow;
    }
    for (const auto& vector : {report.Centroid,report.MomentOrigin,report.Total.Force,report.Total.Moment}) {
        for (int c = 0; c < 3; ++c) { if (!std::isfinite(vector[c])) { throw std::invalid_argument("Physical summation overflow"); } }
    }
}
template<size_t N, class Evaluate> MeshCutPhysicsReport integrate(const MeshCutResult& cut,
    const std::array<int,N>& components, const MeshCutPhysicsOptions& options, Evaluate evaluate)
{
    validate(cut,options);
    if (cut.Definition.FollowDeformation && !cut.MaterialNormal && options.ReferenceGeometry.IsNull() && !options.Normal && options.ElementNormals.empty()) {
        throw std::invalid_argument("Material section physics requires current-configuration local normals");
    }
    MeshCutPhysicsReport report; report.Cut = cut.Definition.Id; report.Group = options.GroupName;
    const auto mesh = cut.Portions[1]->GetMeshData(); const auto source = cut.Roots[1]->GetMeshData();
    const auto field = !cut.PhysicalFields[1].IsNull() ? cut.PhysicalFields[1] : cut.Fields[1];
    if (cut.DisplayRecovered && cut.PhysicalFields[1].IsNull()) { throw std::invalid_argument("Cannot integrate recovered display fields"); }
    constexpr double quadrature[3][3] = {{2.0/3,1.0/6,1.0/6},{1.0/6,2.0/3,1.0/6},{1.0/6,1.0/6,2.0/3}};
    auto faces = [&](const auto& adaptor) {
        for (Id e = 0; e < adaptor.NbElements(); ++e) {
            const auto& elem = adaptor.GetElement(e); Id origin = 0;
            if (cut.PhysicalProvenance) { origin = cut.PhysicalProvenance(elem.GlobalIndex(),false).Element; }
            else if (!cut.Portions[1]->OriginElement(MeshGeomElem(e,adaptor.GetElementType(),false),origin)) { throw std::invalid_argument("Missing face provenance"); }
            if (!included(options,origin)) { continue; }
            if(origin>=source->NbIndexedElems() || dimension(source,origin)!=3) { throw std::invalid_argument("Physical face lacks a volume owner"); }
            const auto mapping = options.Sections.find(origin);
            if (mapping != options.Sections.end() && mapping->second.Kind != MeshCutSectionKind::Volume) { throw std::invalid_argument("Surface adapter assigned to a volume section"); }
            for (RWCae::MeshElemEntityId i = 1; i+1 < elem.NbNodes(); ++i) {
                const Id ids[] = {elem[0],elem[i],elem[i+1]}; Vec p[] = {Vec(mesh->Nodes()[ids[0]]),Vec(mesh->Nodes()[ids[1]]),Vec(mesh->Nodes()[ids[2]])};
                const double area = Vec::Cross(p[1]-p[0],p[2]-p[0]).Modulus()/2;
                if (area <= 0) { continue; }
                auto localNormal = [&](const Vec& position) {
                    if (!cut.Definition.FollowDeformation || options.Normal || !options.ElementNormals.empty()) { return normal(cut.Definition,origin,position,options); }
                    auto n = unit(Vec::Cross(p[1]-p[0],p[2]-p[0]));
                    return (n.Dot(cut.MaterialNormal(elem.GlobalIndex(),false)) < 0 ? -n : n)*options.NormalSign;
                };
                MeshCutDistributedResult item; item.Element = origin; item.Position = (p[0]+p[1]+p[2])/3;
                item.Normal = localNormal(item.Position);
                item.Result.Area = area*options.Units.LengthToSI*options.Units.LengthToSI;
                for (const auto& w : quadrature) {
                    const auto position = p[0]*w[0]+p[1]*w[1]+p[2]*w[2];
                    evaluate(item.Result,localNormal(position),position,
                        values(field,components,elem.GlobalIndex(),{{ids[0],w[0]},{ids[1],w[1]},{ids[2],w[2]}}),area/3,options.FieldFrame);
                }
                add(report,item,options);
            }
        }
    };
    RWCae::MeshAdaptorsIterator2dElems::For(faces,mesh);
    const auto slice = Handle(MeshGeometrySlice)::DownCast(cut.Portions[1]);
    if (!slice.IsNull()) {
        const auto lineField = !cut.PhysicalLineField.IsNull() ? cut.PhysicalLineField : cut.LineField;
        for (size_t l = 0; l < slice->Lines().size(); ++l) {
            const auto& line = slice->Lines()[l];
            const Id origin = cut.PhysicalProvenance ? cut.PhysicalProvenance(l,true).Element : line.SourceElement;
            if (!included(options,origin)) { continue; }
            const auto found = options.Sections.find(origin);
            if (found == options.Sections.end() || (found->second.Kind != MeshCutSectionKind::ShellStress
                && found->second.Kind != MeshCutSectionKind::CompositeStress)) { throw std::invalid_argument("Shell section requires explicit thickness/layer mapping"); }
            const auto& mapping = found->second;
            std::vector<Id> support; source->ElementNodes(origin,support);
            if(support.size()<3) { throw std::invalid_argument("Shell support is degenerate"); }
            auto point=[&](Id id) { Graphic3d_Vec3 p=source->Nodes()[id]; cut.Roots[1]->ModifyNode(id,p); return Vec(p); };
            const auto p0=point(support[0]), p1=point(support[1]), p2=point(support[2]);
            const auto supportNormal=unit(Vec::Cross(p1-p0,p2-p0));
            if(std::abs(supportNormal.Dot(mapping.Frame.Z))<1-1e-6) { throw std::invalid_argument("Shell frame Z must match source support normal"); }
            for(Id node:support) { if(std::abs((point(node)-p0).Dot(mapping.Frame.Z))>1e-6*std::max(1.0,(p1-p0).Modulus())) { throw std::invalid_argument("Nonplanar shell needs an explicit varying section adapter"); } }
            const auto a = Vec(mesh->Nodes()[line.Nodes[0]]), b = Vec(mesh->Nodes()[line.Nodes[1]]);
            const auto tangent = unit(b-a); auto coNormal = unit(Vec::Cross(tangent,mapping.Frame.Z));
            const auto direction = cut.Definition.FollowDeformation && cut.MaterialNormal && !options.Normal && options.ElementNormals.empty()
                ? cut.MaterialNormal(l,true)*options.NormalSign : normal(cut.Definition,origin,(a+b)/2,options);
            if (coNormal.Dot(direction) < 0) { coNormal = -coNormal; }
            const double length = (b-a).Modulus(); MeshCutDistributedResult item; item.Element = origin; item.Position = (a+b)/2;
            item.Normal = coNormal;
            // Two-point Gauss is exact for linear membrane force and its quadratic moment.
            for (double t : {0.5-std::sqrt(1.0/12),0.5+std::sqrt(1.0/12)}) {
                const auto position = a*(1-t)+b*t;
                if (mapping.Kind == MeshCutSectionKind::ShellStress) {
                    evaluate(item.Result,coNormal,position,values(lineField,components,(Id)l,{{line.Nodes[0],1-t},{line.Nodes[1],t}}),length*mapping.Thickness/2,mapping.Frame);
                } else {
                    if constexpr (N != 9) { throw std::invalid_argument("Composite heat flow needs an explicit flux layer adapter"); }
                    else {
                        if (!cut.PhysicalProvenance) { throw std::invalid_argument("Composite layers require original FE trace weights"); }
                        const auto& trace = cut.PhysicalProvenance(l,true);
                        std::map<Id,double> sourceWeights;
                        for (int endpoint = 0; endpoint < 2; ++endpoint) {
                            const auto weights = trace.Weights.find(line.Nodes[endpoint]);
                            if (weights == trace.Weights.end()) { throw std::invalid_argument("Missing composite FE endpoint weights"); }
                            for (const auto& w : weights->second.BaseNodes()) { sourceWeights[w.first] += w.second*(endpoint ? t : 1-t); }
                        }
                        const std::vector<std::pair<Id,double>> weights(sourceWeights.begin(),sourceWeights.end());
                        for (const auto& layer : mapping.Layers) {
                            evaluate(item.Result,coNormal,position+mapping.Frame.Z*layer.Offset,
                                values(layer.Field,layer.Components,origin,weights),length*layer.Thickness/2,layer.FieldFrame);
                        }
                    }
                }
            }
            double thickness = mapping.Thickness;
            if (mapping.Kind == MeshCutSectionKind::CompositeStress) { thickness = 0; for (const auto& layer : mapping.Layers) { thickness += layer.Thickness; } }
            item.Result.Area = length*thickness*options.Units.LengthToSI*options.Units.LengthToSI;
            add(report,item,options);
        }
    }
    for (const auto& entry : options.Sections) {
        const Id e = entry.first; const auto& mapping = entry.second;
        if (!included(options,e) || (mapping.Kind != MeshCutSectionKind::BeamStress && mapping.Kind != MeshCutSectionKind::BeamResultants)) { continue; }
        if constexpr (N != 9) { throw std::invalid_argument("Beam heat flow requires an explicit flux section adapter"); }
        else {
            if (cut.Definition.FollowDeformation && options.ReferenceGeometry.IsNull()) { throw std::invalid_argument("Material beam physics requires reference crossing weights"); }
            if (cut.Definition.Shape != MeshCutShape::Plane || options.SourceGeometry.IsNull()) { throw std::invalid_argument("Beam physics requires a plane and explicit source configuration"); }
            std::vector<Id> nodes; source->ElementNodes(e,nodes);
            const auto& indexed = source->IndexedElements()[e];
            if (indexed.Type != RWCae::MeshElemType::Generic || source->GenericCells().at(indexed.Index).Type != 3 || nodes.size() != 2) { throw std::invalid_argument("Beam adapter currently requires a VTK_LINE linear segment"); }
            Graphic3d_Vec3 pa = source->Nodes()[nodes[0]], pb = source->Nodes()[nodes[1]];
            options.SourceGeometry->ModifyNode(nodes[0],pa); options.SourceGeometry->ModifyNode(nodes[1],pb);
            const Vec a(pa), b(pb); Vec n = normal(cut.Definition,e,a,options);
            if(std::abs(unit(b-a).Dot(mapping.Frame.X))<1-1e-6) { throw std::invalid_argument("Beam frame X must follow the source beam axis"); }
            const auto& plane = cut.Definition.Plane;
            Vec crossingA=a,crossingB=b;
            if (cut.Definition.FollowDeformation) {
                if (options.ReferenceGeometry->GetMeshData()->NbNodes() != source->NbNodes()) { throw std::invalid_argument("Material beam reference correspondence mismatch"); }
                auto ra=options.ReferenceGeometry->GetMeshData()->Nodes()[nodes[0]], rb=options.ReferenceGeometry->GetMeshData()->Nodes()[nodes[1]];
                options.ReferenceGeometry->ModifyNode(nodes[0],ra); options.ReferenceGeometry->ModifyNode(nodes[1],rb);
                crossingA=Vec(ra); crossingB=Vec(rb);
                const double sign = n.Dot(unit(crossingB-crossingA));
                if (std::abs(sign)<1e-12) { throw std::invalid_argument("Material beam lies in reference plane"); }
                n=unit(b-a)*(sign>0?1:-1);
            }
            const double sa = plane.x()*crossingA.x()+plane.y()*crossingA.y()+plane.z()*crossingA.z()+plane.w();
            const double sb = plane.x()*crossingB.x()+plane.y()*crossingB.y()+plane.z()*crossingB.z()+plane.w();
            if ((sa < 0 && sb < 0) || (sa > 0 && sb > 0)) { continue; }
            if (std::abs(sa-sb) <= 1e-12) { throw std::invalid_argument("Beam lies in plane; section crossing is ambiguous"); }
            const double t = sa/(sa-sb); const Vec p = a*(1-t)+b*t;
            MeshCutDistributedResult item; item.Element = e; item.Position = p;
            item.Normal = n;
            if (mapping.Kind == MeshCutSectionKind::BeamStress) {
                evaluate(item.Result,n,p,values(cut.SourceField,components,e,{{nodes[0],1-t},{nodes[1],t}}),mapping.Area,mapping.Frame);
                item.Result.Area = mapping.Area*options.Units.LengthToSI*options.Units.LengthToSI;
            } else {
                const auto v = values(cut.SourceField,mapping.BeamComponents,e,{{nodes[0],1-t},{nodes[1],t}});
                const double sign = n.Dot(mapping.Frame.X) >= 0 ? 1 : -1;
                item.Result.Force = mapping.Frame.VectorToGlobal(Vec(v[0],v[1],v[2]))*(options.Units.ForceToSI*sign);
                item.Result.Moment = mapping.Frame.VectorToGlobal(Vec(v[3],v[4],v[5]))*(options.Units.MomentToSI*sign)
                    + Vec::Cross((p-options.MomentOrigin)*options.Units.LengthToSI,item.Result.Force);
            }
            add(report,item,options);
        }
    }
    finish(report,options); return report;
}
}
namespace ViewCaeGeom {
bool MeshCutUnits::IsValid() const
{
    if (Length.empty() || Stress.empty() || Flux.empty() || Force.empty() || Moment.empty()) { return false; }
    for (double factor : {LengthToSI,StressToSI,FluxToSI,ForceToSI,MomentToSI}) { if (!std::isfinite(factor) || factor <= 0) { return false; } }
    return true;
}
MeshCutPhysicsReport MeshCutPhysics::Stress(const MeshCutResult& cut, const std::array<int,9>& components, const MeshCutPhysicsOptions& options)
{
    return integrate(cut,components,options,[&](MeshCutResultant& result,const Vec& n,const Vec& p,
        const std::array<double,9>& tensor,double weight,const MeshCutFrame& frame) {
        const Vec force = traction(tensor,n,frame)*(weight*options.Units.StressToSI*options.Units.LengthToSI*options.Units.LengthToSI);
        result.Force += force; result.Moment += Vec::Cross((p-options.MomentOrigin)*options.Units.LengthToSI,force);
    });
}
MeshCutPhysicsReport MeshCutPhysics::HeatFlux(const MeshCutResult& cut, const std::array<int,3>& components, const MeshCutPhysicsOptions& options)
{
    return integrate(cut,components,options,[&](MeshCutResultant& result,const Vec& n,const Vec&,
        const std::array<double,3>& v,double weight,const MeshCutFrame& frame) {
        result.HeatFlow += frame.VectorToGlobal(Vec(v[0],v[1],v[2])).Dot(n)*weight*options.Units.FluxToSI*options.Units.LengthToSI*options.Units.LengthToSI;
    });
}
MeshCutPhysicsReport MeshCutPhysics::Beam(const MeshCutDefinition& plane, const Handle(RWCae::MeshNVecProperty)& field,
    const std::array<int,9>& components, const MeshCutPhysicsOptions& options)
{
    if (plane.Shape != MeshCutShape::Plane || options.SourceGeometry.IsNull() || options.Sections.empty()) {
        throw std::invalid_argument("Beam report requires a plane, source configuration and section mappings");
    }
    auto selected = options;
    if (selected.Elements.empty()) {
        selected.GroupName = "beam sections";
        for (const auto& entry : selected.Sections) {
            if (entry.second.Kind != MeshCutSectionKind::BeamStress && entry.second.Kind != MeshCutSectionKind::BeamResultants) {
                throw std::invalid_argument("Beam-only report received a non-beam section mapping");
            }
            selected.Elements.insert(entry.first);
        }
    }
    MeshCutResult cut; cut.Definition = plane; cut.SourceField = field;
    cut.Roots[1] = options.SourceGeometry; cut.Portions[1] = options.SourceGeometry;
    return Stress(cut,components,selected);
}
std::vector<MeshCutPhysicsReport> MeshCutPhysics::Slices(MeshCutViewManager& manager, const std::vector<MeshCutSliceSample>& samples,
    const Handle(RWCae::MeshNVecProperty)& field, const std::array<int,9>& components, const MeshCutPhysicsOptions& options)
{
    // Independent temporary manager: failure never publishes partial generated definitions.
    const auto source = manager.SourceGeometry()->GetMeshData(); MeshCutViewManager staged(source);
    if (!staged.SetGeometryOptions(manager.GeometryOptions())) { throw std::invalid_argument("Invalid slicing geometry options"); }
    if (!staged.SetRecoveryOptions(manager.RecoveryOptions()) || !staged.SetDeformation(manager.DeformationField(),manager.DeformationScale())) { throw std::invalid_argument("Invalid slicing physical configuration"); }
    std::vector<MeshCutPhysicsReport> reports;
    for (const auto& sample : samples) {
        if (!staged.AddPlane(sample.Definition.Id,sample.Definition.Plane)) { throw std::invalid_argument("Invalid distributed slice"); }
        if (!staged.SetTraceOptions(sample.Definition.Id,true,sample.Definition.PhysicalTrace)) { throw std::invalid_argument("Invalid distributed physical trace"); }
        auto report = Stress(staged.EvaluatePhysicalSection(sample.Definition.Id,field),components,options);
        report.IntervalBegin = sample.IntervalBegin; report.IntervalEnd = sample.IntervalEnd; reports.push_back(report);
        staged.Remove(sample.Definition.Id);
    }
    return reports;
}
std::string MeshCutPhysicsReport::Csv() const
{
    auto escaped = [](const std::string& text) { std::string result = "\""; for (char c : text) { if (c == '"') { result += '"'; } result += c; } return result+'"'; };
    std::ostringstream out; out << std::setprecision(17);
    out << "cut,group,element,x_m,y_m,z_m,Fx_N,Fy_N,Fz_N,Mx_Nm,My_Nm,Mz_Nm,area_m2,heat_W,interval_begin,interval_end,interval_units\n";
    auto row = [&](const std::string& element,const Vec& p,const MeshCutResultant& result) {
        out << escaped(Cut) << ',' << escaped(Group) << ',' << element;
        for (int i = 0; i < 3; ++i) { out << ',' << p[i]; }
        for (int i = 0; i < 3; ++i) { out << ',' << result.Force[i]; }
        for (int i = 0; i < 3; ++i) { out << ',' << result.Moment[i]; }
        out << ',' << result.Area << ',' << result.HeatFlow << ',' << IntervalBegin << ',' << IntervalEnd << ',' << escaped(IntervalUnits) << '\n';
    };
    for (const auto& item : Distributed) { row(std::to_string(item.Element),item.Position,item.Result); }
    row("total",MomentOrigin,Total); return out.str();
}
}
