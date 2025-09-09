
#include <RWCaeTest/ScvTestMesh.h>

#include <RWCaeTest/DrawableMeshData.h>

#include <ViewCae/InteractiveCaeMesh.h>

#include <ViewerTest.hxx>

#include <array>

namespace RWCaeTest {

// =======================================================================
// function : ScvTestMesh
// =======================================================================
ScvTestMesh::ScvTestMesh()
{
    AddOnOffNoKey(this, "-display") = [](ScvTestMesh* theCmd, bool theVal) -> bool {
        theCmd->m_toDisplay = theVal ? 1 : 0;
        if (theVal && ViewerTest::GetAISContext().IsNull()) {
            theCmd->Interpretor() << "Syntax error: vinit wasn't called\n";
            return false;
        }
        return true;
    };

    AddKey(this, int(), "-hexcube") = [](ScvTestMesh* theCmd, int theVal) -> bool {
        if (theVal < 2) {
            return false;
        }
        theCmd->m_type        = Type::HexCube;
        theCmd->m_cubeNbNodes = theVal;
        return true;
    };

    AddKey(this, int(), "-setcube") = [](ScvTestMesh* theCmd, int theVal) -> bool {
        if (theVal < 2) {
            return false;
        }
        theCmd->m_type        = Type::SetCube;
        theCmd->m_cubeNbNodes = theVal;
        return true;
    };
    AddOnOffNoKey(this, "-testcachegeom") = &m_testCacheGenGeom;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvTestMesh::ParseAllArguments()
{
    if (!DrawCommand::ParseAllArguments()) {
        return false;
    }
    if (m_prsName.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvTestMesh::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        return true;
    }
    if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);
        return true;
    }
    return false;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvTestMesh::Perform()
{
    // remove previous object with the same name
    RemoveObject(m_prsName);

    Handle(RWCae::MeshData) aMesh = new RWCae::MeshData(m_prsName);

    switch (m_type) {
        case Type::ElemsSet: GenElemsSet(aMesh); break;
        case Type::HexCube: GenSetCube(aMesh, true); break;
        case Type::SetCube: GenSetCube(aMesh, false); break;
        default: return false;
    }

    Handle(RWCaeTest::DrawableMeshData) aDrawable = new RWCaeTest::DrawableMeshData(aMesh);
    Draw::Set(m_prsName.ToCString(), aDrawable);
    aDrawable->SetTestCacheGenGeom(m_testCacheGenGeom);
    if (m_toDisplay == -1) {
        return true;
    }

    Handle(ViewCae::InteractiveCaeMeshOwner) anOwner  = new ViewCae::InteractiveCaeMeshOwner(aMesh);
    Handle(ViewCae::InteractiveCaeMesh)      aMeshPrs = new ViewCae::InteractiveCaeMesh(anOwner);
    aMeshPrs->MeshOwner()->SetUseTestCachePrs(m_testCacheGenGeom);
    aMeshPrs->CreatePresentation(ViewCae::MeshPresentationType::Scalar);
    aMeshPrs->ProcessRedraw(m_context, ViewerTest::CurrentView(), AIS_RedrawProgress_BeforeUpdate);
    if (m_toDisplay == 1) {
        ViewerTest::Display(m_prsName, aMeshPrs);
        for (const Handle(PrsMgr_PresentableObject)& aChildIter : aMeshPrs->Children()) {
            Handle(AIS_InteractiveObject) aChildObj = Handle(AIS_InteractiveObject)::DownCast(aChildIter);
            m_context->Display(aChildObj, false);
        }
    } else {
        RegisterObject(m_prsName, aMeshPrs);
    }
    return true;
}

// =======================================================================
// function : GenElemsSet
// =======================================================================
void ScvTestMesh::GenElemsSet(const Handle(RWCae::MeshData)& theMeshData)
{
    const gp_XYZ aStep(0.0, m_step15, 0.0);
    const gp_XYZ aStep2ndOrder(0.0, 0.0, m_step15);
    gp_XYZ       anOrig;
    {
        RWCae::MeshElement0d anElem0;
        anElem0[0] = theMeshData->AddNodePosition(anOrig);
        theMeshData->AddElement(anElem0);
    }
    anOrig += aStep;
    AddTris(theMeshData, anOrig, false);
    AddTris(theMeshData, anOrig + aStep2ndOrder, true);
    anOrig += aStep;
    AddQuad(theMeshData, anOrig, false);
    AddQuad(theMeshData, anOrig + aStep2ndOrder, true);
    anOrig += aStep;
    AddPolygon(theMeshData, anOrig);
    anOrig += aStep;
    AddTetrahedron(theMeshData, anOrig, false);
    AddTetrahedron(theMeshData, anOrig + aStep2ndOrder, true);
    anOrig += aStep;
    AddPyramid(theMeshData, anOrig, false);
    AddPyramid(theMeshData, anOrig + aStep2ndOrder, true);
    anOrig += aStep;
    AddPrism(theMeshData, anOrig, false);
    AddPrism(theMeshData, anOrig + aStep2ndOrder, true);
    anOrig += aStep;
    AddHex(theMeshData, anOrig, false);
    AddHex(theMeshData, anOrig + aStep2ndOrder, true);
    anOrig += aStep;
    AddPolyhedron(theMeshData, anOrig);
    anOrig += aStep;
    {
        RWCae::MeshElement0d anElem0;
        anElem0[0] = theMeshData->AddNodePosition(anOrig);
        theMeshData->AddElement(anElem0);
    }
    anOrig += aStep;
}

// =======================================================================
// function : GenSetCube
// =======================================================================
void ScvTestMesh::GenSetCube(const Handle(RWCae::MeshData)& theMeshData, bool theOnlyHex)
{
    for (int i = 0; i < m_cubeNbNodes; i++) {
        for (int j = 0; j < m_cubeNbNodes; j++) {
            for (int k = 0; k < m_cubeNbNodes; k++) {
                theMeshData->AddNodePosition(Graphic3d_Vec3((float)i / (m_cubeNbNodes - 1),
                                                            (float)j / (m_cubeNbNodes - 1),
                                                            (float)k / (m_cubeNbNodes - 1)));
            }
        }
    }

    auto getNodeId = [aNbNodes = m_cubeNbNodes](int theI, int theJ, int theK) {
        return theI * aNbNodes * aNbNodes + theJ * aNbNodes + theK;
    };

    for (int i = 0; i < m_cubeNbNodes - 1; i++) {
        for (int j = 0; j < m_cubeNbNodes - 1; j++) {
            for (int k = 0; k < m_cubeNbNodes - 1; k++) {
                int aCubeId = i * m_cubeNbNodes * m_cubeNbNodes + j * m_cubeNbNodes + k;
                if ((aCubeId % 2 == 0) || theOnlyHex) {
                    RWCae::MeshElementHexahedron aHex;
                    aHex[0] = getNodeId(i + 0, j + 0, k + 0);
                    aHex[1] = getNodeId(i + 0, j + 0, k + 1);
                    aHex[2] = getNodeId(i + 0, j + 1, k + 1);
                    aHex[3] = getNodeId(i + 0, j + 1, k + 0);
                    aHex[4] = getNodeId(i + 1, j + 0, k + 0);
                    aHex[5] = getNodeId(i + 1, j + 0, k + 1);
                    aHex[6] = getNodeId(i + 1, j + 1, k + 1);
                    aHex[7] = getNodeId(i + 1, j + 1, k + 0);
                    theMeshData->AddElement(aHex);
                } else {
                    RWCae::MeshElementPrism aPrism;
                    aPrism[0] = getNodeId(i + 0, j + 0, k + 0);
                    aPrism[1] = getNodeId(i + 0, j + 1, k + 1);
                    aPrism[2] = getNodeId(i + 0, j + 0, k + 1);
                    aPrism[3] = getNodeId(i + 1, j + 0, k + 0);
                    aPrism[4] = getNodeId(i + 1, j + 1, k + 1);
                    aPrism[5] = getNodeId(i + 1, j + 0, k + 1);
                    theMeshData->AddElement(aPrism);

                    RWCae::MeshElementPyramid aPyramid;
                    aPyramid[0] = getNodeId(i + 0, j + 0, k + 0);
                    aPyramid[1] = getNodeId(i + 0, j + 1, k + 1);
                    aPyramid[2] = getNodeId(i + 1, j + 1, k + 1);
                    aPyramid[3] = getNodeId(i + 1, j + 0, k + 0);
                    aPyramid[4] = getNodeId(i + 0, j + 1, k + 0);
                    theMeshData->AddElement(aPyramid);

                    RWCae::MeshElementTetrahedron aTetra;
                    aTetra[0] = getNodeId(i + 0, j + 1, k + 0);
                    aTetra[1] = getNodeId(i + 1, j + 1, k + 1);
                    aTetra[2] = getNodeId(i + 1, j + 0, k + 0);
                    aTetra[3] = getNodeId(i + 1, j + 1, k + 0);
                    theMeshData->AddElement(aTetra);
                }
            }
        }
    }
}

// =======================================================================
// function : AddTris
// =======================================================================
void ScvTestMesh::AddTris(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder)
{
    const std::array<gp_XYZ, 3> aNodes = {thePos + gp_XYZ(0.0, 0.0, 0.0),
                                          thePos + gp_XYZ(0.0, m_size10, 0.0),
                                          thePos + gp_XYZ(0.0, m_size10, m_size10)};
    if (!theIs2ndOrder) {
        RWCae::MeshElementTriangle anElem;
        anElem[0] = theMesh->AddNodePosition(aNodes[0]);
        anElem[1] = theMesh->AddNodePosition(aNodes[1]);
        anElem[2] = theMesh->AddNodePosition(aNodes[2]);
        theMesh->AddElement(anElem);
    } else {
        RWCae::MeshElementTriangle2nd anElem;
        anElem[0] = theMesh->AddNodePosition(aNodes[0]);
        anElem[1] = theMesh->AddNodePosition(aNodes[1]);
        anElem[2] = theMesh->AddNodePosition(aNodes[2]);
        anElem[3] = theMesh->AddNodePosition((aNodes[0] + aNodes[1]) * 0.5);
        anElem[4] = theMesh->AddNodePosition((aNodes[1] + aNodes[2]) * 0.5);
        anElem[5] = theMesh->AddNodePosition((aNodes[2] + aNodes[0]) * 0.5);
        theMesh->AddElement(anElem);
    }
}

// =======================================================================
// function : AddQuad
// =======================================================================
void ScvTestMesh::AddQuad(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder)
{
    const std::array<gp_XYZ, 4> aNodes = {thePos + gp_XYZ(0.0, 0.0, 0.0),
                                          thePos + gp_XYZ(0.0, m_size10, 0.0),
                                          thePos + gp_XYZ(0.0, m_size10, m_size10),
                                          thePos + gp_XYZ(0.0, 0.0, m_size10)};
    if (!theIs2ndOrder) {
        RWCae::MeshElementQuad anElem;
        anElem[0] = theMesh->AddNodePosition(aNodes[0]);
        anElem[1] = theMesh->AddNodePosition(aNodes[1]);
        anElem[2] = theMesh->AddNodePosition(aNodes[2]);
        anElem[3] = theMesh->AddNodePosition(aNodes[3]);
        theMesh->AddElement(anElem);
    } else {
        RWCae::MeshElementQuad2nd anElem;
        anElem[0] = theMesh->AddNodePosition(aNodes[0]);
        anElem[1] = theMesh->AddNodePosition(aNodes[1]);
        anElem[2] = theMesh->AddNodePosition(aNodes[2]);
        anElem[3] = theMesh->AddNodePosition(aNodes[3]);
        anElem[4] = theMesh->AddNodePosition((aNodes[0] + aNodes[1]) * 0.5);
        anElem[5] = theMesh->AddNodePosition((aNodes[1] + aNodes[2]) * 0.5);
        anElem[6] = theMesh->AddNodePosition((aNodes[2] + aNodes[3]) * 0.5);
        anElem[7] = theMesh->AddNodePosition((aNodes[3] + aNodes[0]) * 0.5);
        theMesh->AddElement(anElem);
    }
}

// =======================================================================
// function : AddPolygon
// =======================================================================
void ScvTestMesh::AddPolygon(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos)
{
    const std::array<RWCae::MeshEntityId, 5> aPoly = {
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, 0.0, 0.0)),
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, 0.0, m_size10)),
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, m_size5, m_size10)),
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, m_size10, m_size5)),
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, m_size5, 0.0))};
    theMesh->AddElement(
        RWCae::MeshElementPolygon(aPoly.data(), static_cast<RWCae::MeshElemEntityId>(aPoly.size())));
}

