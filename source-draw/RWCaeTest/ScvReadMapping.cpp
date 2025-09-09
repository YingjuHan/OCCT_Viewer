
#include <RWCaeTest/ScvReadMapping.h>

#include <RWCae/MeshFieldMapping.h>

#include <Message.hxx>

namespace RWCaeTest {

// =======================================================================
// function : ScvReadMapping
// =======================================================================
ScvReadMapping::ScvReadMapping() : DrawBase::DrawCommand()
{
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvReadMapping::ParseArgument(int& theArgIter)
{
    if (DrawCommand::ParseArgument(theArgIter)) {
        return true;
    }

    if (m_fileName.IsEmpty()) {
        m_fileName = Argument(theArgIter);
        if (!m_fileName.IsEmpty()) {
            return true;
        }
    }

    return false;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvReadMapping::Perform()
{
    RWCae::MeshFieldMappingReader aMappingReader;
    if (!aMappingReader.Perform(m_fileName)) {
        *myDI << "Error: Cannot read mapping file '" << m_fileName << "'";
        return false;
    }

    *myDI << "Mapping file summary:\n"
          << "    Analysis type               : " << aMappingReader.GetAnalysisType().c_str() << "\n"
          << "    Nb. of result types         : " << int(aMappingReader.GetResultTypes().size()) << "\n"
          << "    Nb. of field mapping entries: " << int(aMappingReader.GetFieldMappingInfo().size());

    return true;
}

}              
