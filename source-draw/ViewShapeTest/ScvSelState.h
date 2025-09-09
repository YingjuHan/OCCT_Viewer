
#ifndef SCPRSSHAPETEST_SCVSELSTATE_HEADERFILE
#define SCPRSSHAPETEST_SCVSELSTATE_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewShape/InteractiveShapeOwner.h>

namespace ViewShapeTest {

//! Command for printing selection state.
class ScvSelState : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvselstate";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvselstate [prsName {-groups|-compSolids|-solids|-faces|-wires|-edges|-verts}]
            [-detected {0|1}]=0
Print state of given object's detected/selected local elements or
state of all detected/selected objects and their elements in viewer.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvSelState();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Print detected.
    void PrintDetected(Draw_Interpretor& theDI) const;

    //! Print information about detected shape owner.
    void PrintDetectedShapeOwner(Draw_Interpretor&                                theDI,
                                 const Handle(ViewShape::InteractiveShapeOwner)& theShapeOwner) const;

    //! Print selected entities.
    void PrintSelected(Draw_Interpretor& theDI) const;

    //! Print information about selected shape owner.
    void PrintSelectedShapeOwner(Draw_Interpretor&                                theDI,
                                 const Handle(ViewShape::InteractiveShapeOwner)& theShapeOwner) const;

    //! Print indexes list.
    void PrintIndexList(Draw_Interpretor& theDI, const Handle(TColStd_HPackedMapOfInteger)& theIds) const;

private:
    TCollection_AsciiString m_prsName;

    bool         m_toPrintDetected = false;
    unsigned int m_toPrintSelModes = 0;
};

}

#endif // SCPRSSHAPETEST_SCVSELSTATE_HEADERFILE