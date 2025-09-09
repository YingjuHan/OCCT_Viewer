#include <ViewShapeTest/ScvDisplay.h>

#include <ViewShape/InteractiveShape.h>

#include <RWShapeTest/DrawableShapeData.h>

#include <DBRep.hxx>
#include <ViewerTest.hxx>

namespace ViewShapeTest {

// =======================================================================
// function : ScvDisplay
// =======================================================================
ScvDisplay::ScvDisplay()
{
    AddKey(this, "", "-prsname") = [](ScvDisplay* theCmd, const char* theVal) -> bool {
        theCmd->m_prsName = theVal;
        return true;
    };
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvDisplay::ParseAllArguments()
{
    if (!ViewerCommand::ParseAllArguments()) {
        return false;
    }
    if (m_names.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvDisplay::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        return true;
    }

    const char* aName = Argument(theArgIter);

    Handle(Draw_Drawable3D) aDrawable = Draw::GetExisting(aName);

    ViewBaseTest::ViewerDrawable* aDisp = dynamic_cast<ViewBaseTest::ViewerDrawable*>(aDrawable.get());
    if (aDisp == nullptr && DBRep::GetExisting(aName).IsNull()) {
        *myDI << "Syntax error: empty shape '" << aName << "'\n";
        return false;
    }

    m_names.Append(aName);
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvDisplay::Perform()
{
    Handle(ViewShape::InteractiveShapeListOwner) aShapeList;
    if (!m_prsName.IsEmpty()) {
        aShapeList = new ViewShape::InteractiveShapeListOwner();
    }

    for (const TCollection_AsciiString& aNameIter : m_names) {
        Handle(Draw_Drawable3D) aDrawableOld = Draw::GetExisting(aNameIter.ToCString());
        Handle(Draw_Drawable3D) aDrawable    = aDrawableOld;
        if (dynamic_cast<ViewBaseTest::ViewerDrawable*>(aDrawable.get()) == nullptr) {
            const TopoDS_Shape           aShape     = DBRep::GetExisting(aNameIter.ToCString());
            Handle(RWShape::ShapeData) aShapeData = new RWShape::ShapeData(aNameIter, aShape);

            aDrawable = new RWShapeTest::DrawableShapeData(aShapeData);
        }

        if (aDrawable != aDrawableOld) {
            Draw::Set(aNameIter.ToCString(), aDrawable);
        }

        ViewBaseTest::ViewerDrawable* aDisp = dynamic_cast<ViewBaseTest::ViewerDrawable*>(aDrawable.get());
        if (!aShapeList.IsNull()) {
            RWShapeTest::DrawableShapeData* aShapeDrawable = dynamic_cast<RWShapeTest::DrawableShapeData*>(
                aDrawable.get());
            if (aShapeDrawable == nullptr) {
                continue;
            }

            Handle(ViewShape::InteractiveShapeOwner) anOwner = new ViewShape::InteractiveShapeOwner(
                aShapeDrawable->ShapeData());
            aShapeList->AddChild(anOwner);
        } else {
            aDisp->Display(PrsMgr_DisplayStatus_Displayed);
        }
    }

    if (!aShapeList.IsNull()) {
        Handle(ViewShape::InteractiveShape) aPrs = Handle(ViewShape::InteractiveShape)::DownCast(
            aShapeList->CreatePresentation());
        ViewerTest::Display(aShapeList->Id(), aPrs, false);
    }

    return true;
}

}

