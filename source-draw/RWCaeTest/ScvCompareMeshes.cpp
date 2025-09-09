#include <RWCaeTest/ScvCompareMeshes.h>

#include <RWCaeTest/DrawableMeshData.h>

#include <Message.hxx>

namespace RWCaeTest {

// =======================================================================
// function : ScvCompareMeshes
// =======================================================================
ScvCompareMeshes::ScvCompareMeshes() : DrawBase::DrawCommand()
{
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvCompareMeshes::ParseArgument(int& theArgIter)
{
    if (DrawCommand::ParseArgument(theArgIter)) {
        return true;
    }

    if (m_prsName1.IsEmpty()) {
        m_prsName1 = Argument(theArgIter);

        Handle(RWCaeTest::DrawableMeshData) aDrawable = Handle(RWCaeTest::DrawableMeshData)::DownCast(
            Draw::GetExisting(m_prsName1.ToCString()));
        if (aDrawable.IsNull()) {
            Message::SendFail() << "Error: object '" << m_prsName1 << "' doesn't exist or isn't CAE mesh";
            return false;
        }
    } else if (m_prsName2.IsEmpty()) {
        m_prsName2 = Argument(theArgIter);

        Handle(RWCaeTest::DrawableMeshData) aDrawable = Handle(RWCaeTest::DrawableMeshData)::DownCast(
            Draw::GetExisting(m_prsName2.ToCString()));
        if (aDrawable.IsNull()) {
            Message::SendFail() << "Error: object '" << m_prsName2 << "' doesn't exist or isn't CAE mesh";
            return false;
        }
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvCompareMeshes::Perform()
{
    Handle(RWCaeTest::DrawableMeshData) aDrawable1 = Handle(RWCaeTest::DrawableMeshData)::DownCast(
        Draw::GetExisting(m_prsName1.ToCString()));
    Handle(RWCaeTest::DrawableMeshData) aDrawable2 = Handle(RWCaeTest::DrawableMeshData)::DownCast(
        Draw::GetExisting(m_prsName2.ToCString()));

    const Handle(RWCae::MeshData)& aMeshData1 = aDrawable1->MeshData();
    const Handle(RWCae::MeshData)& aMeshData2 = aDrawable2->MeshData();

    int aNbNodes1 = (int)aMeshData1->NbNodes();
    int aNbNodes2 = (int)aMeshData2->NbNodes();

    int aNbElems1 = (int)aMeshData1->NbIndexedElems();
    int aNbElems2 = (int)aMeshData2->NbIndexedElems();

    if (aNbNodes1 != aNbNodes2 || aNbElems1 != aNbElems2) {
        *myDI << "Meshes have different geometry\n";
        *myDI << "1: " << aNbNodes1 << " nodes, " << aNbElems1 << " elements\n";
        *myDI << "2: " << aNbNodes2 << " nodes, " << aNbElems2 << " elements\n";
        return true;
    }

    if (aNbNodes1 > 0) {
        compareMeshNodes(aMeshData1, aMeshData2);
    }

    if (aNbElems1 > 0) {
        compareMeshElements(aMeshData1, aMeshData2);
    }

    for (const Handle(RWCae::MeshNVecProperty)& aProp1 : aMeshData1->Properties()) {
        for (const Handle(RWCae::MeshNVecProperty)& aProp2 : aMeshData2->Properties()) {
            if (aProp1->Name() != aProp2->Name() || aProp1->Description() != aProp2->Description()
                || aProp1->MapType() != aProp2->MapType() || aProp1->ComponentsType() != aProp2->ComponentsType()
                || aProp1->NbRawComponents() != aProp2->NbRawComponents() || aProp1->TimeStep() != aProp2->TimeStep()) {
                continue;
            }

            compareMeshProps(aProp1, aProp2);
            break;
        }
    }

    return true;
}

// =======================================================================
// function : compareMeshNodes
// =======================================================================
void ScvCompareMeshes::compareMeshNodes(const Handle(RWCae::MeshData)& theMeshData1,
                                        const Handle(RWCae::MeshData)& theMeshData2) const
{
    const Bnd_Box& aBox1    = theMeshData1->BoundingBox();
    const Bnd_Box& aBox2    = theMeshData2->BoundingBox();
    gp_XYZ         aBoxSize = aBox1.CornerMax().XYZ() - aBox1.CornerMin().XYZ();
    *myDI << "Box 1: (" << aBox1.CornerMin().X() << ", " << aBox1.CornerMin().Y() << ", " << aBox1.CornerMin().Z()
          << "), (" << aBox1.CornerMax().X() << ", " << aBox1.CornerMax().Y() << ", " << aBox1.CornerMax().Z() << ")\n";
    *myDI << "Box 2: (" << aBox2.CornerMin().X() << ", " << aBox2.CornerMin().Y() << ", " << aBox2.CornerMin().Z()
          << "), (" << aBox2.CornerMax().X() << ", " << aBox2.CornerMax().Y() << ", " << aBox2.CornerMax().Z() << ")\n";
    float aMaxDistX = 0.0f, aMaxDistY = 0.0f, aMaxDistZ = 0.0f, aMaxMagn = 0.0f;
    for (RWCae::MeshEntityId i = 0; i < theMeshData1->NbNodes(); i++) {
        const Graphic3d_Vec3& aNode1 = theMeshData1->Nodes()[i];
        const Graphic3d_Vec3& aNode2 = theMeshData2->Nodes()[i];
        float                 aDist  = Abs(aNode1.x() - aNode2.x());
        if (aDist > aMaxDistX) {
            aMaxDistX = aDist;
        }
        aDist = Abs(aNode1.y() - aNode2.y());
        if (aDist > aMaxDistY) {
            aMaxDistY = aDist;
        }
        aDist = Abs(aNode1.z() - aNode2.z());
        if (aDist > aMaxDistZ) {
            aMaxDistZ = aDist;
        }
        aDist = (aNode1 - aNode2).Modulus();
        if (aDist > aMaxDistZ) {
            aMaxMagn = aDist;
        }
    }

    *myDI << "Max X diff: abs = " << aMaxDistX
          << ", rel = " << (aBoxSize.X() > RealSmall() ? aMaxDistX / aBoxSize.X() : 0.0) << "\n";
    *myDI << "Max Y diff: abs = " << aMaxDistY
          << ", rel = " << (aBoxSize.Y() > RealSmall() ? aMaxDistY / aBoxSize.Y() : 0.0) << "\n";
    *myDI << "Max Z diff: abs = " << aMaxDistZ
          << ", rel = " << (aBoxSize.Z() > RealSmall() ? aMaxDistZ / aBoxSize.Z() : 0.0) << "\n";
    *myDI << "Max Magnitude diff: abs = " << aMaxMagn
          << ", rel = " << (aBoxSize.Modulus() > RealSmall() ? aMaxMagn / aBoxSize.Modulus() : 0.0) << "\n";
}

// =======================================================================
// function : compareMeshElements
// =======================================================================
void ScvCompareMeshes::compareMeshElements(const Handle(RWCae::MeshData)& theMeshData1,
                                           const Handle(RWCae::MeshData)& theMeshData2) const
{
    for (int aType = 0; aType < (int)RWCae::MeshElemNbTypes; aType++) {
        const RWCae::MeshElements& anElems1 = theMeshData1->Elements((RWCae::MeshElemType)aType);
        const RWCae::MeshElements& anElems2 = theMeshData2->Elements((RWCae::MeshElemType)aType);

        bool elemsAreSame = compareMeshElements(anElems1.NodesOffsets(), anElems2.NodesOffsets())
                         && compareMeshElements(anElems1.Nodes(), anElems2.Nodes())
                         && compareMeshElements(anElems1.FacesOffsets(), anElems2.FacesOffsets())
                         && compareMeshElements(anElems1.Faces(), anElems2.Faces())
                         && compareMeshElements(anElems1.GlobalIndices(), anElems2.GlobalIndices());
        if (!elemsAreSame) {
            *myDI << "Meshes have different elements of type " << aType << "\n";
        }
    }
}

// =======================================================================
// function : compareMeshElements
// =======================================================================
template<class Elem_T>
bool ScvCompareMeshes::compareMeshElements(const NCollection_Vector<Elem_T>& theElem1,
                                           const NCollection_Vector<Elem_T>& theElem2) const
{
    if (theElem1.Size() != theElem2.Size()) {
        return false;
    }

    for (int i = 0; i < theElem1.Size(); i++) {
        if (theElem1[i] != theElem2[i]) {
            return false;
        }
    }
    return true;
}

// =======================================================================
// function : compareMeshProps
// =======================================================================
void ScvCompareMeshes::compareMeshProps(const Handle(RWCae::MeshNVecProperty)& theProp1,
                                        const Handle(RWCae::MeshNVecProperty)& theProp2) const
{
    bool isNodeVec = theProp1->HasNodeVecComponents();

    *myDI << "Comparing " << (theProp1->IsNodal() ? "nodal" : "elemental") << " property '" << theProp1->Name()
          << "'\n";
    for (int aCompIter = 0; aCompIter < theProp1->NbRawComponents(); aCompIter++) {
        const Handle(RWCae::MeshScalarPropertyBase)& aComp1 = theProp1->GetRawComponent(aCompIter);
        const Handle(RWCae::MeshScalarPropertyBase)& aComp2 = theProp2->GetRawComponent(aCompIter);
        *myDI << "  Component '" << aComp1->Name() << "'\n";

        double aMin1 = 0.0, aMax1 = 0.0, aMin2 = 0.0, aMax2 = 0.0;
        aComp1->ValueRange().GetBounds(aMin1, aMax1);
        aComp2->ValueRange().GetBounds(aMin2, aMax2);
        *myDI << "    1: Min = " << aMin1 << ", Max = " << aMax1 << "\n";
        *myDI << "    2: Min = " << aMin2 << ", Max = " << aMax2 << "\n";

        double aMaxError = 0.0;

        for (int aValIter = 0; aValIter < aComp1->NbValues(); aValIter++) {
            std::vector<int> anIndices;
            if (isNodeVec) {
                Handle(RWCae::MeshNodeVecProperty)::DownCast(aComp1)->SubIndices(aValIter, anIndices);
            } else {
                anIndices.push_back(-1);
            }

            for (int anIndex : anIndices) {
                RWCae::MeshPropertyEntityInfo anInfo(aValIter, anIndex);

                double aValue1 = aComp1->Value(anInfo);
                double aValue2 = aComp2->Value(anInfo);
                if (RWCae::IsNaN(aValue1) != RWCae::IsNaN(aValue2)) {
                    aMaxError = RealLast();
                } else if (!RWCae::IsNaN(aValue1) && !RWCae::IsNaN(aValue2) && Abs(aValue1 - aValue2) > aMaxError) {
                    aMaxError = Abs(aValue1 - aValue2);
                }
            }
        }

        *myDI << "    Max error: abs = " << aMaxError
              << ", rel = " << (aMax1 - aMin1 > RealSmall() ? aMaxError / (aMax1 - aMin1) : 0.0) << "\n";
    }
}

}
