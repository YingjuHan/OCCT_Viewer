#ifndef _SCVMESHTESTGROUP_HEADERFILE
#define _SCVMESHTESTGROUP_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for test groups generating within CAE mesh.
class ScvMeshTestGroup : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshtestgroup";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshtestgroup prsName {-nodal|-elemental} {-id name} {-nbGroups n}
Generate test groups for mesh.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshTestGroup();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Return element's center.
    bool ElementCenter(const Handle(RWCae::MeshData)&         theMeshData,
                       const RWCae::MeshData::IndexedElement& theElem,
                       Graphic3d_Vec3&                          theCenter);

    //! Generate group.
    int GenerateGroup(const Graphic3d_Vec3& thePos, const Bnd_Box& theBox)
    {
        double aSize = Sqrt(theBox.SquareExtent());
        double aPos  = theBox.CornerMin().Distance(gp_Pnt(thePos.x(), thePos.y(), thePos.z()));
        return (int)(aPos / aSize * (m_nbGroups + 1)) - 1;
    }

private:
    TCollection_AsciiString m_id;
    bool                    m_isElemental = true;
    int                     m_nbGroups    = 0;
};

}

#endif // _SCVMESHTESTGROUP_HEADERFILE

