#include <RWVtk/VtkReferenceStager.h>
#include <RWBase/JsonHelper.h>
#include "VtkBoundedTextBuffer.h"
#include <LDOMParser.hxx>
#include <LDOM_XmlWriter.hxx>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <functional>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <cctype>

namespace RWVtk {
bool VtkReferenceStager::Perform(const TCollection_AsciiString& source,const TCollection_AsciiString& directory,const VtkResourceLimits& limits)
{
    namespace fs=std::filesystem;
    mySource.Clear();myReport=VtkImportReport();myReport.source=source.ToCString();
    fs::path work, target;
    try {
        target=fs::absolute(fs::u8path(directory.ToCString())).lexically_normal();fs::create_directories(target);
        static std::atomic<uint64_t> counter{0};
        do {
            work=target/("vtk-import-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(counter++));
        } while(!fs::create_directory(work));
        VtkResourceBudget budget(limits);
        std::map<std::string,fs::path> copied;std::set<std::string> active;
        std::function<fs::path(const fs::path&,uint64_t)> stage;
        stage=[&](const fs::path& file,uint64_t depth)->fs::path {
            if(depth>limits.MaxDepth) {throw std::runtime_error("VTK resource limit: reference depth");}
            const fs::path canonical=fs::canonical(file);std::string key=canonical.generic_u8string();
#ifdef _WIN32
            std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return (char)std::tolower(c);});
#endif
            if(active.count(key)) {throw std::runtime_error("Cyclic VTK reference: "+key);}
            auto existing=copied.find(key);if(existing!=copied.end()) {return existing->second;}
            const fs::path output=work/std::to_string(copied.size())/canonical.filename();
            copied.emplace(key,output);active.insert(key);fs::create_directories(output.parent_path());
            std::ifstream input(canonical,std::ios::binary);if(!input) {throw std::runtime_error("Cannot open referenced file: "+key);}
            std::string text=budget.Read(input), lower=canonical.filename().generic_u8string();
            std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return (char)std::tolower(c);});
            auto reference=[&](const std::string& name) {
                if(name.empty() || name.find('\0')!=std::string::npos) {throw std::runtime_error("Empty/NUL VTK file reference in "+key);}
                const fs::path child=canonical.parent_path()/fs::u8path(name);
                try {return fs::relative(stage(child,depth+1),output.parent_path()).generic_u8string();}
                catch(const std::exception& error) {throw std::runtime_error("Reference '"+name+"' from '"+key+"', resolved '"+child.lexically_normal().generic_u8string()+"': "+error.what());}
            };
            if(lower.size()>=11 && lower.substr(lower.size()-11)==".vtm.series") {
                if(text.find('\0')!=std::string::npos) {throw std::runtime_error("NUL in series JSON");}
                rapidjson::Document doc;doc.Parse<rapidjson::kParseFullPrecisionFlag>(text.c_str());
                if(doc.HasParseError() || !doc.IsObject() || !doc.HasMember("files") || !doc["files"].IsArray()) {throw std::runtime_error("Invalid series manifest: "+key);}
                for(auto& entry:doc["files"].GetArray()) {
                    if(!entry.IsObject() || !entry.HasMember("name") || !entry["name"].IsString()) {throw std::runtime_error("Invalid series reference in "+key);}
                    const auto name=reference(std::string(entry["name"].GetString(),entry["name"].GetStringLength()));entry["name"].SetString(name.c_str(),(rapidjson::SizeType)name.size(),doc.GetAllocator());
                }
                detail::BoundedTextBuffer buffer(limits.MaxFileBytes);std::ostream stream(&buffer);stream.exceptions(std::ios::badbit|std::ios::failbit);
                rapidjson::OStreamWrapper wrapper(stream);rapidjson::Writer<rapidjson::OStreamWrapper> writer(wrapper);doc.Accept(writer);text=buffer.Text();
            } else if(lower.size()>=4 && lower.substr(lower.size()-4)==".vtm") {
                if(text.find('\0')!=std::string::npos) {throw std::runtime_error("NUL in composite XML");}
                LDOMParser parser;std::istringstream xml(text);if(parser.parse(xml)) {throw std::runtime_error("Invalid composite manifest: "+key);}
                auto document=parser.getDocument();auto root=document.getDocumentElement();
                if(TCollection_AsciiString(root.getTagName())!="VTKFile" || TCollection_AsciiString(root.getAttribute("type"))!="vtkMultiBlockDataSet") {throw std::runtime_error("Unsupported composite manifest: "+key);}
                std::function<void(LDOM_Element,uint64_t)> rewrite=[&](LDOM_Element parent,uint64_t level) {
                    if(level>limits.MaxDepth) {throw std::runtime_error("VTK resource limit: block depth");}
                    for(LDOM_Node node=parent.getFirstChild();!node.isNull();node=node.getNextSibling()) {
                        if(node.getNodeType()!=LDOM_Node::ELEMENT_NODE) {continue;} LDOM_Element element=(const LDOM_Element&)node;
                        if(TCollection_AsciiString(element.getTagName())=="DataSet") {
                            TCollection_AsciiString name=element.getAttribute("file");if(name.IsEmpty()) {name=element.getAttribute("File");}
                            const auto relative=reference(name.ToCString());element.setAttribute("file",relative.c_str());
                        } else {rewrite(element,level+1);}
                    }
                };
                rewrite(root,0);detail::BoundedTextBuffer buffer(limits.MaxFileBytes);std::ostream stream(&buffer);stream.exceptions(std::ios::badbit|std::ios::failbit);
                LDOM_XmlWriter writer;writer.Write(stream,document);text=buffer.Text();
            } else if(lower.size()<4 || (lower.substr(lower.size()-4)!=".vtu" && lower.substr(lower.size()-4)!=".vtp" && lower.substr(lower.size()-4)!=".vtk")) {
                throw std::runtime_error("Unsupported referenced VTK format: "+key);
            }
            if(text.size()>limits.MaxFileBytes) {throw std::runtime_error("VTK resource limit: staged manifest bytes");}
            std::ofstream result(output,std::ios::binary);result.write(text.data(),text.size());if(!result) {throw std::runtime_error("Cannot write staged file: "+output.generic_u8string());}
            active.erase(key);return output;
        };
        const auto root=stage(fs::u8path(source.ToCString()),1);
        mySource=root.generic_u8string().c_str();myReport.succeeded=true;return true;
    } catch(const std::exception& error) {
        myReport.errors.push_back(std::string("VTK reference staging '")+source.ToCString()+"' into '"+directory.ToCString()+"': "+error.what());
        // Only the newly created attempt directory can be removed.
        if(!work.empty() && work.parent_path()==target && work.filename().generic_u8string().find("vtk-import-")==0) {
            std::error_code ignored;fs::remove_all(work,ignored);
        }
        return false;
    }
}
}
