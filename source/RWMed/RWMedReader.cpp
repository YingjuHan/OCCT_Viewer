#include <RWMed/RWMedReader.h>

#include <RWCae/MeshAdaptor.h>
#include <RWCae/MeshFieldMapping.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshTools.h>

#include <Message.hxx>
#include <OSD_FileSystem.hxx>
#include <OSD_Path.hxx>

#include <Standard_WarningsDisable.hxx> // NOLINT
#ifdef HAVE_MEDFILE
    #ifdef _WIN32
        // MedFile dllimport - see medC_win_dll.h
        #define PPRO_NT
    #endif
    #include <med.h>
#endif
#include <Standard_WarningsRestore.hxx> // NOLINT

#include <algorithm>
#include <map>
#include <set>
#include <regex>
#include <unordered_map>

namespace RWMed {

using MeshNVecProperty       = RWCae::MeshNVecProperty;
using MeshScalarProperty     = RWCae::MeshScalarProperty;
using MeshPropertyBase       = RWCae::MeshPropertyBase;
using MeshNodeVecProperty    = RWCae::MeshNodeVecProperty;
using MeshScalarPropertyBase = RWCae::MeshScalarPropertyBase;
using MeshNVecPropertyType   = RWCae::MeshNVecPropertyType;
using MeshPropertyEntityInfo = RWCae::MeshPropertyEntityInfo;

#ifdef HAVE_MEDFILE
    #define MED_FIELD_VALUES (unsigned char*)
static constexpr int NB_SHFT2  = 2;
static constexpr int MED_AX_MP = 3;
static constexpr int NB_DIM_1D = 1;
static constexpr int NB_DIM_2D = 2;
static constexpr int NB_DIM_3D = 3;

//! Used to calculate offset for MED field values.
struct MEDPropOffsetI {
    MEDPropOffsetI(int     theComponentInd,
                   int     theComponentIndMpl,
                   int     theNbComponent,
                   int     theNbIntPnt,
                   med_int theGeoType,
                   int     theStartInd)
        : componentInd(theComponentInd),
          componentIndMPl(theComponentIndMpl),
          nbComponent(theNbComponent),
          nbIntPnt(theNbIntPnt),
          geoType(theGeoType),
          startInd(theStartInd)
    {
    }
    int     componentInd;
    int     componentIndMPl;
    int     nbComponent;
    int     nbIntPnt;
    med_int geoType;
    int     startInd;
};

//! Used to store number of elements in the mesh.
struct MeshElementsInfo {
    explicit MeshElementsInfo(med_geometry_type theType, int theNbElems)
        : type(theType),
          nbElements(theNbElems),
          nbOffset(0)
    {
    }
    med_geometry_type type;
    int               nbElements;
    int               nbOffset;
};

//! Generic mesh information.
struct MeshInfo {
    explicit MeshInfo(const Handle(RWCae::MeshData)& theMesh, const std::vector<MeshElementsInfo>& theNbEInfo)
        : outMesh(theMesh),
          nbEInfo(theNbEInfo)
    {
    }

    Handle(RWCae::MeshData)     outMesh;
    std::vector<MeshElementsInfo> nbEInfo;
};

//! Information for a new field extracted from Mapping file.
struct NFInfo {
    NFInfo(const std::string&                  theName,
           const std::string&                  theDescription,
           const std::vector<int>&             theComponentIndices,
           const std::vector<std::string>&     theComponentUnits,
           const std::vector<std::string>&     theSourceUnits,
           const RWCae::MeshMappingFieldType theValueType,
           const std::string&                  thePropId,
           const std::string&                  theTimeStepDescr,
           const std::string&                  theTimeUnits,
           const RWCae::MeshMappingCalcPoint theCalcPoint,
           const bool                          theNormalizeFlag)
        : name(theName),
          description(theDescription),
          componentIndices(theComponentIndices),
          componentUnits(theComponentUnits),
          sourceUnits(theSourceUnits),
          valueType(theValueType),
          propId(thePropId),
          timeStepDescr(theTimeStepDescr),
          timeUnits(theTimeUnits),
          calculationPoint(theCalcPoint),
          normalizeFlag(theNormalizeFlag)
    {
    }

    std::string                   name;
    std::string                   description;
    std::vector<int>              componentIndices;
    std::vector<std::string>      componentUnits;
    std::vector<std::string>      sourceUnits;
    RWCae::MeshMappingFieldType valueType;
    std::string                   propId;
    std::string                   timeStepDescr;
    std::string                   timeUnits;
    RWCae::MeshMappingCalcPoint calculationPoint;
    bool                          normalizeFlag;
};

struct TimeInfo {
    TimeInfo(const med_int   theNumIT,
             const med_int   theNumDT,
             const med_int   theMeshNumIt,
             const med_int   theMeshNumDt,
             const med_float theDT)
        : numIT(theNumIT),
          numDT(theNumDT),
          meshNumIt(theMeshNumIt),
          meshNumDt(theMeshNumDt),
          DT(theDT)
    {
    }

