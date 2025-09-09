#include <ViewCae/MeshPresentation.h>

#include <ViewCae/InteractiveCaeMeshOwner.h>
#include <ViewCae/MeshPresentationScalar.h>
#include <ViewCae/MeshPresentationStreamlines.h>
#include <ViewCae/MeshPresentationVector.h>

#include <ViewCaeGeom/MeshGeometryDeformed.h>
#include <ViewCaeGeom/MeshGeometryIso.h>

namespace ViewCae {

// =======================================================================
// function : MeshPresentation
// =======================================================================
MeshPresentation::MeshPresentation(const Handle(MeshPropertyData)&           thePropData,
                                   int                                       thePropIndex,
                                   const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                   const TCollection_AsciiString&            theId,
                                   bool                                      theToUpdateGeomId)
    : MeshPresentationBase(theGeometry, theId, theToUpdateGeomId),
      myDrawer(new InteractiveCaeMeshDrawer()),
      myPropData(thePropData),
      myPropIndex(thePropIndex)
{
    myHighlightDrawers[(int)MeshHighlightingType::None]      = new InteractiveCaeMeshDrawer();
    myHighlightDrawers[(int)MeshHighlightingType::Highlight] = new InteractiveCaeMeshDrawer();
    myHighlightDrawers[(int)MeshHighlightingType::Highlight]->SetModulateTexture(true);
    myHighlightDrawers[(int)MeshHighlightingType::Selection] = new InteractiveCaeMeshDrawer();
    myHighlightDrawers[(int)MeshHighlightingType::Selection]->SetModulateTexture(true);
}

// =======================================================================
// function : ~MeshPresentation
// =======================================================================
MeshPresentation::~MeshPresentation()
{
    //
}

// =======================================================================
// function : UpdateAspects
// =======================================================================
void MeshPresentation::UpdateAspects(const Handle(InteractiveCaeMeshOwner)&)
{
    RWCae::MeshPropertyComponent aProp = !myPropData.IsNull() ? myPropData->Property(myPropIndex)
                                                                : RWCae::MeshPropertyComponent();
    for (NCollection_DataMap<int, Handle(MeshPrsPartParams)>::Iterator anIter(myPartsParams); anIter.More();
         anIter.Next()) {
        anIter.Value()->SetProperty(aProp);
    }

    myDrawer->UpdateAspects(myHighlightDrawers[(int)myHiType]);
    if (myHiType != MeshHighlightingType::None) {
        // Use common transparency for whole presentation highlighting/selection
        myDrawer->SetShadingTransparency(myHighlightDrawers[(int)MeshHighlightingType::None]->ShadingTransparency());
    }
}

// =======================================================================
// function : GetParts
// =======================================================================
const MeshParts& MeshPresentation::GetParts()
{
    MeshParts& aParts = getParts();

    if (!needToUpdateParts(aParts)) {
        return aParts;
    }

    if (!myToUseGenGeom) {
        for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(myGeometry.get())) {
            aGeom->GetMeshData();
        }
    }

    fillProperties();
    updateParts(aParts);

    return aParts;
}

// =======================================================================
// function : ChangeParts
// =======================================================================
MeshParts& MeshPresentation::ChangeParts()
{
    return getParts();
}

// =======================================================================
// function : GetSensParts
// =======================================================================
const MeshSensParts& MeshPresentation::GetSensParts(const Handle(InteractiveCaeMeshOwner)& theOwner)
{
    if (!myToUseGenGeom) {
        for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(myGeometry.get())) {
            aGeom->GetMeshData();
        }
    }

    updateSensParts(theOwner);

    return mySensitives;
}

// ================================================================
// Function : SetToUpdate
// ================================================================
void MeshPresentation::SetToUpdate(bool theToUpdatePrims, bool theToUpdateProps)
{
    if (theToUpdatePrims) {
        for (NCollection_DataMap<int, MeshParts>::Iterator anIter(myPrimitives); anIter.More(); anIter.Next()) {
            anIter.ChangeValue().ClearAll();
        }
    }
    if (theToUpdateProps) {
        onGeomUpdated();
    }

    SetToUpdateGenGeometry();
}

// ================================================================
// Function : ClearPart
// ================================================================
void MeshPresentation::ClearPart(MeshPartType thePartType, int theTimeStep, bool theWithAuxData, bool theWithGeom)
{
    for (NCollection_DataMap<int, MeshParts>::Iterator anIter(myPrimitives); anIter.More(); anIter.Next()) {
        if (theTimeStep == -1 || anIter.Key() == theTimeStep) {
            anIter.ChangeValue()(thePartType)->Clear(theWithAuxData);
        }
    }

    if (myToUseGenGeom) {
        Handle(ViewCaeGeom::MeshGeometryGen) aPart = GetGenGeometry((int)thePartType);
        if (!aPart.IsNull()) {
            if (theWithGeom) {
                aPart->SetToUpdate();
            }

            onGeomUpdated(aPart->Id());
        }
    }
}

