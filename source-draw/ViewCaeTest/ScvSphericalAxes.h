#ifndef _SCVSPHERICALAXES_HEADERFILE
#define _SCVSPHERICALAXES_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewCae/InteractiveSphericalAxes.h>

namespace ViewCaeTest {

using AngularAxesParams = ViewCae::InteractiveSphericalAxes::AngularAxesParams;
using AxisType          = ViewCae::InteractiveSphericalAxes::AxisType;
using ArrowOrientation  = ViewCae::InteractiveSphericalAxes::ArrowOrientation;

//! Command for displaying a spherical coordinate system.
class ScvSphericalAxes : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvsphericalaxes";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvsphericalaxes name [-pos X Y Z]={0 0 0} [-scale S]=1.0
                      [-orientation {OX|OY|OZ}]=OZ
                      [-angular label1 orientation1 attach1 label2 orientation2 attach2]=Theta +Y OZ Phi +Z OX
Displays a spherical coordinate system at specified position
with a specified scale.
The following options can be used to customize the presentation:
    -orientation : orient the longitude and latitude circles along one of the principal axes.
    -angular     : override the parameters of two angular axes (text labels, orientations and attachment axes),
                   where orientation is defined as rotation axis with a sign, e.g. +Z or -X.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvSphericalAxes();

    //! Process all arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process arguments.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Parse the orientation option
    bool parseOrientation(const char* theAxis);

    //! Parse the parameters of two angular axes
    bool parseAngularAxes(std::vector<AngularAxesParams>& theParams, int& theArgIter);

    TCollection_AsciiString                    m_prsName;                         //!< Presentation name.
    gp_XYZ                                     m_position{0.0, 0.0, 0.0};         //!< Position to apply to axes.
    double                                     m_scale       = 1.0;               //!< Scale to apply to axes.
    AxisType                                   m_orientation = AxisType::Unknown; //!< Circles orientation axis
    std::vector<AngularAxesParams>             m_angularParams;                   //!< Parameters of angular axes
    Handle(ViewCae::InteractiveSphericalAxes) m_axesObj;                         //!< The axes object.
};

}

#endif // _SCVSPHERICALAXES_HEADERFILE