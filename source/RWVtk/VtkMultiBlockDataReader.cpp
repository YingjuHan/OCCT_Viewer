
#include <RWVtk/VtkMultiBlockDataReader.h>

#include <RWCae/MeshTools.h>

#include <RWVtk/VtkPolyDataReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_Path.hxx>

#include <functional>

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

    if (!myIsMeshDataLoaded) {
        myMeshDataTreeRoot = new MeshDataTreeNode();
    }
    return processBlock(myDataElement, myMeshDataTreeRoot);
}

// ================================================================
// function : processBlock
// ================================================================
bool VtkMultiBlockDataReader::processBlock(const LDOM_Element& theElem, const Handle(MeshDataTreeNode)& theTreeNode)
{
    std::vector<LDOM_Element> aSubBlocks = getChildrenElementsByTagName(theElem, "Block");
    for (const LDOM_Element& aSubBlock : aSubBlocks) {
        Handle(MeshDataTreeNode)      aSubNode;
        const TCollection_AsciiString aChildName = getXmlAttribute(aSubBlock, "name", "Name");
        if (!myIsMeshDataLoaded) {
            aSubNode = new MeshDataTreeNode(aChildName);
        } else {
            aSubNode = theTreeNode->GetChild(aChildName);
            if (aSubNode.IsNull()) {
                Message::SendFail() << "Error: Loaded MeshData has no subblock '" << aChildName << "'";
                return false;
            }
        }

        if (!processBlock(aSubBlock, aSubNode)) {
            continue;
        }

        if (!myIsMeshDataLoaded) {
            theTreeNode->SubNodes.push_back(aSubNode);
        }
    }

    std::vector<LDOM_Element> aSubDataSets = getChildrenElementsByTagName(theElem, "DataSet");
    for (const LDOM_Element& aSubDataSet : aSubDataSets) {
        Handle(MeshDataTreeNode)      aSubNode;
        const TCollection_AsciiString aChildName = getXmlAttribute(aSubDataSet, "name", "Name");
        if (!myIsMeshDataLoaded) {
            aSubNode = new MeshDataTreeNode(aChildName);
        } else {
            aSubNode = theTreeNode->GetChild(aChildName);
            if (aSubNode.IsNull()) {
                Message::SendFail() << "Error: Loaded MeshData has no subblock '" << aChildName << "'";
                return false;
            }
        }

        TCollection_AsciiString aFileName = getXmlAttribute(aSubDataSet, "file", "File");
        if (!processDataSet(myPath + aFileName, aSubNode)) {
            continue;
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
        aVtuReader.SetMappingInfo(myMappingInfo);
        aVtuReader.SetTimeStep(myTimeStep);
        if (myIsMeshDataLoaded) {
            aVtuReader.SetLoadedMeshData(theTreeNode->NodeMeshData, theTreeNode->StoredElements);
        }
        if (!aVtuReader.Perform(theFileName)) {
            return false;
        }

        if (!myIsMeshDataLoaded) {
            theTreeNode->NodeMeshData   = aVtuReader.GetMeshData();
            theTreeNode->StoredElements = aVtuReader.GetStoredElements();
        }
    } else if (aProbeFileName.EndsWith(".vtp")) {
        VtkPolyDataReader aVtpReader;
        aVtpReader.SetMappingInfo(myMappingInfo);
        aVtpReader.SetTimeStep(myTimeStep);
        if (myIsMeshDataLoaded) {
            aVtpReader.SetLoadedMeshData(theTreeNode->NodeMeshData, theTreeNode->StoredElements);
        }
        if (!aVtpReader.Perform(theFileName)) {
            return false;
        }

        if (!myIsMeshDataLoaded) {
            theTreeNode->NodeMeshData   = aVtpReader.GetMeshData();
            theTreeNode->StoredElements = aVtpReader.GetStoredElements();
        }
    } else {
        Message::SendInfo() << "Unknown file format '" << theFileName << "'";
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