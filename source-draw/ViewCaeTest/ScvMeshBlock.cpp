#include <ViewCaeTest/ScvMeshBlock.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMeshBlock
// =======================================================================
ScvMeshBlock::ScvMeshBlock()
{
    AddVoidKey(this, "-info") = [](ScvMeshBlock* theCmd) -> bool {
        theCmd->m_toPrintBlocks = true;
        return true;
    };

    AddKey(this, "", "-show") = [](ScvMeshBlock* theCmd, int&, const char* theVal) -> bool {
        return theCmd->parseBlocks(theCmd->m_blocksToShow, theVal);
    };

    AddKey(this, "", "-hide") = [](ScvMeshBlock* theCmd, int&, const char* theVal) -> bool {
        return theCmd->parseBlocks(theCmd->m_blocksToHide, theVal);
    };

    AddOnOffNoKey(this, "-showall") = &m_toShowAll;
}

// =======================================================================
// function : parseBlocks
// =======================================================================
bool ScvMeshBlock::parseBlocks(Handle(RWCae::MeshDataBlock)& theBlocks, const char* theVal)
{
    std::stringstream aStream(theVal);
    std::string       aNodeStr;

    if (!std::getline(aStream, aNodeStr, '/')) {
        return false;
    }

    if (theBlocks.IsNull()) {
        theBlocks = new RWCae::MeshDataBlock(aNodeStr.c_str());
    } else if (theBlocks->Name() != aNodeStr.c_str()) {
        return false;
    }

    Handle(RWCae::MeshDataBlock) aBlock = theBlocks;
    while (std::getline(aStream, aNodeStr, '/')) {
        aBlock = aBlock->AppendChild(aNodeStr.c_str());
    }

    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshBlock::Perform()
{
    if (m_toPrintBlocks) {
        std::function<void(const Handle(RWCae::MeshDataBlock)&, int)> printBlock =
            [&](const Handle(RWCae::MeshDataBlock)& theNode, int theLevel) {
                for (int i = 0; i < theLevel; i++) {
                    *myDI << " ";
                }

                *myDI << theNode->Name() << "\n";

                for (const Handle(RWCae::MeshDataBlock)& aChild : theNode->Children()) {
                    printBlock(aChild, theLevel + 1);
                }
            };

        printBlock(m_mesh->GetMeshData()->Blocks(), 0);
    }

    const TCollection_AsciiString& aPrsId = m_mesh->PresentationByIndex(m_prsIndex)->Id();

    if (m_toShowAll != -1) {
        Handle(RWCae::MeshDataBlock) aRootBlock = new RWCae::MeshDataBlock(m_mesh->GetMeshData()->Blocks()->Name());
        m_mesh->SetBlocksVisible(aPrsId, aRootBlock, m_toShowAll == 1);
    }

    if (!m_blocksToShow.IsNull()) {
        m_mesh->SetBlocksVisible(aPrsId, m_blocksToShow, true);
    }

    if (!m_blocksToHide.IsNull()) {
        m_mesh->SetBlocksVisible(aPrsId, m_blocksToHide, false);
    }

    UpdateMesh();

    return true;
}

}
