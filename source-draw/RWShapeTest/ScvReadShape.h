
#ifndef RWSHAPETEST_SCVREADSHAPE_HEADERFILE
#define RWSHAPETEST_SCVREADSHAPE_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <RWShapeTest/DrawableShapeData.h>

#include <RWShapeConv/ShapeLoader.h>

namespace RWShapeTest {

//! Command command for reading shape file.
class ScvReadShape : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvreadshape";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvreadshape prsName model.glb [-explode {0|1}]=0 [-depth ExplodeDepth]=-1
             [-propagateId {0|1}]=1 [-genId {0|1}]=1 [-load {0|1}]=0 [-singlePrec {0|1}]=0
             [-display {0|1}]=-1
Read shape from .gltf or .brep file.
 -explode     explode shape into subshapes
 -depth       exploding depth
 -load        load deferred data
 -genId       generate unique ids or use initial ones in case of existing attributes
 -propagateId generate id depending on parents in case of existing attributes
 -singlePrec  force conversion into single-precision from BREP file
 -display     display shape in the viewer
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvReadShape();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Parse argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    Handle(TCollection_HAsciiString) m_prsName;
    TCollection_AsciiString          m_filePath;

    RWShapeConv::ShapeLoader::ExplodeParams m_expParams;

    int m_toDisplay = -1;

    bool m_toLoad     = false;
    bool m_isAsync    = true;
    bool m_isMulti    = true;
    bool m_isSingPrec = false;
};

}

#endif // RWSHAPETEST_SCVREADSHAPE_HEADERFILE