    med_int   numIT;
    med_int   numDT;
    med_int   meshNumIt;
    med_int   meshNumDt;
    med_float DT;
};

//! Information about MED field (extracted from MED file).
struct MEDFieldInfo {
    MEDFieldInfo()
        : meshName(""),
          fieldName(""),
          localMesh(MED_FALSE),
          fieldType(MED_DOUBLE),
          DTUnit(""),
          nbSteps(-1),
          nbComponent(-1)
    {
    }
    MEDFieldInfo(const std::string&              theMeshName,
                 const std::string&              theFieldName,
                 med_bool                        theLocalMesh,
                 med_field_type                  theFieldType,
                 const std::string&              theDTUnit,
                 med_int                         theNbSteps,
                 med_int                         theNbComponent,
                 const std::vector<std::string>& theComponentName)
        : meshName(theMeshName),
          fieldName(theFieldName),
          localMesh(theLocalMesh),
          fieldType(theFieldType),
          DTUnit(theDTUnit),
          nbSteps(theNbSteps),
          nbComponent(theNbComponent),
          componentName(theComponentName){};
    std::string              meshName;
    std::string              fieldName;
    med_bool                 localMesh;
    med_field_type           fieldType;
    std::string              DTUnit;
    med_int                  nbSteps;
    med_int                  nbComponent;
    std::vector<std::string> componentName;
};

//! Get string delimiter (used to generate property name for MeshData)
static const std::string& GetDelimiterStr()
{
    static const std::string aDelimStr = "__";
    return aDelimStr;
}

//! Generate artifact file: Generate the mapping JSON file based on the skipped fields from mapping file and dump it.
static bool GenerateArtifactFile(const TCollection_AsciiString&                    theFileName,
                                 const std::vector<RWCae::MeshFieldMappingInfo>& theMapInfo,
                                 const std::vector<int>&                           theSkippedFieldInd)
{
    std::vector<RWCae::MeshFieldMappingInfo> aFileMappingInfo;
    aFileMappingInfo.reserve(theSkippedFieldInd.size());
    for (size_t i = 0; i < theSkippedFieldInd.size(); i++) {
        const int ind = theSkippedFieldInd[i];
        aFileMappingInfo.push_back(theMapInfo[ind]);
    }

    RWCae::MeshFieldMappingWriter aMappingWriter(aFileMappingInfo);
    return aMappingWriter.Perform(theFileName);
}

//! Allows to convert MED index of node for different types of elements (i.e. geotypes)
//! to MeshData index.
//! This function is symmetrical for now (i.e. with currently used types)
//! so it can be used to convert index of MED node element to MeshData node element
//! in both ways.
static int ConvertM2MNodeIndex(int theMedType, int theInd)
{
    switch (theMedType) {
        case MED_POINT1:
        case MED_SEG2:
        case MED_SEG3:
        case MED_TRIA3:
        case MED_TRIA6:
        case MED_QUAD4:
        case MED_QUAD8:
        case MED_QUAD9: return theInd;
        case MED_TETRA4: {
            static constexpr int aT4Arr[4] = {0, 2, 1, 3};
            return aT4Arr[theInd];
        }
        case MED_HEXA8: {
            static constexpr int aH8Arr[8] = {0, 3, 2, 1, 4, 7, 6, 5};
            return aH8Arr[theInd];
        }
        case MED_PENTA6: {
            static constexpr int aP6Arr[6] = {0, 2, 1, 3, 5, 4};
            return aP6Arr[theInd];
        }
        case MED_PYRA5: {
            static constexpr int aPY5Arr[5] = {0, 3, 2, 1, 4};
            return aPY5Arr[theInd];
        }
        default: {
            return theInd; // return the same index for now
        }
    }
}

//! Extract the indices of components from component's masks
static bool ExtractIndicesComponents(const std::vector<std::string>& theComponentVec,
                                     const std::vector<std::string>& theComponentMaskVec,
                                     std::vector<int>&               theIndices)
{
    std::set<int> anIndSet;
    for (size_t i = 0; i < theComponentMaskVec.size(); i++) {
        std::regex aCompMaskRegex(theComponentMaskVec[i]);
        bool       isFound = false;
        for (int j = 0; j < (int)theComponentVec.size(); j++) {
            if (!std::regex_match(theComponentVec[j], aCompMaskRegex)) {
                continue;
            }
            if (anIndSet.count(j) == 0) {
                anIndSet.insert(j);
                theIndices.push_back(j);
                isFound = true;
                break;
            } else {
                return false;
            }
        }
        if (!isFound)
            return false;
    }
    return true;
}

//! Check if there are any field values for the given entity type (NODE, CELL or NDDE ELEMENT).
static bool CheckMedFieldValuesForEntType(const med_idt         theFId,
                                          const char* const     theFieldName,
                                          const med_int         theNumDT,
                                          const med_int         theNumIT,
                                          const med_entity_type theEntityType)
{
    med_int aProfileSize                         = 0;
    char    aProfileName[MED_NAME_SIZE + 1]      = {};
    char    aLocalizationName[MED_NAME_SIZE + 1] = {};
    med_int aNbIntPnt                            = 0;

    for (med_int anIt = 1; anIt <= MED_N_CELL_FIXED_GEO; anIt++) {
        med_geometry_type aGeoType = MED_GET_CELL_GEOMETRY_TYPE[anIt];
        if (MEDfieldnProfile(theFId,
                             theFieldName,
                             theNumDT,
                             theNumIT,
                             theEntityType,
                             aGeoType,
                             aProfileName,
                             aLocalizationName)
            < 1)
            continue;

        med_int aNbValues = MEDfieldnValueWithProfile(theFId,
                                                      theFieldName,
                                                      theNumDT,
                                                      theNumIT,
                                                      theEntityType,
                                                      aGeoType,
                                                      1,
                                                      MED_COMPACT_STMODE,
                                                      aProfileName,
                                                      &aProfileSize,
                                                      aLocalizationName,
                                                      &aNbIntPnt);
        if (aNbValues > 0)
            return true;
    }
    return false;
}

//! Get profile values for all profiles in MED file if they are present.
static bool GetMEDProfileInfo(const med_idt theFId, std::map<std::string, std::vector<med_int>>& theProfileNameToMEDPI)
{
    med_int aNbProf = MEDnProfile(theFId);
    if (aNbProf < 0) {
        Message::SendFail() << "Error: Unable to extract profile information from MED file";
        return false;
    }

    for (int i = 1; i <= aNbProf; i++) {
        char aProfileName[MED_NAME_SIZE + 1] = {};

        med_int aNVal = 0;
        if (MEDprofileInfo(theFId, i, aProfileName, &aNVal) < 0) {
            Message::SendFail() << "Error: Unable to extract profile values from MED file";
            theProfileNameToMEDPI.clear();
            return false;
        }

        std::vector<med_int>& aProfileValues = theProfileNameToMEDPI[aProfileName];
        aProfileValues.resize(aNVal);
        if (MEDprofileRd(theFId, aProfileName, aProfileValues.data()) < 0) {
            Message::SendFail() << "Error: Unable to extract profile values from MED file";
            theProfileNameToMEDPI.clear();
            return false;
        }
    }
    return true;
}

//! Read field values for given field/timestamp/entity type/geotype.
static bool ReadMedFieldValues(const med_idt           theFId,
                               const char* const       theFieldName,
                               const med_int           theNumDT,
                               const med_int           theNumIT,
                               const med_entity_type   theEntityType,
                               const med_geometry_type theGeoType,
                               int                     theNbComponent,
                               med_int&                theNbValues,
                               med_int&                theNbIntPnt,
                               std::vector<med_float>& theOutValues,
                               std::string&            theProfileName)
{
    med_int aProfileSize                         = 0;
    char    aProfileName[MED_NAME_SIZE + 1]      = {};
    char    aLocalizationName[MED_NAME_SIZE + 1] = {};
    if (MEDfieldnProfile(theFId,
                         theFieldName,
                         theNumDT,
                         theNumIT,
                         theEntityType,
                         theGeoType,
                         aProfileName,
                         aLocalizationName)
        < 1) {
        return false;
    }

    theNbValues = MEDfieldnValueWithProfile(theFId,
                                            theFieldName,
                                            theNumDT,
                                            theNumIT,
                                            theEntityType,
                                            theGeoType,
                                            1,
                                            MED_COMPACT_STMODE,
                                            aProfileName,
                                            &aProfileSize,
                                            aLocalizationName,
                                            &theNbIntPnt);
    if (theNbValues < 0) {
        return false;
    }

    // Message::SendInfo() << theFieldName << "  " << theEntityType << "     " << theNbComponent << "  " << aNbIntPnt;

    theOutValues.resize(theNbValues * theNbComponent * theNbIntPnt);

    if (MEDfieldValueWithProfileRd(theFId,
                                   theFieldName,
                                   theNumDT,
                                   theNumIT,
                                   theEntityType,
                                   theGeoType,
                                   MED_COMPACT_STMODE,
                                   aProfileName,
                                   MED_FULL_INTERLACE,
                                   MED_ALL_CONSTITUENT,
                                   MED_FIELD_VALUES(&theOutValues[0]))
        < 0) {
        return false;
    }
    theProfileName = aProfileName;
    return true;
}

//! Convert MED component string to array of string,
//! for example "DX     DY     DZ     " will be converted to a vector of strings:
//! ["DX", "DY", "DZ"].
static std::vector<std::string> ProccessCompName(const std::vector<char>& theCompName)
{
    std::string                        aStr(theCompName.begin(), theCompName.end());
    std::stringstream                  aStrStream(aStr);
    std::istream_iterator<std::string> aB(aStrStream);
    std::istream_iterator<std::string> anE;
    std::vector<std::string>           aRes(aB, anE);
    if (aRes.back().size() > 0 && aRes.back()[0] == '\0')
        aRes.pop_back();
    return aRes;
}

//! Used to set property data from MED structures to MeshData by calculating of offset.
template<typename T>
static void SetPropData(const MEDPropOffsetI&         theOffsetI,
                        const T&                      theElem,
                        const int                     theValueInd,
                        const std::vector<med_float>& theValues,
                        Handle(MeshNodeVecProperty)&  theNodeVecProp)
{
    RWCae::MeshPropertyEntityInfo anInfo(theOffsetI.startInd + theValueInd);
    theNodeVecProp->ReserveValueMap(anInfo.Index, theOffsetI.nbIntPnt);
    for (int i = 0; i < theOffsetI.nbIntPnt; i++) {
        anInfo.SubIndex       = theElem[i];
        const int aMNodeIndex = ConvertM2MNodeIndex(theOffsetI.geoType, i);
        double    aValue      = theValues[theOffsetI.componentInd + aMNodeIndex * theOffsetI.nbComponent
                                  + theValueInd * theOffsetI.nbIntPnt * theOffsetI.nbComponent];
        if (theOffsetI.componentIndMPl != -1)
            aValue *= theValues[theOffsetI.componentIndMPl + aMNodeIndex * theOffsetI.nbComponent
                                + theValueInd * theOffsetI.nbIntPnt * theOffsetI.nbComponent];
        theNodeVecProp->SetValue(anInfo, aValue);
    }
}

//! Set cell data (i.e. volume elements) of MeshData from MED "connection" data
static void SetCellData(const int                   theNumNodPerElem,
                        const std::vector<med_int>& theConnData,
                        med_geometry_type           theGeoType,
                        Handle(RWCae::MeshData)&  theOutMesh)
{
    // Set the volume data of MeshData
    std::vector<uint32_t> aNodeInds(theNumNodPerElem);

    for (int i = 0; i < (int)theConnData.size(); i += theNumNodPerElem) {
        for (int j = 0; j < theNumNodPerElem; j++) {
            int aNewInd  = ConvertM2MNodeIndex(theGeoType, j);
            aNodeInds[j] = theConnData[i + aNewInd] - 1;
        }
        switch (theGeoType) {
            case MED_POINT1: theOutMesh->AddElement(RWCae::MeshElement0d(aNodeInds.data())); break;
            case MED_SEG2: break;
            case MED_TRIA3: theOutMesh->AddElement(RWCae::MeshElementTriangle(aNodeInds.data())); break;
            case MED_TRIA6: theOutMesh->AddElement(RWCae::MeshElementTriangle2nd(aNodeInds.data())); break;
            case MED_QUAD4: theOutMesh->AddElement(RWCae::MeshElementQuad(aNodeInds.data())); break;
            case MED_QUAD8: theOutMesh->AddElement(RWCae::MeshElementQuad2nd(aNodeInds.data())); break;
            case MED_TETRA4: theOutMesh->AddElement(RWCae::MeshElementTetrahedron(aNodeInds.data())); break;
            case MED_TETRA10: theOutMesh->AddElement(RWCae::MeshElementTetrahedron2nd(aNodeInds.data())); break;
            case MED_HEXA8: theOutMesh->AddElement(RWCae::MeshElementHexahedron(aNodeInds.data())); break;
            case MED_HEXA20: theOutMesh->AddElement(RWCae::MeshElementHexahedron2nd(aNodeInds.data())); break;
            case MED_PENTA6: theOutMesh->AddElement(RWCae::MeshElementPrism(aNodeInds.data())); break;
            case MED_PENTA15: theOutMesh->AddElement(RWCae::MeshElementPrism2nd(aNodeInds.data())); break;
            case MED_PYRA5: theOutMesh->AddElement(RWCae::MeshElementPyramid(aNodeInds.data())); break;
            case MED_PYRA13: theOutMesh->AddElement(RWCae::MeshElementPyramid2nd(aNodeInds.data())); break;
            default: break;
        }
    }
}

//! Update property Id for two purposes:
//! - Use the field Id specified in the mapping file,
//! - Ensure that the same property Id is used for all time steps of a given field.
static void UpdatePropID(Handle(MeshNVecProperty)& theProp, NFInfo& theCNF)
{
    // theCNF.propId can be empty if no field Id was specified in a mapping file and we're processing the first
    // time step, in such a case store theProp's Id in theCNF.propId for further reuse.
    // Otherwise set theCNF.propId as theProp's Id.
    if (theCNF.propId.empty()) {
        theCNF.propId = std::string(theProp->Id().ToCString());
    } else {
        theProp->SetId(theCNF.propId.c_str());
    }
}

//! Set the measurement units to be used when displaying the property components to the users.
//! No value conversion occurs here as all imported field values are supposed to be in base SI units
//! and they are stored also in SI.
static void UpdatePropUnits(Handle(MeshNVecProperty)& theProp, NFInfo& theCNF)
{
    RWCae::MeshTools::UpdateMeshPropertyUnits(theProp, theCNF.valueType, theCNF.componentUnits);

    // Convert property value to base SI units if custom source units are used
    if (theCNF.sourceUnits.size() > 0) {
        RWCae::MeshTools::ConvertMeshProperty(theProp, theCNF.valueType, theCNF.sourceUnits);
    }
}

//! Set description, time, time step to property.
//! Also, update property Id in case of multiple timeStep fields.
static void UpdateProperty(const med_int                  theNumDT,
                           const med_float                theDT,
                           const TCollection_AsciiString& thePropsTempFolder,
                           NFInfo&                        theCNF,
                           Handle(MeshNVecProperty)&      theProp,
                           Handle(RWCae::MeshData)&     theOutMesh)
{
    theProp->SetTime(theDT, theNumDT);
    theProp->SetDescription(theCNF.description.c_str());
    theProp->SetTimeStepDescription(theCNF.timeStepDescr.c_str());
    theProp->SetTimeDisplayUnits(theCNF.timeUnits.c_str());

    UpdatePropID(theProp, theCNF);

    theProp->UpdateComponents();

    UpdatePropUnits(theProp, theCNF);

    if (theCNF.normalizeFlag) {
        theProp->NormalizeVector();
    }

    theOutMesh->ChangeProperties().Append(theProp);

    if (!thePropsTempFolder.IsEmpty()) {
        TCollection_AsciiString aPropFileName = theProp->Id() + "_" + theProp->TimeStep() + ".mdb";
        TCollection_AsciiString aPropPath     = OSD_Path::AbsolutePath(thePropsTempFolder, aPropFileName);

        if (!RWCae::MeshTools::DumpMeshProperty(theOutMesh, theProp, aPropPath)) {
            Message::SendWarning() << "MED field reader: Cannot dump property '" << theProp->Id()
                                   << "' to the temp file '" << aPropPath << "'";
            return;
        }

        theProp->Unload();
    }
}

//! Process tensor or set field (TENSOR6, TENSOR9, SET types)
static void ProcessTensorOrSetField(NFInfo&                             theCNF,
                                    const med_int                       theNbComp,
                                    const med_int                       theNumDT,
                                    const med_float                     theDT,
                                    const std::vector<std::string>&     theCompVecName,
                                    const std::unordered_map<int, int>* theProfInd2NewNodeInd,
                                    Handle(RWCae::MeshData)&          theOutMesh,
                                    const std::vector<med_float>&       theValues,
                                    MeshNVecPropertyType                type,
                                    const int                           aNbComponentsToStore,
                                    const TCollection_AsciiString&      thePropsTempFolder)
{
    int                      aNbNodes = theOutMesh->NbNodes();
    Handle(MeshNVecProperty) aSetProp =
        new MeshNVecProperty(theCNF.name.c_str(), aNbComponentsToStore, MeshPropertyBase::MappingType::Nodal, type);
    aSetProp->Init<MeshScalarProperty>();

    for (int i = 0; i < aNbComponentsToStore; i++) {
        Handle(MeshScalarProperty) aComp = Handle(MeshScalarProperty)::DownCast(aSetProp->GetComponent(i));
        aComp->Init(aNbNodes);
        aComp->SetName(theCompVecName[i].c_str());
    }

    std::vector<double> aSetValue(aNbComponentsToStore);
    for (int k = 0; k < aNbNodes; k++) {
        if (theProfInd2NewNodeInd == nullptr) {
            for (int i = 0; i < aNbComponentsToStore; i++)
                aSetValue[i] = theValues[theNbComp * k + theCNF.componentIndices[i]];

            aSetProp->SetVector(MeshPropertyEntityInfo(k), aSetValue);

        } else if (theProfInd2NewNodeInd->count(k) > 0) {
            int ni = theProfInd2NewNodeInd->at(k);
            for (int i = 0; i < aNbComponentsToStore; i++)
                aSetValue[i] = theValues[theNbComp * ni + theCNF.componentIndices[i]];

            aSetProp->SetVector(MeshPropertyEntityInfo(k), aSetValue);
        }
    }

    UpdateProperty(theNumDT, theDT, thePropsTempFolder, theCNF, aSetProp, theOutMesh);
}

//! Init property and set component vector names.
static void InitScalarProp(const Handle(MeshNVecProperty)& theVec3Prop,
                           const int                       theNbNodes,
                           const std::vector<std::string>& theCompVecName,
                           const std::vector<int>&         theCompInds,
                           const bool                      theIsVec3)
{
    theVec3Prop->Init<MeshScalarProperty>();
    const size_t aNbCompInds = !theIsVec3 ? theCompInds.size() - 1 : theCompInds.size();
    for (size_t j = 0; j < aNbCompInds; j++) {
        Handle(MeshScalarProperty) aComp = Handle(MeshScalarProperty)::DownCast(theVec3Prop->GetComponent(j));
        aComp->Init(theNbNodes);
        const std::string aCompName = theIsVec3
                                        ? theCompVecName[theCompInds[j]]
                                        : theCompVecName[theCompInds[j]] + " * " + theCompVecName[theCompInds.back()];
        aComp->SetName(aCompName.c_str());
        aComp->SetEnabled(theIsVec3);
    }
}

//! Process vector field (VECTOR & VECTOR4 types)
static void ProcessVectorField(NFInfo&                             theCNF,
                               const med_int                       theNbComp,
                               const med_int                       theNumDT,
                               const med_float                     theDT,
                               const std::vector<std::string>&     theCompVecName,
                               const std::unordered_map<int, int>* theProfInd2NewNodeInd,
                               Handle(RWCae::MeshData)&          theOutMesh,
                               const std::vector<med_float>&       theValues,
                               const bool                          isVec3,
                               const TCollection_AsciiString&      thePropsTempFolder)
{
    constexpr int aNbComponents = 3;
    int           aNbNodes      = theOutMesh->NbNodes();
    Standard_ASSERT_VOID((isVec3 && theCNF.componentIndices.size() == 3)
                             || (!isVec3 && theCNF.componentIndices.size() == 4),
                         "ProcessVectorField: number of components is not equal to 3 or 4");
    const int Xc = theCNF.componentIndices[0];
    const int Yc = theCNF.componentIndices[1];
    const int Zc = theCNF.componentIndices[2];

    Handle(MeshNVecProperty) aVec3Prop = new MeshNVecProperty(theCNF.name.c_str(),
                                                              aNbComponents,
                                                              MeshPropertyBase::MappingType::Nodal,
                                                              MeshNVecPropertyType::Vector);
    InitScalarProp(aVec3Prop, aNbNodes, theCompVecName, theCNF.componentIndices, isVec3);

    Graphic3d_Vec3d aVec3Value;
    for (int k = 0; k < aNbNodes; k++) {
        if (theProfInd2NewNodeInd == nullptr) {
            isVec3 ? aVec3Value.SetValues(theValues[theNbComp * k + Xc],
                                          theValues[theNbComp * k + Yc],
                                          theValues[theNbComp * k + Zc])
                   : aVec3Value.SetValues(
                       theValues[theNbComp * k + Xc] * theValues[theNbComp * k + theCNF.componentIndices[3]],
                       theValues[theNbComp * k + Yc] * theValues[theNbComp * k + theCNF.componentIndices[3]],
                       theValues[theNbComp * k + Zc] * theValues[theNbComp * k + theCNF.componentIndices[3]]);
            aVec3Prop->SetVector(MeshPropertyEntityInfo(k), aVec3Value);

        } else if (theProfInd2NewNodeInd->count(k) > 0) {
            int ni = theProfInd2NewNodeInd->at(k);
            isVec3 ? aVec3Value.SetValues(theValues[theNbComp * ni + Xc],
                                          theValues[theNbComp * ni + Yc],
                                          theValues[theNbComp * ni + Zc])
                   : aVec3Value.SetValues(
                       theValues[theNbComp * ni + Xc] * theValues[theNbComp * ni + theCNF.componentIndices[3]],
                       theValues[theNbComp * ni + Yc] * theValues[theNbComp * ni + theCNF.componentIndices[3]],
                       theValues[theNbComp * ni + Zc] * theValues[theNbComp * ni + theCNF.componentIndices[3]]);
            aVec3Prop->SetVector(MeshPropertyEntityInfo(k), aVec3Value);
        }
    }

    UpdateProperty(theNumDT, theDT, thePropsTempFolder, theCNF, aVec3Prop, theOutMesh);
}

//! Process scalar field (SCALAR type)
static void ProcessScalarField(NFInfo&                             theCNF,
                               const med_int                       theNbComponent,
                               const med_int                       theNumDT,
                               const med_float                     theDT,
                               const std::string&                  theCompVecName,
                               const std::unordered_map<int, int>* theProfInd2NewNodeInd,
                               Handle(RWCae::MeshData)&          theOutMesh,
                               const std::vector<med_float>&       theValues,
                               const TCollection_AsciiString&      thePropsTempFolder)
{
    Standard_ASSERT_VOID(theCNF.componentIndices.size() == 1,
                         "ProcessScalarField: number of components is not equal to 1");
    const int aNbNodes = theOutMesh->NbNodes();
    const int c        = theCNF.componentIndices[0];

    Handle(MeshNVecProperty) aProp = new MeshNVecProperty(theCNF.name.c_str(), 1, MeshPropertyBase::MappingType::Nodal);
    aProp->Init<MeshScalarProperty>();
    Handle(MeshScalarProperty) aScalarProp = Handle(MeshScalarProperty)::DownCast(aProp->GetComponent(0));
    aScalarProp->Init(aNbNodes);
    aScalarProp->SetName(theCompVecName.c_str());

    for (int k = 0; k < aNbNodes; k++) {
        if (theProfInd2NewNodeInd == nullptr) {
            double aValue = theValues[c + theNbComponent * k];
            aScalarProp->SetValue(RWCae::MeshPropertyEntityInfo(k), aValue);
        } else if (theProfInd2NewNodeInd->count(k) > 0) {
            int    ni     = theProfInd2NewNodeInd->at(k);
            double aValue = theValues[c + theNbComponent * ni];
            aScalarProp->SetValue(RWCae::MeshPropertyEntityInfo(k), aValue);
        }
    }

    UpdateProperty(theNumDT, theDT, thePropsTempFolder, theCNF, aProp, theOutMesh);
}

//! Process fields on NODES.
//! Read and store scalar, vector, tensor and set fields to mesh data.
static void ProcessField(const med_int                       theNbComponent,
                         const med_int                       theNumDT,
                         const med_float                     theDT,
                         const std::vector<std::string>&     theCompVecName,
                         std::vector<NFInfo>&                theNFArr,
                         const std::unordered_map<int, int>* theProfInd2NewNodeInd,
                         Handle(RWCae::MeshData)&          theOutMesh,
                         const std::vector<med_float>&       theValues,
                         const TCollection_AsciiString&      thePropsTempFolder)
{
    for (NFInfo& aCNF : theNFArr) {
        // Filter out fields with non-matching calculation point
        if (aCNF.calculationPoint != RWCae::MeshMappingCalcPoint::Unknown
            && aCNF.calculationPoint != RWCae::MeshMappingCalcPoint::Nodes) {
            continue;
        }

        const RWCae::MeshMappingFieldType aVT = aCNF.valueType;
        if (aVT == RWCae::MeshMappingFieldType::Vector || aVT == RWCae::MeshMappingFieldType::Vector4) {
            ProcessVectorField(aCNF,
                               theNbComponent,
                               theNumDT,
                               theDT,
                               theCompVecName,
                               theProfInd2NewNodeInd,
                               theOutMesh,
                               theValues,
                               aVT == RWCae::MeshMappingFieldType::Vector,
                               thePropsTempFolder);
        } else if (aVT == RWCae::MeshMappingFieldType::Scalar) {
            ProcessScalarField(aCNF,
                               theNbComponent,
                               theNumDT,
                               theDT,
                               theCompVecName[aCNF.componentIndices[0]],
                               theProfInd2NewNodeInd,
                               theOutMesh,
                               theValues,
                               thePropsTempFolder);
        } else if (aVT == RWCae::MeshMappingFieldType::Tensor6 || aVT == RWCae::MeshMappingFieldType::Tensor9
                   || aVT == RWCae::MeshMappingFieldType::Set) {
            ProcessTensorOrSetField(aCNF,
                                    theNbComponent,
                                    theNumDT,
                                    theDT,
                                    theCompVecName,
                                    theProfInd2NewNodeInd,
                                    theOutMesh,
                                    theValues,
                                    aVT == RWCae::MeshMappingFieldType::Set ? MeshNVecPropertyType::ScalarSet
                                                                              : MeshNVecPropertyType::Tensor,
                                    (int)aCNF.componentIndices.size(),
                                    thePropsTempFolder);
        } else {
            Message::SendWarning() << "MED field reader: Can't process the following type: " << (int)aCNF.valueType
                                   << ", (skipped)";
        }
    }
}

//! Process node information.
//! Read MED fields values for NODES and fill corresponding MeshData properties.
static void ProcessNodes(const med_idt                                              theFId,
                         const std::string&                                         theFNameStr,
                         const med_int                                              theNbComponent,
                         const med_int                                              theNumIT,
                         const med_int                                              theNumDT,
                         const med_float                                            theDT,
                         const std::vector<std::string>&                            theCompVecName,
                         std::vector<NFInfo>&                                       theNFArr,
                         const std::map<std::string, std::unordered_map<int, int>>& theProfileNameToInvPI,
                         const TCollection_AsciiString&                             thePropsTempFolder,
                         Handle(RWCae::MeshData)&                                 theOutMesh)
{
    std::vector<med_float> aValues;
    med_int                aNbValues = 0;
    med_int                aNbIntPnt = 0;
    std::string            aProfileName;
    if (!ReadMedFieldValues(theFId,
                            theFNameStr.c_str(),
                            theNumDT,
                            theNumIT,
                            MED_NODE,
                            MED_NONE,
                            theNbComponent,
                            aNbValues,
                            aNbIntPnt,
                            aValues,
                            aProfileName)) {
        return;
    }

    std::map<std::string, std::unordered_map<int, int>>::const_iterator anITPN = theProfileNameToInvPI.find(
        aProfileName);
    const std::unordered_map<int, int>* aProfInd2NewNodeInd = anITPN == theProfileNameToInvPI.end() ? nullptr
                                                                                                    : &(anITPN->second);

    Standard_ASSERT_VOID(aNbIntPnt == 1, "ProcessNodes: aNbIntPnt is not equal to 1");
    Standard_ASSERT_VOID(theNbComponent == (int)theCompVecName.size(),
                         "ProcessNodes: Component vector size is different from actual number of components");

    ProcessField(theNbComponent,
                 theNumDT,
                 theDT,
                 theCompVecName,
                 theNFArr,
                 aProfInd2NewNodeInd,
                 theOutMesh,
                 aValues,
                 thePropsTempFolder);
}

//! Template function to store property data for 3D elements
template<typename T>
void StorePropData(const MEDPropOffsetI&          theMOP,
                   const med_int                  theNbValues,
                   const RWCae::MeshAdaptor<T>& theMeshAdaptor,
                   const std::vector<med_float>&  theValues,
                   Handle(MeshNodeVecProperty)&   theNodeVecProp)
{
    int aNbElems = theMeshAdaptor.NbElements();
    if (theNbValues > 0 && theNbValues == aNbElems) {
        for (int k = 0; k < aNbElems; k++) {
            SetPropData(theMOP, theMeshAdaptor.GetElement(k), k, theValues, theNodeVecProp);
        }
    }
}

//! Used to store property data from MED structures to MeshData.
static void StorePropDataExt(const std::vector<MeshElementsInfo>& theMEI,
                             const int                            theCompInd0,
                             const int                            theCompIndMpl0,
                             const med_int                        theNbComponent,
                             const med_int                        theNbValues,
                             const med_int                        theNbIntPnt,
                             const int                            theNbEl,
                             const med_geometry_type              theGeoType,
                             const size_t                         theInd,
                             const std::vector<med_float>&        theValues,
                             Handle(MeshNodeVecProperty)          theNodeVecProp,
                             const Handle(RWCae::MeshData)&     theOutMesh)
{
    const int      aStartInd = theMEI[theInd].nbOffset - theNbEl;
    MEDPropOffsetI aMOP(theCompInd0, theCompIndMpl0, theNbComponent, theNbIntPnt, theGeoType, aStartInd);

    auto store = [&aMOP, theNbValues, &theValues, &theNodeVecProp](const auto& theAdaptor) {
        StorePropData(aMOP, theNbValues, theAdaptor, theValues, theNodeVecProp);
    };

    switch (theGeoType) {
        case MED_TRIA3: store(RWCae::MeshAdaptorTriangle(*theOutMesh)); break;
        case MED_TRIA6: store(RWCae::MeshAdaptorTriangle2nd(*theOutMesh)); break;
        case MED_QUAD4: store(RWCae::MeshAdaptorQuad(*theOutMesh)); break;
        case MED_QUAD8: store(RWCae::MeshAdaptorQuad2nd(*theOutMesh)); break;
        case MED_TETRA4: store(RWCae::MeshAdaptorTetrahedron(*theOutMesh)); break;
        case MED_TETRA10: store(RWCae::MeshAdaptorTetrahedron2nd(*theOutMesh)); break;
        case MED_PYRA5: store(RWCae::MeshAdaptorPyramid(*theOutMesh)); break;
        case MED_PYRA13: store(RWCae::MeshAdaptorPyramid2nd(*theOutMesh)); break;
        case MED_PENTA6: store(RWCae::MeshAdaptorPrism(*theOutMesh)); break;
        case MED_PENTA15: store(RWCae::MeshAdaptorPrism2nd(*theOutMesh)); break;
        case MED_HEXA8: store(RWCae::MeshAdaptorHexahedron(*theOutMesh)); break;
        case MED_HEXA20: store(RWCae::MeshAdaptorHexahedron2nd(*theOutMesh)); break;
        default: break;
    }
}

//! Read and store MED field values in Mesh data.
static void ReadAndStoreMEDFieldValues(const med_idt                        theFId,
                                       const std::string&                   theFNameStr,
                                       const med_int                        theNumIT,
                                       const med_int                        theNumDT,
                                       const std::vector<MeshElementsInfo>& theMEI,
                                       const std::vector<int>               theCompInds,
                                       const med_int                        theNbComponent,
                                       Handle(MeshNVecProperty)&            theNVec,
                                       const Handle(RWCae::MeshData)&     theOutMesh,
                                       const bool                           isVec4,
                                       bool&                                theToStore)
{
    med_int                aNbValues = 0;
    med_int                aNbIntPnt = 0;
    std::vector<med_float> aValues;
    for (size_t i = 0; i < theMEI.size(); i++) {
        med_geometry_type aGeoType = theMEI[i].type;
        const int         aNbEl    = theMEI[i].nbElements;
        if (aNbEl == 0)
            continue;

        std::string aProfileName;
        if (!ReadMedFieldValues(theFId,
                                theFNameStr.c_str(),
                                theNumDT,
                                theNumIT,
                                MED_NODE_ELEMENT,
                                aGeoType,
                                theNbComponent,
                                aNbValues,
                                aNbIntPnt,
                                aValues,
                                aProfileName))
            continue;

        theToStore                      = true;
        const int theNbComponentToStore = !isVec4 ? theCompInds.size() : theCompInds.size() - 1;
        for (int j = 0; j < theNbComponentToStore; j++) {
            StorePropDataExt(theMEI,
                             theCompInds[j],
                             !isVec4 ? -1 : theCompInds.back(),
                             theNbComponent,
                             aNbValues,
                             aNbIntPnt,
                             aNbEl,
                             aGeoType,
                             i,
                             aValues,
                             Handle(MeshNodeVecProperty)::DownCast(theNVec->GetComponent(j)),
                             theOutMesh);
        }
    }
}

//! Process N component property.
//! Init property, set name of components and time stamps,
//! read and store MED field values in Mesh data.
static void ProcessPropertyInt(const med_idt                        theFId,
                               const std::string&                   theFNameStr,
                               const med_int                        theNbComponent,
                               const med_int                        theNumIT,
                               const med_int                        theNumDT,
                               const med_float                      theDT,
                               const std::vector<std::string>&      theCompVecName,
                               const std::vector<MeshElementsInfo>& theMEI,
                               const int                            theNbElem,
                               Handle(RWCae::MeshData)&           theOutMesh,
                               NFInfo&                              theCNF,
                               const std::vector<int>&              theCompInds,
                               Handle(MeshNVecProperty)             theProp,
                               const bool                           theIsVec4,
                               const int                            theNbNodes,
                               bool                                 theNodeVecAsScalar,
                               const TCollection_AsciiString&       thePropsTempFolder)
{
    if (!theProp.IsNull()) {
        theProp->Init<MeshNodeVecProperty>();
        size_t NbCompInds = theIsVec4 ? theCompInds.size() - 1 : theCompInds.size();
        for (size_t j = 0; j < NbCompInds; j++) {
            Handle(MeshNodeVecProperty) aNodeVecProp = Handle(MeshNodeVecProperty)::DownCast(theProp->GetComponent(j));
            aNodeVecProp->Init(theNbElem, theNbNodes, false, theNodeVecAsScalar);
            const std::string compName = !theIsVec4 ? theCompVecName[theCompInds[j]]
                                                    : theCompVecName[theCompInds[j]] + " * "
                                                          + theCompVecName[theCompInds.back()];

            if (theIsVec4)
                aNodeVecProp->SetEnabled(false);
            aNodeVecProp->SetName(compName.c_str());
        }

        bool toStore = false;
        ReadAndStoreMEDFieldValues(theFId,
                                   theFNameStr,
                                   theNumIT,
                                   theNumDT,
                                   theMEI,
                                   theCompInds,
                                   theNbComponent,
                                   theProp,
                                   theOutMesh,
                                   theIsVec4,
                                   toStore);
        if (toStore) {
            UpdateProperty(theNumDT, theDT, thePropsTempFolder, theCNF, theProp, theOutMesh);
        }
    }
}

//! Process N component property: scalar, vector, tensor or set type are supported.
//! Init property, set name of components and time stamps,
//! read and store MED field values in Mesh data.
static void ProcessProperty(const std::vector<int>&              theCompInds,
                            const bool                           theIsVec4,
                            const RWCae::MeshMappingFieldType  theVT,
                            NFInfo&                              theCNF,
                            const med_idt                        theFId,
                            const std::string&                   theFNameStr,
                            const med_int                        theNbComponent,
                            const med_int                        theNumIT,
                            const med_int                        theNumDT,
                            const med_float                      theDT,
                            const std::vector<std::string>&      theCompVecName,
                            const std::vector<MeshElementsInfo>& theMEI,
                            bool                                 theNodeVecAsScalar,
                            const int                            theNbElem,
                            Handle(RWCae::MeshData)&           theOutMesh,
                            const int                            theNbNodes,
                            const TCollection_AsciiString&       thePropsTempFolder)
{
    Handle(MeshNVecProperty) aProp;
    if (theVT == RWCae::MeshMappingFieldType::Scalar) {
        Standard_ASSERT_VOID(theCompInds.size() == 1, "ProcessProperty: theCompInds is not equal to 1");
        aProp = new MeshNVecProperty(theCNF.name.c_str(), 1, MeshPropertyBase::MappingType::Elemental);
    } else if (theVT == RWCae::MeshMappingFieldType::Vector || theIsVec4) {
        aProp = new MeshNVecProperty(theCNF.name.c_str(),
                                     3,
                                     MeshPropertyBase::MappingType::Elemental,
                                     MeshNVecPropertyType::Vector);
    } else if (theVT == RWCae::MeshMappingFieldType::Tensor6 || theVT == RWCae::MeshMappingFieldType::Tensor9
               || theVT == RWCae::MeshMappingFieldType::Set) {
        aProp = new MeshNVecProperty(theCNF.name.c_str(),
                                     theCompInds.size(),
                                     MeshPropertyBase::MappingType::Elemental,
                                     theVT == RWCae::MeshMappingFieldType::Set ? MeshNVecPropertyType::ScalarSet
                                                                                 : MeshNVecPropertyType::Tensor);
    } else {
        Message::SendWarning() << "MED field reader: Can't process the following type: " << (int)theVT << ", (skipped)";
    }

    ProcessPropertyInt(theFId,
                       theFNameStr,
                       theNbComponent,
                       theNumIT,
                       theNumDT,
                       theDT,
                       theCompVecName,
                       theMEI,
                       theNbElem,
                       theOutMesh,
                       theCNF,
                       theCompInds,
                       aProp,
                       theIsVec4,
                       theNbNodes,
                       theNodeVecAsScalar,
                       thePropsTempFolder);
}

//! Process cell information.
//! Read MED fields values for NODES and fill corresponding MeshData properties
//! TO DO Currently allows only to read from MED, without any further processing.
/* TO DO: Process field Values on CELLS, disabled for now. @code
static void ProcessCells(const med_idt      theFId,
                         const std::string& theFNameStr,
                         const med_int      theNbComponent,
                         const med_int      theNumIT,
                         const med_int      theNumDT)
{
    med_int                aNbValues = 0;
    med_int                aNbIntPnt = 0;
    std::vector<med_float> aValues;
    for (med_int anTt = 1; anTt <= MED_N_CELL_FIXED_GEO; anTt++) {
        med_geometry_type aGeoType = MED_GET_CELL_GEOMETRY_TYPE[anTt];
        ReadMedFieldValues(theFId,
                           theFNameStr.c_str(),
                           theNumDT,
                           theNumIT,
                           MED_CELL,
                           aGeoType,
                           theNbComponent,
                           aNbValues,
                           aNbIntPnt,
                           aValues);
    }
} @endcode */

//! Process node elements information.
//! Read MED fields values for NODE ELEMENTs and fill corresponding MeshData properties
//! This function checks if there are any MED field values for MED_NODE_ELEMENT type;
//! then iterates through different supported geotypes (traingles, quadrangles, etc.)
//! and fills the corresponding range by using SetPropData().
//! If there are no values for some geotype then such property in MeshData remain unfilled.
static void ProcessNodeElements(const med_idt                        theFId,
                                const std::string&                   theFNameStr,
                                const med_int                        theNbComponent,
                                const med_int                        theNumIT,
                                const med_int                        theNumDT,
                                const med_float                      theDT,
                                const std::vector<std::string>&      theCompVecName,
                                const std::vector<MeshElementsInfo>& theMEI,
                                bool                                 theNodeVecAsScalar,
                                const TCollection_AsciiString&       thePropsTempFolder,
                                int                                  theNbElem,
                                std::vector<NFInfo>&                 theNFArr,
                                Handle(RWCae::MeshData)&           theOutMesh)
{
    if (!CheckMedFieldValuesForEntType(theFId, theFNameStr.c_str(), theNumDT, theNumIT, MED_NODE_ELEMENT)) {
        return;
    }

    const int aNbNodes = theOutMesh->NbNodes();
    for (size_t i = 0; i < theNFArr.size(); i++) {
        NFInfo& aCNF = theNFArr[i];

        // Filter out fields with non-matching calculation point
        if (aCNF.calculationPoint != RWCae::MeshMappingCalcPoint::Unknown
            && aCNF.calculationPoint != RWCae::MeshMappingCalcPoint::ElementNodes) {
            continue;
        }

        const std::vector<int>&             aCompInds = aCNF.componentIndices;
        Handle(MeshNVecProperty)            aProp;
        const bool                          isVec4 = (aCNF.valueType == RWCae::MeshMappingFieldType::Vector4);
        const RWCae::MeshMappingFieldType aVT    = aCNF.valueType;

        ProcessProperty(aCompInds,
                        isVec4,
                        aVT,
                        aCNF,
                        theFId,
                        theFNameStr,
                        theNbComponent,
                        theNumIT,
                        theNumDT,
                        theDT,
                        theCompVecName,
                        theMEI,
                        theNodeVecAsScalar,
                        theNbElem,
                        theOutMesh,
                        aNbNodes,
                        thePropsTempFolder);
    }
}

//! Get MED field information (filling of array)
static bool GetMEDFieldsInfo(const med_idt theFId, std::vector<MEDFieldInfo>& theFIArr)
{
    char           aMeshName[MED_NAME_SIZE + 1]  = {};
    char           aFieldName[MED_NAME_SIZE + 1] = {};
    med_bool       aLocalMesh                    = MED_FALSE;
    med_field_type aFieldType                    = MED_DOUBLE;
    char           aDTUnit[MED_SNAME_SIZE + 1]   = {};
    med_int        aNbSteps                      = 0;

    // Read the number of fields
    const med_idt aNbFields = MEDnField(theFId);
    if (aNbFields < 0) {
        Message::SendFail() << "Error while reading the number of fields in the MED file.";
        return false;
    }

    theFIArr.clear();
    theFIArr.resize(aNbFields);

    // Read actual fields
    for (int f = 0; f < aNbFields; f++) {
        const med_int aNbComponent = MEDfieldnComponent(theFId, f + 1);
        if (aNbComponent < 0) {
            Message::SendFail() << "Error while reading the number of components of field in the MED file.";
            theFIArr.clear();
            return false;
        }

        std::vector<char> aComponentName(aNbComponent * MED_NAME_SIZE + 1);
        std::vector<char> aComponentUnit(aNbComponent * MED_NAME_SIZE + 1);
        if (MEDfieldInfo(theFId,
                         f + 1,
                         aFieldName,
                         aMeshName,
                         &aLocalMesh,
                         &aFieldType,
                         aComponentName.data(),
                         aComponentUnit.data(),
                         aDTUnit,
                         &aNbSteps)
            < 0) {
            Message::SendFail() << "Error while reading of field information in the MED file.";
            theFIArr.clear();
            return false;
        }
        theFIArr[f] = MEDFieldInfo(aMeshName,
                                   aFieldName,
                                   aLocalMesh,
                                   aFieldType,
                                   aDTUnit,
                                   aNbSteps,
                                   aNbComponent,
                                   ProccessCompName(aComponentName));
    }
    return true;
}

//! Extract the component indices from component array by component mask
//! and check that number of values corresponds to valueType.
static bool CheckAndExtract(const std::vector<std::string>&     theComponentVec,
                            const std::vector<std::string>&     theComponentMaskVec,
                            const RWCae::MeshMappingFieldType theValueType,
                            std::vector<int>&                   theCompIndexes)
{
    static constexpr int nbVec3Cmp  = 3;
    static constexpr int nbVec4Cmp  = 4;
    static constexpr int nbTens6Cmp = 6;
    static constexpr int nbTens9Cmp = 9;
    if (!ExtractIndicesComponents(theComponentVec, theComponentMaskVec, theCompIndexes)) {
        Message::SendWarning() << "There is a problem with extracting of component indices";
        return false;
    }
    size_t aNbComp = theCompIndexes.size();
    if (theValueType == RWCae::MeshMappingFieldType::Scalar) {
        if (aNbComp != 1) {
            Message::SendWarning() << "valueType and number of extracted components are inconsistent";
            return false;
        }
    } else if (theValueType == RWCae::MeshMappingFieldType::Vector) {
        if (aNbComp != nbVec3Cmp) {
            Message::SendWarning() << "valueType and number of extracted components are inconsistent";
            return false;
        }
    } else if (theValueType == RWCae::MeshMappingFieldType::Vector4) {
        if (aNbComp != nbVec4Cmp) {
            Message::SendWarning() << "valueType and number of extracted components are inconsistent";
            return false;
        }
    } else if (theValueType == RWCae::MeshMappingFieldType::Tensor6) {
        if (aNbComp != nbTens6Cmp) {
            Message::SendWarning() << "valueType and number of extracted components are inconsistent";
            return false;
        }
    } else if (theValueType == RWCae::MeshMappingFieldType::Tensor9) {
        if (aNbComp != nbTens9Cmp) {
            Message::SendWarning() << "valueType and number of extracted components are inconsistent";
            return false;
        }
    } else if (theValueType == RWCae::MeshMappingFieldType::Set) {
        if (aNbComp < 1) {
            Message::SendWarning() << "valueType and number of extracted components are inconsistent";
            return false;
        }
    } else {
        Message::SendWarning() << "unknown valueType";
        return false;
    }

    return true;
}

//! Process the mapping file: prepare a map of new fields corresponding to mapping file.
static void ExtractTimeStepInfo(std::vector<RWCae::MeshFieldMappingInfo>& theMapInfo,
                                std::vector<std::string>&                   theTimeStepInfo)
{
    for (auto anItem = theMapInfo.begin(); anItem != theMapInfo.end();) {
        if (anItem->FieldMask == "TIME_STEP_ID") {
            theTimeStepInfo[0] = anItem->FieldDescription;
            theMapInfo.erase(anItem);
        } else if (anItem->FieldMask == "TIME_STEP_VALUE") {
            theTimeStepInfo[1] = anItem->ComponentUnits[0];
            theMapInfo.erase(anItem);
        } else {
            ++anItem;
        }
    }
}

//! Process the mapping file: prepare a map of new fields corresponding to mapping file.
static void ProccessFieldMapping(const std::vector<MEDFieldInfo>&            theFIArr,
                                 std::vector<RWCae::MeshFieldMappingInfo>& theMapInfo,
                                 std::map<int, std::vector<NFInfo>>&         theOutMap,
                                 std::vector<int>&                           theSkippedFieldInd)
{
    // Process special mapping entries related to time step information and remove them from theMapInfo
    static constexpr int     nbTimeStepEntries = 2;
    std::vector<std::string> aTimeStepInfo(nbTimeStepEntries);
    ExtractTimeStepInfo(theMapInfo, aTimeStepInfo);

    theOutMap.clear();
    // Process normal mapping entries corresponding to MED fields.
    // We don't filter the MED fields by calculation point here because the necessary information
    // is retrieved from a MED file later in ProcessNodes() and ProcessNodeElements(),
    // hence calculation point filtering is delegated to these downstream functions.
    // Due to this, the mapping entries not found in a MED file due to calculation point mismatch
    // do not appear in GenerateArtifactFile() output and this makes it potentially less suitable for MED import
    // diagnostics.
    // In practice, MED fields written by Code_Aster always have different names for different calculation points
    // and the field name mask includes calculation point implicitly that might not be the case for MED files written
    // by another solver.
    // TO DO: decide if GenerateArtifactFile() is needed or can be dropped.
    for (size_t i = 0; i < theMapInfo.size(); i++) {
        std::regex aFieldMaskRegex(theMapInfo[i].FieldMask);
        bool       aStat = false;
        for (size_t f = 0; f < theFIArr.size(); f++) {
            aStat = std::regex_match(theFIArr[f].fieldName, aFieldMaskRegex);
            if (!aStat)
                continue;

            // Match OK
            std::vector<int> aCompIndexes;
            if (!CheckAndExtract(theFIArr[f].componentName,
                                 theMapInfo[i].ComponentMasks,
                                 theMapInfo[i].ValueType,
                                 aCompIndexes))
                continue;

            theOutMap[f].push_back(NFInfo(theFIArr[f].fieldName,
                                          theMapInfo[i].FieldDescription,
                                          aCompIndexes,
                                          theMapInfo[i].ComponentUnits,
                                          theMapInfo[i].SourceUnits,
                                          theMapInfo[i].ValueType,
                                          theMapInfo[i].FieldId,
                                          aTimeStepInfo[0],
                                          aTimeStepInfo[1],
                                          theMapInfo[i].CalcPoint,
                                          theMapInfo[i].NormalizeFlag));
            break;
        }
        if (!aStat) {
            theSkippedFieldInd.push_back(i);
            Message::SendWarning() << "Nothing found in MED field for mapping with field mask '"
                                   << theMapInfo[i].FieldMask << "'";
        }
    }
}

//! Generate simple mapping, used if there is no mapping file
static void GenerateSimpleMapping(const std::vector<MEDFieldInfo>&    theFIArr,
                                  std::map<int, std::vector<NFInfo>>& theOutMap)
{
    theOutMap.clear();
    for (size_t f = 0; f < theFIArr.size(); f++) {
        const MEDFieldInfo&      aFI        = theFIArr[f];
        const std::string&       aFieldName = aFI.fieldName;
        std::vector<NFInfo>      aNFArr;
        std::vector<std::string> anEmptyUnits;
        for (int i = 0; i < aFI.nbComponent; i++) {
            const std::string& theComponentName = aFI.componentName[i];
            std::vector<int>   aCInd{i};
            aNFArr.push_back(NFInfo(aFieldName + GetDelimiterStr() + theComponentName,
                                    "",
                                    aCInd,
                                    anEmptyUnits,
                                    anEmptyUnits,
                                    RWCae::MeshMappingFieldType::Scalar,
                                    "",
                                    "",
                                    "",
                                    RWCae::MeshMappingCalcPoint::Unknown,
                                    false));
        }
        theOutMap[f] = aNFArr;
    }
}

//! Extract profile information from MED file and prepare inverse map.
static bool ProcessProfiles(const med_idt                                        theFId,
                            std::map<std::string, std::unordered_map<int, int>>& theProfileNameToInvPI)
{
    std::map<std::string, std::vector<med_int>> aProfileNameToMEDPI;
    if (!GetMEDProfileInfo(theFId, aProfileNameToMEDPI)) {
        return false;
    }

    for (std::map<std::string, std::vector<med_int>>::iterator anIt = aProfileNameToMEDPI.begin();
         anIt != aProfileNameToMEDPI.end();
         anIt++) {
        std::unordered_map<int, int>& aProfInd2NewNodeInd = theProfileNameToInvPI[anIt->first];
        for (int i = 0; i < (int)(anIt->second).size(); i++) {
            med_int anIndex              = (anIt->second)[i] - 1;
            aProfInd2NewNodeInd[anIndex] = i;
        }
    }
    return true;
}

//! Store field values in mesh data: for nodes and node elements
static bool StoreFieldValues(const med_idt                                              theFId,
                             const MEDFieldInfo&                                        theCurrFieldI,
                             std::vector<MeshElementsInfo>&                             theMEI,
                             std::vector<NFInfo>&                                       theNFArr,
                             const int                                                  theNbElem,
                             const std::map<std::string, std::unordered_map<int, int>>& theProfileNameToInvPI,
                             bool                                                       theNodeVecAsScalar,
                             const TCollection_AsciiString&                             thePropsTempFolder,
                             TimeInfo&                                                  theTI,
                             Handle(RWCae::MeshData)&                                 theOutMesh)
{
    // Read field values for each computing step
    for (med_int aCSIt = 0; aCSIt < theCurrFieldI.nbSteps; aCSIt++) {
        if (MEDfieldComputingStepMeshInfo(theFId,
                                          theCurrFieldI.fieldName.c_str(),
                                          aCSIt + 1,
                                          &theTI.numDT,
                                          &theTI.numIT,
                                          &theTI.DT,
                                          &theTI.meshNumDt,
                                          &theTI.meshNumIt)
            < 0) {
            Message::SendFail() << "Error while reading of time stamp information in the MED file.";
            return false;
        }

        // Process field Values on NODES
        ProcessNodes(theFId,
                     theCurrFieldI.fieldName,
                     theCurrFieldI.nbComponent,
                     theTI.numIT,
                     theTI.numDT,
                     theTI.DT,
                     theCurrFieldI.componentName,
                     theNFArr,
                     theProfileNameToInvPI,
                     thePropsTempFolder,
                     theOutMesh);

        // Process field Values on CELLS, disabled for now..
        // TO DO: ProcessCells(theFId, aCurrFieldI.FieldName, aCurrFieldI.ComponentName, aNumIT, aNumDT, anOutMesh);

        // Process field Values on NODE ELEMENTS
        ProcessNodeElements(theFId,
                            theCurrFieldI.fieldName,
                            theCurrFieldI.nbComponent,
                            theTI.numIT,
                            theTI.numDT,
                            theTI.DT,
                            theCurrFieldI.componentName,
                            theMEI,
                            theNodeVecAsScalar,
                            thePropsTempFolder,
                            theNbElem,
                            theNFArr,
                            theOutMesh);
    }

    return true;
}

//! Update the mesh element info (number of elements)
static void UpdateMeshElementInfo(const Handle(RWCae::MeshData)& theOutMesh,
                                  const std::string&               theMeshName,
                                  std::map<std::string, MeshInfo>& theName2Mesh)
{
    // ordering should be the same as in MED_N_CELL_FIXED_GEO
    std::vector<MeshElementsInfo> aNbEInfo;
    aNbEInfo.push_back(MeshElementsInfo(MED_POINT1, theOutMesh->NbElems(RWCae::MeshElemType::Elem0d)));
    aNbEInfo.push_back(MeshElementsInfo(MED_TRIA3, theOutMesh->NbElems(RWCae::MeshElemType::Triangle)));
    aNbEInfo.push_back(MeshElementsInfo(MED_TRIA6, theOutMesh->NbElems(RWCae::MeshElemType::Triangle2nd)));
    aNbEInfo.push_back(MeshElementsInfo(MED_QUAD4, theOutMesh->NbElems(RWCae::MeshElemType::Quad)));
    aNbEInfo.push_back(MeshElementsInfo(MED_QUAD8, theOutMesh->NbElems(RWCae::MeshElemType::Quad2nd)));
    aNbEInfo.push_back(MeshElementsInfo(MED_TETRA4, theOutMesh->NbElems(RWCae::MeshElemType::Tetrahedron)));
    aNbEInfo.push_back(MeshElementsInfo(MED_TETRA10, theOutMesh->NbElems(RWCae::MeshElemType::Tetrahedron2nd)));
    aNbEInfo.push_back(MeshElementsInfo(MED_PYRA5, theOutMesh->NbElems(RWCae::MeshElemType::Pyramid)));
    aNbEInfo.push_back(MeshElementsInfo(MED_PYRA13, theOutMesh->NbElems(RWCae::MeshElemType::Pyramid2nd)));
    aNbEInfo.push_back(MeshElementsInfo(MED_PENTA6, theOutMesh->NbElems(RWCae::MeshElemType::Prism)));
    aNbEInfo.push_back(MeshElementsInfo(MED_PENTA15, theOutMesh->NbElems(RWCae::MeshElemType::Prism2nd)));
    aNbEInfo.push_back(MeshElementsInfo(MED_HEXA8, theOutMesh->NbElems(RWCae::MeshElemType::Hexahedron)));
    aNbEInfo.push_back(MeshElementsInfo(MED_HEXA20, theOutMesh->NbElems(RWCae::MeshElemType::Hexahedron2nd)));
    MeshInfo aMI(theOutMesh, aNbEInfo);
    theName2Mesh.insert(std::pair<std::string, MeshInfo>(theMeshName, aMI));
}

//! Read MED fields to MeshData.
static bool ReadMEDFields(const med_idt                               theFId,
                          std::map<std::string, MeshInfo>&            theName2Mesh,
                          bool                                        theUseMapping,
                          bool                                        theNodeVecAsScalar,
                          std::vector<RWCae::MeshFieldMappingInfo>& theMapInfo,
                          const std::string&                          theArtifactFilePath,
                          const TCollection_AsciiString&              thePropsTempFolder,
                          bool&                                       isMissingFields)
{
    TimeInfo                  aTI(0, 0, 0, 0, 0.0f);
    std::vector<MEDFieldInfo> aFIArr;
    if (!GetMEDFieldsInfo(theFId, aFIArr))
        return false;

    std::map<int, std::vector<NFInfo>> anOutMapNF;
    if (theUseMapping) {
        std::vector<int> aSkippedFieldInd;
        ProccessFieldMapping(aFIArr, theMapInfo, anOutMapNF, aSkippedFieldInd);
        isMissingFields = !aSkippedFieldInd.empty();
        if (isMissingFields && !theArtifactFilePath.empty()
            && !GenerateArtifactFile(theArtifactFilePath.c_str(), theMapInfo, aSkippedFieldInd))
            return false;
    } else
        GenerateSimpleMapping(aFIArr, anOutMapNF);

    std::map<std::string, std::unordered_map<int, int>> aProfileNameToInvPI;
    if (!ProcessProfiles(theFId, aProfileNameToInvPI)) {
        return false;
    }

    // Read actual fields
    for (int f = 0; f < (int)aFIArr.size(); f++) {
        const MEDFieldInfo& aCurrFieldI = aFIArr[f];
        if (theName2Mesh.count(aCurrFieldI.meshName) == 0)
            continue;

        if (anOutMapNF.count(f) == 0)
            continue;

        std::vector<NFInfo> aNFArr = anOutMapNF[f];

        MeshInfo&                      aMI       = theName2Mesh.at(aCurrFieldI.meshName);
        Handle(RWCae::MeshData)&     anOutMesh = aMI.outMesh;
        std::vector<MeshElementsInfo>& aMEI      = aMI.nbEInfo;

        int aNbElem = 0;
        for (size_t i = 0; i < aMEI.size(); i++) {
            aMEI[i].nbOffset = aMEI[i].nbElements + aNbElem;
            aNbElem += aMEI[i].nbElements;
        }

        if (!StoreFieldValues(theFId,
                              aCurrFieldI,
                              aMEI,
                              aNFArr,
                              aNbElem,
                              aProfileNameToInvPI,
                              theNodeVecAsScalar,
                              thePropsTempFolder,
                              aTI,
                              anOutMesh)) {
            return false;
        }
    }

    return true;
}

//! Read and store geometry elements data
static bool ReadAndStoreElementData(const med_idt              theFId,
                                    const std::string&         theMeshName,
                                    Handle(RWCae::MeshData)& theOutMesh)
{
    med_bool aCoordinateChangement = MED_FALSE;
    med_bool aGeoTransformation    = MED_FALSE;
    for (med_int anIt = 1; anIt <= MED_N_CELL_FIXED_GEO; anIt++) {
        med_geometry_type aGeoType = MED_GET_CELL_GEOMETRY_TYPE[anIt];
        // get number of elements for ths type
        med_int aNbElem = MEDmeshnEntity(theFId,
                                         theMeshName.c_str(),
                                         MED_NO_DT,
                                         MED_NO_IT,
                                         MED_CELL,
                                         aGeoType,
                                         MED_CONNECTIVITY,
                                         MED_NODAL,
                                         &aCoordinateChangement,
                                         &aGeoTransformation);
        if (aNbElem < 0) {
            Message::SendFail() << "Error while reading of number of elements in the MED file.";
            return false;
        }

        // if number of elements for this type is 0 => skip
        if (aNbElem == 0)
            continue;

        // read indices of element
        const int            aNumNodPerElem = aGeoType % 100;
        std::vector<med_int> aConnData(aNbElem * aNumNodPerElem);

        if (MEDmeshElementConnectivityRd(theFId,
                                         theMeshName.c_str(),
                                         MED_NO_DT,
                                         MED_NO_IT,
                                         MED_CELL,
                                         aGeoType,
                                         MED_NODAL,
                                         MED_FULL_INTERLACE,
                                         &aConnData[0])
            < 0) {
            Message::SendFail() << "Error while reading of element information in the MED file.";
            return false;
        }

        SetCellData(aNumNodPerElem, aConnData, aGeoType, theOutMesh);
    }
    return true;
}

//! Read and store geometry node data
static bool ReadAndStoreNodeData(const med_idt              theFId,
                                 const std::string&         theMeshName,
                                 const med_int              theSpaceDim,
                                 Handle(RWCae::MeshData)& theOutMesh)
{
    med_bool aCoordinateChangement = MED_FALSE;
    med_bool aGeoTransformation    = MED_FALSE;
    // read the number of nodes
    med_int aNbNodes = MEDmeshnEntity(theFId,
                                      theMeshName.c_str(),
                                      MED_NO_DT,
                                      MED_NO_IT,
                                      MED_NODE,
                                      MED_NO_GEOTYPE,
                                      MED_COORDINATE,
                                      MED_NO_CMODE,
                                      &aCoordinateChangement,
                                      &aGeoTransformation);
    if (aNbNodes <= 0) {
        Message::SendFail() << "Error while reading of number of nodes in the MED file.";
        return false;
    }

    // read coordinates
    std::vector<med_float> aCoords(aNbNodes * theSpaceDim);
    if (MEDmeshNodeCoordinateRd(theFId, theMeshName.c_str(), MED_NO_DT, MED_NO_IT, MED_FULL_INTERLACE, &aCoords[0])
        < 0) {
        Message::SendFail() << "Error while reading of node coordinates in the MED file.";
        return false;
    }

    theOutMesh->SetName(theMeshName.c_str());
    for (size_t k = 0; k < aCoords.size(); k += theSpaceDim) {
        if (theSpaceDim == NB_DIM_1D)
            theOutMesh->AddNodePosition(gp_XYZ(aCoords[k], 0, 0));
        else if (theSpaceDim == NB_DIM_2D)
            theOutMesh->AddNodePosition(gp_XYZ(aCoords[k], aCoords[k + 1], 0));
        else if (theSpaceDim == NB_DIM_3D)
            theOutMesh->AddNodePosition(gp_XYZ(aCoords[k], aCoords[k + 1], aCoords[k + NB_SHFT2]));
    }
    return true;
}

//! Extract and check mesh information from MED.
static bool CheckMeshInfo(const med_idt theFId, const med_int theInd, std::string& theMeshName, med_int& theSpaceDim)
{
    char             aMeshName[MED_NAME_SIZE + 1]           = {};
    char             aMeshDescription[MED_COMMENT_SIZE + 1] = {};
    med_int          aMeshDim                               = 0;
    med_int          aNbSteps                               = 0;
    med_sorting_type aSortingType                           = MED_SORT_UNDEF;
    med_mesh_type    aMeshType                              = MED_UNDEF_MESH_TYPE;
    med_axis_type    anAxisType                             = MED_UNDEF_AXIS_TYPE;
    char             aDTUnit[MED_SNAME_SIZE + 1]            = {};

    // get space dimension
    theSpaceDim = MEDmeshnAxis(theFId, theInd + 1);
    if (theSpaceDim < 0) {
        Message::SendFail() << "Error while reading of dimension information in the MED file.";
        return false;
    }

    if (theSpaceDim > NB_DIM_3D) {
        Message::SendFail() << "Error : space dimension > 3 is not supported.";
        return false;
    }

    char anAxisName[MED_AX_MP * MED_SNAME_SIZE + 1] = {};
    char anUnitName[MED_AX_MP * MED_SNAME_SIZE + 1] = {};

    // read mesh information
    if (MEDmeshInfo(theFId,
                    theInd + 1,
                    aMeshName,
                    &theSpaceDim,
                    &aMeshDim,
                    &aMeshType,
                    aMeshDescription,
                    aDTUnit,
                    &aSortingType,
                    &aNbSteps,
                    &anAxisType,
                    anAxisName,
                    anUnitName)
        < 0) {
        Message::SendFail() << "Error while reading of mesh information in the MED file.";
        return false;
    }
    theMeshName = aMeshName;
    return true;
}

//! Read MED geometry to MeshData.
static bool ReadMEDGeometry(const med_idt theFId, std::map<std::string, MeshInfo>& theName2Mesh)
{
    std::string aMeshName;
    med_bool    aCoordinateChangement = MED_FALSE;
    med_bool    aGeoTransformation    = MED_FALSE;

    med_int aNbMeshes = MEDnMesh(theFId);
    if (aNbMeshes < 0) {
        Message::SendFail() << "Error while reading of the number of mesh in the MED file.";
        return false;
    }

    for (med_int i = 0; i < aNbMeshes; i++) {
        med_int aSpaceDim = -1;
        if (!CheckMeshInfo(theFId, i, aMeshName, aSpaceDim)) {
            theName2Mesh.clear();
            return false;
        }

        Handle(RWCae::MeshData) anOutMesh = new RWCae::MeshData();
        if (!ReadAndStoreNodeData(theFId, aMeshName, aSpaceDim, anOutMesh)) {
            theName2Mesh.clear();
            return false;
        }
        // read number of types of elements
        int aNbGeo = MEDmeshnEntity(theFId,
                                    aMeshName.c_str(),
                                    MED_NO_DT,
                                    MED_NO_IT,
                                    MED_CELL,
                                    MED_GEO_ALL,
                                    MED_CONNECTIVITY,
                                    MED_NODAL,
                                    &aCoordinateChangement,
                                    &aGeoTransformation);
        if (aNbGeo < 0) {
            Message::SendFail() << "Error while reading of types of elements in the MED file.";
            theName2Mesh.clear();
            return false;
        }

        if (!ReadAndStoreElementData(theFId, aMeshName, anOutMesh)) {
            theName2Mesh.clear();
            return false;
        }
        UpdateMeshElementInfo(anOutMesh, aMeshName, theName2Mesh);
    }

    return true;
}

//! Update value range.
static void UpdateValueRange(RWCae::MeshNVecProperties& theProps)
{
    RWCae::PropertyRanges aPropsLimits;
    RWCae::MeshTools::fillPropertiesRange(theProps, aPropsLimits);
    RWCae::MeshTools::restorePropertiesRange(theProps, aPropsLimits);
}

//! Perform reading of geometry and fields from MED file.
static bool PerformMED(const med_idt                    theFid,
                       const std::string&               theMappingFilePath,
                       const std::string&               theArtifactFilePath,
                       const TCollection_AsciiString&   thePropsTempFolder,
                       bool                             theNodeVecAsScalar,
                       std::map<std::string, MeshInfo>& theNameToMeshData,
                       RWMedReader::ReaderStatus&       theStatus)
{
    if (!ReadMEDGeometry(theFid, theNameToMeshData)) {
        return false;
    }

    std::vector<RWCae::MeshFieldMappingInfo> aFileMappingInfo;
    bool                                       useMapping = false;
    if (!theMappingFilePath.empty()) {
        // Message::SendInfo() << "Mapping file '" << theMappingFilePath << "' will be used";
        useMapping = true;

        RWCae::MeshFieldMappingReader aMappingReader;
        if (!aMappingReader.Perform(theMappingFilePath.c_str())) {
            return false;
        }

        aFileMappingInfo = aMappingReader.GetFieldMappingInfo();
    } else {
        Message::SendWarning() << "Warning: mapping file is not defined for RWMedReader";
    }

    bool isMissingFields = false;
    if (!ReadMEDFields(theFid,
                       theNameToMeshData,
                       useMapping,
                       theNodeVecAsScalar,
                       aFileMappingInfo,
                       theArtifactFilePath.c_str(),
                       thePropsTempFolder,
                       isMissingFields)) {
        return false;
    }
    for (std::map<std::string, MeshInfo>::iterator aMIt = theNameToMeshData.begin(); aMIt != theNameToMeshData.end();
         aMIt++) {
        RWCae::MeshNVecProperties& aProps = aMIt->second.outMesh->ChangeProperties();
        UpdateValueRange(aProps);

        aMIt->second.outMesh->UpdateCachedMinMax();
    }
    theStatus = isMissingFields ? RWMedReader::ReaderStatus::MissingFields : RWMedReader::ReaderStatus::Done;
    return true;
}

//! Class wrapper for MED I/O used to open/close MED file.
class RWMedReader::MEDFileIOWrapper {
public:
    explicit MEDFileIOWrapper(const std::string& theFilePath) : myFid(-1)
    {
        myFid = MEDfileOpen(theFilePath.c_str(), MED_ACC_RDONLY);
    }

