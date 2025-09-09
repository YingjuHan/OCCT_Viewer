#include <ViewCae/MeshPresentationBase.h>

#include <RWCae/MeshTools.h>

#include <Standard_Atomic.hxx>

namespace {
static volatile int g_meshPrsCounter = 0; //!< counter used in auto-generated mesh presentation id

static constexpr const char* NAME_PROPID   = "propid";
static constexpr const char* NAME_PROPCOMP = "propcomp";
}

namespace ViewCae {
// =======================================================================
// function : ToJson
// =======================================================================
void MeshPrsPartParams::ToJson(RWBase::JsonHelper::Writer& theJson)
{
    theJson.Key(NAME_PROPID);
    theJson.String(myProperty.IsDefined() ? myProperty.Property->Id().ToCString() : "");
    theJson.Key(NAME_PROPCOMP);
    theJson.Int(myProperty.ComponentIndex);
}

// =======================================================================
// function : FromJson
// =======================================================================
bool MeshPrsPartParams::FromJson(const RWBase::JsonHelper::Value& theJson,
                                 const Handle(RWCae::MeshData)&   theMeshData,
                                 int                                theTime,
                                 bool&                              theIsChanged)
{
    const RWBase::JsonHelper::Value* aPropIdField   = RWBase::JsonHelper::FindStringMember(theJson, NAME_PROPID);
    const RWBase::JsonHelper::Value* aPropCompField = RWBase::JsonHelper::FindIntMember(theJson, NAME_PROPCOMP);
    if (aPropIdField == nullptr || aPropCompField == nullptr) {
        return false;
    }

    RWCae::MeshPropertyComponent aProp(theMeshData->GetPropertyById(aPropIdField->GetString(), theTime),
                                         aPropCompField->GetInt());
    if (!RWCae::MeshPropertyComponent::IsEqual(aProp, myProperty)) {
        myProperty = aProp;
        RWCae::MeshTools::LoadMeshProperty(theMeshData, aProp.Property);
        theIsChanged = true;
    }

    return true;
}

// =======================================================================
// function : MeshPresentationBase
// =======================================================================
MeshPresentationBase::MeshPresentationBase(const Handle(ViewCaeGeom::MeshGeometry)& theGeometry,
                                           const TCollection_AsciiString&            theId,
                                           bool                                      theToUpdateGeomId)
    : myGeometry(theGeometry),
      myId(theId)
{
    if (myId.IsEmpty()) {
        myId = TCollection_AsciiString("prs_") + TCollection_AsciiString(Standard_Atomic_Increment(&g_meshPrsCounter));
    }

    if (theToUpdateGeomId) {
        myGeometry->SetId(myId + "_geom_" + myGeometry->OrderIndex());
    }
}

// =======================================================================
// function : ~MeshPresentationBase
// =======================================================================
MeshPresentationBase::~MeshPresentationBase()
{
    //
}

// =======================================================================
// function : HasMeshGeometry
// =======================================================================
bool MeshPresentationBase::HasMeshGeometry(const Handle(ViewCaeGeom::MeshGeometry)& theGeom) const
{
    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(myGeometry.get())) {
        if (aGeom == theGeom) {
            return true;
        }
    }

    return false;
}

// =======================================================================
// function : AppendMeshGeometry
// =======================================================================
void MeshPresentationBase::AppendMeshGeometry(const Handle(ViewCaeGeom::MeshGeometry)& theGeom, bool theToUpdateGeomId)
{
    myGeometry->Append(theGeom);
    if (theToUpdateGeomId) {
        theGeom->SetId(myId + "_geom_" + theGeom->OrderIndex());
    }
}

// =======================================================================
// function : GetGenGeometry
// =======================================================================
Handle(ViewCaeGeom::MeshGeometryGen) MeshPresentationBase::GetGenGeometry(int thePart) const
{
    Handle(ViewCaeGeom::MeshGeometryGen) aRes;
    myGenGeometry.Find(thePart, aRes);
    return aRes;
}

