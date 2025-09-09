
#include <RWCaeTest/MeshIOCommand.h>

namespace RWCaeTest {

// =======================================================================
// function : ParseTimeRange
// =======================================================================
bool MeshIOCommand::ParseTimeRange(int& theArgIter, RWCae::MeshBinIOBase::TimeRange& theTR)
{
    static constexpr int cmdTRArgCnt = 3;
    if (theArgIter + cmdTRArgCnt >= NbArguments()) {
        return true;
    }

    TCollection_AsciiString anArgRange(Argument(++theArgIter));
    anArgRange.LowerCase();
    if (anArgRange == "-timesteprange") {
        if (!Draw::ParseInteger(Argument(++theArgIter), theTR.firstTimeStep)
            || !Draw::ParseInteger(Argument(++theArgIter), theTR.secondTimeStep)) {
            *myDI << "Syntax error: Incorrect time range: not integer\n";
            return false;
        }
        if (theTR.firstTimeStep > theTR.secondTimeStep) {
            *myDI << "Syntax error: Incorrect time range: second time is less than the first one\n";
            return false;
        }
    } else {
        --theArgIter;
    }
    return true;
}

// =======================================================================
// function : MeshIOCommand
// =======================================================================
MeshIOCommand::MeshIOCommand()
{
    AddVoidKey(this, "+geom") = [](MeshIOCommand* theCmd) -> bool {
        theCmd->m_options &= ~(unsigned int)RWCae::MeshDataImportExportOptions::SKIP_GEOMETRY;
        return true;
    };
    AddVoidKey(this, "-geom") = [](MeshIOCommand* theCmd) -> bool {
        theCmd->m_options |= (unsigned int)RWCae::MeshDataImportExportOptions::SKIP_GEOMETRY;
        return true;
    };
    AddVoidKey(this, "+props") = [](MeshIOCommand* theCmd) -> bool {
        theCmd->m_options &= ~(unsigned int)RWCae::MeshDataImportExportOptions::SKIP_PROPERTIES;
        return true;
    };
    AddVoidKey(this, "-props") = [](MeshIOCommand* theCmd) -> bool {
        theCmd->m_options |= (unsigned int)RWCae::MeshDataImportExportOptions::SKIP_PROPERTIES;
        return true;
    };

    AddKey(this, "", "+propbyid") = [](MeshIOCommand* theCmd, int& theArgIter, const char* theVal) -> bool {
        theCmd->m_options &= ~(unsigned int)RWCae::MeshDataImportExportOptions::SKIP_PROPERTIES;
        RWCae::MeshBinIOBase::TimeRange aTR;
        if (!theCmd->ParseTimeRange(theArgIter, aTR)) {
            return false;
        }

        if (theCmd->m_props.IsNull()) {
            theCmd->m_props = new RWCae::MeshBinIOBase::SharedMapOfAsciiString();
        }
        theCmd->m_props->Bind(theVal, aTR);
        return true;
    };
    AddKey(this, bool(), "-parallel") = [](MeshIOCommand* theCmd, bool theVal) -> bool {
        if (theVal) {
            theCmd->m_options |= (int)RWCae::MeshDataImportExportOptions::PARALLEL_PROCESSING;
        } else {
            theCmd->m_options &= ~(int)RWCae::MeshDataImportExportOptions::PARALLEL_PROCESSING;
        }
        return true;
    };
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool MeshIOCommand::ParseAllArguments()
{
    if (!DrawCommand::ParseAllArguments()) {
        return false;
    }
    if (m_prsName.IsEmpty() || m_fileName.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

}