    ~MEDFileIOWrapper()
    {
        if (myFid != -1) {
            MEDfileClose(myFid);
            myFid = -1;
        }
    }

    med_idt GetFId() const { return myFid; }

private:
    med_idt myFid;
};

#endif

// ================================================================
// Function : Constructor
// ================================================================
RWMedReader::RWMedReader() : myStatus(ReaderStatus::NotDone)
{
}

// ================================================================
// Function : Destructor
// ================================================================
RWMedReader::~RWMedReader()
{
}

// =======================================================================
// Function : Perform
// =======================================================================
bool RWMedReader::Perform(const std::string& theFilePath,
                          const std::string& theMappingFilePath,
                          const std::string& theArtifactFilePath)
{
    myStatus = ReaderStatus::NotDone;
#ifdef HAVE_MEDFILE
    MEDFileIOWrapper aFIOWrapper(theFilePath);
    if (aFIOWrapper.GetFId() < 0) {
        Message::SendFail() << "Error: MED file '" << theFilePath << "' cannot be opened";
        return false;
    }

    std::map<std::string, MeshInfo> aNameToMeshData;
    if (!PerformMED(aFIOWrapper.GetFId(),
                    theMappingFilePath,
                    theArtifactFilePath,
                    myPropsTempFolder,
                    myToReadNodeVecAsScalar,
                    aNameToMeshData,
                    myStatus)) {
        Message::SendFail() << "Error: MED file was not processed correctly";
        return false;
    }

    for (std::map<std::string, MeshInfo>::const_iterator aMIt = aNameToMeshData.begin(); aMIt != aNameToMeshData.end();
         aMIt++) {
        myOutMeshes.push_back(aMIt->second.outMesh);
    }
    return true;
#else
    (void)theFilePath;
    (void)theMappingFilePath;
    (void)theArtifactFilePath;
    Message::SendFail() << "Error: RWMedReader was built with MED file import disabled";
    return false;
#endif
}

} // namespace