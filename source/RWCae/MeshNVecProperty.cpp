#include <RWCae/MeshNVecProperty.h>

#include <RWCae/MeshGroupProperty.h>
#include <RWCae/MeshNodeVecProperty.h>

#include <gp.hxx>
#include <Graphic3d_Vec2.hxx>

namespace RWCae {

// ================================================================
// Function : Constructor
// ================================================================
MeshNVecProperty::MeshNVecProperty(const TCollection_AsciiString&      theName,
                                   const int                           theNbComponents,
                                   const MeshPropertyBase::MappingType theMapType,
                                   const MeshNVecPropertyType          theType)
    : MeshPropertyBase(theName, theMapType),
      myComponents(1),
      myNbComponents(theNbComponents),
      myNbValues(0),
      myType(theType),
      myTimestamp(std::numeric_limits<double>::quiet_NaN()),
      myTimeStep(-1)
{
    //
}

// ================================================================
// Function : GetComponent
// ================================================================
const Handle(MeshScalarPropertyBase)& MeshNVecProperty::GetComponent(int theIndex)
{
    Standard_ASSERT_RAISE(theIndex >= 0 && theIndex < myNbComponents, "Error! Incorrect index of property component.");
    return myComponents[theIndex + getNbAuxComponents()];
}

// ================================================================
// Function : GetRawComponent
// ================================================================
const Handle(MeshScalarPropertyBase)& MeshNVecProperty::GetRawComponent(int theIndex)
{
    Standard_ASSERT_RAISE(theIndex >= 0 && theIndex < NbRawComponents(),
                          "Error! Incorrect index of property component.");
    return myComponents[theIndex];
}

// ================================================================
// Function : EnabledComponents
// ================================================================
NCollection_IndexedMap<int> MeshNVecProperty::EnabledComponents()
{
    NCollection_IndexedMap<int> aRes;

    int aNbComponents = NbRawComponents();
    for (int i = 0; i < aNbComponents; i++) {
        if (myComponents[i]->IsEnabled()) {
            aRes.Add(i);
        }
    }

    return aRes;
}

// ================================================================
// Function : NormalizeVector
// ================================================================
void MeshNVecProperty::NormalizeVector()
{
    if (!IsVector3()) {
        return;
    }

    // Get the vector magnitude range - we need the max magnitude
    Graphic3d_Vec2d aMagnitudeMinMax(0.0);
    myComponents[0]->ValueRange().GetBounds(aMagnitudeMinMax[0], aMagnitudeMinMax[1]);

    // Avoid normalizing zero-length vectors
    if (aMagnitudeMinMax[1] <= gp::Resolution()) {
        return;
    }

    // Normalize the value ranges for all components
    for (int aCompIndex = 0; aCompIndex < NbRawComponents(); aCompIndex++) {
        Handle(MeshScalarPropertyBase) aComp = GetRawComponent(aCompIndex);
        Graphic3d_Vec2d                aCompMinMax(0.0);
        aComp->ValueRange().GetBounds(aCompMinMax[0], aCompMinMax[1]);
        Bnd_Range aNormRange(aCompMinMax[0] / aMagnitudeMinMax[1], aCompMinMax[1] / aMagnitudeMinMax[1]);
        aComp->SetValueRange(aNormRange);
    }

    // Iterate through all vector components and normalize their values for mesh entity
    // specified through theIndex.
    auto normalizeComponents = [this, aMagnitudeMinMax](const MeshPropertyEntityInfo& theIndex) {
        for (int aCompIndex = 0; aCompIndex < NbRawComponents(); aCompIndex++) {
            const Handle(MeshScalarPropertyBase)& aComp  = GetRawComponent(aCompIndex);
            double                                aValue = aComp->Value(theIndex);
            if (!IsNaN(aValue)) {
                aValue /= aMagnitudeMinMax[1];
                // We don't update the value range here - this is done above
                aComp->SetValue(theIndex, aValue, false);
            }
        }
    };

    // Need this to understand if we deal with vectors defined at mesh nodes or at mesh element's nodes
    Handle(MeshNodeVecProperty) aNodeVecProp = Handle(MeshNodeVecProperty)::DownCast(myComponents[0]);

    // Iterate through all mesh entities and normalize property values for them
    for (int i = 0; i < myNbValues; i++) {
        std::vector<int> anIndices;
        if (!aNodeVecProp.IsNull()) {
            aNodeVecProp->SubIndices(i, anIndices);
        } else {
            anIndices.push_back(-1);
        }

        for (const int& aNodeIndex : anIndices) {
            MeshPropertyEntityInfo aValueIndex(i, aNodeIndex);
            normalizeComponents(aValueIndex);
        }
    }
}

// ================================================================
// Function : UpdateComponents
// ================================================================
void MeshNVecProperty::UpdateComponents(bool toUpdateAuxComponents)
{
    myNbValues = GetComponent(0)->NbValues();
    for (int i = 1; i < myNbComponents; i++) {
        int aNbValues = GetComponent(i)->NbValues();
        Standard_ASSERT_RAISE(aNbValues == myNbValues, "Error! Incorrect number of values in the property component.");
    }

    myIsLoaded = GetComponent(0)->IsLoaded();
    SetRefFilePath(GetComponent(0)->RefFilePath());

    Standard_ASSERT_RAISE(!EnabledComponents().IsEmpty(), "Error! No enabled components in the property vector.");

    if (!toUpdateAuxComponents) {
        return;
    }

    if (IsVector()) {
        if (HasNodeVecComponents()) {
            fillNodeVecVector();
        } else if (HasScalarComponents()) {
            fillScalarVector();
        }
    }

    if (HasNodeVecComponents()) {
        replaceNodeVecWithScalar();
    }
}

// ================================================================
// Function : fillScalarVector
// ================================================================
void MeshNVecProperty::fillScalarVector()
{
    Handle(MeshScalarProperty) aVecProp = Handle(MeshScalarProperty)::DownCast(myComponents[0]);

    // If some property values are present, reset the value range to update it correctly below.
    // For an empty property keep the value range unchanged, it might have been loaded from metadata.
    if (myNbValues > 0) {
        aVecProp->SetValueRange(Bnd_Range());
    }
    aVecProp->Init(myNbValues, myComponents[1]->ElemType());

    for (int i = 0; i < myNbValues; i++) {
        double                 aVecValue = 0;
        MeshPropertyEntityInfo aValueIndex(i);
        for (int aCompIndex = 0; aCompIndex < NbComponents(); aCompIndex++) {
            const Handle(MeshScalarPropertyBase)& aProp = myComponents[aCompIndex + 1];

            double aValue = aProp->Value(aValueIndex);
            if (IsNaN(aValue)) {
                aVecValue = aValue;
                break;
            }
            aVecValue = std::hypot(aVecValue, aValue);
        }
        aVecProp->SetValue(aValueIndex, aVecValue);
    }
}

// ================================================================
// Function : fillNodeVecVector
// ================================================================
void MeshNVecProperty::fillNodeVecVector()
{
    Handle(MeshNodeVecProperty) aVecProp = Handle(MeshNodeVecProperty)::DownCast(myComponents[0]);
    Handle(MeshNodeVecProperty) aComp1   = Handle(MeshNodeVecProperty)::DownCast(myComponents[1]);

    // If some property values are present, reset the value range to update it correctly below.
    // For an empty property keep the value range unchanged, it might have been loaded from metadata.
    if (myNbValues > 0) {
        aVecProp->SetValueRange(Bnd_Range());
    }
    aVecProp->Init(myNbValues,
                   !aComp1->NodalProperty().IsNull() ? aComp1->NodalProperty()->NbValues() : 0,
                   !aComp1->NodalProperty().IsNull(),
                   !aComp1->ElementalProperty().IsNull(),
                   myComponents[1]->ElemType());

    auto processVector = [this, &aVecProp](const MeshPropertyEntityInfo& theIndex) {
        double aVecValue = 0;
        for (int aCompIndex = 0; aCompIndex < NbComponents(); aCompIndex++) {
            double aValue = myComponents[aCompIndex + 1]->Value(theIndex);
            if (IsNaN(aValue)) {
                aVecValue = aValue;
                break;
            }
            aVecValue = std::hypot(aVecValue, aValue);
        }
        aVecProp->SetValue(theIndex, aVecValue);
    };

    for (int i = 0; i < myNbValues; i++) {
        std::vector<int> anIndices;
        dynamic_cast<MeshNodeVecProperty*>(myComponents[1].get())->SubIndices(i, anIndices);
        for (const int aNodeIndex : anIndices) {
            MeshPropertyEntityInfo aValueIndex(i, aNodeIndex);
            processVector(aValueIndex);
        }
    }
}

// ================================================================
// Function : replaceNodeVecWithScalar
// ================================================================
void MeshNVecProperty::replaceNodeVecWithScalar()
{
    int aNbComponents              = NbComponents();
    int aNbCompsToReplaceElemental = 0;
    int aNbCompsToReplaceNodal     = 0;

    // Try to replace with elemental properties
    for (const Handle(MeshScalarPropertyBase)& aComp : myComponents) {
        const Handle(MeshNodeVecProperty)& aNodeVecComp = Handle(MeshNodeVecProperty)::DownCast(aComp);
        if (!aNodeVecComp->ElementalProperty().IsNull()) {
            aNbCompsToReplaceElemental++;
        }
        if (!aNodeVecComp->NodalProperty().IsNull()) {
            aNbCompsToReplaceNodal++;
        }
    }

    bool toReplaceElemental = aNbComponents + getNbAuxComponents() == aNbCompsToReplaceElemental;
    bool toReplaceNodal     = aNbComponents + getNbAuxComponents() == aNbCompsToReplaceNodal;

    for (Handle(MeshScalarPropertyBase)& aComp : myComponents) {
        if (toReplaceElemental || toReplaceNodal) {
            Handle(MeshScalarProperty) aScalarProp =
                toReplaceElemental ? Handle(MeshNodeVecProperty)::DownCast(aComp)->ElementalProperty()
                                   : Handle(MeshNodeVecProperty)::DownCast(aComp)->NodalProperty();
            aScalarProp->SetId(aComp->Id());
            aScalarProp->SetName(aComp->Name());
            aScalarProp->SetDescription(aComp->Description());
            aScalarProp->SetEnabled(aComp->IsEnabled());
            aComp = aScalarProp;
        } else {
            Handle(MeshNodeVecProperty)::DownCast(aComp)->NullifyAuxProperties();
        }
    }

    if (toReplaceNodal) {
        myNbValues = myComponents[0]->NbValues();
        myMapType  = MeshPropertyBase::MappingType::Nodal;
    }
}

// ================================================================
// Function : CopyRaw
// ================================================================
Handle(MeshNVecProperty) MeshNVecProperty::CopyRaw(int theNbValues, int theCompIdx) const
{
    int                      aNbComponents = theCompIdx == -1 ? myNbComponents : 1;
    MeshNVecPropertyType     aType         = theCompIdx == -1 ? myType : MeshNVecPropertyType::ScalarSet;
    Handle(MeshNVecProperty) aRes          = new MeshNVecProperty(myName, aNbComponents, myMapType, aType);

    if (HasScalarComponents()) {
        aRes->Init<MeshScalarProperty>();
        for (int i = 0; i < aRes->NbRawComponents(); i++) {
            dynamic_cast<MeshScalarProperty*>(aRes->GetRawComponent(i).get())
                ->Init(theNbValues, myComponents[theCompIdx == -1 ? i : theCompIdx]->ElemType());
        }
    } else if (HasNodeVecComponents()) {
        aRes->Init<MeshNodeVecProperty>();
        for (int i = 0; i < aRes->NbRawComponents(); i++) {
            dynamic_cast<MeshNodeVecProperty*>(aRes->GetRawComponent(i).get())
                ->Init(theNbValues, 0, false, false, myComponents[theCompIdx == -1 ? i : theCompIdx]->ElemType());
        }
    } else if (HasGroupComponents()) {
        aRes->Init<MeshGroupProperty>();
        for (int i = 0; i < aRes->NbRawComponents(); i++) {
            Handle(MeshGroupProperty) aGroupComp = Handle(MeshGroupProperty)::DownCast(
                myComponents[theCompIdx == -1 ? i : theCompIdx]);
            Handle(MeshGroupProperty) aResGroupComp = Handle(MeshGroupProperty)::DownCast(aRes->GetRawComponent(i));
            aResGroupComp->Init(theNbValues);
            for (int aGroup = 0; aGroup < MeshGroupProperty::MaxNbGroups; aGroup++) {
                aResGroupComp->SetGroupName(aGroup, aGroupComp->GetGroupName(aGroup));
                aResGroupComp->SetGroupNbEntities(aGroup, aGroupComp->GetGroupNbEntities(aGroup));
            }
        }
    } else {
        return nullptr;
    }

    for (int i = 0; i < aRes->NbRawComponents(); i++) {
        const Handle(MeshScalarPropertyBase)& aComp    = myComponents[theCompIdx == -1 ? i : theCompIdx];
        const Handle(MeshScalarPropertyBase)& aResComp = aRes->GetRawComponent(i);
        aResComp->SetName(aComp->Name());
        aResComp->SetDescription(aComp->Description());
        aResComp->SetEnabled(aComp->IsEnabled());
        aResComp->SetDisplayUnits(aComp->DisplayUnits());
        aResComp->SetRefFilePath(aComp->RefFilePath());
    }

    aRes->SetDescription(myDescription);
    aRes->SetTime(myTimestamp, myTimeStep);
    aRes->SetTimeStepDescription(myTimeStepDescr);
    aRes->SetTimeDisplayUnits(myTimeDispUnits);
    aRes->SetRefFilePath(myRefFilePath);

    return aRes;
}

// ================================================================
// Function : Unload
// ================================================================
void MeshNVecProperty::Unload()
{
    myIsLoaded = false;
    for (Handle(MeshScalarPropertyBase)& aComp : myComponents) {
        aComp->Unload();
    }
}

} // namespace