// =======================================================================
// function : SetToUpdateGenGeometry
// =======================================================================
void MeshPresentationBase::SetToUpdateGenGeometry(int theMeshPart)
{
    if (theMeshPart != -1) {
        Handle(ViewCaeGeom::MeshGeometryGen) aGeom;
        if (myGenGeometry.Find(theMeshPart, aGeom)) {
            aGeom->SetToUpdate();
        }
        return;
    }

    for (NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>::Iterator anIter(myGenGeometry); anIter.More();
         anIter.Next()) {
        anIter.ChangeValue()->SetToUpdate();
    }
}

// ================================================================
// Function : CreateGenGeometry
// ================================================================
void MeshPresentationBase::CreateGenGeometry()
{
    myToUseGenGeom = true;

    Handle(ViewCaeGeom::MeshGeometryGen) aGeom3d = new ViewCaeGeom::MeshGeometryGen(myGeometry->GetBvhData());
    myGenGeometry.Bind((int)MeshPartType::Triangles3d, aGeom3d);
    myGenGeometry.Bind((int)MeshPartType::Edges3d, aGeom3d);
    myGenGeometry.Bind((int)MeshPartType::FreeEdges3d, aGeom3d);
    myGenGeometry.Bind((int)MeshPartType::SharpEdges3d, aGeom3d);
    myGenGeometry.Bind((int)MeshPartType::Centroids3d, aGeom3d);

    Handle(ViewCaeGeom::MeshGeometryGen) aGeom2d = new ViewCaeGeom::MeshGeometryGen(myGeometry->GetBvhData());
    myGenGeometry.Bind((int)MeshPartType::Triangles2d, aGeom2d);
    myGenGeometry.Bind((int)MeshPartType::Edges2d, aGeom2d);
    myGenGeometry.Bind((int)MeshPartType::FreeEdges2d, aGeom2d);
    myGenGeometry.Bind((int)MeshPartType::SharpEdges2d, aGeom2d);
    myGenGeometry.Bind((int)MeshPartType::Centroids2d, aGeom2d);

    Handle(ViewCaeGeom::MeshGeometryGen) aGeom0d = new ViewCaeGeom::MeshGeometryGen(myGeometry->GetBvhData());
    myGenGeometry.Bind((int)MeshPartType::Elems0d, aGeom0d);

    Handle(ViewCaeGeom::MeshGeometryGen) aGeomPoints = new ViewCaeGeom::MeshGeometryGen(myGeometry->GetBvhData());
    myGenGeometry.Bind((int)MeshPartType::Points, aGeomPoints);

    Handle(ViewCaeGeom::MeshGeometryGen) aGeomMinMax = new ViewCaeGeom::MeshGeometryGen(myGeometry->GetBvhData());
    myGenGeometry.Bind((int)MeshPartType::MinMaxAnnotations, aGeomMinMax);

    Handle(ViewCaeGeom::MeshGeometryGen) aGeomGroups = new ViewCaeGeom::MeshGeometryGen(myGeometry->GetBvhData());
    myGenGeometry.Bind((int)MeshPartType::GroupsInfo, aGeomGroups);

    Handle(ViewCaeGeom::MeshGeometryGen) aGeomVectors = new ViewCaeGeom::MeshGeometryGen(myGeometry->GetBvhData());
    myGenGeometry.Bind((int)MeshPartType::Vectors, aGeomVectors);

    Handle(ViewCaeGeom::MeshGeometryGen) aGeomStreamlines = new ViewCaeGeom::MeshGeometryGen(
        myGeometry->GetBvhData());
    myGenGeometry.Bind((int)MeshPartType::Streamlines, aGeomStreamlines);
}

// ================================================================
// Function : SetActiveTimeStep
// ================================================================
void MeshPresentationBase::SetActiveTimeStep(const int theTimeStep)
{
    if (myActiveTimeStep == theTimeStep) {
        return;
    }

    myActiveTimeStep = theTimeStep;
    for (NCollection_DataMap<int, Handle(ViewCaeGeom::MeshGeometryGen)>::Iterator anIter(myGenGeometry); anIter.More();
         anIter.Next()) {
        anIter.ChangeValue()->SetToUpdate();
    }
}

// ================================================================
// Function : ResetStaticCounter
// ================================================================
void MeshPresentationBase::ResetStaticCounter()
{
    g_meshPrsCounter = 0;
}

} // namespace