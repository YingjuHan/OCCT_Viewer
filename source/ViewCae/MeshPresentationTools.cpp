#include <ViewCae/MeshPresentationTools.h>

#include <ViewCae/MeshPresentationScalar.h>
#include <ViewCae/MeshPresentationStreamlines.h>
#include <ViewCae/MeshPresentationVector.h>

#include <ViewCaeGeom/MeshGeometryTools.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>

namespace {
using MeshPartType = ViewCae::MeshPartType;

static constexpr const char* NAME_ID     = "id";
static constexpr const char* NAME_TIME   = "time";
static constexpr const char* NAME_GEOM   = "geom";
static constexpr const char* NAME_PART   = "part";
static constexpr const char* NAME_PARAMS = "params";

//! Read mesh geometry from Json string and retrieve corresponding presentation.
bool RestorePrs(const std::string&                       theGeomStr,
                const Handle(RWCae::MeshData)&         theMeshData,
                const Handle(ViewCaeGeom::MeshBvhData)& theBvhData,
                const TCollection_AsciiString&           thePrsId,
                int                                      theTime,
                ViewCae::ListOfMeshPresentations&       thePrsList,
                ViewCaeGeom::MapOfMeshGeometries&       theGeoms,
                Handle(ViewCae::MeshPresentationBase)&  thePrs)
{
    bool hasGeomChanged = false;

    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom;
    if (!ViewCaeGeom::MeshGeometryTools::RestoreFromJson(theGeomStr,
                                                          theMeshData,
                                                          theBvhData,
                                                          theTime,
                                                          theGeoms,
                                                          aMeshGeom,
                                                          hasGeomChanged)) {
        return false;
    }

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
        aGeom->GetMeshData();
    }

    for (const Handle(ViewCae::MeshPresentationBase)& aPrs : thePrsList) {
        if (aPrs->Id() == thePrsId) {
            thePrs = aPrs;
            break;
        }
    }

    if (thePrs.IsNull()) {
        thePrs = new ViewCae::MeshPresentationBase(aMeshGeom, thePrsId);
        thePrs->CreateGenGeometry();
        thePrsList.Append(thePrs);
    } else if (thePrs->GetMeshGeometry() != aMeshGeom) {
        Message::SendFail() << "MeshPresentationTools::RestoreFromJson: "
                            << "cannot redefine geometry for existing presentation";
        return false;
    } else if (hasGeomChanged) {
        for (const Handle(ViewCae::MeshPresentationBase)& aPrs : thePrsList) {
            if (aPrs->GetMeshGeometry() == aMeshGeom) {
                aPrs->SetToUpdateGenGeometry();
            }
        }
    }

    return true;
}

//! Read mesh presentation parameters from Json.
bool RestorePrsParams(const RWBase::JsonHelper::Value&            theParams,
                      const Handle(RWCae::MeshData)&              theMeshData,
                      const Handle(ViewCae::MeshPresentationBase)& thePrs,
                      int                                           thePart,
                      int                                           theTime)
{
    if (thePrs->ActiveTimeStep() != theTime) {
        thePrs->SetActiveTimeStep(theTime);
    }

    Handle(ViewCae::MeshPrsPartParams) aParams = thePrs->GetPartParams(thePart);
    if (aParams.IsNull()) {
        aParams = ViewCae::MeshPrsPartParamsFactory::Create(theMeshData, (MeshPartType)thePart);
        thePrs->SetPartParams(thePart, aParams);
    }

    bool isPrsPropsChanged = false;
    if (!aParams->FromJson(theParams, theMeshData, theTime, isPrsPropsChanged)) {
        Message::SendFail() << "MeshPresentationTools::RestoreFromJson: cannot parse part parameters";
        return false;
    }

    if (isPrsPropsChanged) {
        thePrs->SetToUpdateGenGeometry(thePart);
    }

    return true;
}