// =======================================================================
// function : AddTetrahedron
// =======================================================================
void ScvTestMesh::AddTetrahedron(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder)
{
    const std::array<gp_XYZ, 4> aNodes = {thePos + gp_XYZ(0.0, 0.0, 0.0),
                                          thePos + gp_XYZ(0.0, 0.0, m_size10),
                                          thePos + gp_XYZ(0.0, m_size10, m_size10),
                                          thePos + gp_XYZ(m_size10, m_size5, m_size5)};
    if (!theIs2ndOrder) {
        RWCae::MeshElementTetrahedron anElem;
        anElem[0] = theMesh->AddNodePosition(aNodes[0]);
        anElem[1] = theMesh->AddNodePosition(aNodes[1]);
        anElem[2] = theMesh->AddNodePosition(aNodes[2]);
        anElem[3] = theMesh->AddNodePosition(aNodes[3]);
        theMesh->AddElement(anElem);
    } else {
        RWCae::MeshElementTetrahedron2nd anElem;
        anElem[0] = theMesh->AddNodePosition(aNodes[0]);
        anElem[1] = theMesh->AddNodePosition(aNodes[1]);
        anElem[2] = theMesh->AddNodePosition(aNodes[2]);
        anElem[3] = theMesh->AddNodePosition(aNodes[3]);
        anElem[4] = theMesh->AddNodePosition((aNodes[0] + aNodes[1]) * 0.5);
        anElem[5] = theMesh->AddNodePosition((aNodes[1] + aNodes[2]) * 0.5);
        anElem[6] = theMesh->AddNodePosition((aNodes[2] + aNodes[0]) * 0.5);
        anElem[7] = theMesh->AddNodePosition((aNodes[3] + aNodes[0]) * 0.5);
        anElem[8] = theMesh->AddNodePosition((aNodes[3] + aNodes[1]) * 0.5);
        anElem[9] = theMesh->AddNodePosition((aNodes[3] + aNodes[2]) * 0.5);
        theMesh->AddElement(anElem);
    }
}

