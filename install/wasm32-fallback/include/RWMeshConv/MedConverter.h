#ifndef _MEDCONVERTER_HEADERFILE
#define _MEDCONVERTER_HEADERFILE

#include <RWCae/MeshData.h>

#include <string>

namespace RWMeshConv {

//! Enumeration for input/output file formats
enum class FileFormat : int { UNKNOWN = -1, MDB, MED, GMSH22, BREP };

//! Enumeration for conversion status:
//! Done - conversion was done completely: all fields from mapping file have been found in MED file.
//! MissingFields - indicates two potential issues:
//!     - solver failure to write all requested MED output fields according to the command file.
//!     - inconsistent JSON mapping file might have been written by preprocessor.
//!     Missing fields is written to the JSON file by converter. This information might be helpful
//!     for remote diagnostics of the preprocessor, solver launcher and MED conversion services.
//! NotDone - conversion failed.
enum class ConversionStatus : int { Done = 0, MissingFields, NotDone = 100 };

//! Interface for conversion of CAE solver outputs to internal SimStudio lightweight CAE data format.
class MedConverter {
public:
    DEFINE_STANDARD_ALLOC

public:
    //! Returns true if a given input format is supported
    static bool IsInputFormatSupported(const FileFormat theFormat);

    //! Returns true if a given output format is supported
    static bool IsOutputFormatSupported(const FileFormat theFormat);

public:
    //! Constructs an empty Converter object
    Standard_EXPORT MedConverter();

    //! Destructor.
    Standard_EXPORT virtual ~MedConverter();

    //! Returns the input file format
    FileFormat InputFormat() const { return myInputFormat; }

    //! Sets the input file format
    void SetInputFormat(const FileFormat theFormat) { myInputFormat = theFormat; }

    //! Returns the output file format
    FileFormat OutputFormat() const { return myOutputFormat; }

    //! Sets the output file format
    void SetOutputFormat(const FileFormat theFormat) { myOutputFormat = theFormat; }

    //! Returns the input file path
    const std::string& InputPath() const { return myInputPath; }

    //! Sets the input file path
    void SetInputPath(const std::string& thePath) { myInputPath = thePath; }

    //! Returns the output file path
    const std::string& OutputPath() const { return myOutputPath; }

    //! Sets the output file path
    void SetOutputPath(const std::string& thePath) { myOutputPath = thePath; }

    //! Returns the mapping file path
    const std::string& MappingPath() const { return myMappingPath; }

    //! Sets the mapping file path
    void SetMappingPath(const std::string& thePath) { myMappingPath = thePath; }

    //! Returns the file with missing fields
    const std::string& MissingFieldsPath() const { return myMissingFieldsPath; }

    //! Sets the file with missing fields
    void SetMissingFieldsPath(const std::string& thePath) { myMissingFieldsPath = thePath; }

    //! Sort mesh data nodes, elements and properties before writing.
    void SetSortMeshData(bool theToSort) { myToSortMeshData = theToSort; }

    //! Define if nodevec properties should be converted to scalar if possible.
    void SetReadNodeVecAsScalar(bool theEnabled) { myToReadNodeVecAsScalar = theEnabled; }

    //! Enable possibility to dump to the temp folder and unload processed properties.
    void SetDumpPropsToTempFolder(const std::string& theTempFolder) { myPropsTempFolder = theTempFolder; }

public:
    //! Performs file conversion
    Standard_EXPORT bool Perform();

    //! Return extended conversion state.
    ConversionStatus GetStatus() const { return myConvStatus; }

public:
    //! Copy constructor
    MedConverter(const MedConverter&) = delete;
    //! Copy assignment
    MedConverter& operator=(const MedConverter&) = delete;

private:
    //! Read input mesh.
    Handle(RWCae::MeshData) readMesh();

    //! Writes a MeshData container to lightweight MDB format
    bool writeMesh(const Handle(RWCae::MeshData)& theMeshData);

    //! Writes a MeshData container to BREP format
    bool writeBrepMesh(const Handle(RWCae::MeshData)& theMeshData);

    //! Check folder for existing and create if needed
    bool checkAndCreateFolder(const TCollection_AsciiString& thePath);

private:
    FileFormat       myInputFormat;
    FileFormat       myOutputFormat;
    std::string      myInputPath;
    std::string      myOutputPath;
    std::string      myMappingPath;
    std::string      myMissingFieldsPath;
    std::string      myPropsTempFolder;
    ConversionStatus myConvStatus;
    bool             myToSortMeshData        = false;
    bool             myToReadNodeVecAsScalar = false;
};

} // namespace

#endif // _MEDCONVERTER_HEADERFILE

