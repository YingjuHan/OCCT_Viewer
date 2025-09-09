#include <RWCaeTest/ScvTestMeshProp.h>

#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <RWCae/MeshTools.h>

#include <RWCaeTest/DrawableMeshData.h>

#include <Message.hxx>

namespace RWCaeTest {

// =======================================================================
// function : ScvTestMeshProp
// =======================================================================
ScvTestMeshProp::ScvTestMeshProp()
{
    AddKey(this, "", "-nodal") = [](ScvTestMeshProp* theCmd, int& theArgIter, const char*) -> bool {
        PropInfo anInfo;
        if (!theCmd->ParsePropInfo(theArgIter, anInfo))
            return false;
        theCmd->m_propsToGen.push_back(std::make_pair(Type::Nodal, anInfo));
        return true;
    };
    AddKey(this, "", "-nodalvector") = [](ScvTestMeshProp* theCmd, int& theArgIter, const char*) -> bool {
        PropInfo anInfo;
        if (!theCmd->ParsePropInfo(theArgIter, anInfo))
            return false;
        theCmd->m_propsToGen.push_back(std::make_pair(Type::NodalVector, anInfo));
        return true;
    };
    AddKey(this, "", "-elemental") = [](ScvTestMeshProp* theCmd, int& theArgIter, const char*) -> bool {
        PropInfo anInfo;
        if (!theCmd->ParsePropInfo(theArgIter, anInfo))
            return false;
        theCmd->m_propsToGen.push_back(std::make_pair(Type::Elemental, anInfo));
        return true;
    };
    AddKey(this, "", "-nodevec") = [](ScvTestMeshProp* theCmd, int& theArgIter, const char*) -> bool {
        PropInfo anInfo;
        if (!theCmd->ParsePropInfo(theArgIter, anInfo))
            return false;
        theCmd->m_propsToGen.push_back(std::make_pair(Type::NodeVec, anInfo));
        return true;
    };
    AddKey(this, "", "-nodalfromelem") = [](ScvTestMeshProp* theCmd, int& theArgIter, const char*) -> bool {
        PropInfo anInfo;
        if (!theCmd->ParsePropInfo(theArgIter, anInfo))
            return false;
        theCmd->m_propsToGen.push_back(std::make_pair(Type::NodalFromElem, anInfo));
        return true;
    };
    AddOnOffNoKey(this, "-sort")  = &m_toSort;
    AddOnOffNoKey(this, "-byloc") = &m_byLoc;
}

// =======================================================================
// function : ParsePropInfo
// =======================================================================
bool ScvTestMeshProp::ParsePropInfo(int& theArgIter, PropInfo& theInfo)
{
    static constexpr int cmdTRArgCnt = 3;
    if (theArgIter + cmdTRArgCnt > NbArguments()) {
        return false;
    }

    theInfo.Id = Argument(theArgIter);
    if (theInfo.Id.IsEmpty()) {
        return false;
    }

    TCollection_AsciiString anArgRange(Argument(++theArgIter));
    anArgRange.LowerCase();
    if (anArgRange == "-time") {
        int aTimeMin = 0;
        int aTimeMax = 0;
        if (!Draw::ParseInteger(Argument(++theArgIter), aTimeMin)
            || !Draw::ParseInteger(Argument(++theArgIter), aTimeMax)) {
            *myDI << "Syntax error: Incorrect time range\n";
            return false;
        }

        if (aTimeMin > aTimeMax) {
            *myDI << "Syntax error: Incorrect time range\n";
            return false;
        }
        for (int aTime = aTimeMin; aTime <= aTimeMax; aTime++) {
            theInfo.TimeSteps.push_back(aTime);
        }
    }

    return true;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvTestMeshProp::ParseAllArguments()
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
bool ScvTestMeshProp::ParseArgument(int& theArgIter)
{
    if (DrawCommand::ParseArgument(theArgIter)) {
        return true;
    }
    if (m_prsName.IsEmpty()) {
        m_prsName = Argument(theArgIter);

        Handle(RWCaeTest::DrawableMeshData) aDrawable = Handle(RWCaeTest::DrawableMeshData)::DownCast(
            Draw::GetExisting(m_prsName.ToCString()));
        if (aDrawable.IsNull()) {
            Message::SendFail() << "Error: object '" << m_prsName << "' doesn't exist or isn't CAE mesh";
            return false;
        }
        return true;
    }
    return false;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvTestMeshProp::Perform()
{
    Handle(RWCaeTest::DrawableMeshData) aDrawable = Handle(RWCaeTest::DrawableMeshData)::DownCast(
        Draw::GetExisting(m_prsName.ToCString()));

    const Handle(RWCae::MeshData)& aMeshData = aDrawable->MeshData();

    for (const std::pair<Type, PropInfo>& aProp : m_propsToGen) {
        switch (aProp.first) {
            case Type::Nodal: AddNodalProp(aMeshData, aProp.second); break;
            case Type::NodalVector: AddNodalVectorProp(aMeshData, aProp.second); break;
            case Type::Elemental: AddElementalProp(aMeshData, aProp.second); break;
            case Type::NodeVec: AddNodeVecProp(aMeshData, aProp.second); break;
            case Type::NodalFromElem: AddNodalPropFromElem(aMeshData, aProp.second); break;
            default: break;
        }
    }

    if (m_toSort) {
        RWCae::PropertyRanges aRanges;
        RWCae::MeshTools::fillPropertiesRange(aMeshData->ChangeProperties(), aRanges);
        RWCae::MeshTools::restorePropertiesRange(aMeshData->Properties(), aRanges);
    }

    return true;
}

// =======================================================================
// function : AddNodalProp
// =======================================================================
void ScvTestMeshProp::AddNodalProp(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo)
{
    const RWCae::MeshEntityId aNbNodes = theMesh->NbNodes();

    const Bnd_Box& aBox = theMesh->BoundingBox();
    Graphic3d_Vec3 aBoxMin((float)aBox.CornerMin().X(), (float)aBox.CornerMin().Y(), (float)aBox.CornerMin().Z());
    float          aSize = (float)(aBox.CornerMax().XYZ() - aBox.CornerMin().XYZ()).Modulus();

    for (int aTime : theInfo.TimeSteps) {
        Handle(RWCae::MeshNVecProperty) aProp =
            new RWCae::MeshNVecProperty("Test nodal prop", 1, RWCae::MeshPropertyBase::MappingType::Nodal);
        aProp->Init<RWCae::MeshScalarProperty>();

        aProp->SetId(theInfo.Id);

        double aCoeff = (double)aTime;
        aProp->SetTime((double)aTime, aTime);

        Handle(RWCae::MeshScalarProperty)::DownCast(aProp->GetRawComponent(0))->Init(aNbNodes);

        if (m_byLoc) {
            for (RWCae::MeshEntityId i = 0; i < aNbNodes; i++) {
                const Graphic3d_Vec3& aNode  = theMesh->Nodes()[i];
                double                aValue = (aNode - aBoxMin).Modulus() / aSize;
                aProp->GetRawComponent(0)->SetValue(RWCae::MeshPropertyEntityInfo(i), aValue);
            }
        } else {
            for (RWCae::MeshEntityId i = 0; i < aNbNodes; i++) {
                aProp->GetRawComponent(0)->SetValue(RWCae::MeshPropertyEntityInfo(i), i * aCoeff);
            }
        }

        aProp->UpdateComponents();

        theMesh->ChangeProperties().Append(aProp);
    }
}

// =======================================================================
// function : AddNodalVectorProp
// =======================================================================
void ScvTestMeshProp::AddNodalVectorProp(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo)
{
    const RWCae::MeshEntityId aNbNodes = theMesh->NbNodes();

    const Bnd_Box& aBox = theMesh->BoundingBox();
    Graphic3d_Vec3 aBoxMin((float)aBox.CornerMin().X(), (float)aBox.CornerMin().Y(), (float)aBox.CornerMin().Z());
    gp_XYZ         aSize = aBox.CornerMax().XYZ() - aBox.CornerMin().XYZ();

    for (int aTime : theInfo.TimeSteps) {
        Handle(RWCae::MeshNVecProperty) aProp =
            new RWCae::MeshNVecProperty("Test nodal vector prop",
                                          3,
                                          RWCae::MeshPropertyBase::MappingType::Nodal,
                                          RWCae::MeshNVecPropertyType::Vector);
        aProp->Init<RWCae::MeshScalarProperty>();

        aProp->SetId(theInfo.Id);

        double aCoeff = (double)aTime / theInfo.TimeSteps.size();
        aProp->SetTime((double)aTime, aTime);

        for (int aCompIter = 0; aCompIter < 3; aCompIter++) {
            Handle(RWCae::MeshScalarProperty)::DownCast(aProp->GetComponent(aCompIter))->Init(aNbNodes);
            if (m_byLoc) {
                for (RWCae::MeshEntityId i = 0; i < aNbNodes; i++) {
                    const Graphic3d_Vec3& aNode  = theMesh->Nodes()[i];
                    double                aValue = aSize.Coord(aCompIter + 1) > RealEpsilon()
                                                     ? (aNode[aCompIter] - aBoxMin[aCompIter]) / aSize.Coord(aCompIter + 1)
                                                     : 0.0;
                    aProp->GetComponent(aCompIter)->SetValue(RWCae::MeshPropertyEntityInfo(i), aValue);
                }
            } else {
                for (RWCae::MeshEntityId i = 0; i < aNbNodes; i++) {
                    aProp->GetComponent(aCompIter)->SetValue(RWCae::MeshPropertyEntityInfo(i),
                                                             (double)i / aNbNodes * aCoeff
                                                                 / aSize.Coord(aCompIter + 1));
                }
            }
        }

        aProp->UpdateComponents();

        theMesh->ChangeProperties().Append(aProp);
    }
}

// =======================================================================
// function : AddElementalProp
// =======================================================================
void ScvTestMeshProp::AddElementalProp(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo)
{
    const RWCae::MeshEntityId aNbElems = theMesh->NbIndexedElems();

    const Bnd_Box& aBox = theMesh->BoundingBox();
    Graphic3d_Vec3 aBoxMin((float)aBox.CornerMin().X(), (float)aBox.CornerMin().Y(), (float)aBox.CornerMin().Z());
    float          aSize = (float)(aBox.CornerMax().XYZ() - aBox.CornerMin().XYZ()).Modulus();

    for (int aTime : theInfo.TimeSteps) {
        Handle(RWCae::MeshNVecProperty) aProp =
            new RWCae::MeshNVecProperty("Test elem prop", 1, RWCae::MeshPropertyBase::MappingType::Elemental);
        aProp->Init<RWCae::MeshScalarProperty>();

        aProp->SetId(theInfo.Id);

        double aCoeff = (double)aTime;
        aProp->SetTime((double)aTime, aTime);

        Handle(RWCae::MeshScalarProperty)::DownCast(aProp->GetRawComponent(0))->Init(aNbElems);

        if (m_byLoc) {
            Graphic3d_Vec3 aCenter;
            for (RWCae::MeshEntityId i = 0; i < aNbElems; i++) {
                ElementCenter(theMesh, theMesh->IndexedElements()[i], aCenter);
                double aValue = (aCenter - aBoxMin).Modulus() / aSize;
                aProp->GetRawComponent(0)->SetValue(RWCae::MeshPropertyEntityInfo(i), aValue);
            }
        } else {
            for (RWCae::MeshEntityId i = 0; i < aNbElems; i++) {
                aProp->GetRawComponent(0)->SetValue(RWCae::MeshPropertyEntityInfo(i), i * aCoeff);
            }
        }

        aProp->UpdateComponents();

        theMesh->ChangeProperties().Append(aProp);
    }
}

// =======================================================================
// function : AddNodevecProp
// =======================================================================
void ScvTestMeshProp::AddNodeVecProp(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo)
{
    const RWCae::MeshEntityId aNbElems = theMesh->NbIndexedElems();

    for (int aTime : theInfo.TimeSteps) {
        Handle(RWCae::MeshNVecProperty) aProp =
            new RWCae::MeshNVecProperty("Test nodevec prop", 1, RWCae::MeshPropertyBase::MappingType::Elemental);
        aProp->Init<RWCae::MeshNodeVecProperty>();

        aProp->SetId(theInfo.Id);

        double aCoeff = (double)aTime;
        aProp->SetTime((double)aTime, aTime);

        Handle(RWCae::MeshNodeVecProperty) aComp = Handle(RWCae::MeshNodeVecProperty)::DownCast(
            aProp->GetRawComponent(0));
        aComp->Init(aNbElems, 0, false, false);

        auto fillElem = [aCoeff](const Handle(RWCae::MeshNodeVecProperty)& theComp, const auto& theElem) {
            theComp->ReserveValueMap(theElem.GlobalIndex(), theElem.NbNodes());
            for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < theElem.NbNodes(); aNodeIter++) {
                theComp->SetValue(RWCae::MeshPropertyEntityInfo(theElem.GlobalIndex(), theElem[aNodeIter]),
                                  (theElem.GlobalIndex() + theElem[aNodeIter]) * aCoeff);
            }
        };

        for (RWCae::MeshEntityId i = 0; i < aNbElems; i++) {
            const RWCae::MeshData::IndexedElement& anElem = theMesh->IndexedElements()[i];

            auto compute = [&fillElem, &aComp, &anElem](const auto& theAdaptor) {
                fillElem(aComp, theAdaptor.GetElement(anElem.Index));
                return true;
            };

            RWCae::MeshAdaptorsIteratorAllElems::Switch(anElem.Type, compute, theMesh);
        }

        aProp->UpdateComponents();

        theMesh->ChangeProperties().Append(aProp);
    }
}

// =======================================================================
// function : ElementCenter
// =======================================================================
bool ScvTestMeshProp::ElementCenter(const Handle(RWCae::MeshData)&         theMeshData,
                                    const RWCae::MeshData::IndexedElement& theElem,
                                    Graphic3d_Vec3&                          theCenter)
{
    auto compute = [&theCenter, &theElem](const auto& theAdaptor) {
        theCenter = theAdaptor.ElementCenter(theElem.Index);
        return true;
    };

    return RWCae::MeshAdaptorsIteratorAllElems::Switch(theElem.Type, compute, theMeshData);
}

// =======================================================================
// function : AddNodalPropFromElem
// =======================================================================
void ScvTestMeshProp::AddNodalPropFromElem(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo)
{
    const RWCae::MeshEntityId aNbNodes = theMesh->NbNodes();

    for (int aTime : theInfo.TimeSteps) {
        const Handle(RWCae::MeshNVecProperty)& aBaseProp = theMesh->GetPropertyById(theInfo.Id, aTime);
        if (aBaseProp.IsNull() || !aBaseProp->IsElemental()) {
            return;
        }

        Handle(RWCae::MeshNVecProperty) aProp =
            new RWCae::MeshNVecProperty("Test nodal prop",
                                          aBaseProp->NbComponents(),
                                          RWCae::MeshPropertyBase::MappingType::Nodal,
                                          aBaseProp->Type());
        aProp->Init<RWCae::MeshScalarProperty>();

        aProp->SetId(theInfo.Id + "_nodal");

        aProp->SetTime((double)aTime, aTime);

        for (int aCompIter = 0; aCompIter < aProp->NbComponents(); aCompIter++) {
            Handle(RWCae::MeshScalarProperty)::DownCast(aProp->GetComponent(aCompIter))
                ->Init(aNbNodes, aBaseProp->GetComponent(aCompIter)->ElemType());
        }

        NCollection_Array1<int> aNodesNbValues(0, aNbNodes - 1);
        aNodesNbValues.Init(0);

        auto compute = [this, &aBaseProp, &aProp, &aNodesNbValues](const auto& theAdaptor) {
            genNodalPropT(theAdaptor, aBaseProp, aProp, aNodesNbValues);
        };

        RWCae::MeshAdaptorsIteratorAllElems::For(compute, theMesh);

        for (int aCompIter = 0; aCompIter < aProp->NbComponents(); aCompIter++) {
            const Handle(RWCae::MeshScalarPropertyBase)& aComp = aProp->GetComponent(aCompIter);
            for (RWCae::MeshEntityId i = 0; i < aNbNodes; i++) {
                int aNbValues = aNodesNbValues[i];
                if (aNbValues == 0) {
                    continue;
                }

                RWCae::MeshPropertyEntityInfo anInfo(i);

                double aValue = aComp->Value(anInfo);
                if (RWCae::IsNaN(aValue)) {
                    continue;
                }

                aComp->SetValue(anInfo, aComp->Value(anInfo) / aNbValues);
            }
        }

        aProp->UpdateComponents();

        theMesh->ChangeProperties().Append(aProp);
    }
}

// =======================================================================
// function : genNodalPropT
// =======================================================================
template<class Elem_T>
void ScvTestMeshProp::genNodalPropT(const RWCae::MeshAdaptor<Elem_T>&      theAdaptor,
                                    const Handle(RWCae::MeshNVecProperty)& thePropIn,
                                    const Handle(RWCae::MeshNVecProperty)& thePropOut,
                                    NCollection_Array1<int>&                 theNodesNbValues) const
{
    RWCae::MeshEntityId aNbElems = theAdaptor.NbElements();

    for (RWCae::MeshEntityId anElemIdx = 0; anElemIdx < aNbElems; anElemIdx++) {
        const Elem_T& anElem = theAdaptor.GetElement(anElemIdx);

        for (RWCae::MeshElemEntityId aNodeIndex = 0; aNodeIndex < anElem.NbNodes(); aNodeIndex++) {
            for (int aCompIter = 0; aCompIter < thePropOut->NbComponents(); aCompIter++) {
                const Handle(RWCae::MeshScalarPropertyBase)& aCompIn  = thePropIn->GetComponent(aCompIter);
                const Handle(RWCae::MeshScalarPropertyBase)& aCompOut = thePropOut->GetComponent(aCompIter);

                double aValueIn = aCompIn->Value(
                    RWCae::MeshPropertyEntityInfo(anElem.GlobalIndex(), anElem[aNodeIndex]));
                if (RWCae::IsNaN(aValueIn)) {
                    continue;
                }
                double aValueOut = aCompOut->Value(RWCae::MeshPropertyEntityInfo(anElem[aNodeIndex]));
                aCompOut->SetValue(RWCae::MeshPropertyEntityInfo(anElem[aNodeIndex]),
                                   !RWCae::IsNaN(aValueOut) ? aValueOut + aValueIn : aValueIn,
                                   false);
            }
            theNodesNbValues[anElem[aNodeIndex]]++;
        }
    }
}

}

