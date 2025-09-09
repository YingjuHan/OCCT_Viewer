
#ifndef RWSHAPETEST_SCVLOAD_HEADERFILE
#define RWSHAPETEST_SCVLOAD_HEADERFILE

#include <DrawBase/DrawCommand.h>

#include <RWShapeTest/DrawableShapeData.h>

namespace RWShapeTest {

//! Command for loading deferred data in shape.
class ScvLoad : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvload";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvload prsName1 [prsName2]...[prsNameN] [-async 0|1]=1 [-multi 0|1]=1
Load deferred data.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvLoad();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    NCollection_List<Handle(RWShapeTest::DrawableShapeData)> m_loadList;

    bool m_isAsync = true;
    bool m_isMulti = true;
};

}

#endif // RWSHAPETEST_SCVLOAD_HEADERFILE