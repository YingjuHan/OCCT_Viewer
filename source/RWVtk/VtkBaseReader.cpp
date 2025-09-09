#include <RWVtk/VtkBaseReader.h>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_FileSystem.hxx>

namespace RWVtk {

// ================================================================
// function : VtkBaseReader
// ================================================================
VtkBaseReader::VtkBaseReader()
{
    //
}

// ================================================================
// function : ~VtkBaseReader
// ================================================================
VtkBaseReader::~VtkBaseReader()
{
    myIStream = nullptr;
}

// ================================================================
// function : Perform
// ================================================================
bool VtkBaseReader::Perform(const TCollection_AsciiString& theFileName)
{
    const Handle(OSD_FileSystem)& aFileSystem = OSD_FileSystem::DefaultFileSystem();
    std::shared_ptr<std::istream> aFile       = aFileSystem->OpenIStream(theFileName, std::ios::in | std::ios::binary);
    if (aFile.get() == nullptr) {
        Message::SendFail() << "Error: Input file '" << theFileName << "' is not accessible";
        return false;
    }
    return PerformStream(*aFile, theFileName);
}

// =======================================================================
// function : PerformStream
// =======================================================================
bool VtkBaseReader::PerformStream(std::istream& theStream, const TCollection_AsciiString& theFileName)
{
    if (!myMappingFileName.IsEmpty()) {
        RWCae::MeshFieldMappingReader aMappingReader;
        if (!aMappingReader.Perform(myMappingFileName)) {
            Message::SendFail() << "Error: Cannot read mapping file '" << myMappingFileName << "'";
            return false;
        }

        myMappingInfo = aMappingReader.GetFieldMappingInfo();
    }

    myIStream  = &theStream;
    myFileName = theFileName;

    return true;
}

}

