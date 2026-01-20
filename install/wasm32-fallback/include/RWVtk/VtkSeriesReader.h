#ifndef _VTKSERIESREADER_HEADERFILE
#define _VTKSERIESREADER_HEADERFILE

#include <RWBase/JsonHelper.h>

#include <RWCae/MeshData.h>

#include <RWVtk/VtkBaseReader.h>
#include <RWVtk/VtkDataArray.h>
#include <RWVtk/VtkMultiBlockDataReader.h>

#include <memory>

namespace RWVtk {
//! Reader class for VTK data stored as series of timestamps.
class VtkSeriesReader : public VtkBaseReader {
public:
    typedef rapidjson::Document::ValueType JsonValue;

public:
    //! Constructor.
    Standard_EXPORT VtkSeriesReader();

    //! Destructor.
    Standard_EXPORT virtual ~VtkSeriesReader();

    //! Perform reading from the stream.
    Standard_EXPORT virtual bool PerformStream(std::istream&                  theStream,
                                               const TCollection_AsciiString& theFileName) override;

    //! Return merged mesh data.
    const Handle(RWCae::MeshData)& GetMergedMeshData()
    {
        mergeMeshData();
        return myMergedMeshData;
    }

protected:
    //! Read .vtm files.
    Standard_EXPORT bool readFiles(const JsonValue* theValue);

    //! Merge all mesh data.
    Standard_EXPORT void mergeMeshData();

protected:
    TCollection_AsciiString myPath;

    std::shared_ptr<rapidjson::Document> myReader;

    Handle(VtkMultiBlockDataReader::MeshDataTreeNode) myAccumMeshDataTreeRoot;
    Handle(RWCae::MeshData)                         myMergedMeshData;
};

}

#endif // _VTKSERIESREADER_HEADERFILE
