
#include <RWCaeTest/ScvMeshPropDependency.h>

namespace RWCaeTest {

// =======================================================================
// function : ScvMeshPropDependency
// =======================================================================
ScvMeshPropDependency::ScvMeshPropDependency()
{
    AddVoidKey(this, "-set") = [](ScvMeshPropDependency* theCmd, int& theArgIter) -> bool {
        return theCmd->parseDependencyIdArgs(theArgIter);
    };
    AddVoidKey(this, "-clear") = [](ScvMeshPropDependency* theCmd, int&) -> bool {
        theCmd->m_dependencyIds.clear();
        return true;
    };
    AddVoidKey(this, "-print") = [](ScvMeshPropDependency* theCmd, int&) -> bool {
        theCmd->m_toPrint = true;
        return true;
    };
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvMeshPropDependency::ParseArgument(int& theArgIter)
{
    if (ViewCaeTest::MeshPrsCommand::ParseArgument(theArgIter)) {
        return true;
    }
    const char* anArg = Argument(theArgIter);
    if (anArg[0] == '\0') {
        return false;
    }
    if (m_propId.IsEmpty()) {
        m_propId = anArg;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : parseDependencyIdArgs
// =======================================================================
bool ScvMeshPropDependency::parseDependencyIdArgs(int& theArgIter)
{
    for (int anIter = theArgIter + 1; anIter < NbArguments(); anIter++) {
        TCollection_AsciiString anArgDepId(Argument(anIter));
        TCollection_AsciiString anArgCase(anArgDepId);
        anArgCase.LowerCase();
        if (myKeyMap.IsBound(anArgCase))
            break;

        m_dependencyIds.push_back(anArgDepId.ToCString());
    }

    if (m_dependencyIds.empty())
        return false;

    theArgIter += static_cast<int>(m_dependencyIds.size());
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvMeshPropDependency::Perform()
{
    const Handle(RWCae::MeshData)&         aMeshData = m_mesh->GetMeshData();
    const Handle(RWCae::MeshNVecProperty)& aProp     = aMeshData->GetPropertyById(m_propId);
    if (aProp.IsNull()) {
        Message::SendFail() << "Error: Could not find relevant property \"" << m_propId << "\".";
        return false;
    }

    if (m_toPrint) {
        const RWCae::MeshNVecProperties& aDependencyProps = aProp->DependencyProps();
        const int                          aNbDeps          = aDependencyProps.Length();
        for (int anIter = 0; anIter < aNbDeps; anIter++) {
            const Handle(RWCae::MeshNVecProperty)& aDepProp = aDependencyProps[anIter];
            *myDI << aDepProp->Id();
            if (anIter != aNbDeps - 1) {
                *myDI << " ";
            }
        }
        *myDI << "\n";
        return true;
    }

    // Check that all specified dependencies exist in MeshData.
    RWCae::MeshNVecProperties aDependencies;
    for (const std::string& aDepId : m_dependencyIds) {
        const Handle(RWCae::MeshNVecProperty)& aFoundProp = aMeshData->GetPropertyById(aDepId.c_str());
        if (aFoundProp.IsNull()) {
            Message::SendFail() << "Error: Specified dependency property doesn't exist.";
            return false;
        }
        aDependencies.Append(aFoundProp);
    }

    aProp->ChangeDependencyProps() = aDependencies;
    m_mesh->OnPropertyDependencyUpdate();

    UpdateMesh();

    return true;
}

}
