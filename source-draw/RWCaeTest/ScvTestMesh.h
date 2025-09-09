
#ifndef RWCAETEST_SCVTESTMESH_HEADERFILE
#define RWCAETEST_SCVTESTMESH_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <RWCae/MeshData.h>

namespace RWCaeTest {

//! Command for generating test CAE mesh.
class ScvTestMesh : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvtestmesh";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvtestmesh prsName [-display {0|1}]=-1 [-hexCube nbNodes] [-setCube nbNodes]
                    [-testCacheGeom {0|1}]=0
Generate sample mesh.
By default it generates a set of mesh elements for all available types.
[-hexCube nbNodes] generate (nbNodes-1)^2 hexahedrons in cube form.
[-setCube nbNodes] generate mix of (nbNodes-1)^2 tetrahedrons/pyramids/prisms/hexahedrons in cube form.
[-testCacheGeom]   toggle usage of test custom geometry for mesh presentations.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvTestMesh();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process arguments.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    static constexpr double m_size5  = 5;
    static constexpr double m_size10 = 10;
    static constexpr double m_step15 = 15;

private:
    //! Generate set of different mesh elements.
    void GenElemsSet(const Handle(RWCae::MeshData)& theMeshData);

    //! Generate cube object consisting of tetrahedron/pyramid/prism/hexahedron mesh elements.
    void GenSetCube(const Handle(RWCae::MeshData)& theMeshData, bool theOnlyHex);

    //! Create simple triangular elements.
    void AddTris(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder);

    //! Create simple quad element.
    void AddQuad(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder);

    //! Create simple polygonal element.
    void AddPolygon(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos);

    //! Create simple tetrahedron element.
    void AddTetrahedron(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder);

    //! Create simple pyramid element.
    void AddPyramid(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder);

    //! Create simple prism element.
    void AddPrism(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder);

    //! Create simple hex element.
    void AddHex(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder);

    //! Create simple polyhedral element.
    void AddPolyhedron(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos);

private:
    enum class Type { ElemsSet, HexCube, SetCube };

    TCollection_AsciiString m_prsName;

    int  m_toDisplay        = -1;
    Type m_type             = Type::ElemsSet;
    int  m_cubeNbNodes      = 10;
    bool m_testCacheGenGeom = false;
};

}

#endif // RWCAETEST_SCVTESTMESH_HEADERFILE

