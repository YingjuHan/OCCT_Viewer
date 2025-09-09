

#include <RWShapeTest/ScvExplode.h>

#include <RWShapeTest/DrawableShapeData.h>

#include <ViewShape/InteractiveShape.h>

#include <Message.hxx>
#include <ViewerTest.hxx>

namespace RWShapeTest {

// =======================================================================
// function : ScvExplode
// =======================================================================
ScvExplode::ScvExplode()
{
    AddRangeKey(this, -1, IntegerLast(), "-depth") = &m_expParams.explodeDepth;

    AddOnOffNoKey(this, "-propagateid") = &m_expParams.toPropagateId;
    AddOnOffNoKey(this, "-genid")       = &m_expParams.toGenerateId;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvExplode::ParseAllArguments()
{
    if (!DrawCommand::ParseAllArguments()) {
        return false;
    }
    if (m_prsName.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments\n";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvExplode::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        return true;
    }
    if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);

        Handle(RWShapeTest::DrawableShapeData) aDrawable = Handle(RWShapeTest::DrawableShapeData)::DownCast(
            Draw::GetExisting(m_prsName.ToCString()));
        if (aDrawable.IsNull()) {
            Message::SendFail() << "Error: object '" << m_prsName
                                << "' doesn't exist or isn't shape and cannot be exploded";
            return false;
        }
        return true;
    }
    return false;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvExplode::Perform()
{
    Handle(RWShapeTest::DrawableShapeData) aDrawOld = Handle(RWShapeTest::DrawableShapeData)::DownCast(
        Draw::GetExisting(m_prsName.ToCString()));

    const Handle(AIS_InteractiveContext)& aCtx = ViewerTest::GetAISContext();
    Handle(ViewShape::InteractiveShape)  aPrevObj;
    const bool                            isPrevVisible = !aCtx.IsNull()
                            && ViewBase::IInteractiveObject::FindObjectById(aPrevObj, *aCtx, m_prsName)
                            && aPrevObj->DisplayStatus() == PrsMgr_DisplayStatus_Displayed;
    if (!isPrevVisible) {
        m_autoUpdater.Invalidate();
    }

    NCollection_List<Handle(RWShape::ShapeData)> aShapes;
    RWShapeConv::ShapeLoader::ExplodeShape(aShapes, aDrawOld->ShapeData(), m_expParams);
    if (aShapes.IsEmpty()) {
        Message::SendFail() << "Error: unable to explode shape '" << m_prsName << "'";
        return false;
    }

    // Remove previous object with the same name
    RemoveObject(m_prsName);

    NCollection_List<Handle(ViewShape::InteractiveShape)> aResObjects;
    for (const Handle(RWShape::ShapeData)& aShapeIter : aShapes) {
        if (aShapeIter.IsNull()) {
            continue;
        }

        const TCollection_AsciiString& aName = aShapeIter->Id();
        *myDI << aName << " "; // print exploded objects
        RemoveObject(aName);

        Handle(RWShapeTest::DrawableShapeData) aDrawable = new RWShapeTest::DrawableShapeData(aShapeIter);
        Draw::Set(aName.ToCString(), aDrawable);
        if (!aPrevObj.IsNull()) {
            aDrawable->Display(isPrevVisible ? PrsMgr_DisplayStatus_Displayed : PrsMgr_DisplayStatus_Erased, aPrevObj);
        }
    }
    return true;
}

}