#include <ViewBaseTest/ScvClone.h>

#include <ViewBase/InteractiveOwner.h>

#include <ViewerTest.hxx>

namespace ViewBaseTest {

// =======================================================================
// function : ScvClone
// =======================================================================
ScvClone::ScvClone()
{
    //
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvClone::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }

    if (m_oldName.IsEmpty() || m_newName.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    } else if (m_newName == m_oldName) {
        *myDI << "Syntax error: new name should be different";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvClone::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_oldName.IsEmpty()) {
        m_oldName = Argument(theArgIter);
    } else if (m_newName.IsEmpty()) {
        m_newName = Argument(theArgIter);
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvClone::Perform()
{
    Handle(ViewBase::InteractiveOwner) anOldOwner =
        ViewBase::InteractiveOwner::FindPrsOwnerById(*ViewerTest::GetAISContext(), m_oldName);
    if (anOldOwner.IsNull()) {
        *myDI << "Syntax error: object '" << m_oldName << "' not found or of unsupported type";
        return false;
    }

    Handle(ViewBase::InteractiveOwner) aClone = anOldOwner->Clone(m_newName);
    if (aClone.IsNull()) {
        *myDI << "Error: non-clonable object";
        return false;
    }

    Handle(AIS_InteractiveObject) aPrs = aClone->CreatePresentation();
    if (aPrs.IsNull()) {
        *myDI << "Error: non-displayable object";
        return false;
    }

    ViewerTest::Display(m_newName, aPrs, false);
    return true;
}

}

