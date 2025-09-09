
#include <ViewDraw/ScvRead.h>

#include <RWMeshConv/MeshLoader.h>
#include <RWShapeConv/ShapeLoader.h>

#include <Message.hxx>

namespace ViewDraw {

// =======================================================================
// function : ScvRead
// =======================================================================
ScvRead::ScvRead()
{
    //
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvRead::ParseAllArguments()
{
    static constexpr int argNameInd2 = 2;
    static constexpr int maxNbArgs3  = 3;
    if (myNbArgs < maxNbArgs3) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }

    m_fileName = Argument(argNameInd2);
    for (int anArgIter = 1; anArgIter < myNbArgs; ++anArgIter) {
        TCollection_AsciiString anArgCase(Argument(anArgIter));
        anArgCase.LowerCase();
        if (CheckArgName(anArgCase, "-probe", "-probeformat")) {
            m_toProbe = true;
            if (anArgIter != 1) {
                m_fileName = Argument(1);
            }
            break;
        } else {
            if (*Argument(anArgIter) == '\0') {
                m_args += TCollection_AsciiString(" \"\""); // preserve empty strings
            } else {
                m_args += TCollection_AsciiString(" ") + Argument(anArgIter);
            }
        }
    }
    if (m_toProbe && myNbArgs != maxNbArgs3) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

//! Probe mesh file format.
static std::string ProbeMeshFormat(const std::string& theName, const char* theBuffer, int theDataLen)
{
    return RWMeshConv::MeshLoader::ProbeFileFormat(theName, theBuffer, theDataLen);
}

//! Probe shape file format.
static std::string ProbeShapeFormat(const std::string& theName, const char* theBuffer, int theDataLen)
{
    return RWShapeConv::ShapeLoader::ProbeFileFormat(theName, theBuffer, theDataLen);
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvRead::Perform()
{
    // parse header
    char aBytes[2048] = {};
    {
        std::shared_ptr<std::istream> aFile = OSD_FileSystem::DefaultFileSystem()->OpenIStream(m_fileName,
                                                                                               std::ios::in
                                                                                                   | std::ios::binary);
        if (aFile.get() == nullptr) {
            Message::SendFail() << "Error: the file '" << m_fileName << "' cannot be opened";
            return false;
        }
        aFile->read(aBytes, sizeof(aBytes) - 1);
    }

    const std::string aMeshForm  = ProbeMeshFormat(m_fileName.ToCString(), aBytes, (int)sizeof(aBytes));
    const std::string aShapeForm = ProbeShapeFormat(m_fileName.ToCString(), aBytes, (int)sizeof(aBytes));
    if (m_toProbe) {
        TCollection_AsciiString aList;
        if (!aMeshForm.empty()) {
            if (!aList.IsEmpty()) {
                aList += " ";
            }
            aList += aMeshForm.c_str();
        }
        if (!aShapeForm.empty()) {
            if (!aList.IsEmpty()) {
                aList += " ";
            }
            aList += aShapeForm.c_str();
        }
        *myDI << (!aList.IsEmpty() ? aList : "UNKNOWN");
        return true;
    } else if (!aMeshForm.empty()) {
        m_args = TCollection_AsciiString("scvreadmesh") + m_args;
        return myDI->Eval(m_args.ToCString()) == 0;
    } else if (!aShapeForm.empty()) {
        m_args = TCollection_AsciiString("scvreadshape") + m_args;
        return myDI->Eval(m_args.ToCString()) == 0;
    }

    Message::SendFail() << "Error: the file '" << m_fileName << "' has unsupported format";
    return false;
}

}

