#ifndef SCPRSDRAW_SCVREAD_HEADERFILE
#define SCPRSDRAW_SCVREAD_HEADERFILE

#include <DrawBase/DrawCommand.h>

namespace ViewDraw {

//! Command for reading model file.
class ScvRead : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvread";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvread {prsName|-probeFormat} {file.glb|file.brep|file.mdb|file.gmsh}
Read model (shape, mesh) from file in supported format.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvRead();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    TCollection_AsciiString m_fileName;
    TCollection_AsciiString m_args;
    Standard_Boolean        m_toProbe = false;
};

}

#endif // SCPRSDRAW_SCVREAD_HEADERFILE