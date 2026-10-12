
#include <RWVtk/VtkMultiBlockDataReader.h>

#include <RWCae/MeshTools.h>

#include <RWVtk/VtkPolyDataReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_Path.hxx>

#include <functional>
#include <set>

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

    if (!myIsMeshDataLoaded) {
        myMeshDataTreeRoot = new MeshDataTreeNode();
    }
    if (!processBlock(myDataElement, myMeshDataTreeRoot)) { return false; }
    // Dense union fields can greatly exceed sparse input arrays. Budget their
    // complete merged sizes before MeshTools::CopyRaw allocates them.
    std::set<std::string> fields;
    std::function<void(const Handle(MeshDataTreeNode)&)> check = [&](const Handle(MeshDataTreeNode)& node) {
        if (!node->NodeMeshData.IsNull()) {
            for (const auto& p : node->NodeMeshData->Properties()) {
                const std::string key = std::string(p->Name().ToCString()) + ":" + p->Description().ToCString()
                    + ":" + std::to_string((int)p->MapType()) + ":" + std::to_string((int)p->Type())
                    + ":" + std::to_string(p->NbComponents()) + ":" + std::to_string(p->TimeStep());
                if (fields.insert(key).second) {
                    const uint64_t count = p->IsNodal() ? myReport.points : myReport.cells;
                    for (int c = 0; c < p->NbRawComponents(); ++c) {
                        myBudget->Array(count*(p->GetRawComponent(c)->ElemType() == RWCae::MeshPropertyElemType::Float ? sizeof(float) : sizeof(double)));
                    }
                }
            }
        }
        for (const auto& child : node->SubNodes) { check(child); }
    };
    check(myMeshDataTreeRoot);
    return true;
}

// ================================================================
// function : processBlock
// ================================================================
bool VtkMultiBlockDataReader::processBlock(const LDOM_Element& theElem, const Handle(MeshDataTreeNode)& theTreeNode)
{
    std::vector<LDOM_Element> aSubBlocks = getChildrenElementsByTagName(theElem, "Block");
    for (const LDOM_Element& aSubBlock : aSubBlocks) {
        Handle(MeshDataTreeNode)      aSubNode;
        TCollection_AsciiString aChildName = getXmlAttribute(aSubBlock, "name", "Name");
        if (aChildName.IsEmpty()) {
            aChildName = TCollection_AsciiString("block_") + TCollection_AsciiString(getXmlAttribute(aSubBlock, "index", "Index"));
        }
        if (!myIsMeshDataLoaded) {
            aSubNode = new MeshDataTreeNode(aChildName);
        } else {
            aSubNode = theTreeNode->GetChild(aChildName);
            if (aSubNode.IsNull()) {
                return fail(std::string("Loaded MeshData has no subblock '") + aChildName.ToCString() + "'");
            }
        }

        if (!processBlock(aSubBlock, aSubNode)) {
            return fail(std::string("Cannot read VTK block: ") + aChildName.ToCString());
        }

        if (!myIsMeshDataLoaded) {
            theTreeNode->SubNodes.push_back(aSubNode);
        }
    }

    std::vector<LDOM_Element> aSubDataSets = getChildrenElementsByTagName(theElem, "DataSet");
    for (const LDOM_Element& aSubDataSet : aSubDataSets) {
        Handle(MeshDataTreeNode)      aSubNode;
        TCollection_AsciiString aChildName = getXmlAttribute(aSubDataSet, "name", "Name");
        if (aChildName.IsEmpty()) {
            aChildName = TCollection_AsciiString("dataset_") + TCollection_AsciiString(getXmlAttribute(aSubDataSet, "index", "Index"));
        }
        if (!myIsMeshDataLoaded) {
            aSubNode = new MeshDataTreeNode(aChildName);
        } else {
            aSubNode = theTreeNode->GetChild(aChildName);
            if (aSubNode.IsNull()) {
                return fail(std::string("Loaded MeshData has no subblock '") + aChildName.ToCString() + "'");
            }
        }

        TCollection_AsciiString aFileName = getXmlAttribute(aSubDataSet, "file", "File");
        if (aFileName.IsEmpty()) { return fail("VTK DataSet has no file reference"); }
        if (!processDataSet(myPath + aFileName, aSubNode)) {
            return fail(std::string("Cannot read VTK dataset: ") + aFileName.ToCString());
        }

        if (!myIsMeshDataLoaded) {
            theTreeNode->SubNodes.push_back(aSubNode);
        }
    }

    return !theTreeNode->SubNodes.empty();
}

