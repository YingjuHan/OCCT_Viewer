
#ifndef _MESHBINIOBASE_HEADERFILE
#define _MESHBINIOBASE_HEADERFILE

#include <RWCae/MeshCompressionHelper.h>
#include <RWCae/MeshData.h>
#include <RWCae/MeshImportExport.h>

#include <NCollection_Shared.hxx>

#include <limits>

namespace RWCae {

//! Base class for reader/writer for CAE mesh @link RWCae::MeshData @endlink binary storage format.
class MeshBinIOBase {
public:
    //! Used to describe a time step range (lower index + upper index; integer value).
    struct TimeRange {
        TimeRange(int theFirstTimeStep, int theSecondTimeStep)
            : firstTimeStep(theFirstTimeStep),
              secondTimeStep(theSecondTimeStep)
        {
            Standard_ASSERT_RAISE(theFirstTimeStep <= theSecondTimeStep,
                                  "TimeRange Error: First Time should be less than Last Time.");
        }
        TimeRange() : firstTimeStep(std::numeric_limits<int>::min()), secondTimeStep(std::numeric_limits<int>::max())
        {
            //
        }
        int firstTimeStep;
        int secondTimeStep;
    };

    //! Map: property Id to time step range.
    typedef NCollection_Shared<NCollection_DataMap<TCollection_AsciiString, TimeRange>> SharedMapOfAsciiString;

public:
    //! Main constructor.
    explicit MeshBinIOBase(const Handle(MeshData)& theMeshData) : myMeshData(theMeshData) {}

    //! Destructor.
    virtual ~MeshBinIOBase() {}

    //! Return mesh data.
    const Handle(MeshData)& GetMeshData() const { return myMeshData; }

    //! Set options @sa MeshDataImportExportOptions.
    void SetOptions(unsigned int theOptions) { myOptions = theOptions; }

    //! Returns true if theOption flag is set in myOptions
    bool CheckOption(const MeshDataImportExportOptions theOption) const
    {
        return (myOptions & static_cast<unsigned int>(theOption)) != 0;
    }

    //! Specify properties (by ids) to process.
    void SetPropsToProcess(const Handle(SharedMapOfAsciiString)& theProps) { myPropsToProcess = theProps; }

protected:
    //! Information about sizes of different parts and locations of corresponding fields in the file.
    struct FileOffsets {
        uint64_t totalLength   = 0;
        uint64_t totalOffset   = 0;
        uint64_t jsonLength    = 0;
        uint64_t jsonOffset    = 0;
        uint64_t binDataLength = 0;
        uint64_t binDataOffset = 0;
    };

    //! Return true if property should be processed.
    bool shouldPropBeProcessed(const Handle(MeshNVecProperty)& theProp) const
    {
        if (myPropsToProcess.IsNull())
            return true;

        const TCollection_AsciiString& anId = theProp->Id();
        const TimeRange*               aTR  = myPropsToProcess->Seek(anId);
        if (!aTR) {
            return false;
        }

        const int aTimeStep = theProp->TimeStep();
        return (aTR->firstTimeStep <= aTimeStep) && (aTimeStep <= aTR->secondTimeStep);
    }

protected:
    Handle(MeshData)               myMeshData;
    TCollection_AsciiString        myFileName;
    FileOffsets                    myFileOffsets;
    unsigned int                   myOptions = static_cast<unsigned int>(MeshDataImportExportOptions::DEFAULTS);
    Handle(SharedMapOfAsciiString) myPropsToProcess; //!< map of ids to filter out which properties to process

    MeshCompressionType      myCompressionType = MeshCompressionType::None;
    MeshDracoCompressionInfo myDracoCompressionInfo;
};

}

#endif // _MESHBINIOBASE_HEADERFILE
