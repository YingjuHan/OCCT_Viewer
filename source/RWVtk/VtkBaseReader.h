#ifndef _VTKBASEREADER_HEADERFILE
#define _VTKBASEREADER_HEADERFILE

#include <RWCae/MeshFieldMapping.h>

namespace RWVtk {
//! Base reader class for VTK data.
class VtkBaseReader {
public:
    //! Constructor.
    Standard_EXPORT VtkBaseReader();

    //! Destructor.
    Standard_EXPORT virtual ~VtkBaseReader();

    //! Perform reading from the file.
    Standard_EXPORT bool Perform(const TCollection_AsciiString& theFileName);

    //! Perform reading from the stream.
    Standard_EXPORT virtual bool PerformStream(std::istream& theStream, const TCollection_AsciiString& theFileName);

    //! Set path to property mapping file.
    void SetMappingFile(const TCollection_AsciiString& theFileName) { myMappingFileName = theFileName; }

    //! Set property mapping info.
    void SetMappingInfo(const std::vector<RWCae::MeshFieldMappingInfo>& theInfo) { myMappingInfo = theInfo; }

protected:
    std::istream*           myIStream = nullptr;
    TCollection_AsciiString myFileName;

    std::vector<RWCae::MeshFieldMappingInfo> myMappingInfo;
    TCollection_AsciiString                    myMappingFileName;
};

}

#endif // _VTKBASEREADER_HEADERFILE

