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
    myReport = VtkImportReport();
    const Handle(OSD_FileSystem)& aFileSystem = OSD_FileSystem::DefaultFileSystem();
    std::shared_ptr<std::istream> aFile       = aFileSystem->OpenIStream(theFileName, std::ios::in | std::ios::binary);
    if (aFile.get() == nullptr) {
        return fail(std::string("Input file is not accessible: ") + theFileName.ToCString());
    }
    return PerformStream(*aFile, theFileName);
}

// =======================================================================
// function : PerformStream
// =======================================================================
bool VtkBaseReader::PerformStream(std::istream& theStream, const TCollection_AsciiString& theFileName)
{
    myReport = VtkImportReport();
    if (!myMappingFileName.IsEmpty()) {
        RWCae::MeshFieldMappingReader aMappingReader;
        if (!aMappingReader.Perform(myMappingFileName)) {
            return fail(std::string("Cannot read mapping file: ") + myMappingFileName.ToCString());
        }

        myMappingInfo = aMappingReader.GetFieldMappingInfo();
    }

    myIStream  = &theStream;
    myFileName = theFileName;

    return true;
}

bool VtkBaseReader::fail(const std::string& theMessage)
{
    myReport.errors.push_back(theMessage);
    Message::SendFail() << "[ERR_VTK_IMPORT] " << theMessage;
    return false;
}

void VtkBaseReader::warn(const std::string& theMessage)
{
    myReport.warnings.push_back(theMessage);
    Message::SendWarning() << "[VTK_IMPORT] " << theMessage;
}

}

