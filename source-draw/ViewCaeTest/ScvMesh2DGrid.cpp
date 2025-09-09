
#include <ViewCaeTest/ScvMesh2DGrid.h>

namespace ViewCaeTest {

// =======================================================================
// function : ScvMesh2DGrid
// =======================================================================
ScvMesh2DGrid::ScvMesh2DGrid()
{
    AddVoidKey(this, "-on") = [](ScvMesh2DGrid* theCmd, int&) -> bool {
        theCmd->m_showGrid = true;
        return true;
    };
    AddVoidKey(this, "-off") = [](ScvMesh2DGrid* theCmd, int&) -> bool {
        theCmd->m_showGrid = false;
        return true;
    };
    AddKey(this, "", "-xname") = [](ScvMesh2DGrid* theCmd, int&, const char* theVal) -> bool {
        theCmd->m_nameX = theVal;
        return true;
    };
    AddKey(this, "", "-yname") = [](ScvMesh2DGrid* theCmd, int&, const char* theVal) -> bool {
        theCmd->m_nameY = theVal;
        return true;
    };
    AddVoidKey(this, "-xticks") = [](ScvMesh2DGrid* theCmd, int& theArgIter) -> bool {
        return theCmd->fillTicksByArray(theCmd->m_ticksX, theCmd->m_ticksModeX, theArgIter);
    };
    AddVoidKey(this, "-yticks") = [](ScvMesh2DGrid* theCmd, int& theArgIter) -> bool {
        return theCmd->fillTicksByArray(theCmd->m_ticksY, theCmd->m_ticksModeY, theArgIter);
    };
    AddKey(this, Graphic3d_Vec3i(), "-xticksrange") = [](ScvMesh2DGrid* theCmd, int&, Graphic3d_Vec3i theVal) -> bool {
        return theCmd->fillTicksByRange(theCmd->m_ticksX, theCmd->m_ticksModeX, theVal);
    };
    AddKey(this, Graphic3d_Vec3i(), "-yticksrange") = [](ScvMesh2DGrid* theCmd, int&, Graphic3d_Vec3i theVal) -> bool {
        return theCmd->fillTicksByRange(theCmd->m_ticksY, theCmd->m_ticksModeY, theVal);
    };
}

// ================================================================
// Function : validateRangeDescription
// ================================================================
bool ScvMesh2DGrid::validateRangeDescription(const TickRangeDesc& theDesc)
{
    if (theDesc.Start == theDesc.End)
        return false;

    if (theDesc.Start < theDesc.End && theDesc.Step <= 0)
        return false;

    if (theDesc.Start > theDesc.End && theDesc.Step >= 0)
        return false;

    return true;
}

// ================================================================
// Function : isFlag
// ================================================================
bool ScvMesh2DGrid::isFlag(const TCollection_AsciiString& theArg) const
{
    TCollection_AsciiString anArgCase(theArg);
    anArgCase.LowerCase();
    // These flags are not added as keys but instead have custom parsing.
    if (anArgCase == "-xticks" || anArgCase == "-yticks")
        return true;

    return myKeyMap.IsBound(anArgCase);
}

// ================================================================
// Function : fillTicksByArray
// ================================================================
bool ScvMesh2DGrid::fillTicksByArray(std::vector<std::string>& theTargetTicks,
                                     TicksSettingMode&         theTargetMode,
                                     int&                      theArgIter)
{
    // Return false if the ticks on this axis have been already set.
    if (theTargetMode != TicksSettingMode::Undetermined) {
        Message::SendFail() << "Error: conflicting arguments.";
        return false;
    }

    for (int anIter = theArgIter + 1; anIter < NbArguments(); anIter++) {
        TCollection_AsciiString anArgTick(Argument(anIter));
        if (isFlag(anArgTick))
            break;

        theTargetTicks.push_back(anArgTick.ToCString());
    }

    if (theTargetTicks.empty())
        return false;

    theArgIter += static_cast<int>(theTargetTicks.size());
    theTargetMode = TicksSettingMode::ByArray;
    return true;
}

// ================================================================
// Function : fillTicksByRange
// ================================================================
bool ScvMesh2DGrid::fillTicksByRange(std::vector<std::string>& theTicks,
                                     TicksSettingMode&         theTargetMode,
                                     const TickRangeDesc&      theDesc)
{
    if (theTargetMode != TicksSettingMode::Undetermined) {
        Message::SendFail() << "Error: conflicting arguments.";
        return false;
    }

    if (ScvMesh2DGrid::validateRangeDescription(theDesc) == false)
        return false;

    const int aStart = theDesc.Start;
    const int anEnd  = theDesc.End;
    const int aStep  = theDesc.Step;

    const auto isInRange = [](int theStart, int theEnd, int theValue) -> bool {
        if (theStart > theEnd)
            std::swap(theStart, theEnd);

        return theValue >= theStart && theValue <= theEnd;
    };

    for (int aValue = aStart; isInRange(aStart, anEnd, aValue); aValue += aStep) {
        theTicks.push_back(std::to_string(aValue).c_str());
    }

    theTargetMode = TicksSettingMode::ByRange;
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMesh2DGrid::Perform()
{
    m_mesh->SetShow2DGrid(m_showGrid);

    ViewCae::InteractiveCaeMesh::Axes2DNamingParams aParams{m_nameX, m_nameY, m_ticksX, m_ticksY};
    m_mesh->Set2DAxesNaming(aParams);

    UpdateMesh();
    return true;
}

}
