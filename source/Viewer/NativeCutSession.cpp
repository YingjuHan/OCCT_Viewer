#include <Viewer/NativeCutSession.h>
#include <Viewer/NativeCutDigest.h>
#include <Viewer/NativeCutAnalysis.h>
#include <RWMeshConv/MeshLoader.h>
#include <OSD_FileSystem.hxx>
#include <algorithm>
#include <RWVtk/VtkReferenceStager.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <Graphic3d_Camera.hxx>
#include <BRepTools.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <LDOMParser.hxx>
#include <sstream>
#include <BRep_Builder.hxx>
#include <TColStd_ListOfInteger.hxx>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <cstring>
#include <atomic>
#include <chrono>
#include <set>
#include <RWCae/MeshNodeVecProperty.h>
#include <cctype>
#ifdef _WIN32
#include <windows.h>
#undef near
#undef far
#endif

namespace {
namespace fs=std::filesystem;
using V=rapidjson::Value; using W=rapidjson::Writer<rapidjson::StringBuffer>;
const V& get(const V& v,const char* key) { if(!v.IsObject()||!v.HasMember(key)) { throw std::invalid_argument(std::string("Missing session member: ")+key); } return v[key]; }
std::string str(const V& v) { if(!v.IsString()||v.GetStringLength()>4096) { throw std::invalid_argument("Invalid session string"); } std::string s(v.GetString(),v.GetStringLength()); if(s.find('\0')!=std::string::npos) { throw std::invalid_argument("NUL in session string"); } return s; }
double num(const V& v) { if(!v.IsNumber()||!std::isfinite(v.GetDouble())) { throw std::invalid_argument("Nonfinite session number"); } return v.GetDouble(); }
int integer(const V& v) { if(!v.IsInt()) { throw std::invalid_argument("Invalid session integer"); } return v.GetInt(); }
uint64_t uint64(const V& v) { if(!v.IsUint64()) { throw std::invalid_argument("Invalid session unsigned integer"); } return v.GetUint64(); }
bool boolean(const V& v) { if(!v.IsBool()) { throw std::invalid_argument("Invalid session boolean"); } return v.GetBool(); }
const V& array(const V& v,size_t max=10000) { if(!v.IsArray()||v.Size()>max) { throw std::invalid_argument("Invalid/oversized session array"); } return v; }
void text(W& w,const std::string& s) { w.String(s.data(),(rapidjson::SizeType)s.size()); }
std::string json(const V& v) { rapidjson::StringBuffer b; W w(b); v.Accept(w); return {b.GetString(),b.GetSize()}; }
void raw(W& w,const std::string& s) { rapidjson::Document d; d.Parse<rapidjson::kParseFullPrecisionFlag>(s.data(),s.size()); if(d.HasParseError()) { throw std::invalid_argument("Invalid nested session JSON"); } d.Accept(w); }
void validateTree(const V& v,int depth=0)
{
    if(depth>64) { throw std::invalid_argument("Session JSON nesting exceeds 64 levels"); }
    if(v.IsObject()) {
        std::set<std::string> keys;
        for(auto i=v.MemberBegin();i!=v.MemberEnd();++i) {
            if(!keys.insert(str(i->name)).second) { throw std::invalid_argument("Duplicate session JSON member"); }
            validateTree(i->value,depth+1);
        }
    } else if(v.IsArray()) { for(const auto& item:v.GetArray()) { validateTree(item,depth+1); } }
}
Viewer::CutFieldBinding binding(const V& v) { return {str(get(v,"name")),boolean(get(v,"nodal")),integer(get(v,"component")),integer(get(v,"step"))}; }
void binding(W& w,const Viewer::CutFieldBinding& b) { w.StartObject(); w.Key("name"); text(w,b.Name); w.Key("nodal"); w.Bool(b.Nodal); w.Key("component"); w.Int(b.Component); w.Key("step"); w.Int(b.Step); w.EndObject(); }
void vec(W& w,const Graphic3d_Vec3d& v) { w.StartArray(); for(int c=0;c<3;++c) { w.Double(v[c]); } w.EndArray(); }
Graphic3d_Vec3d vec(const V& v) { if(array(v,3).Size()!=3) { throw std::invalid_argument("Expected three session coordinates"); } return Graphic3d_Vec3d(num(v[0]),num(v[1]),num(v[2])); }
void frame(W& w,const ViewCaeGeom::MeshCutFrame& f) { w.StartArray(); for(const auto& v:{f.Origin,f.X,f.Y,f.Z}) { for(int c=0;c<3;++c) { w.Double(v[c]); } } w.EndArray(); }
ViewCaeGeom::MeshCutFrame frame(const V& v) { if(array(v,12).Size()!=12) { throw std::invalid_argument("Invalid session frame"); } ViewCaeGeom::MeshCutFrame f; for(int c=0;c<3;++c) { f.Origin[c]=num(v[c]); f.X[c]=num(v[3+c]); f.Y[c]=num(v[6+c]); f.Z[c]=num(v[9+c]); } if(!f.IsValid()) { throw std::invalid_argument("Invalid session frame axes"); } return f; }
void style(W& w,const ViewCae::MeshCutDisplayStyle& s) { w.StartObject(); w.Key("color"); vec(w,Graphic3d_Vec3d(s.Color.Red(),s.Color.Green(),s.Color.Blue())); w.Key("alpha"); w.Double(s.Transparency); w.Key("width"); w.Double(s.LineWidth); w.Key("mode"); w.Int((int)s.Mode); w.EndObject(); }
ViewCae::MeshCutDisplayStyle style(const V& v) { ViewCae::MeshCutDisplayStyle s; const auto c=vec(get(v,"color")); for(int i=0;i<3;++i) { if(c[i]<0||c[i]>1) { throw std::invalid_argument("Invalid session color"); } } s.Color=Quantity_Color(c.x(),c.y(),c.z(),Quantity_TOC_RGB); s.Transparency=(float)num(get(v,"alpha")); s.LineWidth=(float)num(get(v,"width")); s.Mode=(ViewCae::InteractiveCaeMeshPrsMode)integer(get(v,"mode")); return s; }
void recovery(W& w,const ViewCaeGeom::MeshCutRecoveryOptions& o) { w.StartObject(); w.Key("threshold"); w.Double(o.ThresholdPercent); w.Key("before"); w.Bool(o.InvariantsBeforeAveraging); w.Key("measure"); w.Int((int)o.Tensor.Measure); w.Key("layout"); w.Int((int)o.Tensor.Layout); w.Key("shear"); w.Bool(o.Tensor.EngineeringShear); w.Key("regions"); w.StartArray(); for(auto r:o.Regions) { w.Int64(r); } w.EndArray(); w.EndObject(); }
ViewCaeGeom::MeshCutRecoveryOptions recovery(const V& v) { ViewCaeGeom::MeshCutRecoveryOptions o; o.ThresholdPercent=num(get(v,"threshold")); o.InvariantsBeforeAveraging=boolean(get(v,"before")); o.Tensor.Measure=(ViewCaeGeom::MeshCutTensorMeasure)integer(get(v,"measure")); o.Tensor.Layout=(ViewCaeGeom::MeshCutTensorLayout)integer(get(v,"layout")); o.Tensor.EngineeringShear=boolean(get(v,"shear")); for(const auto& r:array(get(v,"regions"),10000000).GetArray()) { if(!r.IsInt64()) { throw std::invalid_argument("Invalid region ID"); } o.Regions.push_back(r.GetInt64()); } return o; }
void trace(W& w,const ViewCaeGeom::MeshCutTraceOptions& o) { w.StartObject(); w.Key("policy"); w.Int((int)o.Policy); w.Key("region"); w.Int64(o.Region); w.Key("regions"); w.StartArray(); for(auto r:o.Regions) { w.Int64(r); } w.EndArray(); w.EndObject(); }
ViewCaeGeom::MeshCutTraceOptions trace(const V& v) { ViewCaeGeom::MeshCutTraceOptions o; o.Policy=(ViewCaeGeom::MeshCutTracePolicy)integer(get(v,"policy")); const auto& r=get(v,"region"); if(!r.IsInt64()) { throw std::invalid_argument("Invalid trace region"); } o.Region=r.GetInt64(); for(const auto& x:array(get(v,"regions"),10000000).GetArray()) { if(!x.IsInt64()) { throw std::invalid_argument("Invalid trace regions"); } o.Regions.push_back(x.GetInt64()); } return o; }
fs::path relative(const std::string& name) { auto p=fs::u8path(name); if(name.empty()||p.is_absolute()||p.has_root_name()) { throw std::invalid_argument("Session path must be relative"); } for(const auto& part:p) { if(part==".."||part==".") { throw std::invalid_argument("Session path traversal"); } } return p; }
fs::path contained(const fs::path& base,const std::string& name) { const auto root=fs::canonical(base),p=fs::canonical(root/relative(name)); auto a=root.begin(),b=p.begin(); for(;a!=root.end();++a,++b) { if(b==p.end()||*a!=*b) { throw std::invalid_argument("Session source escapes its bundle"); } } return p; }
void validateReferences(const fs::path& bundle,const std::set<std::string>& declared,const RWVtk::VtkResourceLimits& limits)
{
    std::set<fs::path> paths; for(const auto& name:declared) { paths.insert(contained(bundle,name)); }
    for(const auto& name:declared) {
        const auto file=contained(bundle,name); auto extension=file.extension().generic_u8string();
        std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c) { return (char)std::tolower(c); });
        if(extension!=".vtm" && extension!=".series") { continue; }
        std::ifstream in(file,std::ios::binary); std::string body((std::istreambuf_iterator<char>(in)),{});
        auto reference=[&](const std::string& value) { if(value.empty()||value.find(char(0))!=std::string::npos) { throw std::invalid_argument("Invalid manifest reference"); } const auto p=fs::u8path(value); if(p.is_absolute()||p.has_root_name()||!paths.count(fs::canonical(file.parent_path()/p))) { throw std::invalid_argument("Undigested/escaping session manifest reference"); } };
        if(extension==".series") { rapidjson::Document d; d.Parse(body.data(),body.size()); if(d.HasParseError()) { throw std::invalid_argument("Invalid session source manifest"); } for(const auto& f:array(get(d,"files"),limits.MaxFiles).GetArray()) { reference(str(get(f,"name"))); } }
        else { LDOMParser parser; std::istringstream input(body); if(parser.parse(input)) { throw std::invalid_argument("Invalid session composite manifest"); }
            std::function<void(LDOM_Element,uint64_t)> scan=[&](LDOM_Element parent,uint64_t depth) { if(depth>limits.MaxDepth) { throw std::length_error("Session manifest depth limit"); } for(LDOM_Node n=parent.getFirstChild();!n.isNull();n=n.getNextSibling()) { if(n.getNodeType()!=LDOM_Node::ELEMENT_NODE) { continue; } const auto e=(const LDOM_Element&)n; if(TCollection_AsciiString(e.getTagName())=="DataSet") { TCollection_AsciiString value=e.getAttribute("file"); if(value.IsEmpty()) { value=e.getAttribute("File"); } reference(value.ToCString()); } else { scan(e,depth+1); } } };
            scan(parser.getDocument().getDocumentElement(),0);
        }
    }
}
std::string meshDigest(const Handle(RWCae::MeshData)& mesh)
{
    Viewer::NativeCutDigest d; auto bytes=[&](uint64_t v) { unsigned char b[8]; for(int c=0;c<8;++c) { b[c]=(unsigned char)(v>>(8*c)); } d.Update(b,8); };
    auto number=[&](double v) { uint64_t bits; if(std::isnan(v)) { bits=0x7ff8000000000000ULL; } else { std::memcpy(&bits,&v,8); } bytes(bits); };
    auto string=[&](const TCollection_AsciiString& s) { bytes(s.Length()); d.Update(s.ToCString(),s.Length()); };
    bytes(mesh->NbNodes()); for(const auto& p:mesh->Nodes()) { for(int c=0;c<3;++c) { number(p[c]); } }
    bytes(mesh->NbIndexedElems()); std::vector<RWCae::MeshEntityId> nodes;
    for(RWCae::MeshEntityId e=0;e<mesh->NbIndexedElems();++e) { const auto& entry=mesh->IndexedElements()[e]; bytes((int)entry.Type); mesh->ElementNodes(e,nodes); bytes(nodes.size()); for(auto n:nodes) { bytes(n); } if(entry.Type==RWCae::MeshElemType::Generic) { const auto& c=mesh->GenericCells().at(entry.Index); bytes(c.Type); bytes(c.Faces.size()); for(const auto& face:c.Faces) { bytes(face.size()); for(auto n:face) { bytes(n); } } bytes(3); for(auto n:c.Degrees) { bytes(n); } bytes(c.RationalWeights.size()); for(auto n:c.RationalWeights) { number(n); } } }
    bytes(mesh->Properties().Size()); for(const auto& f:mesh->Properties()) { string(f->Name()); string(f->Description()); string(f->TimeStepDescription()); string(f->TimeDisplayUnits()); bytes(f->IsNodal()); bytes(f->TimeStep()); number(f->Time()); bytes((int)f->Type()); bytes(f->NbRawComponents()); bytes(f->NbValues());
        for(int c=0;c<f->NbRawComponents();++c) { const auto v=f->GetRawComponent(c); bytes((int)v->BaseType()); bytes((int)v->ElemType()); string(v->Description()); string(v->DisplayUnits()); bytes(v->IsEnabled()); for(int n=0;n<v->NbValues();++n) { if(f->HasNodeVecComponents()) { std::vector<int> subs; Handle(RWCae::MeshNodeVecProperty)::DownCast(v)->SubIndices(n,subs); std::sort(subs.begin(),subs.end()); bytes(subs.size()); for(int sub:subs) { bytes(sub); number(v->Value(RWCae::MeshPropertyEntityInfo(n,sub))); } } else { number(v->Value(RWCae::MeshPropertyEntityInfo(n))); } } }
    } return d.Finish();
}
std::string labelText(const TCollection_ExtendedString& s) { std::vector<char> bytes((size_t)s.LengthOfCString()+1); char* p=bytes.data(); s.ToUTF8CString(p); return bytes.data(); }
std::string cadDigest(const TopoDS_Shape& shape)
{
    // Transient algorithm flags and triangulations do not define CAD geometry.
    auto copy=BRepBuilderAPI_Copy(shape,true,false).Shape(); TopTools_IndexedMapOfShape entries; TopExp::MapShapes(copy,entries);
    for(int i=1;i<=entries.Extent();++i) { auto part=entries(i); part.Free(false); part.Modified(false); part.Checked(false); }
    std::ostringstream body; BRepTools::Write(copy,body,false,false,TopTools_FormatVersion_VERSION_3); return Viewer::NativeCutDigest::Text(body.str());
}
void atomicWrite(const fs::path& file,const std::string& text)
{
    static std::atomic<uint64_t> count{0}; const auto temp=file.parent_path()/(file.filename().generic_u8string()+".tmp-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(count++));
    try { std::ofstream out(temp,std::ios::binary); out.write(text.data(),text.size()); out.close(); if(!out) { throw std::runtime_error("Session write failed"); }
#ifdef _WIN32
        if(!MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) { throw std::runtime_error("Atomic session replacement failed"); }
#else
        fs::rename(temp,file);
#endif
    } catch(...) { std::error_code ignored; fs::remove(temp,ignored); throw; }
}
}