// ================================================================
// function : processDataSet
// ================================================================
bool VtkMultiBlockDataReader::processDataSet(const TCollection_AsciiString&  theFileName,
                                             const Handle(MeshDataTreeNode)& theTreeNode)
{
    TCollection_AsciiString aProbeFileName = theFileName;
    aProbeFileName.LowerCase();
    if (aProbeFileName.EndsWith(".vtu")) {
        VtkUnstructuredGridReader aVtuReader;
        aVtuReader.SetToReportMessages(myToReportMessages);
        aVtuReader.SetResourceBudget(myBudget);
        aVtuReader.SetMappingInfo(myMappingInfo);
        aVtuReader.SetTimeStep(myTimeStep);
        if (myHasExplicitTimeStamp) { aVtuReader.SetTimeStamp(myTimeStamp); }
        if (myIsMeshDataLoaded) {
            aVtuReader.SetLoadedMeshData(theTreeNode->NodeMeshData, theTreeNode->StoredElements);
        }
        if (!aVtuReader.Perform(theFileName)) {
            for (const auto& anError : aVtuReader.Report().errors) { myReport.errors.push_back(anError); }
            myReport.warnings.insert(myReport.warnings.end(), aVtuReader.Report().warnings.begin(), aVtuReader.Report().warnings.end());
            return false;
        }

        myReport.warnings.insert(myReport.warnings.end(), aVtuReader.Report().warnings.begin(), aVtuReader.Report().warnings.end());
        if (!myIsMeshDataLoaded) {
            theTreeNode->NodeMeshData   = aVtuReader.GetMeshData();
            theTreeNode->StoredElements = aVtuReader.GetStoredElements();
        }
        myReport.points += aVtuReader.Report().points;
        myReport.cells += aVtuReader.Report().cells;
        myReport.fields += aVtuReader.Report().fields;
    } else if (aProbeFileName.EndsWith(".vtp")) {
        VtkPolyDataReader aVtpReader;
        aVtpReader.SetToReportMessages(myToReportMessages);
        aVtpReader.SetResourceBudget(myBudget);
        aVtpReader.SetMappingInfo(myMappingInfo);
        aVtpReader.SetTimeStep(myTimeStep);
        if (myHasExplicitTimeStamp) { aVtpReader.SetTimeStamp(myTimeStamp); }
        if (myIsMeshDataLoaded) {
            aVtpReader.SetLoadedMeshData(theTreeNode->NodeMeshData, theTreeNode->StoredElements);
        }
        if (!aVtpReader.Perform(theFileName)) {
            for (const auto& anError : aVtpReader.Report().errors) { myReport.errors.push_back(anError); }
            myReport.warnings.insert(myReport.warnings.end(), aVtpReader.Report().warnings.begin(), aVtpReader.Report().warnings.end());
            return false;
        }

        myReport.warnings.insert(myReport.warnings.end(), aVtpReader.Report().warnings.begin(), aVtpReader.Report().warnings.end());
        if (!myIsMeshDataLoaded) {
            theTreeNode->NodeMeshData   = aVtpReader.GetMeshData();
            theTreeNode->StoredElements = aVtpReader.GetStoredElements();
        }
        myReport.points += aVtpReader.Report().points;
        myReport.cells += aVtpReader.Report().cells;
        myReport.fields += aVtpReader.Report().fields;
    } else {
        return fail(std::string("Unsupported VTK composite child: ") + theFileName.ToCString());
    }

    return !theTreeNode->NodeMeshData.IsNull() && theTreeNode->NodeMeshData->NbNodes() > 0;
}

// ================================================================
// function : MergeMeshData
// ================================================================
void VtkMultiBlockDataReader::MergeMeshData(const Handle(MeshDataTreeNode)& theMeshDataTree,
                                            Handle(RWCae::MeshData)&      theMeshData)
{
    if (!theMeshData.IsNull()) {
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