// =======================================================================
// function : AddPyramid
// =======================================================================
void ScvTestMesh::AddPyramid(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder)
{
    const std::array<gp_XYZ, 5> aNodes = {thePos + gp_XYZ(0.0, 0.0, 0.0),
                                          thePos + gp_XYZ(0.0, m_size10, 0.0),
                                          thePos + gp_XYZ(0.0, m_size10, m_size10),
                                          thePos + gp_XYZ(0.0, 0.0, m_size10),
                                          thePos + gp_XYZ(m_size10, m_size5, m_size5)};
    if (!theIs2ndOrder) {
        RWCae::MeshElementPyramid anElem;
        anElem[0] = theMesh->AddNodePosition(aNodes[0]);
        anElem[1] = theMesh->AddNodePosition(aNodes[1]);
        anElem[2] = theMesh->AddNodePosition(aNodes[2]);
        anElem[3] = theMesh->AddNodePosition(aNodes[3]);
        anElem[4] = theMesh->AddNodePosition(aNodes[4]);
        theMesh->AddElement(anElem);
    } else {
        RWCae::MeshElementPyramid2nd anElem;
        anElem[0]  = theMesh->AddNodePosition(aNodes[0]);
        anElem[1]  = theMesh->AddNodePosition(aNodes[1]);
        anElem[2]  = theMesh->AddNodePosition(aNodes[2]);
        anElem[3]  = theMesh->AddNodePosition(aNodes[3]);
        anElem[4]  = theMesh->AddNodePosition(aNodes[4]);
        anElem[5]  = theMesh->AddNodePosition((aNodes[0] + aNodes[1]) * 0.5);
        anElem[6]  = theMesh->AddNodePosition((aNodes[1] + aNodes[2]) * 0.5);
        anElem[7]  = theMesh->AddNodePosition((aNodes[2] + aNodes[3]) * 0.5);
        anElem[8]  = theMesh->AddNodePosition((aNodes[3] + aNodes[0]) * 0.5);
        anElem[9]  = theMesh->AddNodePosition((aNodes[4] + aNodes[0]) * 0.5);
        anElem[10] = theMesh->AddNodePosition((aNodes[4] + aNodes[1]) * 0.5);
        anElem[11] = theMesh->AddNodePosition((aNodes[4] + aNodes[2]) * 0.5);
        anElem[12] = theMesh->AddNodePosition((aNodes[4] + aNodes[3]) * 0.5);
        theMesh->AddElement(anElem);
    }
}

