#ifndef RWCAETEST_SCVREADMAPPING_HEADERFILE
#define RWCAETEST_SCVREADMAPPING_HEADERFILE

#include <DrawBase/DrawCommand.h>

namespace RWCaeTest {

//! Command for testing RWCae::MeshFieldMappingReader class.
class ScvReadMapping : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvreadmapping";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvreadmapping <mapping_file_name.json>
Read CAE results mapping file and print content summary.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvReadMapping();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_fileName;
};

}

#endif // RWCAETEST_SCVREADMAPPING_HEADERFILE 
