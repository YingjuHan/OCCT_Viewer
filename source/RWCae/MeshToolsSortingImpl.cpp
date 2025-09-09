#include <RWCae/MeshTools.h>

#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshGroupProperty.h>
#include <RWCae/MeshNodeVecProperty.h>

#include <algorithm>
#include <functional>

namespace {
//! Auxiliary class for sorting of mesh data nodes and elements.
struct MeshDataSorter {
public:
    //! Init the sorter.
    bool Init(const Handle(RWCae::MeshData)& theMeshData)
    {
        myMeshData = theMeshData;

        static constexpr int aMaxDepth = 10;
        static constexpr int aTwo      = 2;
        static constexpr int aThree    = 3;

        // Compute depth of space-filling Hilbert curve by number of mesh elements.
        int aNbElements = theMeshData->NbIndexedElems();
        if (aNbElements <= 0) {
            return false;
        }

        int aNbSideElements = static_cast<int>(Pow(aNbElements, 1.0 / aThree));
        myDepth             = Min(static_cast<int>(Ceiling(log2(aNbSideElements))), aMaxDepth);

        if (myDepth < 1) {
            return false;
        }

        myNbCurveSideElements = static_cast<int>(Pow(aTwo, myDepth));
        myNbCurveElems        = static_cast<int>(Pow(myNbCurveSideElements, aThree));

        myCurveBoxElems.resize(myNbCurveElems);
        myCurveBoxNodes.resize(myNbCurveElems);
        myProcessedNodes.resize(myMeshData->NbNodes(), false);

        return true;
    }

    //! Sort mesh data.
    void Sort()
    {
        // Divide mesh bounding box into set of small boxes,
        // sort them according to the Hilbert curve
        // and store for each box internal mesh nodes and elements.
        SortMeshEntitiesByHilbert();

        myNewElemsIndices.resize(myMeshData->NbIndexedElems());

        // Gather mesh data blocks.
        GatherMeshBlocksRanges();

        // Keep the initial mesh data geometry.
        CreateAndSwapMeshDataGeometry();

        // Iterate over sorted boxes and fill mesh nodes according to the them.
        myNewNodesIndices.resize(myMeshData->NbNodes());
        SortMeshDataNodes();

        // Iterate over sorted boxes and fill mesh elements according to the them.
        SortMeshDataElems();

        // Update mesh properties values according to the sorted mesh nodes and elements.
        SortMeshDataProperties();
    }

private:
    //! Encode a 3D location as a Hilbert index.
    unsigned int ComputeHilbertIndex(const Graphic3d_Vec3i& theCoords)
    {
        static constexpr int          aOne    = 1;
        static constexpr int          aTwo    = 2;
        static constexpr int          aThree  = 3;
        static constexpr int          aFour   = 4;
        static constexpr unsigned int anEight = 8;

        Graphic3d_Vec3i aCoords = theCoords;

        int aMaxComp  = aCoords.maxComp();
        int aDepthMin = aMaxComp > 0 ? static_cast<int>(log2(aMaxComp)) + 1 : 1;
        int aT        = (myDepth - aDepthMin) % aThree;

        if (aT == aOne) {
            aCoords = aCoords.zxy();
        } else if (aT == aTwo) {
            aCoords = aCoords.yzx();
        }

        auto modifyXYZ = [](int theW, unsigned int theOctant, Graphic3d_Vec3i& theXYZ) {
            switch (theOctant) {
                case 0: theXYZ.SetValues(theXYZ.z(), theXYZ.x(), theXYZ.y()); break;
                case 1: theXYZ.SetValues(theXYZ.y(), theXYZ.z(), theXYZ.x() - theW); break;
                case 2: theXYZ.SetValues(theXYZ.y(), theXYZ.z() - theW, theXYZ.x() - theW); break;
                case 3: theXYZ.SetValues(theW - theXYZ.x() - 1, theXYZ.y(), aTwo * theW - theXYZ.z() - 1); break;
                case 4: theXYZ.SetValues(theW - theXYZ.x() - 1, theXYZ.y() - theW, aTwo * theW - theXYZ.z() - 1); break;
                case 5:
                    theXYZ.SetValues(aTwo * theW - theXYZ.y() - 1, aTwo * theW - theXYZ.z() - 1, theXYZ.x() - theW);
                    break;
                case 6: theXYZ.SetValues(aTwo * theW - theXYZ.y() - 1, theW - theXYZ.z() - 1, theXYZ.x() - theW); break;
                case 7: theXYZ.SetValues(theXYZ.z(), theW - theXYZ.x() - 1, aTwo * theW - theXYZ.y() - 1); break;
                default: break;
            }
        };

        unsigned int aRes = 0;
        int          aW   = static_cast<int>(Pow(aTwo, aDepthMin - 1));

        static constexpr unsigned int indexToOctant[anEight] = {0, 3, 7, 4, 1, 2, 6, 5};
        for (int i = aDepthMin; i > 0; i--) {
            unsigned int anIndex  = (unsigned int)(aCoords.x() / aW * aFour + aCoords.y() / aW * aTwo
                                                  + aCoords.z() / aW);
            unsigned int anOctant = indexToOctant[anIndex < anEight ? anIndex : 0];

            aRes = aRes * anEight + anOctant;

            modifyXYZ(aW, anOctant, aCoords);
            aW /= aTwo;
        }

        return aRes;
    }

