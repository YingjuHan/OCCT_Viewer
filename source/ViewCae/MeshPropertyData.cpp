#include <ViewCae/MeshPropertyData.h>

#include <Standard_Atomic.hxx>

namespace {
static volatile int g_meshPropsCounter = 0;
}

namespace ViewCae {

// =======================================================================
// function : MeshPropertyData
// =======================================================================
MeshPropertyData::MeshPropertyData()
{
}

// =======================================================================
// function : ~MeshPropertyData
// =======================================================================
MeshPropertyData::~MeshPropertyData()
{
    release();
}

// =======================================================================
// function : release
// =======================================================================
void MeshPropertyData::release() noexcept
{
    for (MapOfColorMappers::Iterator anIter(myAttachedColorMappers); anIter.More(); anIter.Next()) {
        anIter.Value()->Clear();
    }
    for (const Handle(MeshColorMapper)& aColorMapper : myFreeColorMappers) {
        aColorMapper->Clear();
    }
}

// =======================================================================
// function : Property
// =======================================================================
RWCae::MeshPropertyComponent MeshPropertyData::Property(const int thePropIndex) const
{
    const RWCae::MeshPropertyComponent* aProp = myProperties.Seek(thePropIndex);
    return aProp != nullptr ? *aProp : RWCae::MeshPropertyComponent();
}

// =======================================================================
// function : SetProperty
// =======================================================================
void MeshPropertyData::SetProperty(const Handle(RWCae::MeshNVecProperty)& theProperty, const int thePropIndex)
{
    const RWCae::MeshPropertyComponent* anOldPropPtr = myProperties.Seek(thePropIndex);
    RWCae::MeshPropertyComponent        anOldProp    = anOldPropPtr != nullptr ? *anOldPropPtr
                                                                                 : RWCae::MeshPropertyComponent();

    int aPropCompIndex = !theProperty.IsNull() ? theProperty->EnabledComponents().FindKey(1) : -1;
    myProperties.Bind(thePropIndex, RWCae::MeshPropertyComponent(theProperty, aPropCompIndex));

    onColorMapperUpdated(anOldProp, thePropIndex);
}

// =======================================================================
// function : AddProperty
// =======================================================================
int MeshPropertyData::AddProperty(const Handle(RWCae::MeshNVecProperty)& theProperty)
{
    int aPropId = Standard_Atomic_Increment(&g_meshPropsCounter);
    SetProperty(theProperty, aPropId);
    return aPropId;
}

// =======================================================================
// function : RemoveProperty
// =======================================================================
void MeshPropertyData::RemoveProperty(const int thePropIndex)
{
    const RWCae::MeshPropertyComponent* anOldPropPtr = myProperties.Seek(thePropIndex);
    RWCae::MeshPropertyComponent        anOldProp    = anOldPropPtr != nullptr ? *anOldPropPtr
                                                                                 : RWCae::MeshPropertyComponent();

    myProperties.UnBind(thePropIndex);

    onColorMapperUpdated(anOldProp, thePropIndex);
}

// =======================================================================
// function : SetPropertyComponentIndex
// =======================================================================
void MeshPropertyData::SetPropertyComponentIndex(const int thePropertyComponentIndex, const int thePropIndex)
{
    const RWCae::MeshPropertyComponent* anOldPropPtr = myProperties.Seek(thePropIndex);
    RWCae::MeshPropertyComponent        anOldProp    = anOldPropPtr != nullptr ? *anOldPropPtr
                                                                                 : RWCae::MeshPropertyComponent();

    RWCae::MeshPropertyComponent* aProp = myProperties.ChangeSeek(thePropIndex);
    if (aProp == nullptr) {
        return;
    }

    aProp->ComponentIndex = thePropertyComponentIndex;

    onColorMapperUpdated(anOldProp, thePropIndex);
}

// =======================================================================
// function : PropertyUsedIndices
// =======================================================================
void MeshPropertyData::PropertyUsedIndices(const RWCae::MeshPropertyComponent& theProp,
                                           TColStd_PackedMapOfInteger&           theIndices)
{
    for (NCollection_DataMap<int, RWCae::MeshPropertyComponent>::Iterator anIter(myProperties); anIter.More();
         anIter.Next()) {
        if (anIter.Value() == theProp) {
            theIndices.Add(anIter.Key());
        }
    }
}

// =======================================================================
// function : UpdatePropertiesTimeStep
// =======================================================================
bool MeshPropertyData::UpdatePropertiesTimeStep(const Handle(RWCae::MeshData)& theMeshData, int theTimeStep)
{
    bool aRes = true;
    for (NCollection_DataMap<int, RWCae::MeshPropertyComponent>::Iterator anIter(myProperties); anIter.More();
         anIter.Next()) {
        if (anIter.Value().Property.IsNull()) {
            continue;
        }
        const Handle(RWCae::MeshNVecProperty)& aNewProp = theMeshData->GetPropertyById(anIter.Value().Property->Id(),
                                                                                         theTimeStep);
        if (aNewProp.IsNull()) {
            aRes = false;
            continue;
        }

        anIter.ChangeValue().Property = aNewProp;
    }

    MapOfColorMappers aNewAttachedMappers;
    for (MapOfColorMappers::Iterator anIter(myAttachedColorMappers); anIter.More(); anIter.Next()) {
        if (anIter.Key().Property.IsNull()) {
            aNewAttachedMappers.Bind(anIter.Key(), anIter.Value());
            continue;
        }

        const Handle(RWCae::MeshNVecProperty)& aNewProp = theMeshData->GetPropertyById(anIter.Key().Property->Id(),
                                                                                         theTimeStep);
        if (aNewProp.IsNull()) {
            aRes = false;
            aNewAttachedMappers.Bind(anIter.Key(), anIter.Value());
            continue;
        }

        anIter.ChangeValue()->SetMeshProperty(aNewProp, anIter.Key().ComponentIndex, false);
        aNewAttachedMappers.Bind(RWCae::MeshPropertyComponent(aNewProp, anIter.Key().ComponentIndex), anIter.Value());
    }

    myAttachedColorMappers = aNewAttachedMappers;

    return aRes;
}

// =======================================================================
// function : ColorMapper
// =======================================================================
Handle(MeshColorMapper) MeshPropertyData::ColorMapper(const RWCae::MeshPropertyComponent& theProp) const
{
    Handle(MeshColorMapper) aRes;
    if (theProp.Property.IsNull()) {
        return aRes;
    }
    myAttachedColorMappers.Find(theProp, aRes);
    return aRes;
}

// =======================================================================
// function : SetColorMapper
// =======================================================================
void MeshPropertyData::SetColorMapper(const Handle(MeshColorMapper)&        theColorMapper,
                                      const RWCae::MeshPropertyComponent& theProp)
{
    if (theColorMapper.IsNull()) {
        return;
    }

    if (theProp.Property.IsNull()) {
        theColorMapper->SetMeshProperty(nullptr, -1);
        if (myToKeepFreeColorMappers) {
            myFreeColorMappers.Append(theColorMapper);
        }
    } else if (Handle(MeshColorMapper)* aColorMapper = myAttachedColorMappers.ChangeSeek(theProp)) {
        *aColorMapper = theColorMapper;
        (*aColorMapper)->SetMeshProperty(theProp.Property, theProp.ComponentIndex);
    }
}

// =======================================================================
// function : UpdateColorMapper
// =======================================================================
void MeshPropertyData::UpdateColorMapper(const RWCae::MeshPropertyComponent& theProp)
{
    if (!theProp.IsDefined()) {
        return;
    }

    Handle(MeshColorMapper) aColorMapper = ColorMapper(theProp);
    if (aColorMapper.IsNull()) {
        return;
    }

    aColorMapper->SetMeshProperty(theProp.Property, theProp.ComponentIndex);
    if (myOnColorMapperChangeCallback != nullptr) {
        myOnColorMapperChangeCallback(theProp, aColorMapper);
    }
}

// =======================================================================
// function : ColorMappers
// =======================================================================
std::vector<Handle(MeshColorMapper)> MeshPropertyData::ColorMappers() const
{
    std::vector<Handle(MeshColorMapper)> aRes;
    for (MapOfColorMappers::Iterator anIter(myAttachedColorMappers); anIter.More(); anIter.Next()) {
        aRes.push_back(anIter.Value());
    }
    for (const Handle(MeshColorMapper)& aColorMapper : myFreeColorMappers) {
        aRes.push_back(aColorMapper);
    }
    return aRes;
}

// =======================================================================
// function : onColorMapperUpdated
// =======================================================================
void MeshPropertyData::onColorMapperUpdated(const RWCae::MeshPropertyComponent& theOldProp, int theNewPropIndex)
{
    const RWCae::MeshPropertyComponent* aNewPropPtr = myProperties.Seek(theNewPropIndex);

    RWCae::MeshPropertyComponent aNewProp = aNewPropPtr != nullptr ? *aNewPropPtr : RWCae::MeshPropertyComponent();
    if (theOldProp == aNewProp) {
        return;
    }

    bool toKeepOldProp = false;
    for (NCollection_DataMap<int, RWCae::MeshPropertyComponent>::Iterator anIter(myProperties);
         anIter.More() && !toKeepOldProp;
         anIter.Next()) {
        if (anIter.Key() == theNewPropIndex) {
            continue;
        }

        const RWCae::MeshPropertyComponent& aProp = anIter.Value();
        if (aProp.Property.IsNull()) {
            continue;
        }

        if (aProp == theOldProp) {
            toKeepOldProp = myAttachedColorMappers.IsBound(aProp);
        }
    }

    toKeepOldProp      = toKeepOldProp || !myAttachedColorMappers.IsBound(theOldProp);
    bool toKeepNewProp = aNewProp.Property.IsNull() || myAttachedColorMappers.IsBound(aNewProp);
    if (toKeepNewProp && toKeepOldProp) {
        return;
    }

    attachColorMapper(theOldProp, aNewProp, toKeepOldProp, toKeepNewProp);
}

// =======================================================================
// function : attachColorMapper
// =======================================================================
void MeshPropertyData::attachColorMapper(const RWCae::MeshPropertyComponent& theOldProp,
                                         const RWCae::MeshPropertyComponent& theNewProp,
                                         bool                                  toKeepOldProp,
                                         bool                                  toKeepNewProp)
{
    RWCae::MeshPropertyComponent aNewProp = theNewProp;

    Handle(MeshColorMapper) aColorMapper;

    if (!toKeepNewProp && !toKeepOldProp) {
        aColorMapper = myAttachedColorMappers.Find(theOldProp);
        myAttachedColorMappers.UnBind(theOldProp);
        if (!myToKeepFreeColorMappers) {
            if (myOnColorMapperChangeCallback != nullptr) {
                myOnColorMapperChangeCallback(RWCae::MeshPropertyComponent(), aColorMapper);
            }
            aColorMapper = new MeshColorMapper();
        }
        aColorMapper->SetMeshProperty(theNewProp.Property, theNewProp.ComponentIndex);
        myAttachedColorMappers.Bind(theNewProp, aColorMapper);
    } else if (!toKeepNewProp && toKeepOldProp) {
        if (!myFreeColorMappers.IsEmpty()) {
            aColorMapper = myFreeColorMappers.First();
            myFreeColorMappers.RemoveFirst();
        } else {
            aColorMapper = new MeshColorMapper();
        }
        aColorMapper->SetMeshProperty(theNewProp.Property, theNewProp.ComponentIndex);
        myAttachedColorMappers.Bind(theNewProp, aColorMapper);
    } else if (toKeepNewProp && !toKeepOldProp) {
        aColorMapper = myAttachedColorMappers.Find(theOldProp);
        myAttachedColorMappers.UnBind(theOldProp);
        if (myToKeepFreeColorMappers) {
            aColorMapper->SetMeshProperty(nullptr, -1);
            myFreeColorMappers.Append(aColorMapper);
        } else {
            aNewProp = RWCae::MeshPropertyComponent();
        }
    }

    if (myOnColorMapperChangeCallback != nullptr) {
        myOnColorMapperChangeCallback(aNewProp, aColorMapper);
    }
}

// ================================================================
// Function : ResetStaticCounter
// ================================================================
void MeshPropertyData::ResetStaticCounter()
{
    g_meshPropsCounter = 0;
}

} // namespace