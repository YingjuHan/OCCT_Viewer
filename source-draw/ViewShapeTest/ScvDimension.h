#ifndef SCPRSSHAPETEST_SCVDIMENSION_HEADERFILE
#define SCPRSSHAPETEST_SCVDIMENSION_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewBase/IInteractiveObject.h>

#include <PrsDim_Dimension.hxx>

namespace ViewShapeTest {

//! Command for displaying interactive dimensions.
class ScvDimension : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvdimension";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvdimension prsName [{-length|-angle|-radius}] ...
Create interactive dimension. Print `help vdimension` for other options.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvDimension();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

    //! Handle presentation name argument.
    void ParsePrsName(Standard_CString theName);

private:
    TCollection_AsciiString m_args   = "vdimension";
    int                     m_nbArgs = 1;

    TCollection_AsciiString m_prsName;
    PrsDim_KindOfDimension  m_dimType = PrsDim_KOD_NONE;

    Handle(PrsDim_Dimension) m_prsObj;
};

}

#endif // SCPRSSHAPETEST_SCVDIMENSION_HEADERFILE