//! Dump mesh part to a separate mesh data.
bool ExtractMeshData(const Handle(ViewCaeGeom::MeshGeometry)&  theGeom,
                     MeshPartType                               thePartType,
                     const Handle(ViewCae::MeshPrsPartParams)& theParams,
                     const Handle(ViewCaeGeom::MeshGeometry)&  theGen)
{
    switch (thePartType) {
        case MeshPartType::Triangles3d:
            return ViewCae::MeshPartPrimArray<MeshPartType::Triangles3d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::Edges3d:
            return ViewCae::MeshPartPrimArray<MeshPartType::Edges3d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::Centroids3d:
            return ViewCae::MeshPartPrimArray<MeshPartType::Centroids3d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::SharpEdges3d:
            return ViewCae::MeshPartPrimArray<MeshPartType::SharpEdges3d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::FreeEdges3d:
            return ViewCae::MeshPartPrimArray<MeshPartType::FreeEdges3d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::Triangles2d:
            return ViewCae::MeshPartPrimArray<MeshPartType::Triangles2d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::Edges2d:
            return ViewCae::MeshPartPrimArray<MeshPartType::Edges2d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::Centroids2d:
            return ViewCae::MeshPartPrimArray<MeshPartType::Centroids2d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::SharpEdges2d:
            return ViewCae::MeshPartPrimArray<MeshPartType::SharpEdges2d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::FreeEdges2d:
            return ViewCae::MeshPartPrimArray<MeshPartType::FreeEdges2d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::Elems0d:
            return ViewCae::MeshPartPrimArray<MeshPartType::Elems0d>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::Points:
            return ViewCae::MeshPartPrimArray<MeshPartType::Points>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::MinMaxAnnotations:
            return ViewCae::MeshPartAnnotations<MeshPartType::MinMaxAnnotations>::ExtractMeshData(theGeom,
                                                                                                   theParams,
                                                                                                   theGen);
        case MeshPartType::GroupsInfo:
            return ViewCae::MeshPartAnnotations<MeshPartType::GroupsInfo>::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::Vectors: return ViewCae::MeshPartVectors::ExtractMeshData(theGeom, theParams, theGen);
        case MeshPartType::Streamlines:
            return ViewCae::MeshPartStreamlines::ExtractMeshData(theGeom, theParams, theGen);
        default: return false;
    }
}
}

