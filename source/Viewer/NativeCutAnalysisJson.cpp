#include <Viewer/NativeCutAnalysis.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <stdexcept>
#include <cmath>

namespace {
using V = rapidjson::Value;
using W = rapidjson::Writer<rapidjson::StringBuffer>;
const V& member(const V& v,const char* key) { if (!v.IsObject() || !v.HasMember(key)) { throw std::invalid_argument(std::string("Missing physics option: ")+key); } return v[key]; }
double number(const V& v) { if (!v.IsNumber() || !std::isfinite(v.GetDouble())) { throw std::invalid_argument("Physics option must be finite"); } return v.GetDouble(); }
int integer(const V& v) { if (!v.IsInt()) { throw std::invalid_argument("Physics option must be an integer"); } return v.GetInt(); }
std::string string(const V& v) { if (!v.IsString() || std::string(v.GetString(),v.GetStringLength()).find('\0') != std::string::npos) { throw std::invalid_argument("Invalid physics option string"); } return {v.GetString(),v.GetStringLength()}; }
bool boolean(const V& v) { if (!v.IsBool()) { throw std::invalid_argument("Physics option must be boolean"); } return v.GetBool(); }
void text(W& w,const std::string& s) { w.String(s.data(),(rapidjson::SizeType)s.size()); }
Viewer::CutFieldBinding binding(const V& v) { return {string(member(v,"name")),boolean(member(v,"nodal")),integer(member(v,"component")),integer(member(v,"step"))}; }
void binding(W& w,const Viewer::CutFieldBinding& b) { w.StartObject(); w.Key("name"); text(w,b.Name); w.Key("nodal"); w.Bool(b.Nodal); w.Key("component"); w.Int(b.Component); w.Key("step"); w.Int(b.Step); w.EndObject(); }
ViewCaeGeom::MeshCutFrame frame(const V& v) {
    if (!v.IsArray() || v.Size()!=12) { throw std::invalid_argument("Frame requires origin/X/Y/Z (12 numbers)"); }
    ViewCaeGeom::MeshCutFrame f; for (int c=0;c<3;++c) { f.Origin[c]=number(v[c]); f.X[c]=number(v[3+c]); f.Y[c]=number(v[6+c]); f.Z[c]=number(v[9+c]); } return f;
}
void frame(W& w,const ViewCaeGeom::MeshCutFrame& f) { w.StartArray(); for (const auto& v : {f.Origin,f.X,f.Y,f.Z}) { for(int c=0;c<3;++c) { w.Double(v[c]); } } w.EndArray(); }
template<size_t N> void components(const V& v,std::array<int,N>& result) { if (!v.IsArray() || v.Size()!=N) { throw std::invalid_argument("Invalid physics component count"); } for(size_t i=0;i<N;++i) { result[i]=integer(v[(rapidjson::SizeType)i]); } }
template<size_t N> void components(W& w,const std::array<int,N>& c) { w.StartArray(); for(int v:c) { w.Int(v); } w.EndArray(); }
}
namespace Viewer {
NativeCutAnalysisRequest NativeCutAnalysis::ReadRequestJson(const std::string& json) const
{
    if (json.size()>4*1024*1024) { throw std::invalid_argument("Physics request is too large"); }
    rapidjson::Document doc; doc.Parse(json.data(),json.size());
    if (doc.HasParseError() || !doc.IsObject()) { throw std::invalid_argument("Invalid physics request JSON"); }
    NativeCutAnalysisRequest r; r.Cut=string(member(doc,"cut")); r.Field=binding(member(doc,"field"));
    if(doc.HasMember("heat")) { r.Heat=boolean(doc["heat"]); }
    if(doc.HasMember("beam")) { r.Beam=boolean(doc["beam"]); }
    if(doc.HasMember("components")) { components(doc["components"],r.Components); }
    if(doc.HasMember("overlay")) { r.Overlay=boolean(doc["overlay"]); }
    if(doc.HasMember("arrowScale")) { r.ArrowScale=number(doc["arrowScale"]); }
    auto& o=r.Options;
    if(doc.HasMember("group")) { o.GroupName=string(doc["group"]); }
    if(doc.HasMember("elements")) { const auto& e=doc["elements"]; if(!e.IsArray()) { throw std::invalid_argument("Group must be an element list"); } for(const auto& v:e.GetArray()) { const int id=integer(v); if(id<0 || !o.Elements.insert(id).second) { throw std::invalid_argument("Invalid/duplicate group element"); } } }
    if(doc.HasMember("absolute")) { o.Summation=boolean(doc["absolute"]) ? ViewCaeGeom::MeshCutSummation::AbsoluteContributions : ViewCaeGeom::MeshCutSummation::Signed; }
    if(doc.HasMember("fieldFrame")) { o.FieldFrame=frame(doc["fieldFrame"]); }
    if(doc.HasMember("outputFrame")) { o.OutputFrame=frame(doc["outputFrame"]); }
    if(doc.HasMember("momentOrigin")) { const auto& p=doc["momentOrigin"]; if(!p.IsArray()||p.Size()!=3) { throw std::invalid_argument("Moment origin requires three numbers"); } for(int c=0;c<3;++c) { o.MomentOrigin[c]=number(p[c]); } }
    if(doc.HasMember("units")) { const auto& u=doc["units"]; auto read=[&](const char* key,std::string& name,double& factor) { const auto& v=member(u,key); if(!v.IsArray()||v.Size()!=2) { throw std::invalid_argument("Unit requires name and SI conversion factor"); } name=string(v[0]); factor=number(v[1]); }; read("length",o.Units.Length,o.Units.LengthToSI); read("stress",o.Units.Stress,o.Units.StressToSI); read("flux",o.Units.Flux,o.Units.FluxToSI); read("force",o.Units.Force,o.Units.ForceToSI); read("moment",o.Units.Moment,o.Units.MomentToSI); }
    if(doc.HasMember("normals")) { const auto& ns=doc["normals"]; if(!ns.IsArray()) { throw std::invalid_argument("Normals must be an array"); } for(const auto& n:ns.GetArray()) { if(!n.IsArray()||n.Size()!=4||integer(n[0])<0) { throw std::invalid_argument("Normal requires element and vector"); } if(!o.ElementNormals.emplace(integer(n[0]),Graphic3d_Vec3d(number(n[1]),number(n[2]),number(n[3]))).second) { throw std::invalid_argument("Duplicate normal"); } } }
    if(doc.HasMember("sections")) {
        const auto& ss=doc["sections"]; if(!ss.IsArray()) { throw std::invalid_argument("Sections must be an array"); }
        for(const auto& s:ss.GetArray()) {
            const int id=integer(member(s,"element")); if(id<0) { throw std::invalid_argument("Negative section element"); }
            ViewCaeGeom::MeshCutSectionMapping m; m.Kind=(ViewCaeGeom::MeshCutSectionKind)integer(member(s,"kind"));
            if(s.HasMember("frame")) { m.Frame=frame(s["frame"]); }
            if(s.HasMember("thickness")) { m.Thickness=number(s["thickness"]); }
            if(s.HasMember("area")) { m.Area=number(s["area"]); }
            if(s.HasMember("beamComponents")) { components(s["beamComponents"],m.BeamComponents); }
            if(s.HasMember("layers")) { const auto& ls=s["layers"]; if(!ls.IsArray()) { throw std::invalid_argument("Layers must be an array"); } for(const auto& l:ls.GetArray()) {
                ViewCaeGeom::MeshCutLayer layer; layer.Field=myOwner->resolveField(binding(member(l,"field"))); layer.Thickness=number(member(l,"thickness")); layer.Offset=number(member(l,"offset"));
                if(l.HasMember("frame")) { layer.FieldFrame=frame(l["frame"]); } if(l.HasMember("components")) { components(l["components"],layer.Components); } m.Layers.push_back(layer);
            } }
            if(!o.Sections.emplace(id,m).second) { throw std::invalid_argument("Duplicate section element"); }
        }
    }
    return r;
}
bool NativeCutAnalysis::SetJson(const std::string& id,const std::string& json)
{
    return myOwner->attempt([&] { const auto r=ReadRequestJson(json); if(!Set(id,r)) { throw std::invalid_argument(myOwner->LastError()); } });
}
std::string NativeCutAnalysis::RequestJson(const NativeCutAnalysisRequest& r)
{
    rapidjson::StringBuffer b; W w(b); w.StartObject(); w.Key("cut"); text(w,r.Cut); w.Key("field"); binding(w,r.Field);
    w.Key("heat"); w.Bool(r.Heat); w.Key("beam"); w.Bool(r.Beam); w.Key("components"); components(w,r.Components); w.Key("overlay"); w.Bool(r.Overlay); w.Key("arrowScale"); w.Double(r.ArrowScale);
    const auto& o=r.Options; w.Key("group"); text(w,o.GroupName); w.Key("elements"); w.StartArray(); for(auto e:o.Elements) { w.Uint(e); } w.EndArray();
    w.Key("absolute"); w.Bool(o.Summation==ViewCaeGeom::MeshCutSummation::AbsoluteContributions); w.Key("fieldFrame"); frame(w,o.FieldFrame); w.Key("outputFrame"); frame(w,o.OutputFrame);
    w.Key("momentOrigin"); w.StartArray(); for(int c=0;c<3;++c) { w.Double(o.MomentOrigin[c]); } w.EndArray(); w.Key("units"); w.StartObject();
    auto unit=[&](const char* key,const std::string& name,double factor) { w.Key(key); w.StartArray(); text(w,name); w.Double(factor); w.EndArray(); };
    unit("length",o.Units.Length,o.Units.LengthToSI); unit("stress",o.Units.Stress,o.Units.StressToSI); unit("flux",o.Units.Flux,o.Units.FluxToSI); unit("force",o.Units.Force,o.Units.ForceToSI); unit("moment",o.Units.Moment,o.Units.MomentToSI); w.EndObject();
    w.Key("normals"); w.StartArray(); for(const auto& n:o.ElementNormals) { w.StartArray(); w.Uint(n.first); for(int c=0;c<3;++c) { w.Double(n.second[c]); } w.EndArray(); } w.EndArray();
    w.Key("sections"); w.StartArray(); for(const auto& s:o.Sections) { w.StartObject(); w.Key("element"); w.Uint(s.first); w.Key("kind"); w.Int((int)s.second.Kind); w.Key("frame"); frame(w,s.second.Frame); w.Key("thickness"); w.Double(s.second.Thickness); w.Key("area"); w.Double(s.second.Area); w.Key("beamComponents"); components(w,s.second.BeamComponents);
        w.Key("layers"); w.StartArray(); for(const auto& l:s.second.Layers) { if(l.Field.IsNull()) { throw std::invalid_argument("Missing layer binding"); } w.StartObject(); w.Key("field"); binding(w,{l.Field->Name().ToCString(),l.Field->IsNodal(),0,l.Field->TimeStep()}); w.Key("components"); components(w,l.Components); w.Key("thickness"); w.Double(l.Thickness); w.Key("offset"); w.Double(l.Offset); w.Key("frame"); frame(w,l.FieldFrame); w.EndObject(); } w.EndArray(); w.EndObject();
    } w.EndArray(); w.EndObject(); return {b.GetString(),b.GetSize()};
}
}
