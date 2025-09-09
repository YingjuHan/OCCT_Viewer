
#ifndef SCPRSSHAPETEST_SCVASPECTS_HEADERFILE
#define SCPRSSHAPETEST_SCVASPECTS_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewShape/InteractiveShape.h>

namespace ViewShapeTest {

//! Command for managing presentation aspects of interactive object.
class ScvAspects : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvaspects";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvaspects prsName [-prsMode {wireframe|smoothShading|edgeShading|flatShading|hiddenLine}]
                   [-forcedColor Color][-noForcedColor]
                   [-forcedTransparency Value][-noForcedTransparency]
                   [-resetMaterial] [-resetSubcolors]
                   [-baseColor RgbaColor] [-metallic Value] [-roughness Value]
                   [-emission  Color] [-refractionIndex Value]
                   [-infinite {0|1}]=0
Manage presentation aspects.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvAspects();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Parse presentation mode.
    bool ParsePrsMode(const char* theName);

private:
    //! Apply aspects to the presentation.
    void ProcessShape(const Handle(ViewShape::InteractiveShape)& theObj);

    //! Apply PBR material properties.
    void ApplyPbrMaterial(const Handle(ViewShape::InteractiveShapeOwner)& theOwner);

    //! PBR properties.
    void AddPbrKeys();

    //! Forced keys.
    void AddForcedKeys();

private:
    AIS_ListOfInteractive m_prsList;

    Graphic3d_Vec3 m_forcedColor  = Graphic3d_Vec3(ShortRealLast());
    double         m_forcedTransp = RealLast();

    Graphic3d_Vec4 m_baseColor       = Graphic3d_Vec4(ShortRealLast());
    Graphic3d_Vec3 m_emissiveColor   = Graphic3d_Vec3(ShortRealLast());
    double         m_metallic        = RealLast();
    double         m_roughness       = RealLast();
    double         m_refractionIndex = RealLast();
    bool           m_resetMaterial   = false;
    bool           m_unsetSubcolors  = false;

    int m_shapeMode  = -1;
    int m_isInfinite = -1;
    int m_nbModif    = 0;
};

}

#endif // SCPRSSHAPETEST_SCVASPECTS_HEADERFILE
