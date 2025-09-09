
#ifndef SCPRSSHAPETEST_SCVSELMODE_HEADERFILE
#define SCPRSSHAPETEST_SCVSELMODE_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewShape/InteractiveShape.h>

namespace ViewShapeTest {

//! Command for switching shape selection modes.
class ScvSelmode : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvselmode";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvselmode prsName [-global]
                   [-compSolid] [-solid] [-shell] [-face] [-wire] [-edge] [-vertex]
                   [-groups [<Compound1> ... <CompoundN>]]
Set selection mode to shape object.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvSelmode();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

    //! Process groups arguments.
    Standard_EXPORT bool ParseGroups(int& theArgIter);

private:
    NCollection_List<Handle(ViewShape::InteractiveShapeOwner)> m_shapeList;

    std::vector<TopoDS_Compound> m_groups;

    int m_shapeSelMode = (int)ViewShape::InteractiveShapeSelMode::Global;
};

}

#endif // SCPRSSHAPETEST_SCVSELMODE_HEADERFILE