namespace Viewer {
bool NativeCutSession::Save(const std::string& filename,const std::string& meshSource,const std::string& cadSource)
{
    return myOwner.attempt([&] {
        const auto file=fs::absolute(fs::u8path(filename)).lexically_normal(); fs::create_directories(file.parent_path()); const auto assets=file.parent_path()/(file.filename().generic_u8string()+".assets");
        RWVtk::VtkReferenceStager stager; if(!stager.Perform(meshSource.c_str(),assets.generic_u8string().c_str(),myLimits)) { throw std::runtime_error(stager.Report().errors.empty()?"Session source staging failed":stager.Report().errors.front()); }
        const auto source=fs::u8path(stager.StagedSource().ToCString()),bundle=source.parent_path().parent_path();
        try {
            RWMeshConv::MeshLoader loader; loader.SetVtkResourceLimits(myLimits); const auto imported=loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),source.generic_u8string().c_str());
            if(imported.IsNull()) { throw std::invalid_argument("Frozen session source cannot be imported"); }
            const int step=myTimeline.Step(); const auto snapshot=imported->SeriesSteps().empty()?imported:imported->SeriesSteps().at(step).Mesh;
            if(imported->SeriesSteps().size()!=myTimeline.Source()->SeriesSteps().size()) { throw std::invalid_argument("Frozen source series differs from the live source"); }
            for(size_t i=0;i<imported->SeriesSteps().size();++i) { if(imported->SeriesSteps()[i].Time!=myTimeline.Source()->SeriesSteps()[i].Time || meshDigest(imported->SeriesSteps()[i].Mesh)!=meshDigest(myTimeline.Source()->SeriesSteps()[i].Mesh)) { throw std::invalid_argument("Frozen source series snapshot differs from the live source"); } }
            const auto live=myOwner.Display()->SourceMesh();
            const auto expected=step==0&&live==myTimeline.Source()?imported:snapshot;
            const auto signature=meshDigest(snapshot); if(meshDigest(expected)!=meshDigest(live)) { throw std::invalid_argument("Live source differs from supplied immutable source files"); }
            std::string cad;
            if(!myOwner.myCadSource.IsNull()) {
                if(cadSource.empty()) { throw std::invalid_argument("CAD section session requires an explicit CAD source file"); }
                TopoDS_Shape supplied; BRep_Builder builder;
                if(!BRepTools::Read(supplied,cadSource.c_str(),builder)) { throw std::invalid_argument("Cannot read supplied CAD source"); }
                if(cadDigest(myOwner.myCadSource)!=cadDigest(supplied)) { throw std::invalid_argument("Live CAD differs from supplied immutable CAD source"); }
                fs::create_directory(bundle/"cad"); fs::copy_file(fs::u8path(cadSource),bundle/"cad/source.brep"); cad="cad/source.brep";
            }
            rapidjson::StringBuffer buffer; W w(buffer); w.StartObject(); w.Key("format"); w.String("occt-cut-native"); w.Key("version"); w.Int(3);
            w.Key("source"); w.StartObject(); w.Key("bundle"); text(w,fs::relative(bundle,file.parent_path()).generic_u8string()); w.Key("path"); text(w,fs::relative(source,bundle).generic_u8string()); w.Key("meshDigest"); text(w,signature);
            w.Key("files"); w.StartArray(); uint64_t total=0,count=0;
            for(const auto& item:fs::recursive_directory_iterator(bundle)) { if(!item.is_regular_file()) { continue; } const auto size=item.file_size(); RWVtk::VtkResourceBudget::Consume(total,size,myLimits.MaxInputBytes,"session source bytes"); RWVtk::VtkResourceBudget::Consume(count,1,myLimits.MaxFiles,"session source files"); w.StartObject(); w.Key("path"); text(w,fs::relative(item.path(),bundle).generic_u8string()); w.Key("bytes"); w.Uint64(size); w.Key("sha256"); text(w,NativeCutDigest::File(item.path().generic_u8string(),myLimits.MaxFileBytes)); w.EndObject(); } w.EndArray(); w.EndObject();
            auto& cuts=myOwner.Display()->Cuts(); w.Key("definitions"); raw(w,cuts.DumpDefinitionsJson()); w.Key("field"); binding(w,myOwner.FieldBinding()); w.Key("deformation"); binding(w,myTimeline.DeformationBinding().Name.empty()&&!cuts.DeformationField().IsNull()?CutFieldBinding{cuts.DeformationField()->Name().ToCString(),true,0,step}:myTimeline.DeformationBinding()); w.Key("deformationScale"); w.Double(cuts.DeformationScale()); w.Key("step"); w.Int(step);
            const auto& g=cuts.GeometryOptions(); w.Key("geometry"); w.StartObject(); w.Key("mode"); w.Int((int)g.Mode); w.Key("absolute"); w.Double(g.AbsoluteTolerance); w.Key("relative"); w.Double(g.RelativeTolerance); w.Key("face"); w.Int(g.FaceSubdivisionLimit); w.Key("line"); w.Int(g.LineSubdivisionLimit); w.Key("primitives"); w.Uint64(g.MaxSourcePrimitives); w.EndObject(); w.Key("cache"); w.StartArray(); w.Uint64(cuts.CachePolicy().SourceBytes); w.Uint64(cuts.CachePolicy().ResultBytes); w.EndArray();
            w.Key("styles"); w.StartArray(); for(int p=0;p<3;++p) { style(w,myOwner.Display()->Style((ViewCaeGeom::MeshCutPortion)p)); } w.EndArray(); w.Key("overrides"); w.StartArray(); for(const auto& s:myOwner.Display()->StyleOverrides()) { w.StartObject(); w.Key("cut"); text(w,s.first.first); w.Key("portion"); w.Int(s.first.second); w.Key("style"); style(w,s.second); w.EndObject(); } w.EndArray();
            const auto& c=myOwner.Display()->ContourOptions(); w.Key("contour"); w.StartObject(); w.Key("auto"); w.Bool(c.AutomaticRange); w.Key("min"); w.Double(c.Minimum); w.Key("max"); w.Double(c.Maximum); w.Key("legend"); w.Bool(c.ShowLegend); w.Key("intervals"); w.Int(c.Intervals); w.EndObject();
            w.Key("vectors"); w.StartObject(); const auto v=myOwner.Display()->VectorField(); w.Key("field"); binding(w,v.IsNull()?CutFieldBinding():CutFieldBinding{v->Name().ToCString(),v->IsNodal(),0,v->TimeStep()}); w.Key("scale"); w.Double(myOwner.Display()->VectorScale()); w.Key("sampling"); w.Double(myOwner.Display()->VectorSampling()); w.Key("orientation"); w.Bool(myOwner.Display()->VectorOrientation()); w.EndObject();
            w.Key("datums"); w.StartArray(); for(const auto& d:myOwner.Datums()) { w.StartObject(); w.Key("id"); text(w,d.first); w.Key("frame"); frame(w,d.second); w.EndObject(); } w.EndArray(); w.Key("axes"); w.StartObject(); w.Key("datum"); text(w,myOwner.ReferenceDatum()); w.Key("show"); w.Bool(myOwner.ReferenceAxesVisible()); w.Key("length"); w.Double(myOwner.ReferenceAxesLength()); w.EndObject();
            w.Key("materialFrames"); w.StartArray(); for(const auto& f:myTimeline.myMaterialFrames) { w.StartObject(); w.Key("id"); text(w,f.first); w.Key("configuration"); w.Int((int)f.second.Configuration); w.Key("anchors"); w.StartArray(); for(const auto& a:f.second.Anchors) { w.StartArray(); w.Int(a.first); for(auto n:a.second) { w.Uint(n); } w.EndArray(); } w.EndArray(); w.EndObject(); } w.EndArray(); w.Key("following"); w.StartArray(); for(const auto& f:myTimeline.myCutFrames) { w.StartArray(); text(w,f.first); text(w,f.second); w.EndArray(); } w.EndArray();
            w.Key("recoverySteps"); w.StartArray(); for(const auto& r:myTimeline.myRecoverySteps) { w.StartObject(); w.Key("step"); w.Int(r.first); w.Key("options"); recovery(w,r.second); w.EndObject(); } w.EndArray(); w.Key("traceSteps"); w.StartArray(); for(const auto& s:myTimeline.myTraceSteps) { for(const auto& t:s.second) { w.StartObject(); w.Key("step"); w.Int(s.first); w.Key("cut"); text(w,t.first); w.Key("display"); trace(w,t.second.first); w.Key("physical"); trace(w,t.second.second); w.EndObject(); } } w.EndArray();
            w.Key("playback"); w.StartObject(); w.Key("playing"); w.Bool(myTimeline.myPlaying); w.Key("loop"); w.Bool(myTimeline.myLoop); w.Key("timestamps"); w.Bool(myTimeline.myUseTimes); w.Key("period"); w.Double(myTimeline.myPeriod); w.Key("elapsed"); w.Double(myTimeline.myElapsed); w.EndObject();
            w.Key("analysis"); w.StartArray(); if(myOwner.myAnalysis) { for(const auto& r:myOwner.myAnalysis->Requests()) { w.StartObject(); w.Key("id"); text(w,r.first); w.Key("request"); raw(w,NativeCutAnalysis::RequestJson(r.second)); w.EndObject(); } } w.EndArray(); w.Key("analysisSteps"); w.StartArray(); if(myOwner.myAnalysis) { for(const auto& s:myOwner.myAnalysis->StepRequests()) { for(const auto& r:s.second) { w.StartObject(); w.Key("step"); w.Int(s.first); w.Key("id"); text(w,r.first); w.Key("request"); raw(w,NativeCutAnalysis::RequestJson(r.second)); w.EndObject(); } } } w.EndArray();
            w.Key("automatic"); w.StartArray(); if(myOwner.myAnalysis) { for(const auto& a:myOwner.myAnalysis->AutomaticBindings()) { w.StartObject(); w.Key("id"); text(w,a.first); w.Key("field"); binding(w,a.second.Field); const auto& m=a.second.Mapping; w.Key("kind"); w.Int((int)m.Kind); w.Key("threshold"); w.Double(m.Threshold); w.Key("sign"); w.Int(m.Sign); w.Key("min"); w.Double(m.Minimum); w.Key("max"); w.Double(m.Maximum); w.Key("mask"); w.Uint(m.Visibility); w.EndObject(); } } w.EndArray();
            w.Key("annotations"); w.StartArray(); for(const auto& a:myOwner.Annotations()) { w.StartObject(); w.Key("id"); text(w,a.first); w.Key("text"); text(w,labelText(a.second->Text())); const auto p=a.second->Position(); w.Key("position"); vec(w,Graphic3d_Vec3d(p.X(),p.Y(),p.Z())); w.EndObject(); } w.EndArray();
            w.Key("cad"); if(cad.empty()) { w.Null(); } else { w.StartObject(); w.Key("file"); text(w,cad); w.Key("cut"); text(w,myOwner.myCadCut); w.Key("sync"); w.Bool(myOwner.mySyncClip); w.EndObject(); }
            w.Key("clip"); if(myOwner.GlobalClip().IsNull()) { w.Null(); } else { w.StartObject(); w.Key("plane"); w.StartArray(); for(int p=0;p<4;++p) { w.Double(myOwner.GlobalClip()->GetEquation()[p]); } w.EndArray(); w.Key("on"); w.Bool(myOwner.GlobalClip()->IsOn()); w.EndObject(); }
            w.Key("camera"); if(myOwner.myViewport.IsNull()) { w.Null(); } else { const auto cam=myOwner.myViewport->Camera(); w.StartObject(); w.Key("eye"); vec(w,Graphic3d_Vec3d(cam->Eye().X(),cam->Eye().Y(),cam->Eye().Z())); w.Key("center"); vec(w,Graphic3d_Vec3d(cam->Center().X(),cam->Center().Y(),cam->Center().Z())); w.Key("up"); vec(w,Graphic3d_Vec3d(cam->Up().X(),cam->Up().Y(),cam->Up().Z())); w.Key("projection"); w.Int((int)cam->ProjectionType()); w.Key("scale"); w.Double(cam->Scale()); w.Key("aspect"); w.Double(cam->Aspect()); w.Key("fovy"); w.Double(cam->FOVy()); w.Key("near"); w.Double(cam->ZNear()); w.Key("far"); w.Double(cam->ZFar()); w.EndObject(); }
            w.EndObject(); if(buffer.GetSize()>32*1024*1024) { throw std::length_error("Native session JSON exceeds 32 MiB"); }
            auto savedSource=source.generic_u8string(),savedCad=cad.empty()?std::string():(bundle/cad).generic_u8string();
            const std::string serialized(buffer.GetString(),buffer.GetSize());
            const auto preview=file.parent_path()/(file.filename().generic_u8string()+".validate-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            try {
                atomicWrite(preview,serialized);
                NativeCutView check(snapshot); NativeCutTimeline checkTime(check); NativeCutSession checkSession(check,checkTime); checkSession.SetLimits(myLimits);
                if(!check.Display()->Cuts().ShareCacheFrom(cuts)) { throw std::invalid_argument("Cannot share session save validation accounting"); }
                if(!checkSession.Load(preview.generic_u8string())) { throw std::invalid_argument("Session save validation failed: "+check.LastError()); }
                fs::remove(preview);
            } catch(...) { std::error_code ignored; fs::remove(preview,ignored); throw; }
            atomicWrite(file,serialized); mySource=std::move(savedSource); myCadSource=std::move(savedCad);
        } catch(...) { if(bundle.parent_path()==assets && bundle.filename().generic_u8string().find("vtk-import-")==0) { std::error_code ignored; fs::remove_all(bundle,ignored); } throw; }
    });
}

