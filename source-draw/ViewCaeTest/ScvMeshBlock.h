#ifndef _SCVMESHBLOCK_HEADERFILE
#define _SCVMESHBLOCK_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for setting visibility of blocks for CAE mesh.
class ScvMeshBlock : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshblock";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshblock prsName [-prs index] [-showAll {0|1}] [-show block] [-hide block] [-info]
Set visibility of mesh blocks.
Block is defined by chain of names divided by '/'.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshBlock();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

protected:
    //! Parse blocks from string.
    bool parseBlocks(Handle(RWCae::MeshDataBlock)& theBlocks, const char* theVal);

private:
    Handle(RWCae::MeshDataBlock) m_blocksToShow;
    Handle(RWCae::MeshDataBlock) m_blocksToHide;
    int                            m_toShowAll     = -1;
    bool                           m_toPrintBlocks = false;
};

}

#endif // _SCVMESHBLOCK_HEADERFILE
