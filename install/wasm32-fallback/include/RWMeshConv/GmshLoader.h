
#ifndef _GMSHLOADER_HEADERFILE
#define _GMSHLOADER_HEADERFILE

#include <RWCae/MeshTools.h>

#include <RWMeshConv/GmshLoaderContext.h>

#include <Message_ProgressRange.hxx>
#include <RWMesh_CoordinateSystemConverter.hxx>
#include <Standard_Macro.hxx>

namespace RWMeshConv {
//! This class provides parsing of files in **GMSH 2.2** ASCII file format.
//! WARNING! Current reader implementation has limited support of GMSH structures,
//! and intended for testing purposes only - could be removed in future library releases!
class GmshLoader {
public:
    //! Mesh statistics.
    struct MeshStats {
        int nbNodes     = 0;
        int nbElems     = 0;
        int nbVolElems  = 0;
        int nbSubMeshes = 0;
    };

public:
    //! Empty constructor.
    Standard_EXPORT GmshLoader();

    //! Initialize loader with existing volume mesh data.
    Standard_EXPORT explicit GmshLoader(const Handle(RWCae::MeshData)& theMeshData);

    //! Destructor.
    Standard_EXPORT ~GmshLoader();

    //! Read the data from specified file.
    bool Perform(const TCollection_AsciiString& theFile,
                 const Message_ProgressRange&   theProgress = Message_ProgressRange())
    {
        return performMesh(theFile, theProgress);
    }

    //! Return loaded meshes.
    const GmshSubMeshMap& Meshes() const { return myMeshMap; }

    //! Define types of sections to read, @sa GmshSectionType.
    void SetSectionTypesToRead(unsigned int theTypes) { mySectionTypes = theTypes; }

protected:
    //! Mesh stats.
    Standard_EXPORT MeshStats collectMeshStats();

    //! Read the mesh from specified file.
    Standard_EXPORT bool performMesh(const TCollection_AsciiString& theFile, const Message_ProgressRange& theProgress);

    //! Read version, file type, float size from header line.
    Standard_EXPORT bool parseHeader(const char* theStr,
                                     int&        theVerMaj,
                                     int&        theVerMin,
                                     int&        theFileType,
                                     int&        theFloatSize);

    //! Read header using the specified loader context.
    Standard_EXPORT bool readHeader(GmshLoaderContext& theCtx);

    //! Read sections using the specified loader context.
    Standard_EXPORT bool readSections(GmshLoaderContext&     theCtx,
                                      Message_ProgressScope& thePSEntry,
                                      int&                   theNbMiBPassed,
                                      bool&                  hasFailed);

    //! Read section using the specified loader context.
    Standard_EXPORT void readSection(GmshLoaderContext& theCtx, GmshSection theSection, bool& hasFailed);

    //! Perform post-processing actions.
    Standard_EXPORT void performPostprocessing(const GmshLoaderContext& theCtx);

    //! Replace MeshNodeVecProperty objects with simple nodal/elemental properties
    //! whenever possible
    void rebuildProperties(const GmshLoaderContext& theCtx);

private:
    bool                             myToSplitSubMeshes;
    RWMesh_CoordinateSystemConverter myCoordSysConverter; //!< coordinate system converter
    GmshSubMeshMap                   myMeshMap;           //!< loaded sub-meshes
    unsigned int                     mySectionTypes;
};

} // namespace

#endif // _GMSHLOADER_HEADERFILE