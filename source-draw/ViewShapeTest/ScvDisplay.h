#ifndef SCPRSSHAPETEST_SCVDISPLAY_HEADERFILE
#define SCPRSSHAPETEST_SCVDISPLAY_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <Draw_Drawable3D.hxx>

namespace ViewShapeTest {

//! Command for displaying interactive shape as InteractiveShape.
class ScvDisplay : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvdisplay";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvdisplay shape1 [shape2 ... [shapeN]] [-prsName Name]
Display ViewerDrawable within 3D viewer.
If input object is a Drawable Shape, it will be displayed as InteractiveShape.
Multiple Shapes could be packed into as a single InteractiveShape via -prsName parameter.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvDisplay();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    NCollection_List<TCollection_AsciiString> m_names;
    TCollection_AsciiString                   m_prsName;
};

}

#endif // SCPRSSHAPETEST_SCVDISPLAY_HEADERFILE    