// =======================================================================
// function : AddPrism
// =======================================================================
void ScvTestMesh::AddPrism(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder)
{
    const std::array<gp_XYZ, 6> aNodes = {thePos + gp_XYZ(0.0, 0.0, 0.0),
                                          thePos + gp_XYZ(0.0, 0.0, m_size10),
                                          thePos + gp_XYZ(0.0, m_size10, m_size10),
                                          thePos + gp_XYZ(m_size10, 0.0, 0.0),
                                          thePos + gp_XYZ(m_size10, 0.0, m_size10),
                                          thePos + gp_XYZ(m_size10, m_size10, m_size10)};
    if (!theIs2ndOrder) {
        RWCae::MeshElementPrism anElem;
        anElem[0] = theMesh->AddNodePosition(aNodes[0]);
        anElem[1] = theMesh->AddNodePosition(aNodes[1]);
        anElem[2] = theMesh->AddNodePosition(aNodes[2]);
        anElem[3] = theMesh->AddNodePosition(aNodes[3]);
        anElem[4] = theMesh->AddNodePosition(aNodes[4]);
        anElem[5] = theMesh->AddNodePosition(aNodes[5]);
        theMesh->AddElement(anElem);
    } else {
        RWCae::MeshElementPrism2nd anElem;
        anElem[0]  = theMesh->AddNodePosition(aNodes[0]);
        anElem[1]  = theMesh->AddNodePosition(aNodes[1]);
        anElem[2]  = theMesh->AddNodePosition(aNodes[2]);
        anElem[3]  = theMesh->AddNodePosition(aNodes[3]);
        anElem[4]  = theMesh->AddNodePosition(aNodes[4]);
        anElem[5]  = theMesh->AddNodePosition(aNodes[5]);
        anElem[6]  = theMesh->AddNodePosition((aNodes[0] + aNodes[1]) * 0.5);
        anElem[7]  = theMesh->AddNodePosition((aNodes[1] + aNodes[2]) * 0.5);
        anElem[8]  = theMesh->AddNodePosition((aNodes[2] + aNodes[0]) * 0.5);
        anElem[9]  = theMesh->AddNodePosition((aNodes[3] + aNodes[4]) * 0.5);
        anElem[10] = theMesh->AddNodePosition((aNodes[4] + aNodes[5]) * 0.5);
        anElem[11] = theMesh->AddNodePosition((aNodes[5] + aNodes[3]) * 0.5);
        anElem[12] = theMesh->AddNodePosition((aNodes[0] + aNodes[3]) * 0.5);
        anElem[13] = theMesh->AddNodePosition((aNodes[1] + aNodes[4]) * 0.5);
        anElem[14] = theMesh->AddNodePosition((aNodes[2] + aNodes[5]) * 0.5);
        theMesh->AddElement(anElem);
    }
}

