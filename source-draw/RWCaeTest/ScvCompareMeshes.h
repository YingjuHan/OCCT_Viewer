
#ifndef RWCAETEST_SCVCOMPAREMESHES_HEADERFILE
#define RWCAETEST_SCVCOMPAREMESHES_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <RWCae/MeshData.h>
#include <RWCae/MeshNodeVecProperty.h>

namespace RWCaeTest {

//! Command for CAE meshes comparing.
class ScvCompareMeshes : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvcomparemeshes";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvcomparemeshes prsName1 prsName2
Compare two CAE meshes.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvCompareMeshes();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

protected:
    //! Compare mesh nodes.
    Standard_EXPORT void compareMeshNodes(const Handle(RWCae::MeshData)& theMeshData1,
                                          const Handle(RWCae::MeshData)& theMeshData2) const;

    //! Compare mesh elements.
    Standard_EXPORT void compareMeshElements(const Handle(RWCae::MeshData)& theMeshData1,
                                             const Handle(RWCae::MeshData)& theMeshData2) const;

    //! Compare mesh elements.
    template<class Elem_T>
    Standard_EXPORT bool compareMeshElements(const NCollection_Vector<Elem_T>& theElem1,
                                             const NCollection_Vector<Elem_T>& theElem2) const;

    //! Compare mesh properties.
    Standard_EXPORT void compareMeshProps(const Handle(RWCae::MeshNVecProperty)& theProp1,
                                          const Handle(RWCae::MeshNVecProperty)& theProp2) const;

private:
    TCollection_AsciiString m_prsName1;
    TCollection_AsciiString m_prsName2;
};

}

#endif // RWCAETEST_SCVCOMPAREMESHES_HEADERFILE