bool NativeCutSession::Load(const std::string& filename)
{
    return myOwner.attempt([&] {
        const auto file=fs::absolute(fs::u8path(filename)).lexically_normal(); if(fs::file_size(file)>32*1024*1024) { throw std::length_error("Native session JSON exceeds 32 MiB"); }
        std::ifstream input(file,std::ios::binary); std::string body((std::istreambuf_iterator<char>(input)),{}); if(!input.eof()&&input.fail()) { throw std::runtime_error("Cannot read native session"); }
        rapidjson::Document doc; doc.Parse<rapidjson::kParseIterativeFlag|rapidjson::kParseFullPrecisionFlag>(body.data(),body.size()); if(doc.HasParseError()||!doc.IsObject()||str(get(doc,"format"))!="occt-cut-native") { throw std::invalid_argument("Invalid native session format"); }
        validateTree(doc);
        const int version=integer(get(doc,"version")); if(version<1||version>3) { throw std::invalid_argument("Unsupported native session version"); }
        if(version==3) { for(const auto key:{"deformation","deformationScale","geometry","cache","styles","overrides","contour","vectors","datums","axes","materialFrames","following","recoverySteps","traceSteps","playback","analysis","analysisSteps","automatic","annotations","cad","clip","camera"}) { get(doc,key); } }
        const auto& src=get(doc,"source"); const auto bundle=contained(file.parent_path(),str(get(src,"bundle"))),source=contained(bundle,str(get(src,"path")));
        uint64_t total=0,count=0; std::set<std::string> declared;
        for(const auto& f:array(get(src,"files"),myLimits.MaxFiles).GetArray()) { const auto name=str(get(f,"path")); if(!declared.insert(name).second) { throw std::invalid_argument("Duplicate session source file"); } const auto p=contained(bundle,name); const auto bytes=uint64(get(f,"bytes")); RWVtk::VtkResourceBudget::Consume(total,bytes,myLimits.MaxInputBytes,"session bytes"); RWVtk::VtkResourceBudget::Consume(count,1,myLimits.MaxFiles,"session files"); if(bytes!=fs::file_size(p)||NativeCutDigest::File(p.generic_u8string(),myLimits.MaxFileBytes)!=str(get(f,"sha256"))) { throw std::invalid_argument("Session source size/SHA-256 mismatch: "+name); } }
        validateReferences(bundle,declared,myLimits);
        if(!declared.count(str(get(src,"path")))) { throw std::invalid_argument("Session root has no source digest"); }
        RWMeshConv::MeshLoader loader; loader.SetVtkResourceLimits(myLimits); const auto mesh=loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(),source.generic_u8string().c_str()); if(mesh.IsNull()) { throw std::invalid_argument("Session source import failed"); }
        const int step=integer(get(doc,"step")); if((mesh->SeriesSteps().empty()&&step!=-1)||(!mesh->SeriesSteps().empty()&&(step<0||(size_t)step>=mesh->SeriesSteps().size()))) { throw std::invalid_argument("Session step is outside source series"); }
        const auto snapshot=mesh->SeriesSteps().empty()?mesh:mesh->SeriesSteps()[step].Mesh; if(meshDigest(snapshot)!=str(get(src,"meshDigest"))) { throw std::invalid_argument("Session source schema/content mismatch"); }
        NativeCutView staged(snapshot); NativeCutTimeline timeline(staged); timeline.mySource=mesh; timeline.myStep=step; auto display=staged.Display(); auto& cuts=display->Cuts();
        if(!cuts.ShareCacheFrom(myOwner.Display()->Cuts())) { throw std::runtime_error("Cannot share session cache accounting"); }
        if(doc.HasMember("cache")) { const auto& c=array(doc["cache"],2); if(c.Size()!=2||!cuts.SetCachePolicy({uint64(c[0]),uint64(c[1])})) { throw std::invalid_argument("Invalid session cache limits"); } }
        if(doc.HasMember("geometry")) { const auto& v=doc["geometry"]; auto g=cuts.GeometryOptions(); g.Mode=(ViewCaeGeom::MeshCutGeometryMode)integer(get(v,"mode")); g.AbsoluteTolerance=num(get(v,"absolute")); g.RelativeTolerance=num(get(v,"relative")); g.FaceSubdivisionLimit=integer(get(v,"face")); g.LineSubdivisionLimit=integer(get(v,"line")); g.MaxSourcePrimitives=uint64(get(v,"primitives")); if(!cuts.SetGeometryOptions(g)) { throw std::invalid_argument("Invalid session geometry options"); } }
        const auto deform=doc.HasMember("deformation")?binding(doc["deformation"]):CutFieldBinding(); const double scale=doc.HasMember("deformationScale")?num(doc["deformationScale"]):0; if(!cuts.SetDeformation(staged.resolveField(deform),scale)) { throw std::invalid_argument("Invalid session deformation binding"); } timeline.myDeformation=deform; timeline.myScale=scale;
        if(!cuts.RestoreDefinitionsJson(json(get(doc,"definitions")))) { throw std::invalid_argument("Invalid/missing session definitions or field bindings"); }
        staged.myFieldBinding=binding(get(doc,"field")); display->SetField(staged.resolveField(staged.myFieldBinding),staged.myFieldBinding.Component);
        if(doc.HasMember("styles")) { const auto& s=array(doc["styles"],3); if(s.Size()!=3) { throw std::invalid_argument("Invalid common session styles"); } for(int p=0;p<3;++p) { display->SetStyle((ViewCaeGeom::MeshCutPortion)p,style(s[p])); } }
        if(doc.HasMember("overrides")) {
            std::set<std::pair<std::string,int>> seen;
            for(const auto& s:array(doc["overrides"]).GetArray()) {
                const auto id=str(get(s,"cut")); const int portion=integer(get(s,"portion"));
                if(!seen.emplace(id,portion).second) { throw std::invalid_argument("Duplicate cut style override"); }
                display->SetStyle(id,(ViewCaeGeom::MeshCutPortion)portion,style(get(s,"style")));
            }
        }
        if(doc.HasMember("contour")) { const auto& v=doc["contour"]; ViewCae::MeshCutContourOptions c; c.AutomaticRange=boolean(get(v,"auto")); c.Minimum=num(get(v,"min")); c.Maximum=num(get(v,"max")); c.ShowLegend=boolean(get(v,"legend")); c.Intervals=integer(get(v,"intervals")); display->SetContourOptions(c); }
        if(doc.HasMember("vectors")) { const auto& v=doc["vectors"]; display->SetVectorField(staged.resolveField(binding(get(v,"field"))),num(get(v,"scale")),num(get(v,"sampling")),boolean(get(v,"orientation"))); }
        if(doc.HasMember("datums")) { for(const auto& d:array(doc["datums"]).GetArray()) { const auto id=str(get(d,"id")); if(staged.myDatums.count(id)||!staged.SetDatum(id,frame(get(d,"frame")))) { throw std::invalid_argument("Invalid/duplicate session datum"); } } }
        if(doc.HasMember("axes")) { const auto& v=doc["axes"]; if(!staged.SetReferenceAxes(str(get(v,"datum")),boolean(get(v,"show")),num(get(v,"length")))) { throw std::invalid_argument(staged.LastError()); } }
        if(doc.HasMember("materialFrames")) { for(const auto& f:array(doc["materialFrames"]).GetArray()) { CutMaterialFrame b; const auto id=str(get(f,"id")); const int configuration=integer(get(f,"configuration")); if(configuration<0||configuration>1||timeline.myMaterialFrames.count(id)) { throw std::invalid_argument("Invalid material frame configuration/ID"); } b.Configuration=(CutFrameConfiguration)configuration; for(const auto& a:array(get(f,"anchors")).GetArray()) { if(array(a,4).Size()!=4) { throw std::invalid_argument("Invalid material anchors"); } const int target=integer(a[0]); if((configuration==0 && target!=-1) || (configuration==1 && (mesh->SeriesSteps().empty()?target!=-1:(target<0 || (size_t)target>=mesh->SeriesSteps().size())))) { throw std::invalid_argument("Invalid material anchor step"); } std::array<RWCae::MeshEntityId,3> ns; for(int n=0;n<3;++n) { if(!a[1+n].IsUint()) { throw std::invalid_argument("Invalid anchor node"); } ns[n]=a[1+n].GetUint(); const auto targetMesh=configuration==0?(mesh->SeriesSteps().empty()?mesh:mesh->SeriesSteps().front().Mesh):(mesh->SeriesSteps().empty()?mesh:mesh->SeriesSteps()[target].Mesh); if(ns[n]>=targetMesh->NbNodes()) { throw std::invalid_argument("Material anchor node outside target snapshot"); } } if(!b.Anchors.emplace(target,ns).second) { throw std::invalid_argument("Duplicate material step anchors"); } } timeline.materialFrame(b,step,snapshot,staged.resolveField(deform),cuts.EffectiveDeformationScale()); if(!staged.myDatums.count(id)) { throw std::invalid_argument("Material frame lacks a datum"); } timeline.myMaterialFrames.emplace(id,b); } }
        if(doc.HasMember("following")) { for(const auto& f:array(doc["following"]).GetArray()) { if(array(f,2).Size()!=2||timeline.myCutFrames.count(str(f[0]))||!timeline.BindCutFrame(str(f[0]),str(f[1]))) { throw std::invalid_argument("Invalid following cut binding"); } } }
        if(doc.HasMember("recoverySteps")) { for(const auto& r:array(doc["recoverySteps"]).GetArray()) { const int target=integer(get(r,"step")); if(target<0||(size_t)target>=mesh->SeriesSteps().size()) { throw std::invalid_argument("Invalid remesh recovery step"); } const auto o=recovery(get(r,"options")); ViewCaeGeom::MeshCutViewManager check(mesh->SeriesSteps()[target].Mesh); if(!check.SetRecoveryOptions(o)||!timeline.myRecoverySteps.emplace(target,o).second) { throw std::invalid_argument("Invalid/duplicate remesh recovery mapping"); } } }
        if(doc.HasMember("traceSteps")) { for(const auto& t:array(doc["traceSteps"]).GetArray()) { const int target=integer(get(t,"step")); if(target<0||(size_t)target>=mesh->SeriesSteps().size()) { throw std::invalid_argument("Invalid remesh trace step"); } const auto cutId=str(get(t,"cut")); const auto definitions=cuts.Definitions(); if(std::none_of(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id==cutId; })) { throw std::invalid_argument("Unknown remesh trace cut"); } const auto a=trace(get(t,"display")),b=trace(get(t,"physical")); if(!a.IsValid(mesh->SeriesSteps()[target].Mesh->NbIndexedElems())||!b.IsValid(mesh->SeriesSteps()[target].Mesh->NbIndexedElems())||!timeline.myTraceSteps[target].emplace(str(get(t,"cut")),std::make_pair(a,b)).second) { throw std::invalid_argument("Invalid/duplicate remesh trace mapping"); } } }
        if(doc.HasMember("analysis")) { for(const auto& r:array(doc["analysis"]).GetArray()) { const auto id=str(get(r,"id")); if(staged.Analysis().Requests().count(id)||!staged.Analysis().SetJson(id,json(get(r,"request")))) { throw std::invalid_argument("Invalid/duplicate physical request: "+staged.LastError()); } } }
        if(doc.HasMember("analysisSteps")) { for(const auto& s:array(doc["analysisSteps"]).GetArray()) {
            const int target=integer(get(s,"step")); const auto id=str(get(s,"id"));
            if(target<0||(size_t)target>=mesh->SeriesSteps().size()||!staged.Analysis().Requests().count(id)
                ||staged.Analysis().mySteps[target].count(id)) { throw std::invalid_argument("Invalid/duplicate physical mapping step/ID"); }
            NativeCutView targetView(mesh->SeriesSteps()[target].Mesh); auto& targetCuts=targetView.Display()->Cuts();
            if(!targetCuts.ShareCacheFrom(cuts)||!targetCuts.SetCachePolicy(cuts.CachePolicy())) { throw std::invalid_argument("Cannot share mapped analysis cache accounting"); }
            const auto request=targetView.Analysis().ReadRequestJson(json(get(s,"request")));
            auto definitions=cuts.Definitions(); const auto found=std::find_if(definitions.begin(),definitions.end(),[&](const auto& d) { return d.Id==request.Cut; });
            if(found==definitions.end()) { throw std::invalid_argument("Unknown mapped physical cut"); }
            bool added=false;
            switch(found->Shape) {
            case ViewCaeGeom::MeshCutShape::Plane: added=targetCuts.AddPlane(found->Id,found->Plane); break;
            case ViewCaeGeom::MeshCutShape::Sphere: added=targetCuts.AddSphere(found->Id,found->Center,found->Radius); break;
            case ViewCaeGeom::MeshCutShape::Cylinder: added=targetCuts.AddCylinder(found->Id,found->Center,found->Axis,found->Radius); break;
            case ViewCaeGeom::MeshCutShape::Isosurface: added=targetCuts.AddIsosurface(found->Id,targetView.resolveField({found->IsoFieldName,found->IsoFieldNodal,found->IsoComponent,target}),found->IsoComponent,found->IsoValue); break;
            }
            const auto field=targetView.resolveField(request.Field);
            if(request.Field.Step!=target) { throw std::invalid_argument("Mapped physical field step disagrees with target"); }
            if(!request.Beam) {
                const int used=request.Heat?3:9; for(int c=0;c<used;++c) { if(request.Components[c]<0||request.Components[c]>=field->NbComponents()) { throw std::invalid_argument("Invalid mapped physical component"); } }
            } else {
                for(const auto& section:request.Options.Sections) {
                    if(section.second.Kind==ViewCaeGeom::MeshCutSectionKind::BeamResultants) { for(int c:section.second.BeamComponents) { if(c<0||c>=field->NbComponents()) { throw std::invalid_argument("Invalid mapped beam resultant component"); } } }
                    else if(section.second.Kind==ViewCaeGeom::MeshCutSectionKind::BeamStress) { for(int c:request.Components) { if(c<0||c>=field->NbComponents()) { throw std::invalid_argument("Invalid mapped beam stress component"); } } }
                }
            }
            if(!added||!targetCuts.SetVisibility(found->Id,2)||!targetView.Analysis().Set(id,request)) { throw std::invalid_argument("Invalid mapped physical request: "+targetView.LastError()); }
            staged.Analysis().SetForStep(target,id,request);
        } }
        if(doc.HasMember("automatic")) { for(const auto& a:array(doc["automatic"]).GetArray()) { const auto id=str(get(a,"id")); const auto b=binding(get(a,"field")); ViewCaeGeom::MeshCutAutomaticMapping m; m.Kind=(ViewCaeGeom::MeshCutAutomaticKind)integer(get(a,"kind")); m.Field=staged.resolveField(b); m.Component=b.Component; m.Threshold=num(get(a,"threshold")); m.Sign=integer(get(a,"sign")); m.Minimum=num(get(a,"min")); m.Maximum=num(get(a,"max")); const int mask=integer(get(a,"mask")); if(mask<0||mask>7) { throw std::invalid_argument("Invalid automatic mask"); } m.Visibility=mask; ViewCaeGeom::MeshCutViewManager check(snapshot); ViewCaeGeom::MeshCutAutomatic::Add(check,id,m); const auto ds=cuts.Definitions(); const auto found=std::find_if(ds.begin(),ds.end(),[&](const auto& d) { return d.Id==id; }); if(found==ds.end()||found->Shape!=ViewCaeGeom::MeshCutShape::Isosurface||found->IsoFieldName!=b.Name||found->IsoFieldNodal!=b.Nodal||found->IsoTimeStep!=b.Step||found->IsoComponent!=b.Component||found->IsoValue!=m.Threshold||found->Visibility!=check.Definitions().front().Visibility||staged.Analysis().myAutomatic.count(id)) { throw std::invalid_argument("Automatic mapping and definition disagree"); } staged.Analysis().myAutomatic[id]={b,m}; } }
        if(doc.HasMember("annotations")) { for(const auto& a:array(doc["annotations"]).GetArray()) { const auto id=str(get(a,"id")); ViewCae::MeshCutProbe p; p.Position=vec(get(a,"position")); if(staged.Annotations().count(id)||!staged.AddAnnotation(id,p,str(get(a,"text")))) { throw std::invalid_argument("Invalid/duplicate annotation"); } } }
        std::string cadSource;
        if(doc.HasMember("cad")&&!doc["cad"].IsNull()) { const auto& c=doc["cad"]; const auto name=str(get(c,"file")); if(!declared.count(name)) { throw std::invalid_argument("CAD source has no digest"); } const auto p=contained(bundle,name); TopoDS_Shape shape; BRep_Builder builder; if(!BRepTools::Read(shape,p.generic_u8string().c_str(),builder)||!staged.SetCadSection(shape,str(get(c,"cut")),boolean(get(c,"sync")))) { throw std::invalid_argument("Invalid CAD source/section"); } cadSource=p.generic_u8string(); }
        bool clipOn=true;
        if(doc.HasMember("clip")&&!doc["clip"].IsNull()) {
            const auto& c=doc["clip"]; const auto& p=array(get(c,"plane"),4); if(p.Size()!=4) { throw std::invalid_argument("Invalid session global clip"); }
            auto equation=Graphic3d_Vec4d(num(p[0]),num(p[1]),num(p[2]),num(p[3])); const double norm=std::hypot(equation.x(),equation.y(),equation.z());
            if(!std::isfinite(norm)||norm<=0) { throw std::invalid_argument("Invalid session global clip normal"); } equation/=norm; clipOn=boolean(get(c,"on"));
            if(staged.mySyncClip) {
                if(staged.myGlobalClip.IsNull()) { throw std::invalid_argument("Synchronized clip is missing"); }
                for(int axis=0;axis<4;++axis) { if(std::abs(staged.myGlobalClip->GetEquation()[axis]-equation[axis])>1e-10) { throw std::invalid_argument("Synchronized clip disagrees with CAD cut"); } }
            } else if(!staged.SetGlobalClip(equation,clipOn)) { throw std::invalid_argument("Invalid session global clip"); }
        } else if(staged.mySyncClip) { throw std::invalid_argument("Synchronized CAD cut requires a saved clip"); }
        if(doc.HasMember("playback")) { const auto& p=doc["playback"]; timeline.myPeriod=num(get(p,"period")); timeline.myElapsed=num(get(p,"elapsed")); timeline.myLoop=boolean(get(p,"loop")); timeline.myUseTimes=boolean(get(p,"timestamps")); timeline.myPlaying=boolean(get(p,"playing")); if(timeline.myPeriod<=0||(!timeline.myUseTimes&&timeline.myPeriod<.001)||timeline.myElapsed<0||(timeline.myPlaying&&timeline.StepCount()<2)) { throw std::invalid_argument("Invalid session playback state"); } if(timeline.myUseTimes) { const auto& steps=mesh->SeriesSteps(); for(size_t i=0;i<steps.size();++i) { if(!std::isfinite(steps[i].Time)||(i&&steps[i].Time<=steps[i-1].Time)) { throw std::invalid_argument("Invalid playback timestamps"); } } } }
        const auto viewport=myOwner.myViewport; const auto context=myOwner.myContext; Handle(Graphic3d_Camera) camera;
        if(doc.HasMember("camera")&&!doc["camera"].IsNull()) { const auto& c=doc["camera"]; const auto eye=vec(get(c,"eye")),center=vec(get(c,"center")),up=vec(get(c,"up")); const double scale=num(get(c,"scale")),aspect=num(get(c,"aspect")),fovy=num(get(c,"fovy")),near=num(get(c,"near")),far=num(get(c,"far")); const int projection=integer(get(c,"projection")); if((eye-center).Modulus()<=1e-12||up.Modulus()<=1e-12||Graphic3d_Vec3d::Cross(center-eye,up).Modulus()<=1e-12||scale<=0||aspect<=0||fovy<=0||fovy>=180||far<=near||projection<0||projection>4||(projection!=0&&near<=0)) { throw std::invalid_argument("Invalid session camera"); } camera=new Graphic3d_Camera(); camera->SetEyeAndCenter(gp_Pnt(eye.x(),eye.y(),eye.z()),gp_Pnt(center.x(),center.y(),center.z())); camera->SetUp(gp_Dir(up.x(),up.y(),up.z())); camera->SetProjectionType((Graphic3d_Camera::Projection)projection); camera->SetScale(scale); camera->SetAspect(aspect); camera->SetFOVy(fovy); camera->SetZRange(near,far); }
        auto savedSource=source.generic_u8string(); auto savedDeformation=timeline.myDeformation;
        if(!staged.Refresh(false)||(!context.IsNull()&&!staged.Attach(context,viewport))) { throw std::invalid_argument(staged.LastError()); }
        if(!staged.myGlobalClip.IsNull()) { staged.myGlobalClip->SetOn(clipOn); }
        if(!context.IsNull()) { TColStd_ListOfInteger modes; context->ActivatedModes(myOwner.Display(),modes); context->Deactivate(staged.Display()); for(TColStd_ListOfInteger::Iterator mode(modes);mode.More();mode.Next()) { context->Activate(staged.Display(),mode.Value()); } }
        const bool cancelledDrag=myOwner.IsDragging();
        const auto previousCamera=viewport.IsNull()?Handle(Graphic3d_Camera)():viewport->Camera();
        if(!camera.IsNull()&&!viewport.IsNull()) { viewport->SetCamera(camera); }
        try { myOwner.adopt(staged); }
        catch(...) { if(!previousCamera.IsNull()) { viewport->SetCamera(previousCamera); } throw; }
        myTimeline.mySource=mesh; myTimeline.myStep=step; myTimeline.myDeformation=std::move(savedDeformation); myTimeline.myScale=timeline.myScale; myTimeline.myMaterialFrames=std::move(timeline.myMaterialFrames); myTimeline.myCutFrames=std::move(timeline.myCutFrames); myTimeline.myRecoverySteps=std::move(timeline.myRecoverySteps); myTimeline.myTraceSteps=std::move(timeline.myTraceSteps); myTimeline.myPlaying=timeline.myPlaying; myTimeline.myLoop=timeline.myLoop; myTimeline.myUseTimes=timeline.myUseTimes; myTimeline.myPeriod=timeline.myPeriod; myTimeline.myElapsed=timeline.myElapsed;
        mySource=std::move(savedSource); myCadSource=std::move(cadSource);
        if(cancelledDrag) { myOwner.event(CutEditEvent::Abort); }
        myOwner.event(CutEditEvent::Update);
    });
}
}