// =======================================================================
// function : AddHex
// =======================================================================
void ScvTestMesh::AddHex(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos, bool theIs2ndOrder)
{
    const std::array<gp_XYZ, 8> aNodes = {thePos + gp_XYZ(0.0, 0.0, 0.0),
                                          thePos + gp_XYZ(0.0, m_size10, 0.0),
                                          thePos + gp_XYZ(0.0, m_size10, m_size10),
                                          thePos + gp_XYZ(0.0, 0.0, m_size10),
                                          thePos + gp_XYZ(m_size10, 2.0, 2.0),
                                          thePos + gp_XYZ(m_size10, 8.0, 2.0),
                                          thePos + gp_XYZ(m_size10, 8.0, 8.0),
                                          thePos + gp_XYZ(m_size10, 2.0, 8.0)};
    if (!theIs2ndOrder) {
        RWCae::MeshElementHexahedron anElem;
        anElem[0] = theMesh->AddNodePosition(aNodes[0]);
        anElem[1] = theMesh->AddNodePosition(aNodes[1]);
        anElem[2] = theMesh->AddNodePosition(aNodes[2]);
        anElem[3] = theMesh->AddNodePosition(aNodes[3]);
        anElem[4] = theMesh->AddNodePosition(aNodes[4]);
        anElem[5] = theMesh->AddNodePosition(aNodes[5]);
        anElem[6] = theMesh->AddNodePosition(aNodes[6]);
        anElem[7] = theMesh->AddNodePosition(aNodes[7]);
        theMesh->AddElement(anElem);
    } else {
        RWCae::MeshElementHexahedron2nd anElem;
        anElem[0]  = theMesh->AddNodePosition(aNodes[0]);
        anElem[1]  = theMesh->AddNodePosition(aNodes[1]);
        anElem[2]  = theMesh->AddNodePosition(aNodes[2]);
        anElem[3]  = theMesh->AddNodePosition(aNodes[3]);
        anElem[4]  = theMesh->AddNodePosition(aNodes[4]);
        anElem[5]  = theMesh->AddNodePosition(aNodes[5]);
        anElem[6]  = theMesh->AddNodePosition(aNodes[6]);
        anElem[7]  = theMesh->AddNodePosition(aNodes[7]);
        anElem[8]  = theMesh->AddNodePosition((aNodes[0] + aNodes[1]) * 0.5);
        anElem[9]  = theMesh->AddNodePosition((aNodes[1] + aNodes[2]) * 0.5);
        anElem[10] = theMesh->AddNodePosition((aNodes[2] + aNodes[3]) * 0.5);
        anElem[11] = theMesh->AddNodePosition((aNodes[3] + aNodes[0]) * 0.5);
        anElem[12] = theMesh->AddNodePosition((aNodes[4] + aNodes[5]) * 0.5);
        anElem[13] = theMesh->AddNodePosition((aNodes[5] + aNodes[6]) * 0.5);
        anElem[14] = theMesh->AddNodePosition((aNodes[6] + aNodes[7]) * 0.5);
        anElem[15] = theMesh->AddNodePosition((aNodes[7] + aNodes[4]) * 0.5);
        anElem[16] = theMesh->AddNodePosition((aNodes[0] + aNodes[4]) * 0.5);
        anElem[17] = theMesh->AddNodePosition((aNodes[1] + aNodes[5]) * 0.5);
        anElem[18] = theMesh->AddNodePosition((aNodes[2] + aNodes[6]) * 0.5);
        anElem[19] = theMesh->AddNodePosition((aNodes[3] + aNodes[7]) * 0.5);
        theMesh->AddElement(anElem);
    }
}