    //! Calculate center of mesh element.
    bool ElementCenter(const RWCae::MeshData::IndexedElement& theElem, Graphic3d_Vec3& theCenter)
    {
        auto compute = [&theElem, &theCenter](const auto& theAdaptor) {
            theCenter = theAdaptor.ElementCenter(theElem.Index);
            return true;
        };

        return RWCae::MeshAdaptorsIteratorAllElems::Switch(theElem.Type, compute, myMeshData);
    }

    //! Sort mesh nodes and elements according to the space-filling Hilbert curve.
    void SortMeshEntitiesByHilbert()
    {
        const Bnd_Box& aBox = myMeshData->BoundingBox();
        Graphic3d_Vec3 aBoxMin((float)aBox.CornerMin().X(), (float)aBox.CornerMin().Y(), (float)aBox.CornerMin().Z());
        Graphic3d_Vec3 aBoxMax((float)aBox.CornerMax().X(), (float)aBox.CornerMax().Y(), (float)aBox.CornerMax().Z());
        Graphic3d_Vec3 aBoxSize = aBoxMax - aBoxMin;
        Graphic3d_Vec3 anInvBoxSize(aBoxSize.x() > ShortRealSmall() ? 1.0f / aBoxSize.x() : 1.0f,
                                    aBoxSize.y() > ShortRealSmall() ? 1.0f / aBoxSize.y() : 1.0f,
                                    aBoxSize.z() > ShortRealSmall() ? 1.0f / aBoxSize.z() : 1.0f);

        Graphic3d_Vec3 aCenter;
        for (const RWCae::MeshData::IndexedElement& anElem : myMeshData->IndexedElements()) {
            if (!ElementCenter(anElem, aCenter)) {
                continue;
            }

            Graphic3d_Vec3i aCoords((aCenter - aBoxMin) * anInvBoxSize * float(myNbCurveSideElements - 1));

            unsigned int anIdx = ComputeHilbertIndex(aCoords);
            AddSortMeshEntity(anElem, anIdx);
        }
    }