namespace ViewCae {

// =======================================================================
// function : DumpToJson
// =======================================================================
bool MeshPresentationTools::DumpToJson(const Handle(MeshPresentation)& thePrs,
                                       MeshPartType                    thePart,
                                       std::string&                    theJsonStr)
{
    std::ostringstream                           aStr;
    rapidjson::OStreamWrapper                    aStrStream(aStr);
    rapidjson::Writer<rapidjson::OStreamWrapper> aWriter(aStrStream);

    aWriter.StartObject();

    aWriter.Key(NAME_ID);
    aWriter.String(thePrs->Id().ToCString());

    aWriter.Key(NAME_TIME);
    aWriter.Int(thePrs->ActiveTimeStep());

    std::string aGeomInfo = "";
    if (!ViewCaeGeom::MeshGeometryTools::DumpToJson(thePrs->GetMeshGeometry(), aGeomInfo)) {
        return false;
    }
    aWriter.Key(NAME_GEOM);
    aWriter.String(aGeomInfo.c_str());

    aWriter.Key(NAME_PART);
    aWriter.Int((int)thePart);

    Handle(MeshPrsPartParams) aParams = thePrs->GetPartParams((int)thePart);
    if (aParams.IsNull()) {
        Message::SendFail() << "MeshPresentationTools::DumpToJson: cannot find parameters for part '" << (int)thePart
                            << "'";
        return false;
    }

    aWriter.Key(NAME_PARAMS);
    aWriter.StartObject();
    aParams->ToJson(aWriter);
    aWriter.EndObject();

    aWriter.EndObject();

    aStr.flush();
    if (!aStr.good()) {
        Message::SendFail() << "MeshPresentationTools::DumpToJson: cannot write presentation parameters";
        return false;
    }

    theJsonStr = aStr.str();

    return true;
}

// =======================================================================
// function : RestoreFromJson
// =======================================================================
bool MeshPresentationTools::RestoreFromJson(const std::string&                       theJsonStr,
                                            const Handle(RWCae::MeshData)&         theMeshData,
                                            const Handle(ViewCaeGeom::MeshBvhData)& theBvhData,
                                            ListOfMeshPresentations&                 thePrsList,
                                            ViewCaeGeom::MapOfMeshGeometries&       theGeoms,
                                            Handle(MeshPresentationBase)&            thePrs,
                                            int&                                     thePart,
                                            bool&                                    theIsChanged)
{
    rapidjson::Document aReader;
    if (!RWBase::JsonHelper::ParseFromString(theJsonStr, aReader)) {
        Message::SendFail() << "MeshPresentationTools::RestoreFromJson: cannot parse json";
        return false;
    }

    const RWBase::JsonHelper::Value* anIdField      = RWBase::JsonHelper::FindStringMember(aReader, NAME_ID);
    const RWBase::JsonHelper::Value* aTimeField     = RWBase::JsonHelper::FindIntMember(aReader, NAME_TIME);
    const RWBase::JsonHelper::Value* aGeomField     = RWBase::JsonHelper::FindStringMember(aReader, NAME_GEOM);
    const RWBase::JsonHelper::Value* aPartField     = RWBase::JsonHelper::FindIntMember(aReader, NAME_PART);
    const RWBase::JsonHelper::Value* aParamsIdField = RWBase::JsonHelper::FindObjectMember(aReader, NAME_PARAMS);

    if (anIdField == nullptr || aTimeField == nullptr || aGeomField == nullptr || aPartField == nullptr
        || aParamsIdField == nullptr) {
        Message::SendFail() << "MeshPresentationTools::RestoreFromJson: cannot parse required fields";
        return false;
    }

    int aTime = aTimeField->GetInt();

    TCollection_AsciiString aPrsId = anIdField->GetString();

    if (!RestorePrs(aGeomField->GetString(), theMeshData, theBvhData, aPrsId, aTime, thePrsList, theGeoms, thePrs)) {
        return false;
    }

    thePart = aPartField->GetInt();

    if (!RestorePrsParams(*aParamsIdField, theMeshData, thePrs, thePart, aTime)) {
        return false;
    }

    Handle(ViewCaeGeom::MeshGeometryGen) aGenGeom = thePrs->GetGenGeometry(thePart);

    if (!aGenGeom->GetMeshData().IsNull()) {
        return true;
    }

    theIsChanged = true;

    Handle(ViewCae::MeshPrsPartParams) aParams = thePrs->GetPartParams(thePart);
    return ExtractMeshData(thePrs->GetMeshGeometry(), (MeshPartType)thePart, aParams, aGenGeom);
}

// =======================================================================
// function : GatherUsedProps
// =======================================================================
void MeshPresentationTools::GatherUsedProps(const ListOfMeshPresentations&                      thePrsList,
                                            const ViewCaeGeom::MapOfMeshGeometries&            theGeoms,
                                            NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps)
{
    ViewCaeGeom::MeshGeometryTools::GatherUsedProps(theGeoms, theProps);

    auto processPart = [&theProps](const Handle(MeshPrsPartParams)& theParams) {
        if (theParams->Property().IsDefined()) {
            theProps.Add(theParams->Property().Property);
        }

        if (Handle(MeshPrsVectorPartParams) aVectorParams = Handle(MeshPrsVectorPartParams)::DownCast(theParams)) {
            if (!aVectorParams->VectorProperty().IsNull()) {
                theProps.Add(aVectorParams->VectorProperty());
            }
        } else if (Handle(MeshPrsStreamlinesPartParams) aPrsStreamlinesParams =
                       Handle(MeshPrsStreamlinesPartParams)::DownCast(theParams)) {
            if (!aPrsStreamlinesParams->StreamlinesProperty().IsNull()) {
                theProps.Add(aPrsStreamlinesParams->StreamlinesProperty());
            }
        }
    };

    for (const Handle(MeshPresentationBase)& aPrsBase : thePrsList) {
        for (NCollection_DataMap<int, Handle(MeshPrsPartParams)>::Iterator anIter(aPrsBase->GetPartsParams());
             anIter.More();
             anIter.Next()) {
            processPart(anIter.Value());
        }
    }
}

} // namespace
