
#include <RWMeshConv/MedConverter.h>

#include <RWCae/MeshData.h>

#include <Message.hxx>
#include <Message_PrinterOStream.hxx>
#include <OSD.hxx>
#include <Standard_Version.hxx>

#include <iostream>
#include <string>

//! Print about / usage.
static void PrintHelp()
{
    std::cout << "CAE mesh data converter. Usage:\n"
              << "  med2md -input meshFilePath -output mdbFilePath [-noColorLog] [-sort] [-nodeVecAsScalar]\n"
              << "  med2md -med medFilePath -mdb mdbFilePath -mapping mappingFilePath -missing missingFieldsFilePath"
              << " [-sort] [-nodeVecAsScalar]\n\n"
              << "Pass -input to rely on format autodetection or -med to handle only MED files on input.\n"
              << "  -mapping          path to mapping file;\n"
              << "  -missing          path to output file with not processed fields;\n"
              << "  -sort             sort mesh nodes/elements and corresponding fields;\n"
              << "  -nodeVecAsScalar  try to replace fields with values in elements nodes with simple scalar fields;\n"
              << "  -propsTempFolder  path to the folder where mesh properties will be temporarily stored\n"
              << "                    (for MED files only for now).\n";
}

//! Disable cout colorization.
static void DisablePrinterColors()
{
    for (const Handle(Message_Printer)& aPrinterIter : Message::DefaultMessenger()->Printers()) {
        if (Handle(Message_PrinterOStream) aPrinter = Handle(Message_PrinterOStream)::DownCast(aPrinterIter)) {
            aPrinter->SetToColorize(false);
        }
    }
}

//! Parse med argument.
static bool parseMedArg(const TCollection_AsciiString&    theArgCase,
                        int                               theArgIter,
                        int                               theNbArgs,
                        const RWMeshConv::MedConverter& theConv)
{
    return (theArgCase == "-med" || theArgCase == "--med") && theArgIter + 1 < theNbArgs && theConv.InputPath().empty();
}

//! Parse input argument.
static bool parseInputArg(const TCollection_AsciiString&    theArgCase,
                          int                               theArgIter,
                          int                               theNbArgs,
                          const RWMeshConv::MedConverter& theConv)
{
    return (theArgCase == "-input" || theArgCase == "--input" || theArgCase == "-i") && theArgIter + 1 < theNbArgs
        && theConv.InputPath().empty();
}

//! Parse mdb argument.
static bool parseMdbArg(const TCollection_AsciiString&    theArgCase,
                        int                               theArgIter,
                        int                               theNbArgs,
                        const RWMeshConv::MedConverter& theConv)
{
    return (theArgCase == "-mdb" || theArgCase == "--mdb") && theArgIter + 1 < theNbArgs
        && theConv.OutputPath().empty();
}

//! Parse output argument.
static bool parseOutputArg(const TCollection_AsciiString&    theArgCase,
                           int                               theArgIter,
                           int                               theNbArgs,
                           const RWMeshConv::MedConverter& theConv)
{
    return (theArgCase == "-output" || theArgCase == "--output" || theArgCase == "-o") && theArgIter + 1 < theNbArgs
        && theConv.OutputPath().empty();
}

//! Parse mapping argument.
static bool parseMappingArg(const TCollection_AsciiString&    theArgCase,
                            int                               theArgIter,
                            int                               theNbArgs,
                            const RWMeshConv::MedConverter& theConv)
{
    return (theArgCase == "-mapping" || theArgCase == "--mapping") && theArgIter + 1 < theNbArgs
        && theConv.MappingPath().empty();
}

//! Parse artifact argument.
static bool parseArtifactArg(const TCollection_AsciiString&    theArgCase,
                             int                               theArgIter,
                             int                               theNbArgs,
                             const RWMeshConv::MedConverter& theConv)
{
    return (theArgCase == "-missing" || theArgCase == "--missing" || theArgCase == "-artifact"
            || theArgCase == "--artifact")
        && theArgIter + 1 < theNbArgs && theConv.MissingFieldsPath().empty();
}