// =======================================================================
// function : AddPolyhedron
// =======================================================================
void ScvTestMesh::AddPolyhedron(const Handle(RWCae::MeshData)& theMesh, const gp_XYZ& thePos)
{
    const std::array<RWCae::MeshEntityId, 10> aPoly = {
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, 0.0, 0.0)),
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, 0.0, m_size10)),
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, m_size5, m_size10)),
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, m_size10, m_size5)),
        theMesh->AddNodePosition(thePos + gp_XYZ(0.0, m_size5, 0.0)),
        theMesh->AddNodePosition(thePos + gp_XYZ(m_size5, 0.0, 0.0)),
        theMesh->AddNodePosition(thePos + gp_XYZ(m_size5, 0.0, m_size10)),
        theMesh->AddNodePosition(thePos + gp_XYZ(m_size5, m_size5, m_size10)),
        theMesh->AddNodePosition(thePos + gp_XYZ(m_size5, m_size10, m_size5)),
        theMesh->AddNodePosition(thePos + gp_XYZ(m_size5, m_size5, 0.0))};

    const std::array<RWCae::MeshElemEntityId, 37> aFaces = {
        4, 0, 1, 6, 5,    // face 1
        4, 1, 2, 7, 6,    // face 2
        4, 2, 3, 8, 7,    // face 3
        4, 3, 4, 9, 8,    // face 4
        4, 4, 0, 5, 9,    // face 5
        5, 0, 1, 2, 3, 4, // face 6
        5, 5, 6, 7, 8, 9  // face 7
    };
    theMesh->AddElement(RWCae::MeshElementPolyhedron(aPoly.data(),
                                                       static_cast<int>(aPoly.size()),
                                                       aFaces.data(),
                                                       static_cast<uint16_t>(aFaces.size())));
}

}
