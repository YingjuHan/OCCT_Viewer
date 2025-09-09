#ifndef _GMSHLOADERCONTEXT_HEADERFILE
#define _GMSHLOADERCONTEXT_HEADERFILE

#include <RWCae/MeshData.h>

#include <NCollection_DataMap.hxx>
#include <RWMesh_CoordinateSystemConverter.hxx>
#include <Standard_ReadLineBuffer.hxx>
#include <TCollection_AsciiString.hxx>

#include <vector>

namespace RWMeshConv {
//! @internal Section within Gmsh file.
enum class GmshSection {
    MeshFormat,
    Nodes,
    Elements,
    NodeData,
    ElementData,
    ElementNodeData,
    InterpolationScheme,
    PhysicalNames,
    Periodic,
    UNKNOWN,  //!< section unsupported (unrecognized) by this reader
    UNDEFINED //!< no active section (empty lines between sections)
};

//! @internal Mesh element type within Gmsh file.
enum class GmshElemType {
    Elem0dPoint = 15, //!< 1-node point
    Elem1dLine  = 1,  //!< 2-node line

    // supported 2D elements (defined by nodes)
    Elem2dTriangle = 2, //!< 3-node triangle
    Elem2dQuad     = 3, //!< 4-node quadrangle

    // supported 3D elements (defined by nodes)
    Elem3dTetrahedron = 4, //!< 4-node tetrahedron
    Elem3dHexahedron  = 5, //!< 8-node hexahedron
    Elem3dPrism       = 6, //!< 6-node prism
    Elem3dPyramid     = 7, //!< 5-node pyramid

    // unsupported
    Elem1dLine_2_1 = 8, //!<  3-node second order line (2 nodes associated with the vertices and 1 with the edge)
    Elem2dTriangle_3_3 =
        9, //!<  6-node second order triangle (3 nodes associated with the vertices and 3 with the edges)
    Elem2dQuad_4_4_1 = 10, //!<  9-node second order quadrangle (4 nodes associated with the vertices,
                           //!<  4 with the edges and 1 with the face)
    Elem3dTetrahedron_4_6 =
        11, //!< 10-node second order tetrahedron (4 nodes associated with the vertices and 6 with the edges)
    Elem3dHexahedron_8_12_6_1 = 12, //!< 27-node second order hexahedron (8 nodes associated with the vertices,
                                    //!< 12 with the edges, 6 with the faces and 1 with the volume)
    Elem3dPrism_6_9_3 = 13,         //!< 18-node second order prism (6 nodes associated with the vertices,
                                    //!< 9 with the edges and 3 with the quadrangular faces)
    Elem3dPyramid_5_8_1 = 14,       //!< 14-node second order pyramid (5 nodes associated with the vertices,
                                    //!< 8 with the edges and 1 with the quadrangular face)
    Elem2dQuad_4_4 =
        16, //!<  8-node second order quadrangle (4 nodes associated with the vertices and 4 with the edges)
    Elem3dHexahedron_8_12 =
        17, //!< 20-node second order hexahedron (8 nodes associated with the vertices and 12 with the edges)
    Elem3dPrism_6_9 = 18, //!< 15-node second order prism (6 nodes associated with the vertices and 9 with the edges)
    Elem3dPyramid_5_8 =
        19, //!< 13-node second order pyramid (5 nodes associated with the vertices and 8 with the edges)
    Elem3dPyramid_3_6 =
        20, //!<  9-node third  order incomplete triangle (3 nodes associated with the vertices, 6 with the edges)
    Elem2dTriangle_3_6_1 =
        21, //!< 10-node third  order triangle (3 nodes associated with the vertices, 6 with the edges, 1 with the face)
    Elem2dTriangle_3_9 =
        22, //!< 12-node fourth order incomplete triangle (3 nodes associated with the vertices, 9 with the edges)
    Elem2dTriangle_3_9_3 =
        23, //!< 15-node fourth order triangle (3 nodes associated with the vertices, 9 with the edges, 3 with the face)
    Elem2dTriangle_3_12 =
        24, //!< 15-node fifth  order incomplete triangle (3 nodes associated with the vertices, 12 with the edges)
    Elem2dTriangle_3_12_6 = 25, //!< 21-node fifth  order complete triangle (3 nodes associated with the vertices,
                                //!< 12 with the edges, 6 with the face)
    Elem1dEdge_2_2 = 26, //!<  4-node third  order edge (2 nodes associated with the vertices, 2 internal to the edge)
    Elem1dEdge_2_3 = 27, //!<  5-node fourth order edge (2 nodes associated with the vertices, 3 internal to the edge)
    Elem1dEdge_2_4 = 28, //!<  6-node fifth  order edge (2 nodes associated with the vertices, 4 internal to the edge)
    Elem3dTetrahedron_4_12_4 = 29,    //!< 20-node third  order tetrahedron (4 nodes associated with the vertices,
                                      //!< 12 with the edges, 4 with the faces)
    Elem3dTetrahedron_4_18_12_1 = 30, //!< 35-node fourth order tetrahedron (4 nodes associated with the vertices,
                                      //!< 18 with the edges, 12 with the faces, 1 in the volume)
    Elem3dTetrahedron_4_24_24_4 = 31, //!< 56-node fifth  order tetrahedron (4 nodes associated with the vertices,
                                      //!< 24 with the edges, 24 with the faces, 4 in the volume)
};

//! @internal Types of sections to read.
enum class GmshSectionType {
    Geometry = 0x1, //!< nodes + elements
    Property = 0x2  //!< node data + element data + element node data
};

//! @internal Submesh definition.
class GmshSubMesh : public Standard_Transient {
    DEFINE_STANDARD_RTTI_INLINE(GmshSubMesh, Standard_Transient)
public:
    Handle(RWCae::MeshData) Data;

