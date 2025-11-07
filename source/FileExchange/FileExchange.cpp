
#include <RWMeshConv/MedConverter.h>

#include <RWCae/MeshData.h>

#include <Message.hxx>
#include <Message_PrinterOStream.hxx>
#include <OSD.hxx>
#include <Standard_Version.hxx>

#include <algorithm>
#include <cctype>
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

enum class ParseStatus
{
    Success,
    HelpRequested,
    Failure
};

static std::string ToLower(std::string theValue)
{
    std::transform(theValue.begin(), theValue.end(), theValue.begin(),
        [](unsigned char theChar) { return static_cast<char>(std::tolower(theChar)); });
    return theValue;
}

//! Parse input arguments.
static ParseStatus parseArguments(RWMeshConv::MedConverter& theConv, int theNbArgs, char* const* theArgVec)
{
    auto requireValue = [theNbArgs, theArgVec](int& theIndex, const std::string& theSwitch) -> const char* {
        if (theIndex + 1 >= theNbArgs) {
            std::cerr << "Missing value for '" << theSwitch << "'.\n";
            return nullptr;
        }
        return theArgVec[++theIndex];
    };

    for (int anArgIter = 1; anArgIter < theNbArgs; ++anArgIter) {
        std::string anArgument(theArgVec[anArgIter]);
        std::string anArgLower = ToLower(anArgument);

        if (anArgLower == "-help" || anArgLower == "--help" || anArgLower == "-h" || anArgLower == "/h") {
            return ParseStatus::HelpRequested;
        } else if (anArgLower == "-med" || anArgLower == "--med") {
            const char* aValue = requireValue(anArgIter, anArgument);
            if (aValue == nullptr) {
                return ParseStatus::Failure;
            }
            if (!theConv.InputPath().empty()) {
                std::cerr << "Multiple input paths are not allowed.\n";
                return ParseStatus::Failure;
            }
            theConv.SetInputFormat(RWMeshConv::FileFormat::MED);
            theConv.SetInputPath(aValue);
        } else if (anArgLower == "-input" || anArgLower == "--input" || anArgLower == "-i") {
            const char* aValue = requireValue(anArgIter, anArgument);
            if (aValue == nullptr) {
                return ParseStatus::Failure;
            }
            if (!theConv.InputPath().empty()) {
                std::cerr << "Multiple input paths are not allowed.\n";
                return ParseStatus::Failure;
            }
            theConv.SetInputPath(aValue);
        } else if (anArgLower == "-mdb" || anArgLower == "--mdb") {
            const char* aValue = requireValue(anArgIter, anArgument);
            if (aValue == nullptr) {
                return ParseStatus::Failure;
            }
            if (!theConv.OutputPath().empty()) {
                std::cerr << "Multiple output paths are not allowed.\n";
                return ParseStatus::Failure;
            }
            theConv.SetOutputFormat(RWMeshConv::FileFormat::MDB);
            theConv.SetOutputPath(aValue);
        } else if (anArgLower == "-output" || anArgLower == "--output" || anArgLower == "-o") {
            const char* aValue = requireValue(anArgIter, anArgument);
            if (aValue == nullptr) {
                return ParseStatus::Failure;
            }
            if (!theConv.OutputPath().empty()) {
                std::cerr << "Multiple output paths are not allowed.\n";
                return ParseStatus::Failure;
            }
            theConv.SetOutputPath(aValue);
        } else if (anArgLower == "-mapping" || anArgLower == "--mapping") {
            const char* aValue = requireValue(anArgIter, anArgument);
            if (aValue == nullptr) {
                return ParseStatus::Failure;
            }
            if (!theConv.MappingPath().empty()) {
                std::cerr << "Multiple mapping files are not allowed.\n";
                return ParseStatus::Failure;
            }
            theConv.SetMappingPath(aValue);
        } else if (anArgLower == "-missing" || anArgLower == "--missing" || anArgLower == "-artifact"
                   || anArgLower == "--artifact") {
            const char* aValue = requireValue(anArgIter, anArgument);
            if (aValue == nullptr) {
                return ParseStatus::Failure;
            }
            if (!theConv.MissingFieldsPath().empty()) {
                std::cerr << "Multiple missing-field logs are not allowed.\n";
                return ParseStatus::Failure;
            }
            theConv.SetMissingFieldsPath(aValue);
        } else if (anArgLower == "-propstempfolder" || anArgLower == "--propstempfolder"
                   || anArgLower == "--props-temp-folder") {
            const char* aValue = requireValue(anArgIter, anArgument);
            if (aValue == nullptr) {
                return ParseStatus::Failure;
            }
            theConv.SetDumpPropsToTempFolder(aValue);
        } else if (anArgLower == "-sort") {
            theConv.SetSortMeshData(true);
        } else if (anArgLower == "-nodevecasscalar") {
            theConv.SetReadNodeVecAsScalar(true);
        } else if (anArgLower == "-nocolorizelog" || anArgLower == "-nocolorlog") {
            DisablePrinterColors();
        } else if (anArgument.rfind("-", 0) == 0) {
            std::cerr << "Unknown option '" << anArgument << "'.\n";
            return ParseStatus::Failure;
        } else if (theConv.InputPath().empty()) {
            theConv.SetInputPath(anArgument.c_str());
        } else if (theConv.OutputPath().empty()) {
            theConv.SetOutputPath(anArgument.c_str());
        } else {
            std::cerr << "Syntax error at '" << anArgument << "'.\n";
            return ParseStatus::Failure;
        }
    }

    return ParseStatus::Success;
}

int main(int theNbArgs, char* theArgVec[])
{
    OSD::SetSignal(false);

    RWMeshConv::MedConverter aConv;
    const ParseStatus aStatus = parseArguments(aConv, theNbArgs, theArgVec);
    if (aStatus == ParseStatus::HelpRequested) {
        PrintHelp();
        return 0;
    }
    if (aStatus == ParseStatus::Failure) {
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
