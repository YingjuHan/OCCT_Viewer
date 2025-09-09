#ifndef _SCVCOLORSCALE_HEADERFILE
#define _SCVCOLORSCALE_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewCae/InteractiveCaeMesh.h>
#include <ViewCae/InteractiveColorScale.h>

namespace ViewCaeTest {

//! Command for displaying colorscale.
class ScvColorScale : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvcolorscale";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvcolorscale prsName {[-attach meshName] | [-attachProp meshName propId componentIndex]}
                      [-colorRange colorMin colorMax] [-colors color1..colorN]
                      [labels lab1 lab2..labN] [skipEmptyLabels {0|1}]
                      [-colorOutOfRange color]
                      [-smooth on|off] [-nbIntervals value]
                      [-xy left bottom] [-size width height]
                      [-font value] [-textPos left|right]
Create and adjust color scale object.
  -attach will add this object to the list of not yet used color scales.
    This should be called before any active property is set.
  -attachProp will replace the already attached color scale with this one.
    This should be called after the specified property was set.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvColorScale();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Parse -colors argument.
    bool ParseColorList(int& theArgIter);

    //! Handle -attachprop argument.
    bool ParseAttachProperty(int& theArgIter);

    //! Handle colorscale name argument.
    void ParseColorscaleName(Standard_CString theName);

    //! Helper function to attach color scale to mesh.
    void AttachColorScale();

private:
    //! Set custom colors to colorscale.
    bool SetCustomColors(Draw_Interpretor& theDI);

    //! Print information about colorscale.
    void PrintParams(Draw_Interpretor& theDI) const;

private:
    TCollection_AsciiString m_args   = "vcolorscale";
    int                     m_nbArgs = 1;

    TCollection_AsciiString                 m_prsName;
    Handle(ViewCae::InteractiveColorScale) m_colorScale;
    Handle(ViewCae::InteractiveCaeMesh)    m_mesh;
    RWCae::MeshPropertyComponent          m_prop;

    Quantity_ColorRGBA m_colorOutOfRange;
    bool               m_hasColorOutOfRange = false;

    NCollection_Sequence<Quantity_ColorRGBA> m_customColors;

    int m_nbIntervals       = -1;
    int m_toSkipEmptyLabels = -1;
    int m_isHorizontal      = -1;
    int m_isLogarithmic     = -1;
};

}

#endif // _SCVCOLORSCALE_HEADERFILE