    //! Add mesh element to the map of sorted entities.
    void AddSortMeshEntity(const RWCae::MeshData::IndexedElement& theIndElem, unsigned int theCurveIdx)
    {
        auto addNodes = [&thisCurveBoxNodes = myCurveBoxNodes, &thisProcessedNodes = myProcessedNodes, theCurveIdx](
                            const auto& theElem) {
            for (int i = 0; i < theElem.NbNodes(); i++) {
                if (!thisProcessedNodes[theElem[i]]) {
                    thisCurveBoxNodes[theCurveIdx < thisCurveBoxNodes.size() ? theCurveIdx : 0].push_back(theElem[i]);
                    thisProcessedNodes[theElem[i]] = true;
                }
            }
        };

        myCurveBoxElems[theCurveIdx < myCurveBoxElems.size() ? theCurveIdx : 0].push_back(theIndElem);

        auto compute = [&addNodes, &theIndElem](const auto& theAdaptor) {
            addNodes(theAdaptor.GetElement(theIndElem.Index));
            return true;
        };

        RWCae::MeshAdaptorsIteratorAllElems::Switch(theIndElem.Type, compute, myMeshData);
    }

    //! Create a new mesh data and move geometry data from the other mesh data to it.
    void CreateAndSwapMeshDataGeometry()
    {
        myOldMeshData = new RWCae::MeshData();

        if (myMeshData->NbNodes() > 0) {
            myOldMeshData->ChangeNodes().SetValue(myMeshData->NbNodes() - 1, Graphic3d_Vec3());
            std::swap(myMeshData->ChangeNodes(), myOldMeshData->ChangeNodes());
        }

        for (int anElemType = 0; anElemType < RWCae::MeshElemNbTypes; anElemType++) {
            RWCae::MeshElemType aType = (RWCae::MeshElemType)anElemType;

            RWCae::MeshElements& anElems    = myMeshData->ChangeElements(aType);
            RWCae::MeshElements& anOldElems = myOldMeshData->ChangeElements(aType);

            std::swap(anElems.NodesOffsets(), anOldElems.NodesOffsets());
            std::swap(anElems.Nodes(), anOldElems.Nodes());
            std::swap(anElems.FacesOffsets(), anOldElems.FacesOffsets());
            std::swap(anElems.Faces(), anOldElems.Faces());
            std::swap(anElems.GlobalIndices(), anOldElems.GlobalIndices());
        }

        myMeshData->ChangeIndexedElements().Clear();
    }

    //! Gather ranges of mesh data blocks.
    void GatherMeshBlocksRanges()
    {
        std::function<void(const Handle(RWCae::MeshDataBlock)&, std::vector<RWCae::MeshDataRange>&)> gatherBlocks =
            [&](const Handle(RWCae::MeshDataBlock)& theNode, std::vector<RWCae::MeshDataRange>& theBlocks) {
                if (!theNode->HasChildren()) {
                    theBlocks.push_back(theNode->Range());
                    return;
                }

                for (const Handle(RWCae::MeshDataBlock)& aChild : theNode->Children()) {
                    gatherBlocks(aChild, theBlocks);
                }
            };

        gatherBlocks(myMeshData->Blocks(), myBlocks);
    }

    //! Sort mesh data nodes and fill correspondence map between old and new indices.
    void SortMeshDataNodes()
    {
        int aNbNodes = myMeshData->NbNodes();

        int aNodeGlobalIndex = 0;

        auto processNodes = [&thisMeshData        = myMeshData,
                             &thisOldMeshData     = myOldMeshData,
                             &thisNewNodesIndices = myNewNodesIndices,
                             &aNodeGlobalIndex](const std::vector<int>&       theNodes,
                                                const RWCae::MeshDataRange& theRng) {
            for (int aNode : theNodes) {
                if (aNode < theRng.NodeLower || aNode > theRng.NodeUpper) {
                    continue;
                }
                thisNewNodesIndices[aNode] = aNodeGlobalIndex;

                thisMeshData->ChangeNodes()[aNodeGlobalIndex] = thisOldMeshData->Nodes()[aNode];

                aNodeGlobalIndex++;
            }
        };

        for (const RWCae::MeshDataRange& aBlockRange : myBlocks) {
            for (const std::vector<int>& aNodes : myCurveBoxNodes) {
                processNodes(aNodes, aBlockRange);
            }

            for (int i = 0; i < aNbNodes; i++) {
                if (myProcessedNodes[i] || i < aBlockRange.NodeLower || i > aBlockRange.NodeUpper) {
                    continue;
                }

                myNewNodesIndices[i] = aNodeGlobalIndex;

                myMeshData->ChangeNodes()[aNodeGlobalIndex] = myOldMeshData->Nodes()[i];
                aNodeGlobalIndex++;
            }
        }
    }

