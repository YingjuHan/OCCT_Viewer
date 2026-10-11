
#ifndef _VTKMULTIBLOCKDATAREADER_HEADERFILE
#define _VTKMULTIBLOCKDATAREADER_HEADERFILE

#include <RWCae/MeshData.h>
#include <RWCae/MeshScalarProperty.h>

#include <RWVtk/VtkBaseXMLReader.h>

#include <LDOMParser.hxx>

#include <vector>

namespace RWVtk {
//! Reader class for VTK multi-block data stored in XML format.
class VtkMultiBlockDataReader : public VtkBaseXMLReader {
public:
    //! Struct for mesh data tree node.
    struct MeshDataTreeNode : public Standard_Transient {
        TCollection_AsciiString                        Name;
        Handle(RWCae::MeshData)                      NodeMeshData;
        std::vector<Handle(MeshDataTreeNode)>          SubNodes;
        std::shared_ptr<std::vector<std::vector<int>>> StoredElements;

        //! Constructor.
        explicit MeshDataTreeNode(const TCollection_AsciiString& theName = "") : Name(theName) {}

        //! Return child block by name.
        Handle(MeshDataTreeNode) GetChild(const TCollection_AsciiString& theName)
        {
            for (const Handle(MeshDataTreeNode)& aChild : SubNodes) {
                if (!aChild.IsNull() && aChild->Name == theName) {
                    return aChild;
                }
            }

            return nullptr;
        }
    };

public:
    //! Constructor.
    Standard_EXPORT VtkMultiBlockDataReader();

    //! Destructor.
    Standard_EXPORT virtual ~VtkMultiBlockDataReader();

    //! Return merged mesh data.
    const Handle(RWCae::MeshData)& GetMergedMeshData()
    {
        MergeMeshData(myMeshDataTreeRoot, myMergedMeshData);
        return myMergedMeshData;
    }

    //! Return mesh data tree.
    const Handle(MeshDataTreeNode)& GetMeshDataTree() { return myMeshDataTreeRoot; }

    //! Set mesh data tree.
    void SetMeshDataTree(const Handle(MeshDataTreeNode)& theMeshData)
    {
        myMeshDataTreeRoot = theMeshData;
        myIsMeshDataLoaded = true;
    }

    //! Set time step for all properties.
    void SetTimeStep(int theTimeStep) { myTimeStep = theTimeStep; }

    void SetTimeStamp(double theTime) { myTimeStamp = theTime; myHasExplicitTimeStamp = true; }

    //! Merge all mesh data.
    Standard_EXPORT static void MergeMeshData(const Handle(MeshDataTreeNode)& theMeshDataTree,
                                              Handle(RWCae::MeshData)&      theMeshData);

protected:
    //! Return type of data.
    virtual TCollection_AsciiString dataType() const override { return "vtkMultiBlockDataSet"; }

    //! Perform reading of element data.
    Standard_EXPORT virtual bool perform() override;

    //! Read 'Block' element of XML file.
    Standard_EXPORT bool processBlock(const LDOM_Element& theElem, const Handle(MeshDataTreeNode)& theTreeNode);

    //! Read 'DataSet' element of XML file.
    Standard_EXPORT bool processDataSet(const TCollection_AsciiString&  theFileName,
                                        const Handle(MeshDataTreeNode)& theTreeNode);

protected:
    TCollection_AsciiString   myPath;
    Handle(MeshDataTreeNode)  myMeshDataTreeRoot;
    Handle(RWCae::MeshData) myMergedMeshData;
    bool                      myIsMeshDataLoaded = false;
    int                       myTimeStep         = -1;
    double myTimeStamp = 0.0;
    bool myHasExplicitTimeStamp = false;
};

}

#endif // _VTKMULTIBLOCKDATAREADER_HEADERFILE
