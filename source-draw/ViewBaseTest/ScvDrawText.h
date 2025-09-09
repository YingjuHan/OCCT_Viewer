#ifndef ViewBASETEST_SCVDRAWTEXT_HEADERFILE
#define ViewBASETEST_SCVDRAWTEXT_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewBase/InteractiveTextLabel.h>

namespace ViewBaseTest {

//! Command for displaying text label @link ViewBase::InteractiveTextLabel @endlink.
class ScvDrawText : public ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvdrawtext";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvdrawtext name text [-pos X Y Z] [-attachPoint X Y Z] [-color Color] [...]
                      [-drawRect {0|1}]=1 [-rectPadding  Value] [-pointPadding Value]
                      [-attachMarker {empty|.|+|x|*|O|xcircle|pointInCircle
                                     |starInCircle|ImagePath}]=pointInCircle
                      [-attachMarkerScale Value]=1
                      [-icon FilePath.png]
Create and display text label attached to 3D point.
See help vdrawtext for more arguments.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvDrawText();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Parse attach marker argument.
    bool ParseAttachMarker(Standard_CString theMarker);

    //! Handle text label name argument.
    void ParseObjectName(Standard_CString theName);

    //! Set parameters to object.
    int SetParameters();

private:
    static constexpr int m_argi1 = 1;
    static constexpr int m_argi2 = 2;
    static constexpr int m_argi3 = 3;

private:
    TCollection_AsciiString m_args   = "vdrawtext";
    int                     m_nbArgs = 1;

    TCollection_AsciiString                 m_prsName;
    Handle(ViewBase::InteractiveTextLabel) m_prsObject;

    gp_XYZ m_attachPnt = gp_XYZ(RealLast(), RealLast(), RealLast());

    bool   m_noAttachPoint     = false;
    bool   m_attachMarker      = false;
    bool   m_toSetIconImage    = false;
    double m_rectPadding       = -1.0;
    double m_pointPadding      = -1.0;
    int    m_toDrawRect        = -1;
    int    m_attachMarkerScale = -1;

    Aspect_TypeOfMarker  m_attachMarkerType = Aspect_TOM_EMPTY;
    Handle(Image_PixMap) m_attachMarkerImage;

    Handle(ViewBase::ScalablePixMap) m_iconImage;
};

}

#endif // ViewBASETEST_SCVDRAWTEXT_HEADERFILE