    //! Sort mesh data elements, update its nodes indices and fill correspondence map between old and new indices.
    template<class Elem_T>
    void SortMeshDataTypedElems(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                                const RWCae::MeshDataRange&       theBlockRange)
    {
        if (theAdaptor.NbElements() == 0) {
            return;
        }

        auto updateNodes = [&thisNewNodesIndices = myNewNodesIndices](auto& theNewElem) {
            for (int aNodeIter = 0; aNodeIter < theNewElem.NbNodes(); aNodeIter++) {
                theNewElem[aNodeIter] = thisNewNodesIndices[theNewElem[aNodeIter]];
            }
        };

        for (const std::vector<RWCae::MeshData::IndexedElement>& anElems : myCurveBoxElems) {
            for (const RWCae::MeshData::IndexedElement& anElem : anElems) {
                if (anElem.Type != Elem_T::Type) {
                    continue;
                }

                const Elem_T& anOldElem = theAdaptor.GetElement(anElem.Index);
                if (anOldElem.GlobalIndex() < theBlockRange.ElemLower
                    || anOldElem.GlobalIndex() > theBlockRange.ElemUpper) {
                    continue;
                }

                Elem_T aNewElem(anOldElem);
                updateNodes(aNewElem);

                myNewElemsIndices[anOldElem.GlobalIndex()] = myMeshData->AddElement(aNewElem);
            }
        }
    }

    //! Sort mesh data elements, update its nodes indices and fill correspondence map between old and new indices.
    void SortMeshDataElems()
    {
        for (const RWCae::MeshDataRange& aRng : myBlocks) {
            auto compute = [this, &aRng](const auto& theAdaptor) {
                SortMeshDataTypedElems(theAdaptor, aRng);
            };

            RWCae::MeshAdaptorsIteratorAllElems::For(compute, myOldMeshData);
        }
    }

    //! Update mesh data property values according to the sorted mesh nodes and elements.
    void SortMeshPropertyComponent(const Handle(RWCae::MeshScalarPropertyBase)& theComp,
                                   const Handle(RWCae::MeshScalarPropertyBase)& theNewComp,
                                   RWCae::MeshScalarPropertyBaseType            thePropType)
    {
        int aNbValues = theComp->NbValues();

        if (thePropType == RWCae::MeshScalarPropertyBaseType::Scalar) {
            for (int aValueIter = 0; aValueIter < aNbValues; aValueIter++) {
                RWCae::MeshPropertyEntityInfo anInfo(aValueIter);
                RWCae::MeshPropertyEntityInfo aNewInfo(theComp->IsNodal() ? myNewNodesIndices[aValueIter]
                                                                            : myNewElemsIndices[aValueIter]);
                theNewComp->SetRawValue(aNewInfo, theComp->RawValue(anInfo), false);
            }
        } else if (thePropType == RWCae::MeshScalarPropertyBaseType::NodeVec) {
            std::vector<int> aSubIndices;
            for (int aValueIter = 0; aValueIter < aNbValues; aValueIter++) {
                Handle(RWCae::MeshNodeVecProperty)::DownCast(theComp)->SubIndices(aValueIter, aSubIndices);
                Handle(RWCae::MeshNodeVecProperty)::DownCast(theNewComp)
                    ->ReserveValueMap(aValueIter, static_cast<int>(aSubIndices.size()));
                for (int aNodeIter : aSubIndices) {
                    RWCae::MeshPropertyEntityInfo anInfo(aValueIter, aNodeIter);
                    RWCae::MeshPropertyEntityInfo aNewInfo(myNewElemsIndices[aValueIter],
                                                             myNewNodesIndices[aNodeIter]);
                    theNewComp->SetRawValue(aNewInfo, theComp->RawValue(anInfo), false);
                }
            }
        } else if (thePropType == RWCae::MeshScalarPropertyBaseType::Group) {
            Handle(RWCae::MeshGroupProperty) aGroupComp    = Handle(RWCae::MeshGroupProperty)::DownCast(theComp);
            Handle(RWCae::MeshGroupProperty) aNewGroupComp = Handle(RWCae::MeshGroupProperty)::DownCast(theNewComp);
            for (int aValueIter = 0; aValueIter < aNbValues; aValueIter++) {
                int anIndex = theComp->IsNodal() ? myNewNodesIndices[aValueIter] : myNewElemsIndices[aValueIter];
                aNewGroupComp->SetGroups(anIndex, aNewGroupComp->GetGroups(aValueIter), false, false);
            }
        }
    }

