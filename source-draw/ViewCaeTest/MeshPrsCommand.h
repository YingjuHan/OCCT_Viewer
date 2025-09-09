
#ifndef _MESHPRSCOMMAND_HEADERFILE
#define _MESHPRSCOMMAND_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewCae/InteractiveCaeMesh.h>

namespace ViewCaeTest {

//! Command for changing CAE mesh presentation.
class MeshPrsCommand : public ViewBaseTest::ViewerCommand {
public:
    //! Constructor.
    Standard_EXPORT MeshPrsCommand();

    //! Destructor.
    Standard_EXPORT ~MeshPrsCommand();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Update mesh and redisplay it if needed.
    Standard_EXPORT void UpdateMesh();

protected:
    TCollection_AsciiString              m_prsName;
    Handle(ViewCae::InteractiveCaeMesh) m_mesh;

    int                     m_prsIndex = 0;
    TCollection_AsciiString m_propId;
};

}

#endif // _MESHPRSCOMMAND_HEADERFILE
