#include <ViewCaeGeom/MeshGeometryDeformed.h>

#include <RWCae/MeshTools.h>

namespace {
static constexpr const char* NAME_PROP  = "prop";
static constexpr const char* NAME_SCALE = "scale";
}

namespace ViewCaeGeom {

// =======================================================================
// function : MeshGeometry
// =======================================================================
MeshGeometryDeformed::MeshGeometryDeformed(const Handle(MeshBvhData)& theBvhData) : MeshGeometry(theBvhData)
{
    myCanModifyNodes = true;
}

// =======================================================================
// function : modifyNode
// =======================================================================
void MeshGeometryDeformed::modifyNode(const RWCae::MeshEntityId theNodeIdx,
                                      const MeshInterpolatedNode* theBaseNodes,
                                      Graphic3d_Vec3&             theNode) const
{
    if (myDeformedProperty.IsNull() || ::IsEqual(myEffectiveScale, 0.0)) {
        return;
    }

    Graphic3d_Vec3d anOffset(0.0);
    if (theBaseNodes == nullptr) {
        myDeformedProperty->GetVector(RWCae::MeshPropertyEntityInfo(theNodeIdx), anOffset);
        if (!RWCae::IsNaN(anOffset.x()) && !RWCae::IsNaN(anOffset.y()) && !RWCae::IsNaN(anOffset.z())) {
            theNode += Graphic3d_Vec3(anOffset) * float(myEffectiveScale);
        }
    } else {
        Graphic3d_Vec3 aResOffset(0.0f);
        for (const std::pair<RWCae::MeshEntityId, double>& aBaseNode : theBaseNodes->BaseNodes()) {
            myDeformedProperty->GetVector(RWCae::MeshPropertyEntityInfo(aBaseNode.first), anOffset);
            if (!RWCae::IsNaN(anOffset.x()) && !RWCae::IsNaN(anOffset.y()) && !RWCae::IsNaN(anOffset.z())) {
                aResOffset += Graphic3d_Vec3(anOffset) * float(aBaseNode.second);
            }
        }
        theNode += aResOffset * float(myEffectiveScale);
    }
}

// =======================================================================
// function : clear
// =======================================================================
void MeshGeometryDeformed::clear()
{
    MeshGeometry::clear();

    onDeformedUpdate();
}

// =======================================================================
// function : onDeformedUpdate
// =======================================================================
void MeshGeometryDeformed::onDeformedUpdate()
{
    SetToUpdate();

    for (MeshGeometry* aGeom : MeshGeomParents(myParentGeometry.get())) {
        myBvhData->RemoveBvhBoxSet(aGeom);
    }

    for (MeshGeometry* aGeom : MeshGeomChildren(myChildGeometry.get())) {
        myBvhData->RemoveBvhBoxSet(aGeom);
    }

    updateEffectiveScale();
}

// =======================================================================
// function : updateEffectiveScale
// =======================================================================
void MeshGeometryDeformed::updateEffectiveScale()
{
    double anEffectiveScale = DeformedScaleCoeff();
    if (!myDeformedProperty.IsNull() && anEffectiveScale != 0.0) {
        anEffectiveScale *= RWCae::MeshTools::ComputeVectorTrsfRatio(GetFirstParent()->GetMeshData(),
                                                                       myDeformedProperty);
    }
    setEffectiveScale(anEffectiveScale);
}

// =======================================================================
// function : ToJson
// =======================================================================
void MeshGeometryDeformed::ToJson(RWBase::JsonHelper::Writer& theJson)
{
    theJson.Key(NAME_PROP);
    theJson.String(!myDeformedProperty.IsNull() ? myDeformedProperty->Id().ToCString() : "");
    theJson.Key(NAME_SCALE);
    theJson.Double(myDeformedScaleCoeff);
}

// =======================================================================
// function : FromJson
// =======================================================================
bool MeshGeometryDeformed::FromJson(const RWBase::JsonHelper::Value& theJson,
                                    const Handle(RWCae::MeshData)&   theMeshData,
                                    int                                theTime,
                                    bool&                              theIsChanged)
{
    const RWBase::JsonHelper::Value* aDefPropIdField = RWBase::JsonHelper::FindStringMember(theJson, NAME_PROP);
    const RWBase::JsonHelper::Value* aDefScaleField  = RWBase::JsonHelper::FindDoubleMember(theJson, NAME_SCALE);
    if (aDefPropIdField == nullptr || aDefScaleField == nullptr) {
        return false;
    }

    const Handle(RWCae::MeshNVecProperty)& aCurProp = theMeshData->GetPropertyById(aDefPropIdField->GetString(),
                                                                                     theTime);
    if (myDeformedProperty != aCurProp) {
        SetDeformedProperty(aCurProp);
        RWCae::MeshTools::LoadMeshProperty(theMeshData, aCurProp);
        theIsChanged = true;
    }

    double aCurScale = aDefScaleField->GetDouble();
    if (!::IsEqual(myDeformedScaleCoeff, aCurScale)) {
        SetDeformedScaleCoeff(aCurScale);
        theIsChanged = true;
    }

    return true;
}

} // namespace               