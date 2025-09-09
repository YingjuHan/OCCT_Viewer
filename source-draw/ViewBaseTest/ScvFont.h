#ifndef ViewBASETEST_SCVFONT_HEADERFILE
#define ViewBASETEST_SCVFONT_HEADERFILE

#include <DrawBase/DrawCommand.h>

namespace ViewBaseTest {

//! Command for managing fonts.
class ScvFont : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvfont";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvfont fontFile [fontName] [-override {0|1}]=0
Register font file.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvFont();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    std::string m_fontFile;
    std::string m_fontName;
    bool        m_toOverride = false;
};

}

#endif // ViewBASETEST_SCVFONT_HEADERFILE

