
#ifndef RWCAETEST_MESHIOCOMMAND_HEADERFILE
#define RWCAETEST_MESHIOCOMMAND_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <RWCae/MeshBinIOBase.h>

namespace RWCaeTest {

//! Command for import/export of CAE mesh files.
class MeshIOCommand : public ViewBaseTest::ViewerCommand {
public:
    //! Constructor.
    Standard_EXPORT MeshIOCommand();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

protected:
    //! Used to parse arguments of time range.
    bool ParseTimeRange(int& theArgIter, RWCae::MeshBinIOBase::TimeRange& theTR);

protected:
    TCollection_AsciiString m_fileName;
    TCollection_AsciiString m_prsName;

    Handle(RWCae::MeshBinIOBase::SharedMapOfAsciiString) m_props;

    unsigned int m_options = (unsigned int)RWCae::MeshDataImportExportOptions::DEFAULTS;
};

}

#endif // RWCAETEST_MESHIOCOMMAND_HEADERFILE