    //! Update mesh data property values according to the sorted mesh nodes and elements.
    void SortMeshDataProperties()
    {
        for (Handle(RWCae::MeshNVecProperty)& aProp : myMeshData->ChangeProperties()) {
            bool aPropCanBeLoaded = !aProp->IsLoaded() && aProp->CanBeLoaded();
            if (aPropCanBeLoaded) {
                if (!RWCae::MeshTools::LoadMeshProperty(myMeshData, aProp)) {
                    continue;
                }
            }

            int aNbValues = aProp->NbValues();

            Handle(RWCae::MeshNVecProperty) aNewProp = aProp->CopyRaw(aNbValues);
            if (aNewProp.IsNull()) {
                if (aPropCanBeLoaded) {
                    aProp->Unload();
                }
                continue;
            }

            aNewProp->SetId(aProp->Id());

            for (int aCompIter = 0; aCompIter < aProp->NbRawComponents(); aCompIter++) {
                const Handle(RWCae::MeshScalarPropertyBase)& aComp    = aProp->GetRawComponent(aCompIter);
                const Handle(RWCae::MeshScalarPropertyBase)& aNewComp = aNewProp->GetRawComponent(aCompIter);

                aNewComp->SetId(aComp->Id());
                aNewComp->SetValueRange(aComp->ValueRange());

                SortMeshPropertyComponent(aComp, aNewComp, aProp->ComponentsType());
            }

            aNewProp->UpdateComponents(false);

            std::swap(aProp, aNewProp);

            if (aPropCanBeLoaded) {
                RWCae::MeshTools::DumpMeshProperty(myMeshData, aProp, aProp->RefFilePath());
                aProp->Unload();
            }
        }
    }

private:
    Handle(RWCae::MeshData) myMeshData;
    Handle(RWCae::MeshData) myOldMeshData;

    std::vector<RWCae::MeshDataRange>                         myBlocks;
    std::vector<std::vector<RWCae::MeshData::IndexedElement>> myCurveBoxElems;
    std::vector<std::vector<int>>                               myCurveBoxNodes;
    std::vector<bool>                                           myProcessedNodes;
    std::vector<int>                                            myNewNodesIndices;
    std::vector<int>                                            myNewElemsIndices;

    int myNbCurveElems        = 0;
    int myNbCurveSideElements = 0;
    int myDepth               = 0;
};

}

namespace RWCae {
// ================================================================
// Function : SortMeshData
// ================================================================
void MeshTools::SortMeshData(const Handle(MeshData)& theMeshData)
{
    MeshDataSorter aSorter;
    if (!aSorter.Init(theMeshData)) {
        return;
    }

    aSorter.Sort();
}

}