//! Parse properties temp folder.
static bool parsePropsTempFolderArg(const TCollection_AsciiString& theArgCase, int theArgIter, int theNbArgs)
{
    return theArgCase == "-propstempfolder" && theArgIter + 1 < theNbArgs;
}

static bool parseHelpArg(const TCollection_AsciiString& theArgCase)
{
    return theArgCase == "-help" || theArgCase == "--help" || theArgCase == "-h" || theArgCase == "/h";
}

bool parseExFunArg(TCollection_AsciiString&    anArgCase,
                   int&                        anArgIter,
                   int                         theNbArgs,
                   RWMeshConv::MedConverter& theConv,
                   const char* const*          theArgVec,
                   bool&                       theResFlag)
{
    theResFlag = true;
    if (parseMedArg(anArgCase, anArgIter, theNbArgs, theConv)) {
        theConv.SetInputFormat(RWMeshConv::FileFormat::MED);
        theConv.SetInputPath(theArgVec[++anArgIter]);
    } else if (parseInputArg(anArgCase, anArgIter, theNbArgs, theConv)) {
        theConv.SetInputPath(theArgVec[++anArgIter]);
    } else if (parseMdbArg(anArgCase, anArgIter, theNbArgs, theConv)) {
        theConv.SetOutputFormat(RWMeshConv::FileFormat::MDB);
        theConv.SetOutputPath(theArgVec[++anArgIter]);
    } else if (parseOutputArg(anArgCase, anArgIter, theNbArgs, theConv)) {
        theConv.SetOutputPath(theArgVec[++anArgIter]);
    } else if (parseMappingArg(anArgCase, anArgIter, theNbArgs, theConv)) {
        theConv.SetMappingPath(theArgVec[++anArgIter]);
    } else if (parseArtifactArg(anArgCase, anArgIter, theNbArgs, theConv)) {
        theConv.SetMissingFieldsPath(theArgVec[++anArgIter]);
    } else if (parsePropsTempFolderArg(anArgCase, anArgIter, theNbArgs)) {
        theConv.SetDumpPropsToTempFolder(theArgVec[++anArgIter]);
    } else if (parseHelpArg(anArgCase)) {
        PrintHelp();
    } else if (anArgCase == "-sort") {
        theConv.SetSortMeshData(true);
    } else if (anArgCase == "-nodevecasscalar") {
        theConv.SetReadNodeVecAsScalar(true);
    } else if (anArgCase == "-nocolorizelog" || anArgCase == "-nocolorlog") {
        DisablePrinterColors();
    } else if (theConv.InputPath().empty()) {
        theConv.SetInputPath(theArgVec[anArgIter]);
    } else if (theConv.OutputPath().empty()) {
        theConv.SetOutputPath(theArgVec[anArgIter]);
    } else {
        std::cerr << "Syntax error at '" << theArgVec[anArgIter] << "'\n";
        return false;
    }
    theResFlag = false;
    return false;
}

//! Parse input arguments.
static bool parseArguments(RWMeshConv::MedConverter& theConv, int theNbArgs, const char* const* theArgVec)
{
    TCollection_AsciiString aLicensePath = "";
    for (int anArgIter = 1; anArgIter < theNbArgs; ++anArgIter) {
        TCollection_AsciiString anArgCase(theArgVec[anArgIter]);
        anArgCase.LowerCase();

        bool aResFlag;
        bool aIsParseExFunArg = parseExFunArg(anArgCase, anArgIter, theNbArgs, theConv, theArgVec, aResFlag);
        if (aResFlag) {
            return aIsParseExFunArg;
        }
    }
    return true;
}

int main(int theNbArgs, char* theArgVec[])
{
    OSD::SetSignal(false);

    RWMeshConv::MedConverter aConv;
    if (!parseArguments(aConv, theNbArgs, theArgVec)) {
        PrintHelp();
        return 1;
    }

    if (aConv.InputPath().empty() || aConv.OutputPath().empty()) {
        std::cerr << "Syntax error: wrong number of arguments.\n\n";
        PrintHelp();
        return 1;
    }

    aConv.Perform();
    if (aConv.GetStatus() == RWMeshConv::ConversionStatus::NotDone) {
        std::cerr << "Failed to convert MED." << std::endl;
        return 1;
    }
    return 0;
}