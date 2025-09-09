

#include <ViewBaseTest/ViewerCommand.h>

#include <Message.hxx>
#include <ViewerTest.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>

Standard_IMPORT ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();

namespace ViewBaseTest {

IMPLEMENT_STANDARD_RTTIEXT(ViewerCommand, DrawCommand)

// =======================================================================
// function : ViewerCommand
// =======================================================================
ViewerCommand::ViewerCommand()
    : m_autoUpdater(ViewerTest::GetAISContext(), ViewerTest::CurrentView()),
      m_context(ViewerTest::GetAISContext())
{
    //
}

// =======================================================================
// function : ~ViewerCommand
// =======================================================================
ViewerCommand::~ViewerCommand()
{
    //
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ViewerCommand::ParseAllArguments()
{
    if (m_context.IsNull()) {
        *myDI << "Syntax error: vinit wasn't called";
        return false;
    }
    return DrawCommand::ParseAllArguments();
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ViewerCommand::ParseArgument(int& theArgIter)
{
    if (m_autoUpdater.parseRedrawMode(Argument(theArgIter))) {
        return true;
    }
    return DrawCommand::ParseArgument(theArgIter);
}

// =======================================================================
// function : RemoveObject
// =======================================================================
bool ViewerCommand::RemoveObject(const TCollection_AsciiString& theName, const bool theToUpdate)
{
    if (theName.IsEmpty()) {
        return false;
    }

    Draw::Set(theName.ToCString(), Handle(Draw_Drawable3D)());
    if (ViewerTest::GetAISContext().IsNull()) {
        return false;
    }

    ViewerTest_DoubleMapOfInteractiveAndName& aMap = GetMapOfAIS();
    if (aMap.IsBound2(theName)) {
        if (Handle(AIS_InteractiveObject) anOldObj = aMap.Find2(theName)) {
            ViewerTest::GetAISContext()->Remove(anOldObj, theToUpdate);
        }
        aMap.UnBind2(theName);
    }
    return true;
}

// =======================================================================
// function : RegisterObject
// =======================================================================
bool ViewerCommand::RegisterObject(const TCollection_AsciiString&       theName,
                                   const Handle(AIS_InteractiveObject)& theObject,
                                   const bool                           theToUpdate,
                                   const bool                           theReplaceIfExists)
{
    if (ViewerTest::GetAISContext().IsNull()) {
        return false;
    }

    ViewerTest_DoubleMapOfInteractiveAndName& aMap = GetMapOfAIS();
    Handle(AIS_InteractiveObject)             anOldObj;
    if (aMap.Find2(theName, anOldObj)) {
        if (!theReplaceIfExists) {
            Message::SendFail() << "Error: other interactive object has been already registered with name: " << theName
                                << ".\n"
                                << "Please use another name.";
            return false;
        }

        ViewerTest::GetAISContext()->Remove(anOldObj, theObject.IsNull() && theToUpdate);
        aMap.UnBind2(theName);
        anOldObj.Nullify();
    }

    if (theObject.IsNull()) {
        // object with specified name has been already unbound
        return true;
    }

    // unbind AIS object if it was bound with another name
    aMap.UnBind1(theObject);

    // can be registered without rebinding
    aMap.Bind(theObject, theName);
    return true;
}

}

