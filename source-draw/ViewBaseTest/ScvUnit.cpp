#include <ViewBaseTest/ScvUnit.h>

namespace ViewBaseTest {

// =======================================================================
// function : ScvUnit
// =======================================================================
ScvUnit::ScvUnit()
{
    AddKeys1();
    AddKeys2();
}

// =======================================================================
// function : AddKeys1
// =======================================================================
void ScvUnit::AddKeys1()
{
    AddVoidKey(this, "-si") = [](ScvUnit* theCmd, int&) -> bool {
        if (theCmd->m_unitCmd == -1) {
            theCmd->m_unitCmd = CmdUnit_PrintSI;
            return true;
        }
        return false;
    };
    AddVoidKey(this, "-todisp", "-todisplay", "-systodisp", "-systemtodisplay") = [](ScvUnit* theCmd, int&) -> bool {
        if (theCmd->m_unitCmd == -1) {
            theCmd->m_unitCmd = CmdUnit_SystemToDisplay;
            return true;
        }
        return false;
    };
    AddVoidKey(this, "-fromdisp", "-fromdisplay", "-sysfromdisp", "-systemfromdisplay") = [](ScvUnit* theCmd,
                                                                                             int&) -> bool {
        if (theCmd->m_unitCmd == -1) {
            theCmd->m_unitCmd = CmdUnit_SystemFromDisplay;
            return true;
        }
        return false;
    };
    AddVoidKey(this, "-tosi", "-systosi", "-systemtosi") = [](ScvUnit* theCmd, int&) -> bool {
        if (theCmd->m_unitCmd == -1) {
            theCmd->m_unitCmd = CmdUnit_SystemToSI;
            return true;
        }
        return false;
    };
    AddVoidKey(this, "-fromsi", "-sysfromsi", "-systemfromsi") = [](ScvUnit* theCmd, int&) -> bool {
        if (theCmd->m_unitCmd == -1) {
            theCmd->m_unitCmd = CmdUnit_SystemFromSI;
            return true;
        }
        return false;
    };
}

// =======================================================================
// function : AddKeys2
// =======================================================================
void ScvUnit::AddKeys2()
{
    AddKey(this, "", "-to") = [](ScvUnit* theCmd, int&, const char* theVal) -> bool {
        if (theCmd->m_unitCmd == -1) {
            theCmd->m_toUnit = theVal;
            return true;
        }
        return false;
    };

    AddVoidKey(this, "-system") = [](ScvUnit* theCmd, int& theArgIter) -> bool {
        if (theCmd->m_unitCmd != -1 || !theCmd->m_sysUnit.IsNull()) {
            return false;
        } else if (theArgIter + 1 < theCmd->NbArguments() && *theCmd->Argument(theArgIter + 1) != '-') {
            theCmd->m_sysUnit = new TCollection_HAsciiString(theCmd->Argument(++theArgIter));
        } else {
            theCmd->m_unitCmd = CmdUnit_PrintSystem;
        }
        return true;
    };

    AddVoidKey(this, "-display") = [](ScvUnit* theCmd, int& theArgIter) -> bool {
        if (theCmd->m_unitCmd != -1 || !theCmd->m_dispUnit.IsNull()) {
            return false;
        } else if (theArgIter + 1 < theCmd->NbArguments() && *theCmd->Argument(theArgIter + 1) != '-') {
            theCmd->m_dispUnit = new TCollection_HAsciiString(theCmd->Argument(++theArgIter));
        } else {
            theCmd->m_unitCmd = CmdUnit_PrintDisplay;
        }
        return true;
    };
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvUnit::ParseArgument(int& theArgIter)
{
    if (DrawCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_value == RealLast() && Draw::ParseReal(Argument(theArgIter), m_value)) {
        //
    } else if (m_quantity == ViewBase::QuantityType::UNKNOWN) {
        const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();

        m_quantStr = Argument(theArgIter);
        m_quantity = aMgr->FindQuantity(m_quantStr);
        if (m_quantity == ViewBase::QuantityType::UNKNOWN) {
            *myDI << "Syntax error: unknown quantity/unit '" << m_quantStr << "'";
            return false;
        }
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvUnit::Perform()
{
    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    if (!m_sysUnit.IsNull()) {
        aMgr->SetSystemUnit(m_quantity, m_sysUnit->String());
    }
    if (!m_dispUnit.IsNull()) {
        aMgr->SetDisplayUnit(m_quantity, m_dispUnit->String());
    }

    const bool isExtUnit = !TCollection_AsciiString::IsSameString(m_quantStr, aMgr->GetQuantityName(m_quantity), false);
    if (m_unitCmd == -1 && m_sysUnit.IsNull() && m_dispUnit.IsNull()) {
        if (isExtUnit && m_value != RealLast() && !m_quantStr.IsEmpty()) {
            TCollection_AsciiString aToUnit = !m_toUnit.IsEmpty() ? m_toUnit : aMgr->GetDisplayUnit(m_quantity);
            *myDI << aMgr->Convert(m_value, m_quantStr, aToUnit) << " " << aToUnit;
            return true;
        }

        PrintQuantity(*myDI, m_quantity);
    }

    HandleUnitCommand(*myDI, isExtUnit);
    return true;
}

// =======================================================================
// function : PrintQuantity
// =======================================================================
void ScvUnit::PrintQuantity(Draw_Interpretor& theDI, ViewBase::QuantityType theQuantity) const
{
    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    if (theQuantity == ViewBase::QuantityType::UNKNOWN) {
        for (int aQIter = 0; aQIter < ViewBase::QuantityType_NB; ++aQIter) {
            PrintQuantity(theDI, (ViewBase::QuantityType)aQIter);
            theDI << "\n";
        }
        return;
    }

    const ViewBase::QuantityUnits& aQuant = aMgr->GetQuantity(theQuantity);
    theDI << "Name:    " << aMgr->GetQuantityName(theQuantity) << "\n";
    theDI << "SI:      " << aMgr->GetSIUnit(theQuantity) << "\n";
    theDI << "System:  " << (aQuant.System.IsEmpty() ? "?? [" : "") << aMgr->GetSystemUnit(theQuantity)
          << (aQuant.System.IsEmpty() ? "]" : "") << "\n";
    theDI << "Display: " << (aQuant.Display.IsEmpty() ? "?? [" : "") << aMgr->GetDisplayUnit(theQuantity)
          << (aQuant.Display.IsEmpty() ? "]" : "") << "\n";
}

// =======================================================================
// function : HandleUnitCommand
// =======================================================================
void ScvUnit::HandleUnitCommand(Draw_Interpretor& theDI, bool theIsExtUnit)
{
    const Handle(ViewBase::UnitManager)& aMgr = ViewBase::UnitManager::Instance();
    switch (m_unitCmd) {
        case CmdUnit_PrintSI: {
            theDI << aMgr->GetSIUnit(m_quantity);
            break;
        }
        case CmdUnit_PrintSystem: {
            theDI << aMgr->GetSystemUnit(m_quantity);
            break;
        }
        case CmdUnit_PrintDisplay: {
            theDI << aMgr->GetDisplayUnit(m_quantity);
            break;
        }
        case CmdUnit_SystemToDisplay: {
            if (theIsExtUnit) {
                theDI << aMgr->Convert(m_value, m_quantStr, aMgr->GetDisplayUnit(m_quantity));
            } else {
                theDI << aMgr->SystemToDisplay(m_quantity, m_value);
            }
            break;
        }
        case CmdUnit_SystemFromDisplay: {
            if (theIsExtUnit) {
                theDI << aMgr->Convert(m_value, m_quantStr, aMgr->GetSystemUnit(m_quantity));
            } else {
                theDI << aMgr->SystemFromDisplay(m_quantity, m_value);
            }
            break;
        }
        case CmdUnit_SystemToSI: {
            if (theIsExtUnit) {
                theDI << aMgr->Convert(m_value, m_quantStr, aMgr->GetSIUnit(m_quantity));
            } else {
                theDI << aMgr->SystemToSI(m_quantity, m_value);
            }
            break;
        }
        case CmdUnit_SystemFromSI: {
            if (theIsExtUnit) {
                theDI << aMgr->Convert(m_value, m_quantStr, aMgr->GetSystemUnit(m_quantity));
            } else {
                theDI << aMgr->SystemFromSI(m_quantity, m_value);
            }
            break;
        }
        default: break;
    }
}

}