    GmshSubMesh() : Data(new RWCae::MeshData()) {}

    explicit GmshSubMesh(const Handle(RWCae::MeshData)& theData) : Data(theData) {}

public:
    //! Hash value, for Map interface.
    static int HashCode(const Graphic3d_Vec3i& theId, const int theUpper)
    {
        unsigned int aHashCode = 0;

        aHashCode = aHashCode ^ (unsigned int)::HashCode(theId.x(), theUpper);
        aHashCode = aHashCode ^ (unsigned int)::HashCode(theId.y(), theUpper);
        aHashCode = aHashCode ^ (unsigned int)::HashCode(theId.z(), theUpper);
        return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
    }

    //! Matching two instances, for Map interface.
    static bool IsEqual(const Graphic3d_Vec3i& theId1, const Graphic3d_Vec3i& theId2)
    {
        return theId1.x() == theId2.x() && theId1.y() == theId2.y() && theId1.z() == theId2.z();
    }
};

typedef NCollection_IndexedDataMap<Graphic3d_Vec3i, Handle(GmshSubMesh), GmshSubMesh> GmshSubMeshMap;

//! @internal Simple wrapper.
struct FileSentry {
    FILE*                   File;
    const char*             Line;
    int64_t                 Position;
    int64_t                 FileLen;
    TCollection_AsciiString FileName;
    Standard_ReadLineBuffer LineBuffer;

    explicit FileSentry(const TCollection_AsciiString& theFile);

    ~FileSentry();

    //! Read line, also considers multi-line syntax (when last line symbol is slash).
    bool ReadLine();

private:
    static constexpr int m_bufSize4KiB = 1024 * 4;
};

//! @internal Auxiliary internal structure for Gmsh reading.
class GmshLoaderContext {
public:
    //! Check if text starts with specified keyword.
    template<unsigned N>
    bool CheckKeyWord(const char* theText, const char (&theKey)[N]) const
    {
        return ::memcmp(theText, theKey, N - 1) == 0;
    }

public:
    //! Main constructor.
    GmshLoaderContext(FileSentry&                             theFile,
                      const RWMesh_CoordinateSystemConverter& theCSConverter,
                      bool                                    theToSplitSubMeshes,
                      unsigned int                            theSectionTypes,
                      GmshSubMeshMap&                         theMeshes);