// ================================================================
// Function : onGeomUpdated
// ================================================================
void MeshPresentation::onGeomUpdated(const TCollection_AsciiString& theGeomId)
{
    if (theGeomId.IsEmpty()) {
        myGeomScalarProps.Clear();
    } else {
        myGeomScalarProps.UnBind(theGeomId);
    }
    myGeomPropsUpdated = false;
}

// ================================================================
// Function : addUsedPerPartProp
// ================================================================
void MeshPresentation::addUsedPerPartProp(MeshPartType theType, const TCollection_AsciiString& thePropId)
{
    myHasUpdatedPropsPerPart.at((int)theType) = true;
    myUsedPerPartProps[(int)theType].Add(thePropId);
}

// ================================================================
// Function : SetCachePrimitives
// ================================================================
void MeshPresentation::SetCachePrimitives(bool theToCache, bool theToClearCached)
{
    myToCachePrimitives = theToCache;
    if (!theToClearCached) {
        return;
    }

    getParts();
    if (myPrimitives.Size() == 1) {
        return;
    }

    std::vector<int> aTimeStepsToClear;
    aTimeStepsToClear.reserve(myPrimitives.Size() - 1);
    for (NCollection_DataMap<int, MeshParts>::Iterator anIter(myPrimitives); anIter.More(); anIter.Next()) {
        if (anIter.Key() != myActiveTimeStep) {
            aTimeStepsToClear.push_back(anIter.Key());
        }
    }

    for (int aTimeStep : aTimeStepsToClear) {
        myPrimitives.UnBind(aTimeStep);
    }
}

// ================================================================
// Function : UsedProps
// ================================================================
void MeshPresentation::UsedProps(int                                       theTimeStep,
                                 NCollection_Map<TCollection_AsciiString>& theComputedProps,
                                 NCollection_Map<TCollection_AsciiString>& theNotComputedProps) const
{
    if (!myPrsVisProps->IsVisible()) {
        return;
    }

    int aTimeStep = theTimeStep != -1 ? theTimeStep : myActiveTimeStep;

    const MeshParts* aParts = myPrimitives.Seek(aTimeStep);
    if (aParts != nullptr) {
        for (int i = 0; i < MeshNbPartTypes; i++) {
            const Handle(MeshPart)& aPart = (*aParts)((MeshPartType)i);
            if (!aPart->ToShow()) {
                continue;
            }

            if (aPart->NeedToUpdate() || myHasUpdatedPropsPerPart[i]) {
                theNotComputedProps.Unite(myUsedPerPartProps[i]);
            } else {
                theComputedProps.Unite(myUsedPerPartProps[i]);
            }
        }
    } else {
        for (int i = 0; i < MeshNbPartTypes; i++) {
            theNotComputedProps.Unite(myUsedPerPartProps[i]);
        }
    }
}

// ================================================================
// Function : OnPropertiesUpdated
// ================================================================
void MeshPresentation::OnPropertiesUpdated(const NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps,
                                           bool                                                      theCachedProps)
{
    bool toUpdateGeom = false;
    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(myGeometry.get())) {
        Handle(RWCae::MeshNVecProperty) aProp;
        if (ViewCaeGeom::MeshGeometryDeformed* aDefGeom = dynamic_cast<ViewCaeGeom::MeshGeometryDeformed*>(aGeom)) {
            aProp = aDefGeom->DeformedProperty();
        } else if (ViewCaeGeom::MeshGeometryIso* anIsoGeom = dynamic_cast<ViewCaeGeom::MeshGeometryIso*>(aGeom)) {
            aProp = anIsoGeom->Property().Property;
        }

        if (!aProp.IsNull() && theProps.Contains(aProp)) {
            toUpdateGeom = true;
            break;
        }
    }

    if (toUpdateGeom) {
        myGeometry->SetToUpdate();
        SetToUpdateGenGeometry();
    }

    RWCae::MeshPropertyComponent aProp = myPropData->Property(myPropIndex);
    if (!aProp.Property.IsNull() && theProps.Contains(aProp.Property)) {
        if (!theCachedProps) {
            myPropData->UpdateColorMapper(aProp);
        }
        myGeomPropsUpdated = false;
        myGeomScalarProps.Clear();
    }

    // Clear updated props flag for all mesh parts.
    std::fill(myHasUpdatedPropsPerPart.begin(), myHasUpdatedPropsPerPart.end(), false);
}

// ================================================================
// Function : getPart
// ================================================================
MeshParts& MeshPresentation::getParts()
{
    MeshParts* aRes = myPrimitives.ChangeSeek(myActiveTimeStep);
    if (aRes != nullptr) {
        return *aRes;
    }

    if (!myToCachePrimitives) {
        myPrimitives.Clear();
    }

    myPrimitives.Bind(myActiveTimeStep, MeshParts());
    return myPrimitives.ChangeFind(myActiveTimeStep);
}

