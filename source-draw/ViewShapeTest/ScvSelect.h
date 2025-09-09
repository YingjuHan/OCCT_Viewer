#ifndef SCPRSSHAPETEST_SCVSELECT_HEADERFILE
#define SCPRSSHAPETEST_SCVSELECT_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewShape/ShapeSelElems.h>

namespace ViewShapeTest {

//! Command performing selection.
class ScvSelect : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvselect";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvselect prsName [-faces id1 id2 .. idN] [-edges id1 id2 .. idN]
                  [-verts id1 id2 .. idN] [-clear 0|1]=0
Select object or its local elements.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvSelect();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Process index list.
    bool ParseIndexList(Standard_Integer& theArgIter, ViewShape::ShapeSelElemType theType);

private:
    TCollection_AsciiString   m_prsName;
    ViewShape::ShapeSelElems m_selElemList;

    bool m_toClearPrevSelected = false;
};

}

#endif // SCPRSSHAPETEST_SCVSELECT_HEADERFILE
