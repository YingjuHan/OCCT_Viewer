
#include <RWShapeTest/ScvLoad.h>

namespace RWShapeTest {

// =======================================================================
// function : ScvLoad
// =======================================================================
ScvLoad::ScvLoad()
{
    AddOnOffNoKey(this, "-async") = &m_isAsync;
    AddOnOffNoKey(this, "-multi") = &m_isMulti;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvLoad::ParseAllArguments()
{
    if (!DrawCommand::ParseAllArguments()) {
        return false;
    }
    if (m_loadList.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }

    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvLoad::ParseArgument(int& theArgIter)
{
    if (DrawCommand::ParseArgument(theArgIter)) {
        return true;
    }

    Handle(RWShapeTest::DrawableShapeData) aDrawable = Handle(RWShapeTest::DrawableShapeData)::DownCast(
        Draw::GetExisting(Argument(theArgIter)));
    if (aDrawable.IsNull()) {
        *myDI << "Syntax error: object '" << Argument(theArgIter) << "' is not a Shape Data\n";
        return false;
    }

    m_loadList.Append(aDrawable);
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvLoad::Perform()
{
    return RWShapeTest::DrawableShapeData::LoadData(m_loadList, m_isMulti, m_isAsync);
}

}