
#ifndef _SCVMESHPRS_HEADERFILE
#define _SCVMESHPRS_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for managing CAE mesh presentations.
class ScvMeshPrs : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshprs";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshprs prsName {-create scalar|vector|streamlines} {-remove index} {-info}
Create/remove mesh presentation.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshPrs();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    int                            m_indexToRemove = -1;
    bool                           m_toShowInfo    = false;
    ViewCae::MeshPresentationType m_type          = ViewCae::MeshPresentationType::Scalar;
    bool                           m_toCreate      = false;
};

}

#endif // _SCVMESHPRS_HEADERFILE
