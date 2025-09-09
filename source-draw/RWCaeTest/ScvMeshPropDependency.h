
#ifndef RWCAETEST_SCVMESHPROPDEPENDENCY_HEADERFILE
#define RWCAETEST_SCVMESHPROPDEPENDENCY_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace RWCaeTest {

//! Draw command for setting property dependencies to properties.
//! Also can clear and print dependency information.
class ScvMeshPropDependency : public ViewCaeTest::MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshpropdependency";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshpropdependency prsName propId {-set propId1 [propId2 ...]}|-clear|-print}
Sets, clears or prints a property's array of dependencies.
Triggers update of prsName interactive mesh.
  -set    set the following properties as property dependencies for propId
  -clear  clears dependencies of propId
  -print  prints the contents of the propId's dependency array
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshPropDependency();

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

protected:
    //! Parse array of dependency property IDs.
    bool parseDependencyIdArgs(int& theArgIter);

private:
    std::vector<std::string> m_dependencyIds;   //!< Dependency ID array to be added to property.
    bool                     m_toPrint = false; //!< Controls whether to print target property's dependency IDs array.
};

}

#endif // RWCAETEST_SCVMESHPROPDEPENDENCY_HEADERFILE
