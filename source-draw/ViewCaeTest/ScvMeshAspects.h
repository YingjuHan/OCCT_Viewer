#ifndef _SCVMESHVISPROPS_HEADERFILE
#define _SCVMESHVISPROPS_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for changing CAE mesh visual attributes.
class ScvMeshAspects : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshaspects";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshaspects prsName {-prs index}=0
               [-prsMode {meshEdges|shaded|shadedEdges|shrink|shadedFreeEdges|shadedSharpEdges}]
               [-transparency value] [-shadingColor color] [-edgesColor color] [-pointsColor color]
               [-edgesWidth width] [-pointsSize size]
               [-volumeAsSurface {0|1}] [-visible {0|1}]
Set mesh visual properties.
 -prsMode         switch presentation mode
 -transparency    adjust default transparency
 -shadingColor    adjust default shading color
 -edgesColor      adjust default edges color
 -edgesWidth      adjust edges width
 -pointsColor     adjust default point color
 -pointsSize      adjust point size
 -volumeAsSurface switch drawing of internal volume mesh parts
 -visible         switch presentation visibility

scvmeshaspects prsName {-prs index}=0
               [{-elements|+elements}] [{-0d|+0d}] [{-2d|+2d}] [{-3d|+3d}]
               [{-centroids|+centroids}]
               [{-nodes|+nodes}] [{-freeNodes|+freeNodes}
               [{-freeEdges|+freeEdges} [{-sharpEdges|+sharpEdges}]
Switch displaying of certain elements of mesh.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshAspects();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Parse presentation mode.
    bool ParsePrsMode(const char* theStr);

    //! Parse element types argument.
    bool ParseMeshElemTypes(const char* theArg);

private:
    Quantity_Color m_shadingColor;
    Quantity_Color m_edgeColor;
    Quantity_Color m_pointColor;

    bool m_hasShadingColor = false;
    bool m_hasEdgeColor    = false;
    bool m_hasPointColor   = false;

    double m_transparency = -1.0;
    double m_pointSize    = -1.0;
    double m_edgeWidth    = -1.0;
    int    m_volumeAsSurf = -1;
    int    m_prsMode      = -1;

    int m_toShowPrs = -1;

    unsigned int m_elemTypesToEnable  = 0;
    unsigned int m_elemTypesToDisable = 0;
};

}

#endif // _SCVMESHVISPROPS_HEADERFILE
