
#ifndef _MESHLOADER_HEADERFILE
#define _MESHLOADER_HEADERFILE

#include <RWCae/MeshBinIOBase.h>
#include <RWCae/MeshData.h>
#include <RWCae/MeshImportExport.h>
#include <RWVtk/VtkImportReport.h>
#include <RWVtk/VtkResourceLimits.h>

#include <NCollection_Map.hxx>
#include <OSD_FileSystem.hxx>

namespace RWMeshConv {
//! Class provides public interface for loading CAE mesh objects from file in supported file formats.
class MeshLoader {
public:
    //! Empty constructor.
    MeshLoader() {}

    //! Constructs an object with specified mesh import options.
    explicit MeshLoader(unsigned int theOptions) : myOptions(theOptions) {}

    //! Probe format from file header.
    Standard_EXPORT static std::string ProbeFileFormat(const std::string& theName,
                                                       const char*        theBuffer,
                                                       int                theDataLen,
                                                       bool               theIsExtra = false);

public:
    //! Open object from file system.
    //! @param[in] theFileSystem file system
    //! @param[in] theFilePath   file path
    //! @param[in] theMappingFilePath mapping file for a MED file format
    //! @param[in] theArtifactFilePath artifact file for a MED file format
    //! @return mesh object
    Standard_EXPORT Handle(RWCae::MeshData) OpenMesh(const Handle(OSD_FileSystem)&  theFileSystem,
                                                       const TCollection_AsciiString& theFilePath,
                                                       const TCollection_AsciiString& theMappingFilePath  = "",
                                                       const TCollection_AsciiString& theArtifactFilePath = "");

    //! Open BREP mesh file.
    Standard_EXPORT Handle(RWCae::MeshData) OpenBrepMesh(const TCollection_AsciiString& theFilePath,
                                                           bool                           theIsBinary);

    //! Open MDB mesh file.
    Standard_EXPORT Handle(RWCae::MeshData) OpenMdbMesh(const TCollection_AsciiString& theFilePath);

    //! Open GMSH mesh file.
    Standard_EXPORT Handle(RWCae::MeshData) OpenGmshMesh(const TCollection_AsciiString& theFilePath);

    //! Open MED mesh file.
    Standard_EXPORT Handle(RWCae::MeshData) OpenMedMesh(const TCollection_AsciiString& theFilePath,
                                                          const TCollection_AsciiString& theMappingFilePath,
                                                          const TCollection_AsciiString& theArtifactFilePath);

    //! Open VTU mesh file.
    Standard_EXPORT Handle(RWCae::MeshData) OpenVtuMesh(const TCollection_AsciiString& theFilePath,
                                                          const TCollection_AsciiString& theMappingFilePath);

    //! Open VTP mesh file.
    Standard_EXPORT Handle(RWCae::MeshData) OpenVtpMesh(const TCollection_AsciiString& theFilePath,
                                                          const TCollection_AsciiString& theMappingFilePath);

    //! Open VTM mesh file.
    Standard_EXPORT Handle(RWCae::MeshData) OpenVtmMesh(const TCollection_AsciiString& theFilePath,
                                                          const TCollection_AsciiString& theMappingFilePath);

    //! Open VTM series mesh file.
    Standard_EXPORT Handle(RWCae::MeshData) OpenVtmSeriesMesh(const TCollection_AsciiString& theFilePath,
                                                                const TCollection_AsciiString& theMappingFilePath);

    //! Load mesh properties from file.
    //! @param[in] theMesh         mesh object to attach loaded properties
    //! @param[in] theFileSystem   file system
    //! @param[in] theFilePath     file path
    //! @param[in] theUpdatedProps properties that were loaded or updated
    //! @return FALSE on error
    Standard_EXPORT bool LoadMeshProperties(const Handle(RWCae::MeshData)&                    theMesh,
                                            const Handle(OSD_FileSystem)&                       theFileSystem,
                                            const TCollection_AsciiString&                      theFilePath,
                                            NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theUpdatedProps);

    //! Return TRUE if missing fields status has been reported by reader.
    bool HasMissingFields() const { return myHasMissingFields; }

    //! Overall VTK import outcome and diagnostics for each attempted backend.
    const RWVtk::VtkImportReport& VtkReport() const { return myVtkReport; }

    //! Keep VTK diagnostics available to callers without printing reader attempts.
    void SetToReportMessages(bool theEnabled) { myToReportMessages = theEnabled; }
    void SetVtkResourceLimits(const RWVtk::VtkResourceLimits& limits) { myVtkLimits = limits; }

    //! Define properties that should be loaded.
    void SetPropsToProcess(const Handle(RWCae::MeshBinIOBase::SharedMapOfAsciiString)& theProps)
    {
        myPropsToProcess = theProps;
    }

    //! Define if nodevec properties should be converted to scalar if possible.
    //! Currently applied to MED reader only.
    void SetReadNodeVecAsScalar(bool theEnabled) { myToReadNodeVecAsScalar = theEnabled; }

    //! Enable possibility to dump to the temp folder and unload processed properties.
    void SetDumpPropsToTempFolder(const TCollection_AsciiString& theTempFolder) { myPropsTempFolder = theTempFolder; }

private:
    Handle(RWCae::MeshData) openVtkMesh(const TCollection_AsciiString& theFilePath,
                                      const TCollection_AsciiString& theMappingFilePath,
                                      const std::string& theFormat);

    RWVtk::VtkImportReport myVtkReport;
    RWVtk::VtkResourceLimits myVtkLimits;
    bool myToReportMessages = true;
    bool         myHasMissingFields = false;
    unsigned int myOptions          = (unsigned int)RWCae::MeshDataImportExportOptions::DEFAULTS;

    Handle(RWCae::MeshBinIOBase::SharedMapOfAsciiString) myPropsToProcess;

    TCollection_AsciiString myPropsTempFolder;

    bool myToReadNodeVecAsScalar = false;
};
}

#endif // _MESHLOADER_HEADERFILE