    //! Return opened file.
    const FileSentry& File() { return myFile; }

    //! Read the next non-empty line.
    bool ReadLine()
    {
        for (;;) {
            if (!myFile.ReadLine()) {
                return false;
            }

            ++myNbLines;
            const char* aStr = myFile.Line;
            for (; *aStr != '\0'; ++aStr) {
                if (!isspace((unsigned char)*aStr)) {
                    return true;
                }
            }
        }
    }

    //! Parse current line pointing to a keyword (starting with $).
    bool ParseKeyWord(GmshSection& theSection);

    //! Return meshes.
    const NCollection_IndexedDataMap<Graphic3d_Vec3i, Handle(GmshSubMesh), GmshSubMesh>& Meshes() const
    {
        return myMeshMap;
    }

    //! Return true if reading should be aborted.
    bool ToAbort() const { return myToAbort; }

    //! Print syntax error at current line.
    void PrintError();

    //! Print messages (at the end of reading file).
    void PrintMessages();

public:
    //! Read the header of section $Nodes.
    bool ReadNodesSectionHeader();

    //! Read next node within section $Nodes.
    bool ReadNode();

public:
    //! Read the header of section $Elements.
    bool ReadElementsSectionHeader();

    //! Read next element within section $Elements.
    bool ReadElement();

public:
    //! Read the header of section $NodeData.
    bool ReadNodeDataSectionHeader()
    {
        return readNodeOrElementDataSectionHeader(RWCae::MeshPropertyBase::MappingType::Nodal);
    }

    //! Read next item within $NodeData.
    bool ReadNodeData() { return readNodeOrElementData(RWCae::MeshPropertyBase::MappingType::Nodal); }

public:
    //! Read the header of section $ElementNodeData.
    bool ReadElementNodeDataSectionHeader()
    {
        return readNodeOrElementDataSectionHeader(RWCae::MeshPropertyBase::MappingType::Elemental, true);
    }

    //! Read next item within $ElementNodeData.
    bool ReadElementNodeData()
    {
        return readNodeOrElementData(RWCae::MeshPropertyBase::MappingType::Elemental, true);
    }

public:
    //! Read the header of section $ElementData.
    bool ReadElementDataSectionHeader()
    {
        return readNodeOrElementDataSectionHeader(RWCae::MeshPropertyBase::MappingType::Elemental);
    }

    //! Read next item within $ElementData.
    bool ReadElementData() { return readNodeOrElementData(RWCae::MeshPropertyBase::MappingType::Elemental); }

private:
    //! Read section header.
    bool readSectionHeader(const GmshSection theSection);

    //! Read index and type of element.
    bool readElementInfo(int& theElemIndex, GmshElemType& theElemType, const char*& thePos);

    //! Return number of nodes in element.
    int getNbElemNodes(GmshElemType theElemType);

    //! Append element to mesh data.
    void addElement(int theElementIndex, GmshElemType theElemType);

    //! Read the header of section $NodeData, $ElementData or $ElementNodeData.
    bool readNodeOrElementDataSectionHeader(RWCae::MeshPropertyBase::MappingType theType, bool isElemNodes = false);

    //! Read string tags of data section.
    bool readDataSectionStringTags(bool theIsSectionAccepted);

    //! Read real tags of data section.
    bool readDataSectionRealTags(bool theIsSectionAccepted);

    //! Read int tags of data section.
    bool readDataSectionIntTags(bool theIsSectionAccepted);

    //! Create a property of specified type.
    Handle(RWCae::MeshNVecProperty) initMeshProperty(const Handle(GmshSubMesh)&             theSubMesh,
                                                       RWCae::MeshPropertyBase::MappingType theType,
                                                       bool                                   isElemNodes);

    //! Read next item within $NodeData, $ElementData or $ElementNodeData.
    bool readNodeOrElementData(RWCae::MeshPropertyBase::MappingType theType, bool isElemNodes = false);

