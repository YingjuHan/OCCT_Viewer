#include <RWMeshConv/GmshLoaderContext.h>

#include <RWCae/MeshNodeVecProperty.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_OpenFile.hxx>
#include <RWMesh_CoordinateSystemConverter.hxx>

namespace {

// Number of components.
static constexpr int NBCOMPS_SCALAR1 = 1, NBCOMPS_VEC3 = 3, NBCOMPS_TENSOR9 = 9;

static constexpr int NBNODES_ELEM0DPOINT1 = 1, NBNODES_ELEM1DLINE2 = 2, NBNODES_ELEM2DTRIANGLE3 = 3,
                     NBNODES_ELEM2DQUAD4 = 4, NBNODES_ELEM3DTETRAHEDRON4 = 4, NBNODES_ELEM3DHEXAHEDRON8 = 8,
                     NBNODES_ELEM3DPRISM6 = 6, NBNODES_ELEM3DPYRAMID5 = 5;

struct GmshSectionDef {
    const char*               name;
    RWMeshConv::GmshSection id;
    int                       len;

    GmshSectionDef(const char* theName, RWMeshConv::GmshSection theId)
        : name(theName),
          id(theId),
          len(static_cast<int>(std::strlen(theName)))
    {
    }
};

static const GmshSectionDef THE_GMSH_SECTIONS[static_cast<int>(RWMeshConv::GmshSection::UNKNOWN)] = {
    GmshSectionDef("MeshFormat", RWMeshConv::GmshSection::MeshFormat),
    GmshSectionDef("Nodes", RWMeshConv::GmshSection::Nodes),
    GmshSectionDef("Elements", RWMeshConv::GmshSection::Elements),
    GmshSectionDef("NodeData", RWMeshConv::GmshSection::NodeData),
    GmshSectionDef("ElementData", RWMeshConv::GmshSection::ElementData),
    GmshSectionDef("ElementNodeData", RWMeshConv::GmshSection::ElementNodeData),
    GmshSectionDef("InterpolationScheme", RWMeshConv::GmshSection::InterpolationScheme),
    GmshSectionDef("PhysicalNames", RWMeshConv::GmshSection::PhysicalNames),
    GmshSectionDef("Periodic", RWMeshConv::GmshSection::Periodic)};

//! Return file length.
static int64_t ftell64(FILE* theFile)
{
#if defined(_WIN32)
    return _ftelli64(theFile);
#else
    return ftello(theFile);
#endif
}

//! Seek within file stream.
static int fseek64(FILE* theFile, int64_t theOffset, int theOrigin)
{
#if defined(_WIN32)
    return _fseeki64(theFile, theOffset, theOrigin);
#else
    return fseeko(theFile, theOffset, theOrigin);
#endif
}

}

