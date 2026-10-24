#ifndef _MEDREADER_HEADERFILE
#define _MEDREADER_HEADERFILE

#include <RWCae/MeshData.h>

#include <Message_ProgressRange.hxx>
#include <Standard_Macro.hxx>

#include <string>
#include <vector>

namespace RWMed {

constexpr int MAPPING_VERSION_MAJOR_0 = 0;
constexpr int MAPPING_VERSION_MAJOR_1 = 1;
constexpr int MAPPING_VERSION_INVALID = -1;

//! MED (V.3+) file reader
//! Allows to read MED file into Mesh Data structures.
//! Performs reading of geometry (nodes & elements) and corresponding fields (NODES & ELEMENTNODES).
//! JSON Mapping file is an optional: it allows to combine certain components
//! of MED fields to user-defined fields.
//! In this case artifact file is used as output json format for storing of missing fields.
class RWMedReader {
public:
    // Standard OCCT memory allocation stuff
    DEFINE_STANDARD_ALLOC
public:
    //! Status of reader.
    enum class ReaderStatus { Done = 0, MissingFields, NotDone = 100 };

    //! Empty constructor.
    Standard_EXPORT RWMedReader();

    //! Destructor.
    Standard_EXPORT ~RWMedReader();

    //! Read MED file and process result to MeshData.
    Standard_EXPORT bool Perform(const std::string& theFilePath,
                                 const std::string& theMappingFilePath,
                                 const std::string& theArtifactFilePath);

    //! Return loaded meshes.
    const std::vector<Handle(RWCae::MeshData)>& Meshes() const { return myOutMeshes; }

    //! Get status of operation.
    ReaderStatus GetStatus() const { return myStatus; }

    //! Define if nodevec properties should be converted to scalar if possible.
    void SetReadNodeVecAsScalar(bool theEnabled) { myToReadNodeVecAsScalar = theEnabled; }

    //! Enable possibility to dump to the temp folder and unload processed properties.
    void SetDumpPropsToTempFolder(const TCollection_AsciiString& theTempFolder) { myPropsTempFolder = theTempFolder; }

private:
    class MEDFileIOWrapper; //!< Wrapper for Opening/Closing of MED

private:
    std::vector<Handle(RWCae::MeshData)> myOutMeshes; //!< stored meshes
    ReaderStatus                           myStatus;    //!< status of MED reader

    TCollection_AsciiString myPropsTempFolder;

    bool myToReadNodeVecAsScalar = false;
};

} // SCMEd

#endif // _RWMEDREADER_HEADERFILE