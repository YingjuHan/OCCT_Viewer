
#ifndef _SCVMESH2DGRID_HEADERFILE
#define _SCVMESH2DGRID_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for displaying/hiding CAE mesh annotations.
class ScvMesh2DGrid : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmesh2dgrid";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmesh2dgrid mesh [-on|-off]=-on [-xname NameX] [-yname NameY]
              [-xticks start end step] [-yticks start end step]
Enable/disable displaying of 2d coordinate grid tied to the mesh.
  -xname NameX                 name of OX axis
  -yname NameY                 name of OY axis
  -xticks tick1 [tick2...]     tick strings for OX axis
  -yticks tick1 [tick2...]     tick strings for OY axis
  -xticksrange start end step  integer description of the range of ticks
                               present on X axis (e.g. 0 10 1)
  -yticksrange start end step  integer description of ticks on Y axis.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMesh2DGrid();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Representation of tick range and step.
    //! E.g. if it is (1, 3, 1), the resulting axis ticks will be { 1, 2, 3 }.
    struct TickRangeDesc {
        TickRangeDesc() {}
        TickRangeDesc(const Graphic3d_Vec3i& theDesc)
        {
            Start = theDesc.x();
            End   = theDesc.y();
            Step  = theDesc.z();
        }
        int Start = 0;
        int End   = 0;
        int Step  = 0;
    };

    //! Signifies the mode by which an axis' ticks can be set.
    enum class TicksSettingMode {
        Undetermined, //!< Axis ticks have not been set yet.
        ByRange,      //!< Axis ticks have been set by a range description.
        ByArray       //!< Axis ticks have been set literally by an array of strings.
    };

    //! Fills an array with ticks taken straight from command arguments until the next flag.
    bool fillTicksByArray(std::vector<std::string>& theTargetTicks, TicksSettingMode& theTargetMode, int& theArgIter);

    //! Fills an array with ticks according to tick range description.
    static bool fillTicksByRange(std::vector<std::string>& theTicks,
                                 TicksSettingMode&         theTargetMode,
                                 const TickRangeDesc&      theDesc);

    //! Validates range description from arguments.
    static bool validateRangeDescription(const TickRangeDesc& theDesc);

    //! Checks whether an argument is a flag that this command recognizes.
    bool isFlag(const TCollection_AsciiString& theArg) const;

private:
    bool             m_showGrid = true; //!< Controls whether to show 2D coordinate grid or not.
    std::string      m_nameX;           //!< Name of X axis.
    std::string      m_nameY;           //!< Name of Y axis.
    TicksSettingMode m_ticksModeX = TicksSettingMode::Undetermined; //!< The mode with which the OX ticks are set.
    TicksSettingMode m_ticksModeY = TicksSettingMode::Undetermined; //!< The mode with which the OY ticks are set.
    std::vector<std::string> m_ticksX;                              //!< Tick strings for X axis.
    std::vector<std::string> m_ticksY;                              //!< Tick strings for Y axis.
};

}

#endif // _SCVMESH2DGRID_HEADERFILE