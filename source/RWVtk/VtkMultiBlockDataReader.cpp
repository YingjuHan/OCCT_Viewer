
#include <RWVtk/VtkMultiBlockDataReader.h>

#include <RWCae/MeshTools.h>

#include <RWVtk/VtkPolyDataReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_Path.hxx>

#include <functional>
#include <set>
#include <map>
#include <filesystem>
#include <algorithm>
#include <RWVtk/VtkDatasetReader.h>

namespace RWVtk {

// ================================================================
// function : VtkMultiBlockDataReader
// ================================================================
VtkMultiBlockDataReader::VtkMultiBlockDataReader() : VtkBaseXMLReader()
{
    //
}

// ================================================================
// function : ~VtkMultiBlockDataReader
// ================================================================
VtkMultiBlockDataReader::~VtkMultiBlockDataReader()
{
    //
}

// ================================================================
// function : perform
// ================================================================
bool VtkMultiBlockDataReader::perform()
{
    TCollection_AsciiString aFileName;
    OSD_Path::FolderAndFileFromPath(myFileName, myPath, aFileName);
    myMergedMeshData.Nullify();

    myMeshDataTreeRoot.Nullify();
    if (myIsMeshDataLoaded) { return fail("Loaded-tree mutation is unsupported; stage a complete time step instead"); }
    if (!myReferenceChain) { myReferenceChain=std::make_shared<std::vector<std::string>>(); }
    const std::string canonical=std::filesystem::weakly_canonical(std::filesystem::u8path(myFileName.ToCString())).generic_u8string();
    if (myReferenceChain->size()>=myLimits.MaxDepth) { return fail("VTK resource limit: reference depth"); }
    if (std::find(myReferenceChain->begin(),myReferenceChain->end(),canonical)!=myReferenceChain->end()) { return fail("Cyclic VTK reference: "+canonical); }
    myReferenceChain->push_back(canonical);
    struct Pop { std::vector<std::string>& Chain; ~Pop() {Chain.pop_back();} } pop{*myReferenceChain};
    Handle(MeshDataTreeNode) staged=new MeshDataTreeNode();
    if (!processBlock(myDataElement,staged)) {return false;}
    // Dense union fields can greatly exceed sparse input arrays. Budget their
    // complete merged sizes before MeshTools::CopyRaw allocates them.
    std::set<std::string> fields;
    std::map<std::string,Handle(RWCae::MeshNVecProperty)> schemas;
    std::map<std::string,size_t> presence;
    size_t leaves=0;
    std::function<void(const Handle(MeshDataTreeNode)&)> check = [&](const Handle(MeshDataTreeNode)& node) {
        if (!node->NodeMeshData.IsNull()) {
            ++leaves;
            for (const auto& p : node->NodeMeshData->Properties()) {
                const std::string key = std::string(p->Name().ToCString()) + ":" + p->Description().ToCString()
                    + ":" + std::to_string((int)p->MapType()) + ":" + std::to_string((int)p->Type())
                    + ":" + std::to_string(p->NbComponents()) + ":" + std::to_string(p->TimeStep());
                const std::string identity=std::string(p->Name().ToCString())+":"+p->Description().ToCString()+":"+std::to_string((int)p->MapType());
                ++presence[identity];
                auto inserted=schemas.emplace(identity,p);
                if (!inserted.second && (inserted.first->second->Type()!=p->Type() || inserted.first->second->NbComponents()!=p->NbComponents())) {throw std::runtime_error("Composite field schema mismatch: "+identity);}
                if (!inserted.second) {
                    for (int c=0;c<p->NbComponents();++c) {
                        if (inserted.first->second->GetComponent(c)->Name()!=p->GetComponent(c)->Name()) {
                            throw std::runtime_error("Composite field component name mismatch: "+identity);
                        }
                    }
                }
                if (fields.insert(key).second) {
                    const uint64_t count = p->IsNodal() ? myReport.points : myReport.cells;
                    for (int c = 0; c < p->NbRawComponents(); ++c) {
                        myBudget->Array(count*sizeof(double));
                    }
                }
            }
        }
        for (const auto& child : node->SubNodes) { check(child); }
    };
    check(staged);
    for (const auto& field:presence) {if(field.second<leaves) {warn("Composite field '"+field.first+"' is absent in some blocks; missing values remain NaN");}}
    Handle(RWCae::MeshData) merged;MergeMeshData(staged,merged);
    if(merged.IsNull()) {return fail("VTK composite contains no mesh");}
    myMeshDataTreeRoot=staged;myMergedMeshData=merged;return true;
}

// ================================================================
// function : processBlock
// ================================================================
bool VtkMultiBlockDataReader::processBlock(const LDOM_Element& element,const Handle(MeshDataTreeNode)& tree,uint64_t depth)
{
    if(depth+myReferenceChain->size()>myLimits.MaxDepth) {return fail("VTK resource limit: block depth");}
    std::set<std::string> names;
    int ordinal=0;
    for(LDOM_Node item=element.getFirstChild();!item.isNull();item=item.getNextSibling()) {
        if(item.getNodeType()!=LDOM_Node::ELEMENT_NODE) {continue;}
        LDOM_Element child=(const LDOM_Element&)item;
        const TCollection_AsciiString tag=child.getTagName();
        if(tag!="Block" && tag!="DataSet") {return fail(std::string("Unsupported VTK composite section: ")+tag.ToCString());}
        TCollection_AsciiString name=getXmlAttribute(child,"name","Name");
        if(name.IsEmpty()) {
            TCollection_AsciiString index=getXmlAttribute(child,"index","Index");if(index.IsEmpty()) {index=TCollection_AsciiString(ordinal);}
            name=TCollection_AsciiString(tag=="Block"?"block_":"dataset_")+index;
        }
        ++ordinal;
        if(!names.insert(name.ToCString()).second) {return fail(std::string("Duplicate VTK block name: ")+name.ToCString());}
        Handle(MeshDataTreeNode) node=new MeshDataTreeNode(name);
        if(tag=="Block") {
            if(!processBlock(child,node,depth+1)) {return false;}
        } else {
            const TCollection_AsciiString reference=getXmlAttribute(child,"file","File");
            if(reference.IsEmpty()) {return fail("VTK DataSet has no file reference");}
            const auto resolved=(std::filesystem::u8path(myPath.ToCString())/std::filesystem::u8path(reference.ToCString())).lexically_normal().generic_u8string();
            if(!processDataSet(resolved.c_str(),node)) {
                return fail(std::string("Composite '")+myFileName.ToCString()+"', block '"+name.ToCString()+"', reference '"+reference.ToCString()+"', resolved '"+resolved+"'");
            }
        }
        tree->SubNodes.push_back(node);
    }
    return !tree->SubNodes.empty() || fail("VTK composite block contains no datasets");
}

bool VtkMultiBlockDataReader::processDataSet(const TCollection_AsciiString& filename,const Handle(MeshDataTreeNode)& tree)
{
    TCollection_AsciiString lower=filename;lower.LowerCase();
    VtkImportReport report;
    if(lower.EndsWith(".vtm")) {
        VtkMultiBlockDataReader reader;reader.SetResourceBudget(myBudget);reader.SetReferenceChain(myReferenceChain);
        reader.SetToReportMessages(false);reader.SetMappingInfo(myMappingInfo);reader.SetTimeStep(myTimeStep);
        if(myHasExplicitTimeStamp) {reader.SetTimeStamp(myTimeStamp);}
        const bool ok=reader.Perform(filename);report=reader.Report();
        if(ok) {tree->SubNodes=reader.GetMeshDataTree()->SubNodes;}
    } else {
        if(myReferenceChain->size()>=myLimits.MaxDepth) {return fail("VTK resource limit: reference depth");}
        VtkDatasetReader reader;reader.SetResourceBudget(myBudget);reader.SetToReportMessages(false);reader.SetMappingInfo(myMappingInfo);
        reader.SetTimeStep(myTimeStep);if(myHasExplicitTimeStamp) {reader.SetTimeStamp(myTimeStamp);}
        const bool ok=reader.Perform(filename);report=reader.Report();if(ok) {tree->NodeMeshData=reader.GetMeshData();}
    }
    for(const auto& error:report.errors) {myReport.errors.push_back(std::string("Child '")+filename.ToCString()+"': "+error);}
    for(const auto& warning:report.warnings) {warn(std::string("Child '")+filename.ToCString()+"': "+warning);}
    if(!report.errors.empty() || (tree->NodeMeshData.IsNull() && tree->SubNodes.empty())) {return false;}
    myReport.points+=report.points;myReport.cells+=report.cells;myReport.fields+=report.fields;
    return true;
}

// ================================================================
// function : MergeMeshData
// ================================================================
void VtkMultiBlockDataReader::MergeMeshData(const Handle(MeshDataTreeNode)& theMeshDataTree,
                                            Handle(RWCae::MeshData)&      theMeshData)
{
    if (theMeshDataTree.IsNull() || !theMeshData.IsNull()) {
        return;
    }

    RWCae::MeshTools::MeshDataList aMeshDataList;

    std::function<void(const Handle(MeshDataTreeNode)&, const std::vector<TCollection_AsciiString>&)> calculateList =
        [&](const Handle(MeshDataTreeNode)& theTreeNode, const std::vector<TCollection_AsciiString>& theBlocks) {
            std::vector<TCollection_AsciiString> aBlocks = theBlocks;
            if (!theTreeNode->Name.IsEmpty()) {
                aBlocks.push_back(theTreeNode->Name);
            }
            for (const Handle(MeshDataTreeNode)& aSubNode : theTreeNode->SubNodes) {
                calculateList(aSubNode, aBlocks);
            }

            if (!theTreeNode->NodeMeshData.IsNull()) {
                aMeshDataList.push_back(std::make_pair(theTreeNode->NodeMeshData, aBlocks));
            }
        };

    calculateList(theMeshDataTree, std::vector<TCollection_AsciiString>());

    theMeshData = RWCae::MeshTools::MergeMeshData(aMeshDataList);
}

}
