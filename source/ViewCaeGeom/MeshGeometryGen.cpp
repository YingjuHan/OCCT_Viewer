#include <ViewCaeGeom/MeshGeometryGen.h>

namespace ViewCaeGeom {

// =======================================================================
// function : MeshGeometryGen
// =======================================================================
MeshGeometryGen::MeshGeometryGen(const Handle(MeshBvhData)& theBvhData) : MeshGeometry(theBvhData)
{
    //
}

// =======================================================================
// function : ~MeshGeometryGen
// =======================================================================
MeshGeometryGen::~MeshGeometryGen()
{
    //
}

// =======================================================================
// function : CalculateProperty
// =======================================================================
Handle(RWCae::MeshNVecProperty) MeshGeometryGen::CalculateProperty(
    const Handle(RWCae::MeshNVecProperty)&  theProp,
    const MeshGeomElem*                       theElem,
    const std::vector<RWCae::MeshEntityId>* theNodes) const
{
    if (theProp.IsNull() || myMeshData.IsNull()) {
        return nullptr;
    }

    for (const Handle(RWCae::MeshNVecProperty)& aProp : myMeshData->Properties()) {
        if (aProp->Id() == theProp->Id()) {
            return genPropValues(aProp, theElem, theNodes);
        }
    }

    return nullptr;
}

// =======================================================================
// function : CalculatePropertyComponent
// =======================================================================
RWCae::MeshPropertyComponent MeshGeometryGen::CalculatePropertyComponent(
    const Handle(RWCae::MeshNVecProperty)&  theProp,
    int                                       theCompIndex,
    const MeshGeomElem*                       theElem,
    const std::vector<RWCae::MeshEntityId>* theNodes) const
{
    if (theProp.IsNull() || myMeshData.IsNull()) {
        return RWCae::MeshPropertyComponent();
    }

    const TCollection_AsciiString& aCompId = theProp->GetRawComponent(theCompIndex)->Id();

    for (const Handle(RWCae::MeshNVecProperty)& aProp : myMeshData->Properties()) {
        if (aProp->Id() != theProp->Id()) {
            continue;
        }

        Handle(RWCae::MeshNVecProperty) aGenProp = genPropValues(aProp, theElem, theNodes);
        if (aGenProp.IsNull()) {
            return RWCae::MeshPropertyComponent();
        }

        for (int aCompIter = 0; aCompIter < aGenProp->NbRawComponents(); aCompIter++) {
            Handle(RWCae::MeshScalarPropertyBase) aComp = aGenProp->GetRawComponent(aCompIter);
            if (aComp->Id() == aCompId) {
                return RWCae::MeshPropertyComponent(aGenProp, aCompIter);
            }
        }
    }

    return RWCae::MeshPropertyComponent();
}

} // namespace               