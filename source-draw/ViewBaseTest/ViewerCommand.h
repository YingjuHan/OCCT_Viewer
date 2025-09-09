#ifndef ViewBASETEST_VIEWERCOMMAND_HEADERFILE
#define ViewBASETEST_VIEWERCOMMAND_HEADERFILE

#include <DrawBase/DrawCommand.h>

#include <ViewerTest_AutoUpdater.hxx>

namespace ViewBaseTest {

//! Command for managing 3D viewer objects.
class ViewerCommand : public DrawBase::DrawCommand {
    DEFINE_STANDARD_RTTIEXT(ViewerCommand, DrawCommand)
public:
    //! Remove object from context and common map.
    Standard_EXPORT static bool RemoveObject(const TCollection_AsciiString& theName, const bool theToUpdate = false);

    //! Add object to context and common map.
    Standard_EXPORT static bool RegisterObject(const TCollection_AsciiString&       theName,
                                               const Handle(AIS_InteractiveObject)& theObject,
                                               const bool                           theToUpdate        = false,
                                               const bool                           theReplaceIfExists = true);

public:
    //! Constructor.
    Standard_EXPORT ViewerCommand();

    //! Destructor.
    Standard_EXPORT virtual ~ViewerCommand();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

protected:
    ViewerTest_AutoUpdater         m_autoUpdater;
    Handle(AIS_InteractiveContext) m_context;
};

}

#endif // ViewBASETEST_VIEWERCOMMAND_HEADERFILE