namespace RWMeshConv {

using MeshNVecProperty   = RWCae::MeshNVecProperty;
using MeshScalarProperty = RWCae::MeshScalarProperty;
using MeshPropertyBase   = RWCae::MeshPropertyBase;

// ================================================================
// Function : FileSentry
// ================================================================
FileSentry::FileSentry(const TCollection_AsciiString& theFile)
    : File(OSD_OpenFile(theFile.ToCString(), "rb")),
      Line(NULL),
      Position(0),
      FileLen(0),
      FileName(theFile),
      LineBuffer(m_bufSize4KiB)
{
    if (this->File != NULL) {
        // determine length of file
        ::fseek64(this->File, 0, SEEK_END);
        FileLen = ::ftell64(this->File);
        ::fseek64(this->File, 0, SEEK_SET);
    }
}

// ================================================================
// Function : ~FileSentry
// ================================================================
FileSentry::~FileSentry()
{
    if (File != nullptr) {
        ::fclose(File);
    }
    Line = nullptr;
    File = nullptr;
}

// ================================================================
// Function : ReadLine
// ================================================================
bool FileSentry::ReadLine()
{
    size_t aLineLen = 0;
    this->Line      = LineBuffer.ReadLine(this->File, aLineLen);
    Position        = ::ftell64(this->File);
    return this->Line != NULL;
}

// ================================================================
// Function : GmshLoaderContext
// ================================================================
GmshLoaderContext::GmshLoaderContext(
    FileSentry&                                                                    theFile,
    const RWMesh_CoordinateSystemConverter&                                        theCSConverter,
    bool                                                                           theToSplitSubMeshes,
    unsigned int                                                                   theSectionTypes,
    NCollection_IndexedDataMap<Graphic3d_Vec3i, Handle(GmshSubMesh), GmshSubMesh>& theOutputMeshes)
    : myFile(theFile),
      myCSConverter(theCSConverter),
      myNbLines(0),
      myToAbort(false),
      myToSplitSubMeshes(theToSplitSubMeshes),
      mySectionTypes(theSectionTypes),
      myNbSkippedElems(0),
      myNbInvalidElems(0),
      myNbDuplicatedNodes(0),
      myNbDuplicatedElems(0),
      myElemNodes(1,
                  56), // 56-node fifth order tetrahedron is currently the biggest element defined by Gmsh specs
      myMeshMap(theOutputMeshes)
{
    //
}

// ================================================================
// Function : PrintError
// ================================================================
void GmshLoaderContext::PrintError()
{
    Message::SendFail(TCollection_AsciiString("Syntax error in Gmsh file '") + myFile.FileName + "' at line "
                      + myNbLines);
}

// ================================================================
// Function : PrintMessages
// ================================================================
void GmshLoaderContext::PrintMessages()
{
    if (myNbSkippedElems != 0) {
        Message::SendWarning(TCollection_AsciiString() + myNbSkippedElems
                             + " unknown/unsupported elements have been skipped");
    }
    if (myNbDuplicatedNodes != 0) {
        Message::SendWarning(TCollection_AsciiString() + myNbDuplicatedNodes + " duplicated nodes have been skipped");
    }
    if (myNbDuplicatedElems != 0) {
        Message::SendWarning(TCollection_AsciiString() + myNbDuplicatedElems
                             + " duplicated elements have been skipped");
    }
    if (myNbInvalidElems != 0) {
        Message::SendWarning(TCollection_AsciiString() + myNbInvalidElems + " invalid elements have been skipped");
    }
}

// ================================================================
// Function : ParseKeyWord
// ================================================================
bool GmshLoaderContext::ParseKeyWord(GmshSection& theSection)
{
    if (myFile.Line[0] != '$') {
        return false;
    }

    if (CheckKeyWord(myFile.Line + 1, "End")) {
        if (theSection == GmshSection::UNDEFINED) {
            PrintError();
            return false;
        }
        theSection = GmshSection::UNDEFINED;
        return true;
    } else if (theSection == GmshSection::UNDEFINED) {
        for (int aSecIter = 0; aSecIter < static_cast<int>(GmshSection::UNKNOWN); ++aSecIter) {
            const GmshSectionDef& aSecDef = THE_GMSH_SECTIONS[aSecIter];
            if (memcmp(myFile.Line + 1, aSecDef.name, aSecDef.len) == 0) {
                theSection = aSecDef.id;
                break;
            }
        }
        if (theSection == GmshSection::UNDEFINED) {
            theSection = GmshSection::UNKNOWN;
        }

        return readSectionHeader(theSection);
    } else if (theSection == GmshSection::UNDEFINED) {
        PrintError();
        return false;
    }
    return true;
}

// ================================================================
// Function : readSectionHeader
// ================================================================
bool GmshLoaderContext::readSectionHeader(const GmshSection theSection)
{
    switch (theSection) {
        case GmshSection::Nodes: return ReadNodesSectionHeader();
        case GmshSection::Elements: return ReadElementsSectionHeader();
        case GmshSection::NodeData: return ReadNodeDataSectionHeader();
        case GmshSection::ElementData: return ReadElementDataSectionHeader();
        case GmshSection::ElementNodeData: return ReadElementNodeDataSectionHeader();
        case GmshSection::InterpolationScheme:
        case GmshSection::PhysicalNames:
        case GmshSection::MeshFormat:
        case GmshSection::Periodic:
        case GmshSection::UNKNOWN:
        case GmshSection::UNDEFINED: {
            break;
        }
    }

    return true;
}

// ================================================================
// Function : ReadNodesSectionHeader
// ================================================================
bool GmshLoaderContext::ReadNodesSectionHeader()
{
    if (!ReadLine()) {
        PrintError();
        return false;
    }

    const bool isSectionAccepted = (mySectionTypes & (unsigned int)GmshSectionType::Geometry) != 0;
    if (!isSectionAccepted) {
        return true;
    }

    int aNbNodes = atol(myFile.Line);
    (void)aNbNodes;

    if (!myLastSubMesh.IsNull() && myLastSubMesh->Data->NbProperties(MeshPropertyBase::MappingType::Nodal) > 0) {
        PrintError();
        return false;
    }
    return true;
}

// ================================================================
// Function : ReadNode
// ================================================================
bool GmshLoaderContext::ReadNode()
{
    Standard_Character* aNext      = nullptr;
    int                 aNodeIndex = 0;
    gp_XYZ              anXYZ;
    if (!readIndexAndXYZ(myFile.Line, aNext, aNodeIndex, anXYZ)) {
        PrintError();
        return false;
    }

    const bool isSectionAccepted = (mySectionTypes & (unsigned int)GmshSectionType::Geometry) != 0;
    if (!isSectionAccepted) {
        return true;
    }

    if (RWCae::IsNaN(anXYZ.X())) {
        anXYZ.SetX(0.0);
    }
    if (RWCae::IsNaN(anXYZ.Y())) {
        anXYZ.SetY(0.0);
    }
    if (RWCae::IsNaN(anXYZ.Z())) {
        anXYZ.SetZ(0.0);
    }
    myCSConverter.TransformPosition(anXYZ);

    const int aPrevExtent = myNodes.Extent();
    if (myNodes.Add(aNodeIndex, Graphic3d_Vec3((float)anXYZ.X(), (float)anXYZ.Y(), (float)anXYZ.Z())) <= aPrevExtent) {
        ++myNbDuplicatedNodes;
    }
    return true;
}

// ================================================================
// Function : ReadElementsSectionHeader
// ================================================================
bool GmshLoaderContext::ReadElementsSectionHeader()
{
    if (!ReadLine()) {
        PrintError();
        return false;
    }

    const bool isSectionAccepted = (mySectionTypes & (unsigned int)GmshSectionType::Geometry) != 0;
    if (!isSectionAccepted) {
        return true;
    }

    int aNbElems = atol(myFile.Line);
    (void)aNbElems;

    if (!myLastSubMesh.IsNull() && myLastSubMesh->Data->NbProperties(MeshPropertyBase::MappingType::Elemental) > 0) {
        PrintError();
        return false;
    }
    return true;
}

// ================================================================
// Function : ReadElement
// ================================================================
bool GmshLoaderContext::ReadElement()
{
    const bool isSectionAccepted = (mySectionTypes & (unsigned int)GmshSectionType::Geometry) != 0;
    if (!isSectionAccepted) {
        return true;
    }

    const char*         aPos        = myFile.Line;
    Standard_Character* aNext       = nullptr;
    int                 anElemIndex = -1;
    GmshElemType        anElemType  = GmshElemType::Elem0dPoint;
    if (!readElementInfo(anElemIndex, anElemType, aPos)) {
        return false;
    }

    int aNbElemNodes = getNbElemNodes(anElemType);

    for (int anElemNodeIter = 0; anElemNodeIter < aNbElemNodes; ++anElemNodeIter) {
        const RWCae::MeshEntityId anElemNodeIndex = (int)Strtod(aPos, &aNext);

        if (aPos == aNext) {
            PrintError();
            return false;
        } else if (!myLastSubMesh->Data->IndexedNodes().Find(anElemNodeIndex,
                                                             myElemNodes.ChangeValue(anElemNodeIter + 1))) {
            const int aGlobNodeIndex = myNodes.FindIndex(anElemNodeIndex);
            if (aGlobNodeIndex == 0) {
                // just skip - do not abort progress
                ++myNbInvalidElems;
                return true;
            }

            const RWCae::MeshEntityId aNewNodeIndex = myLastSubMesh->Data->NbNodes();
            myLastSubMesh->Data->AddNodePosition(myNodes.FindFromIndex(aGlobNodeIndex));
            myLastSubMesh->Data->IndexedNodes().Bind(anElemNodeIndex, aNewNodeIndex);
            myElemNodes.ChangeValue(anElemNodeIter + 1) = aNewNodeIndex;
        }

        aPos = aNext;
    }
    if (myLastSubMesh->Data->IndexedElems().IsBound(anElemIndex)) {
        // just skip duplicates - do not abort progress
        ++myNbDuplicatedElems;
        return true;
    }

    addElement(anElemIndex, anElemType);

    return true;
}

// ================================================================
// Function : readElementInfo
// ================================================================
bool GmshLoaderContext::readElementInfo(int& theElemIndex, GmshElemType& theElemType, const char*& thePos)
{
    Standard_Character* aNext = nullptr;
    theElemIndex              = static_cast<int>(Strtod(thePos, &aNext));
    thePos                    = aNext;
    theElemType               = static_cast<GmshElemType>(static_cast<int>(Strtod(thePos, &aNext)));
    thePos                    = aNext;
    const int aNbTags         = static_cast<int>(Strtod(thePos, &aNext));
    if (thePos == aNext) {
        PrintError();
        return false;
    }
    thePos = aNext;

    Graphic3d_Vec3i aMeshId(0, 0, 0);
    for (int aTagIter = 0; aTagIter < aNbTags; ++aTagIter) {
        const int aTag = (int)Strtod(thePos, &aNext);
        if (thePos == aNext) {
            PrintError();
            return false;
        }
        thePos = aNext;

        if (aTagIter < Graphic3d_Vec3i::Length()) {
            aMeshId[aTagIter] = aTag;
        }
    }
    if (!myToSplitSubMeshes) {
        aMeshId = Graphic3d_Vec3i(0, 0, 0);
    }
    if (!myMeshMap.FindFromKey(aMeshId, myLastSubMesh)) {
        myLastSubMesh = new GmshSubMesh();
        myMeshMap.Add(aMeshId, myLastSubMesh);
    }

    return true;
}

// ================================================================
// Function : getNbElemNodes
// ================================================================
int GmshLoaderContext::getNbElemNodes(GmshElemType theElemType)
{
    switch (theElemType) {
        case GmshElemType::Elem0dPoint: return NBNODES_ELEM0DPOINT1;
        case GmshElemType::Elem1dLine: return NBNODES_ELEM1DLINE2;
        case GmshElemType::Elem2dTriangle: return NBNODES_ELEM2DTRIANGLE3;
        case GmshElemType::Elem2dQuad: return NBNODES_ELEM2DQUAD4;
        case GmshElemType::Elem3dTetrahedron: return NBNODES_ELEM3DTETRAHEDRON4;
        case GmshElemType::Elem3dHexahedron: return NBNODES_ELEM3DHEXAHEDRON8;
        case GmshElemType::Elem3dPrism: return NBNODES_ELEM3DPRISM6;
        case GmshElemType::Elem3dPyramid: return NBNODES_ELEM3DPYRAMID5;
        default: ++myNbSkippedElems; break;
    }

    return 0;
}

// ================================================================
// Function : addElement
// ================================================================
void GmshLoaderContext::addElement(int theElementIndex, GmshElemType theElemType)
{
    switch (theElemType) {
        case GmshElemType::Elem0dPoint: {
            const int a0dElemIndex = myLastSubMesh->Data->AddElement(
                RWCae::MeshElement0d(&myElemNodes.ChangeFirst()));
            myLastSubMesh->Data->IndexedElems().Bind(theElementIndex, a0dElemIndex);
            break;
        }
        case GmshElemType::Elem1dLine: ++myNbSkippedElems; break;
        case GmshElemType::Elem2dTriangle: {
            const int a2dElemIndex = myLastSubMesh->Data->AddElement(
                RWCae::MeshElementTriangle(&myElemNodes.ChangeFirst()));
            myLastSubMesh->Data->IndexedElems().Bind(theElementIndex, a2dElemIndex);
            break;
        }
        case GmshElemType::Elem2dQuad: {
            const int a2dElemIndex = myLastSubMesh->Data->AddElement(
                RWCae::MeshElementQuad(&myElemNodes.ChangeFirst()));
            myLastSubMesh->Data->IndexedElems().Bind(theElementIndex, a2dElemIndex);
            break;
        }
        case GmshElemType::Elem3dTetrahedron: {
            const int aTetIndex = myLastSubMesh->Data->AddElement(
                RWCae::MeshElementTetrahedron(&myElemNodes.ChangeFirst()));
            myLastSubMesh->Data->IndexedElems().Bind(theElementIndex, aTetIndex);
            break;
        }
        case GmshElemType::Elem3dHexahedron: {
            const int aHexIndex = myLastSubMesh->Data->AddElement(
                RWCae::MeshElementHexahedron(&myElemNodes.ChangeFirst()));
            myLastSubMesh->Data->IndexedElems().Bind(theElementIndex, aHexIndex);
            break;
        }
        case GmshElemType::Elem3dPrism: {
            const int aPrismIndex = myLastSubMesh->Data->AddElement(
                RWCae::MeshElementPrism(&myElemNodes.ChangeFirst()));
            myLastSubMesh->Data->IndexedElems().Bind(theElementIndex, aPrismIndex);
            break;
        }
        case GmshElemType::Elem3dPyramid: {
            const int aPyrIndex = myLastSubMesh->Data->AddElement(
                RWCae::MeshElementPyramid(&myElemNodes.ChangeFirst()));
            myLastSubMesh->Data->IndexedElems().Bind(theElementIndex, aPyrIndex);
            break;
        }
        default: break;
    }
}

// ================================================================
// Function : readNodeOrElementDataSectionHeader
// ================================================================
bool GmshLoaderContext::readNodeOrElementDataSectionHeader(MeshPropertyBase::MappingType theType, bool isElemNodes)
{
    const bool isSectionAccepted = (mySectionTypes & (unsigned int)GmshSectionType::Property) != 0;
    if (!readDataSectionStringTags(isSectionAccepted)) {
        return false;
    }
    if (!readDataSectionRealTags(isSectionAccepted)) {
        return false;
    }
    if (!readDataSectionIntTags(isSectionAccepted)) {
        return false;
    }

    if (!isSectionAccepted) {
        return true;
    }

    myDataTags.name             = myDataTags.GetName();
    myDataTags.timestamp        = myDataTags.GetTimestamp();
    myDataTags.timeStepIndex    = myDataTags.GetTimeStepIndex();
    myDataTags.nbDataComponents = myDataTags.GetNbDataComponents();
    myDataTags.nbValues         = myDataTags.GetNbValues();
    myDataTags.dataPartition    = myDataTags.GetDataPartition();
    if (myDataTags.nbDataComponents != NBCOMPS_SCALAR1 && myDataTags.nbDataComponents != NBCOMPS_VEC3
        && myDataTags.nbDataComponents != NBCOMPS_TENSOR9) {
        PrintError();
        return false;
    }

    if (myDataTags.name.Length() > (1 + 1) && myDataTags.name.Value(1) == '\"'
        && myDataTags.name.Value(myDataTags.name.Length()) == '\"') {
        myDataTags.name = myDataTags.name.SubString(1 + 1, myDataTags.name.Length() - 1);
        myDataTags.name.LeftAdjust();
        myDataTags.name.RightAdjust();
    }

    for (NCollection_IndexedDataMap<Graphic3d_Vec3i, Handle(GmshSubMesh), GmshSubMesh>::Iterator aMeshIter(myMeshMap);
         aMeshIter.More();
         aMeshIter.Next()) {
        const Handle(GmshSubMesh)& aSubmesh = aMeshIter.Value();

        Handle(MeshNVecProperty) aProp = initMeshProperty(aSubmesh, theType, isElemNodes);
        if (aProp.IsNull())
            continue;

        aProp->SetTime(myDataTags.timestamp, myDataTags.timeStepIndex);
        for (const Handle(MeshNVecProperty)& anOldPropIter : aSubmesh->Data->Properties()) {
            if (anOldPropIter->Name() == aProp->Name() && anOldPropIter->TimeStep() != aProp->TimeStep()) {
                aProp->SetId(anOldPropIter->Id());
                break;
            }
        }
        aSubmesh->Data->ChangeProperties().Append(aProp);
    }
    return true;
}

// ================================================================
// Function : readDataSectionStringTags
// ================================================================
bool GmshLoaderContext::readDataSectionStringTags(bool theIsSectionAccepted)
{
    if (!ReadLine()) {
        PrintError();
        return false;
    }

    const int aNbStringTags = atol(myFile.Line);
    if (theIsSectionAccepted) {
        myDataTags.stringTags.resize(aNbStringTags > 0 ? aNbStringTags : 0);
    }
    for (int aTagIter = 0; aTagIter < aNbStringTags; ++aTagIter) {
        if (!ReadLine()) {
            PrintError();
            return false;
        }

        if (!theIsSectionAccepted) {
            continue;
        }

        myDataTags.stringTags[aTagIter] = myFile.Line;
        myDataTags.stringTags[aTagIter].LeftAdjust();
        myDataTags.stringTags[aTagIter].RightAdjust();
    }

    return true;
}

// ================================================================
// Function : readDataSectionRealTags
// ================================================================
bool GmshLoaderContext::readDataSectionRealTags(bool theIsSectionAccepted)
{
    if (!ReadLine()) {
        PrintError();
        return false;
    }

    const int aNbRealTags = atol(myFile.Line);
    if (theIsSectionAccepted) {
        myDataTags.realTags.resize(aNbRealTags > 0 ? aNbRealTags : 0);
    }
    for (int aTagIter = 0; aTagIter < aNbRealTags; ++aTagIter) {
        if (!ReadLine()) {
            PrintError();
            return false;
        }

        if (!theIsSectionAccepted) {
            continue;
        }

        Standard_Character* aNextPtr  = nullptr;
        myDataTags.realTags[aTagIter] = Strtod(myFile.Line, &aNextPtr);
        if (myFile.Line == aNextPtr) {
            PrintError();
            return false;
        }
    }

    return true;
}

// ================================================================
// Function : readDataSectionIntTags
// ================================================================
bool GmshLoaderContext::readDataSectionIntTags(bool theIsSectionAccepted)
{
    if (!ReadLine()) {
        PrintError();
        return false;
    }

    const int aNbIntTags = atol(myFile.Line);
    if (theIsSectionAccepted) {
        myDataTags.intTags.resize(aNbIntTags > 0 ? aNbIntTags : 0);
    }
    for (int aTagIter = 0; aTagIter < aNbIntTags; ++aTagIter) {
        if (!ReadLine()) {
            PrintError();
            return false;
        }

        if (!theIsSectionAccepted) {
            continue;
        }

        myDataTags.intTags[aTagIter] = atol(myFile.Line);
    }

    return true;
}

// ================================================================
// Function : initMeshProperty
// ================================================================
Handle(MeshNVecProperty) GmshLoaderContext::initMeshProperty(const Handle(GmshSubMesh)&    theSubMesh,
                                                             MeshPropertyBase::MappingType theType,
                                                             bool                          isElemNodes)
{
    switch (myDataTags.nbDataComponents) {
        case NBCOMPS_SCALAR1: {
            if (theType == MeshPropertyBase::MappingType::Elemental && isElemNodes) {
                // Prepare property container for $ElementNodeData section.
                // So far we have got samples only with scalar values in this section,
                // thus support for vector values per element's node is not needed
                // currently.
                Handle(MeshNVecProperty) aProp = new MeshNVecProperty(myDataTags.name, 1, theType);
                aProp->Init<RWCae::MeshNodeVecProperty>();
                Handle(RWCae::MeshNodeVecProperty)::DownCast(aProp->GetComponent(0))
                    ->Init(theSubMesh->Data->IndexedElements().Size(), theSubMesh->Data->IndexedNodes().Size());
                return aProp;
            } else {
                Handle(MeshNVecProperty) aProp = new MeshNVecProperty(myDataTags.name, 1, theType);
                aProp->Init<MeshScalarProperty>();
                Handle(MeshScalarProperty)::DownCast(aProp->GetComponent(0))
                    ->Init(theType == MeshPropertyBase::MappingType::Nodal
                               ? theSubMesh->Data->Nodes().Size()
                               : theSubMesh->Data->IndexedElements().Size());
                return aProp;
            }
        }
        case NBCOMPS_VEC3: {
            Handle(MeshNVecProperty) aProp =
                new MeshNVecProperty(myDataTags.name, 3, theType, RWCae::MeshNVecPropertyType::Vector);
            aProp->Init<MeshScalarProperty>();
            int aNbValues = theType == MeshPropertyBase::MappingType::Nodal
                              ? theSubMesh->Data->Nodes().Size()
                              : theSubMesh->Data->IndexedElements().Size();
            Handle(MeshScalarProperty)::DownCast(aProp->GetComponent(0))->Init(aNbValues);
            aProp->GetComponent(0)->SetName("dX");
            Handle(MeshScalarProperty)::DownCast(aProp->GetComponent(1))->Init(aNbValues);
            aProp->GetComponent(1)->SetName("dY");
            Handle(MeshScalarProperty)::DownCast(aProp->GetComponent(2))->Init(aNbValues);
            aProp->GetComponent(2)->SetName("dZ");
            return aProp;
        }
        case NBCOMPS_TENSOR9:
        default: {
            break;
        }
    }

    return nullptr;
}

// ================================================================
// Function : readNodeOrElementData
// ================================================================
bool GmshLoaderContext::readNodeOrElementData(MeshPropertyBase::MappingType theType, bool isElemNodes)
{
    const bool isSectionAccepted = (mySectionTypes & (unsigned int)GmshSectionType::Property) != 0;
    if (!isSectionAccepted) {
        return true;
    }

    const char*         aPos        = myFile.Line;
    Standard_Character* aNext       = nullptr;
    const int           anItemIndex = (int)Strtod(aPos, &aNext);
    if (aPos == aNext) {
        PrintError();
        return false;
    }
    aPos = aNext;

    for (int aMeshIter = 1; aMeshIter <= myMeshMap.Extent(); ++aMeshIter) {
        const Handle(GmshSubMesh)& aSubmesh = myMeshMap.FindFromIndex(aMeshIter);
        if (!readPropertyValues(aSubmesh, theType, isElemNodes, anItemIndex, aPos)) {
            return false;
        }
    }
    return true;
}

// ================================================================
// Function : readPropertyValues
// ================================================================
bool GmshLoaderContext::readPropertyValues(const Handle(GmshSubMesh)&    theSubMesh,
                                           MeshPropertyBase::MappingType theType,
                                           bool                          isElemNodes,
                                           int                           theItemIndex,
                                           const char*&                  thePos)
{
    switch (myDataTags.nbDataComponents) {
        case NBCOMPS_SCALAR1:
            // Read a data line from $ElementNodeData section - number of values
            // followed by the values themselves
            if (theType == MeshPropertyBase::MappingType::Elemental && isElemNodes) {
                if (!readScalarNodeElementValues(theSubMesh, theItemIndex, thePos)) {
                    return false;
                }
            } else { // Read a single value per node/element
                if (!readScalarValues(theSubMesh, theType, theItemIndex, thePos)) {
                    return false;
                }
            }
            break;
        case NBCOMPS_VEC3:
            if (!readVectorValues(theSubMesh, theType, theItemIndex, thePos)) {
                return false;
            }
            break;
        default: break;
    }

    return true;
}

// ================================================================
// Function : readScalarNodeElementValues
// ================================================================
bool GmshLoaderContext::readScalarNodeElementValues(const Handle(GmshSubMesh)& theSubMesh,
                                                    int                        theItemIndex,
                                                    const char*&               thePos)
{
    Standard_Character* aNext = nullptr;
    // Read the number of values in the element's nodes
    const int aNbValues = (int)Strtod(thePos, &aNext);
    if (thePos == aNext) {
        PrintError();
        return false;
    }
    thePos = aNext;

    // Get the element's node indices
    const Handle(MeshNVecProperty)&      aProp        = theSubMesh->Data->Properties().Last();
    Handle(RWCae::MeshNodeVecProperty) aNodeVecProp = Handle(RWCae::MeshNodeVecProperty)::DownCast(
        aProp->GetComponent(0));

    const RWCae::MeshData::EntityMap& anIndexMap = theSubMesh->Data->IndexedElems();
    std::vector<uint32_t>               anElemNodes;

    RWCae::MeshEntityId anIndexInSubMesh = 0;
    bool                  indexFound       = false;
    if (anIndexMap.Find(theItemIndex, anIndexInSubMesh)) {
        theSubMesh->Data->ElementNodes(anIndexInSubMesh, anElemNodes);
        indexFound = true;
    }

    // Read the values and map to node indices
    for (int aValueIter = 0; aValueIter < aNbValues; aValueIter++) {
        const Standard_Real aNodalValue = Strtod(thePos, &aNext);
        if (thePos == aNext) {
            PrintError();
            return false;
        }
        thePos = aNext;

        if (indexFound) {
            aNodeVecProp->SetValue(RWCae::MeshPropertyEntityInfo(anIndexInSubMesh, anElemNodes[aValueIter]),
                                   aNodalValue);
        } else {
            aNodeVecProp->SetExternalValue(aNodalValue);
        }
    }

    return true;
}

// ================================================================
// Function : readScalarValues
// ================================================================
bool GmshLoaderContext::readScalarValues(const Handle(GmshSubMesh)&    theSubMesh,
                                         MeshPropertyBase::MappingType theType,
                                         int                           theItemIndex,
                                         const char*&                  thePos)
{
    Standard_Character* aNext  = nullptr;
    const double        aValue = Strtod(thePos, &aNext);
    if (thePos == aNext) {
        PrintError();
        return false;
    }
    thePos = aNext;

    const Handle(MeshNVecProperty)&                aProp       = theSubMesh->Data->Properties().Last();
    const Handle(RWCae::MeshScalarPropertyBase)& aScalarProp = aProp->GetComponent(0);

    const RWCae::MeshData::EntityMap& anIndexMap = theType == MeshPropertyBase::MappingType::Nodal
                                                       ? theSubMesh->Data->IndexedNodes()
                                                       : theSubMesh->Data->IndexedElems();

    RWCae::MeshEntityId anIndexInSubMesh = 0;
    if (anIndexMap.Find(theItemIndex, anIndexInSubMesh)) {
        aScalarProp->SetValue(RWCae::MeshPropertyEntityInfo(anIndexInSubMesh), aValue);
    } else {
        aScalarProp->SetExternalValue(aValue);
    }

    return true;
}

// ================================================================
// Function : readVectorValues
// ================================================================
bool GmshLoaderContext::readVectorValues(const Handle(GmshSubMesh)&    theSubMesh,
                                         MeshPropertyBase::MappingType theType,
                                         int                           theItemIndex,
                                         const char*&                  thePos)
{
    gp_XYZ              anXYZ;
    Standard_Character* aNext = nullptr;
    for (int aCoordIter = 1; aCoordIter <= NBCOMPS_VEC3; ++aCoordIter) {
        anXYZ.SetCoord(aCoordIter, Strtod(thePos, &aNext));
        if (thePos == aNext) {
            PrintError();
            return false;
        }
        thePos = aNext;
    }
    myCSConverter.TransformPosition(anXYZ);

    const Handle(MeshNVecProperty)& aProp = theSubMesh->Data->Properties().Last();

    const RWCae::MeshData::EntityMap& anIndexMap = theType == MeshPropertyBase::MappingType::Nodal
                                                       ? theSubMesh->Data->IndexedNodes()
                                                       : theSubMesh->Data->IndexedElems();

    RWCae::MeshEntityId anIndexInSubMesh = 0;
    if (anIndexMap.Find(theItemIndex, anIndexInSubMesh)) {
        aProp->GetComponent(0)->SetValue(RWCae::MeshPropertyEntityInfo(anIndexInSubMesh), anXYZ.X());
        aProp->GetComponent(1)->SetValue(RWCae::MeshPropertyEntityInfo(anIndexInSubMesh), anXYZ.Y());
        aProp->GetComponent(2)->SetValue(RWCae::MeshPropertyEntityInfo(anIndexInSubMesh), anXYZ.Z());
    } else {
        aProp->GetComponent(0)->SetExternalValue(anXYZ.X());
        aProp->GetComponent(1)->SetExternalValue(anXYZ.Y());
        aProp->GetComponent(2)->SetExternalValue(anXYZ.Z());
    }

    return true;
}

} // namespace

