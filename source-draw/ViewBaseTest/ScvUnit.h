#ifndef ViewBASETEST_SCVUNIT_HEADERFILE
#define ViewBASETEST_SCVUNIT_HEADERFILE

#include <DrawBase/DrawCommand.h>

#include <ViewBase/UnitManager.h>

#include <TCollection_HAsciiString.hxx>

namespace ViewBaseTest {

//! Command for accessing Unit Manager @link ViewBase::UnitManager @endlink.
class ScvUnit : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvunit";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvunit [quantity] [-si] [-system [NewSystemUnit]] [-display [NewDisplayUnit]]
        [value] [-systemToDisplay|-systemFromDisplay|-systemToSI|-systemFromSI] [-to OutUnits]
Manage units conversion.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvUnit();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    enum {
        CmdUnit_PrintSI,
        CmdUnit_PrintSystem,
        CmdUnit_PrintDisplay,
        CmdUnit_SystemToSI,
        CmdUnit_SystemFromSI,
        CmdUnit_SystemToDisplay,
        CmdUnit_SystemFromDisplay
    };

    //! Register keywords.
    void AddKeys1();

    //! Register keywords.
    void AddKeys2();

    //! Print quantity info.
    void PrintQuantity(Draw_Interpretor& theDI, ViewBase::QuantityType theQuantity) const;

    //! Handle command.
    void HandleUnitCommand(Draw_Interpretor& theDI, bool theIsExtUnit);

private:
    TCollection_AsciiString          m_quantStr;
    Handle(TCollection_HAsciiString) m_dispUnit;
    Handle(TCollection_HAsciiString) m_sysUnit;

    TCollection_AsciiString m_toUnit;
    ViewBase::QuantityType m_quantity = ViewBase::QuantityType::UNKNOWN;
    int                     m_unitCmd  = -1;
    double                  m_value    = RealLast();
};

}

#endif // ViewBASETEST_SCVUNIT_HEADERFILE

