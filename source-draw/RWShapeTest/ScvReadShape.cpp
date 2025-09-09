
#include <RWShapeTest/ScvReadShape.h>

#include <RWShapeConv/ShapeLoader.h>

#include <RWBase/EmulatedFileSystem.h>

#include <ViewBase/UnitManager.h>

#include <BRep_Tool.hxx>
#include <DBRep.hxx>
#include <Message.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <ViewerTest.hxx>

namespace RWShapeTest {

// =======================================================================
// function : ScvReadShape
// =======================================================================
ScvReadShape::ScvReadShape()
{
    AddOnOffNoKey(this, "-async")   = &m_isAsync;
    AddOnOffNoKey(this, "-multi")   = &m_isMulti;
    AddOnOffNoKey(this, "-explode") = &m_expParams.toExplode;

    AddRangeKey(this, -1, IntegerLast(), "-depth") = &m_expParams.explodeDepth;

    AddOnOffNoKey(this, "-propagateid") = &m_expParams.toPropagateId;
    AddOnOffNoKey(this, "-genid")       = &m_expParams.toGenerateId;
    AddOnOffNoKey(this, "-load")        = &m_toLoad;

    AddOnOffNoKey(this, "-singleprec", "-singleprecision") = &m_isSingPrec;

    AddOnOffNoKey(this, "-display") = [](ScvReadShape* theCmd, bool theVal) -> bool {
        theCmd->m_toDisplay = theVal ? 1 : 0;
        if (theVal && ViewerTest::GetAISContext().IsNull()) {
            theCmd->Interpretor() << "Syntax error: vinit wasn't called\n";
            return false;
        }
        return true;
    };
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvReadShape::ParseAllArguments()
{
    if (!DrawCommand::ParseAllArguments()) {
        return false;
    }

    if (m_prsName.IsNull() || m_filePath.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }

    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvReadShape::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        return true;
    }

    const char* anArg = Argument(theArgIter);
    if (m_prsName.IsNull()) {
        m_prsName = new TCollection_HAsciiString(anArg);
    } else if (m_filePath.IsEmpty() && anArg[0] != '\0') {
        m_filePath = anArg;
    } else {
        return false;
    }

    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvReadShape::Perform()
{
    // remove previous object with the same name
    RemoveObject(m_prsName->String());

    Handle(OSD_FileSystem) aFileSystem = RWBase::EmulatedFileSystem::Create(m_filePath);
    if (aFileSystem.IsNull()) {
        return false;
    }

    const Handle(ViewBase::UnitManager)& aUnitMgr = ViewBase::UnitManager::Instance();

    RWShapeConv::ShapeLoader aLoader;
    aLoader.SetExplodeParams(m_expParams);
    aLoader.SetSystemLengthUnitScale(aUnitMgr->SystemToSI(ViewBase::QuantityType::Length, 1.0));
    aLoader.SetForceSinglePrecision(m_isSingPrec);

    NCollection_List<Handle(RWShape::ShapeData)> aShapeList;
    aLoader.OpenShape(aShapeList, m_prsName->String(), aFileSystem, m_filePath.ToCString());
    if (aShapeList.IsEmpty()) {
        Message::SendFail() << "Error: unable to open shape '" << m_filePath << "'";
        return false;
    }

    NCollection_List<Handle(RWShapeTest::DrawableShapeData)> aResList;
    for (const Handle(RWShape::ShapeData)& anObjIter : aShapeList) {
        if (anObjIter.IsNull()) {
            continue;
        }

        *myDI << anObjIter->Id() << " "; // print read objects
        RemoveObject(anObjIter->Id());

        Handle(RWShapeTest::DrawableShapeData) aDrawable = new RWShapeTest::DrawableShapeData(anObjIter);
        Draw::Set(anObjIter->Id().ToCString(), aDrawable);
        aResList.Append(aDrawable);
    }

    if (m_toLoad) {
        RWShapeTest::DrawableShapeData::LoadData(aResList, m_isMulti, m_isAsync);
    }
    if (m_toDisplay != -1) {
        Message_Messenger::StreamBuffer anInfo = Message::SendInfo();
        for (const Handle(RWShapeTest::DrawableShapeData)& aDrawIter : aResList) {
            aDrawIter->Display(m_toDisplay == 1 ? PrsMgr_DisplayStatus_Displayed : PrsMgr_DisplayStatus_Erased);
            anInfo << "Mesh displayed as '" << aDrawIter->ShapeData()->Id() << "'\n";
            aDrawIter->PrintInfo(anInfo.Stream());
        }
    } else {
        m_autoUpdater.Invalidate();
    }
    return true;
}

}