    //! Read property values.
    bool readPropertyValues(const Handle(GmshSubMesh)&             theSubMesh,
                            RWCae::MeshPropertyBase::MappingType theType,
                            bool                                   isElemNodes,
                            int                                    theItemIndex,
                            const char*&                           thePos);

    //! Read scalar nodal elemental property values.
    bool readScalarNodeElementValues(const Handle(GmshSubMesh)& theSubMesh, int theItemIndex, const char*& thePos);

    //! Read scalar property values.
    bool readScalarValues(const Handle(GmshSubMesh)&             theSubMesh,
                          RWCae::MeshPropertyBase::MappingType theType,
                          int                                    theItemIndex,
                          const char*&                           thePos);

    //! Read vector property values.
    bool readVectorValues(const Handle(GmshSubMesh)&             theSubMesh,
                          RWCae::MeshPropertyBase::MappingType theType,
                          int                                    theItemIndex,
                          const char*&                           thePos);

    //! Read index and 3 float values.
    inline bool readIndexAndXYZ(const char* thePos, char*& theNext, int& theIndex, gp_XYZ& theVec)
    {
        const char* aPos = thePos;

        theIndex = (int)Strtod(aPos, &theNext);
        aPos     = theNext;
        theVec.SetX(Strtod(aPos, &theNext));
        aPos = theNext;
        theVec.SetY(Strtod(aPos, &theNext));
        aPos = theNext;
        theVec.SetZ(Strtod(aPos, &theNext));
        return aPos != theNext;
    }

private:
    struct GmshDataTags {
        static constexpr int m_tagTimeStepIndex    = 0;
        static constexpr int m_tagNbDataComponents = 1;
        static constexpr int m_tagNbValues         = 2;
        static constexpr int m_tagDataPartition    = 3;

        std::vector<TCollection_AsciiString> stringTags;
        std::vector<double>                  realTags;
        std::vector<int>                     intTags;

        TCollection_AsciiString name;

        double timestamp;
        int    timeStepIndex;
        int    nbDataComponents;
        int    dataPartition;
        int    nbValues;

        GmshDataTags() : timestamp(0.0), timeStepIndex(0), nbDataComponents(0), dataPartition(0), nbValues(0) {}

        //! Get name tag.
        TCollection_AsciiString GetName() const
        {
            return stringTags.size() >= 1 ? stringTags[0] : TCollection_AsciiString();
        }

        //! Get timestamp tag.
        double GetTimestamp() const { return realTags.size() >= 1 ? realTags[0] : 0.0; }

        //! Get timestamp index tag.
        int GetTimeStepIndex() const { return intTags.size() > m_tagTimeStepIndex ? intTags[m_tagTimeStepIndex] : 0; }

        //! Get number of components tag.
        int GetNbDataComponents() const
        {
            return intTags.size() > m_tagNbDataComponents ? intTags[m_tagNbDataComponents] : 0;
        }

        //! Get number of values tag.
        int GetNbValues() const { return intTags.size() > m_tagNbValues ? intTags[m_tagNbValues] : 0; }

        //! Get data partition tag.
        int GetDataPartition() const { return intTags.size() > m_tagDataPartition ? intTags[m_tagDataPartition] : 0; }
    };

private:
    FileSentry&                      myFile;
    RWMesh_CoordinateSystemConverter myCSConverter; //!< units converter

    int  myNbLines; //!< number of parsed lines (e.g. current line)
    bool myToAbort; //!< flag indicating abort state (e.g. syntax error)
    bool myToSplitSubMeshes;

    unsigned int mySectionTypes;

    int myNbSkippedElems;
    int myNbInvalidElems;
    int myNbDuplicatedNodes;
    int myNbDuplicatedElems;

    NCollection_Array1<RWCae::MeshEntityId>       myElemNodes;
    NCollection_IndexedDataMap<int, Graphic3d_Vec3> myNodes;
    GmshSubMeshMap&                                 myMeshMap;
    Handle(GmshSubMesh)                             myLastSubMesh;
    GmshDataTags                                    myDataTags;
};

} // namespace

#endif // _GMSHLOADERCONTEXT_HEADERFILE