// ================================================================
// Function : addDependencyPropIds
// ================================================================
void MeshPresentation::addDependencyPropIds(std::vector<TCollection_AsciiString>&                 thePropIdArray,
                                            const std::vector<Handle(RWCae::MeshNVecProperty)>& theProps)
{
    for (const Handle(RWCae::MeshNVecProperty)& aProp : theProps) {
        const RWCae::MeshNVecProperties& aDepsArray = aProp->DependencyProps();
        for (const Handle(RWCae::MeshNVecProperty)& aDep : aDepsArray) {
            thePropIdArray.push_back(aDep->Id());
        }
    }
}

// ================================================================
// Function : fillProperties
// ================================================================
void MeshPresentation::fillProperties()
{
    if (myGeomPropsUpdated) {
        return;
    }

    RWCae::MeshPropertyComponent aBasePropComponent = myPropData->Property(myPropIndex);

    if (!myToUseGenGeom) {
        Handle(ViewCaeGeom::MeshGeometry) aGeom = myGeometry->GetLastNotModifierChild();
        while (!aGeom.IsNull()) {
            if (myGeomScalarProps.IsBound(aGeom->Id())) {
                aGeom = aGeom->GetChild();
                continue;
            }
            RWCae::MeshPropertyComponent aProp = aGeom->CalculatePropertyComponent(aBasePropComponent.Property,
                                                                                     aBasePropComponent.ComponentIndex);
            // Manage property dependencies - calculate property dependencies too.
            const Handle(RWCae::MeshNVecProperty)& aBaseProp = aBasePropComponent.Property;
            const Handle(RWCae::MeshNVecProperty)& aCalcProp = aProp.Property;
            if (aProp.IsDefined() && aBaseProp != aCalcProp) {
                const RWCae::MeshNVecProperties& anOrigDeps = aBaseProp->DependencyProps();
                RWCae::MeshNVecProperties&       aCalcDeps  = aCalcProp->ChangeDependencyProps();
                const int                          aNbDeps    = anOrigDeps.Length();
                for (int anIter = 0; anIter < aNbDeps; anIter++) {
                    const Handle(RWCae::MeshNVecProperty)& aBaseDep       = anOrigDeps[anIter];
                    Handle(RWCae::MeshNVecProperty)        aCalculatedDep = aGeom->CalculateProperty(aBaseDep);
                    aCalcDeps.SetValue(anIter, aCalculatedDep);
                }
            }
            myGeomScalarProps.Bind(aGeom->Id(), aProp.IsDefined() ? aProp : RWCae::MeshPropertyComponent());
            aGeom = aGeom->GetChild();
        }
    } else {
        for (NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>::Iterator anIter(myGenGeometry);
             anIter.More();
             anIter.Next()) {
            if (myGeomScalarProps.IsBound(anIter.Value()->Id())) {
                continue;
            }
            RWCae::MeshPropertyComponent aProp =
                anIter.Value()->CalculatePropertyComponent(aBasePropComponent.Property,
                                                           aBasePropComponent.ComponentIndex);
            // Property dependency support in case of GenGeom is to be implemented.
            myGeomScalarProps.Bind(anIter.Value()->Id(), aProp.IsDefined() ? aProp : RWCae::MeshPropertyComponent());
        }
    }

    myGeomPropsUpdated = true;
}

// ================================================================
// Function : Create
// ================================================================
Handle(MeshPrsPartParams) MeshPrsPartParamsFactory::Create(const Handle(RWCae::MeshData)& theMeshData,
                                                           MeshPartType                     theType)
{
    switch (theType) {
        case MeshPartType::MinMaxAnnotations: return new MeshPrsMinMaxPartParams();
        case MeshPartType::Vectors: return new MeshPrsVectorPartParams();
        case MeshPartType::Streamlines: {
            Handle(MeshPrsStreamlinesPartParams) aStreamlinesParams = new MeshPrsStreamlinesPartParams();

            const Bnd_Box&       aBox = theMeshData->BoundingBox();
            MeshStreamlinesPlane aPlane((aBox.CornerMax().XYZ() + aBox.CornerMin().XYZ()) * 0.5,
                                        gp_Vec(gp::DY()) * (aBox.CornerMax().Y() - aBox.CornerMin().Y()) * 0.5,
                                        gp_Vec(gp::DZ()) * (aBox.CornerMax().Z() - aBox.CornerMin().Z()) * 0.5);
            aStreamlinesParams->SetPlane(aPlane);
            return aStreamlinesParams;
        }
        default: return new MeshPrsPartParams();
    }
}

} // namespace