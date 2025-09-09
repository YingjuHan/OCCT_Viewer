

#include <RWCaeTest/ScvReadMeshProperties.h>

#include <Viewer/InteractiveCaeMeshManager.h>

#include <ViewerTest.hxx>

namespace RWCaeTest {

// =======================================================================
// function : ScvReadMeshProperties
// =======================================================================
ScvReadMeshProperties::ScvReadMeshProperties()
{
    //
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvReadMeshProperties::ParseAllArguments()
{
    if (ViewerTest::GetAISContext().IsNull()) {
        *myDI << "Syntax error: vinit wasn't called";
        return false;
    }

    static constexpr int cmdIArgName = 1;
    static constexpr int cmdIArgFile = 2;
    static constexpr int cmdNbArgs3  = 3;
    if (myNbArgs != cmdNbArgs3) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }

    m_prsName  = Argument(cmdIArgName);
    m_fileName = Argument(cmdIArgFile);
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvReadMeshProperties::Perform()
{
    if (!Viewer::InteractiveCaeMeshManager::LoadMeshProperties(ViewerTest::GetAISContext(),
                                                                    m_prsName.ToCString(),
                                                                    m_fileName.ToCString())) {
        return false;
    }
    return true;
}

}
