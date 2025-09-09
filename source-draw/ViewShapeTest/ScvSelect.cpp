#include <ViewShapeTest/ScvSelect.h>

#include <ViewShape/InteractiveShape.h>

#include <Message.hxx>
#include <ViewerTest.hxx>

namespace ViewShapeTest {

// =======================================================================
// function : ParseIndexList
// =======================================================================
bool ScvSelect::ParseIndexList(Standard_Integer& theArgIter, ViewShape::ShapeSelElemType theType)
{
    Handle(TColStd_HPackedMapOfInteger)& aMap = m_selElemList.ChangeIndices(theType);

    aMap = new TColStd_HPackedMapOfInteger();
    if (theArgIter + 1 >= NbArguments()) {
        return false;
    }

    while (theArgIter + 1 < NbArguments()) {
        int anIndex = 0;
        if (!Draw::ParseInteger(Argument(theArgIter + 1), anIndex)) {
            return !aMap->Map().IsEmpty();
        }
        aMap->ChangeMap().Add(anIndex);
        ++theArgIter;
    }
    return true;
}

// =======================================================================
// function : ScvSelect
// =======================================================================
ScvSelect::ScvSelect()
{
    AddOnOffNoKey(this, "-clear") = &m_toClearPrevSelected;
    AddVoidKey(this, "-faces")    = [](ScvSelect* theCmd, int& theArgIter) -> bool {
        return theCmd->ParseIndexList(theArgIter, ViewShape::ShapeSelElemType::Face);
    };
    AddVoidKey(this, "-edges") = [](ScvSelect* theCmd, int& theArgIter) -> bool {
        return theCmd->ParseIndexList(theArgIter, ViewShape::ShapeSelElemType::Edge);
    };
    AddVoidKey(this, "-verts") = [](ScvSelect* theCmd, int& theArgIter) -> bool {
        return theCmd->ParseIndexList(theArgIter, ViewShape::ShapeSelElemType::Vertex);
    };
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvSelect::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }
    if (m_prsName.IsEmpty() || !m_selElemList.IsValid()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvSelect::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        //
    } else if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);
        Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
        if (!ViewBase::InteractiveOwner::FindPrsOwnerById(aShapeOwner, *m_context, m_prsName)) {
            *myDI << "Syntax error: object '" << m_prsName << "' is not InteractiveShape\n";
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
bool ScvSelect::Perform()
{
    if (m_toClearPrevSelected) {
        m_context->ClearSelected(false);
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    ViewBase::InteractiveOwner::FindPrsOwnerById(aShapeOwner, *m_context, m_prsName);
    m_context->SetSelectedState(aShapeOwner, true);
    aShapeOwner->SetSelectedElems(m_selElemList);
    m_context->MainSelector()->Clear();
    return true;
}

}

