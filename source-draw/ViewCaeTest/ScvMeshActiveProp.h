#ifndef _SCVMESHACTIVEPROP_HEADERFILE
#define _SCVMESHACTIVEPROP_HEADERFILE

#include <ViewCaeTest/MeshPrsCommand.h>

namespace ViewCaeTest {

//! Command for setting active CAE mesh property.
class ScvMeshActiveProp : public MeshPrsCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvmeshactiveprop";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvmeshactiveprop prsName {-prs index} [{-nodal index|-elemental index|-prop prop_id
                           [-timestep timestep_id]|-unset} [-comp index] [-normalize]]
Set active mesh property.
The property is specified via nodal/elemental property index
OR via property ID with an optional time step ID.
For a multi-component property, a component to be displayed should be specified.
Optionally, a vector property's values can be normalized to [0; 1] magnitude range.
With just a prs name, prints info about all properties available for this mesh presentation.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvMeshActiveProp();

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Find property by index.
    Handle(RWCae::MeshNVecProperty) FindPropertyByIndex(int theIndex);

    //! Print mesh properties info.
    void PrintPropsInfo(Draw_Interpretor& theDI);

    //! Print mesh property info.
    void PrintPropInfo(Draw_Interpretor& theDI, const Handle(RWCae::MeshNVecProperty)& theProp);

private:
    bool m_isElemental = true;
    int  m_index       = -2;
    int  m_comp        = -1;
    int  m_timeStep    = -1;
    bool m_toNormalize = false;
};

}

#endif // _SCVMESHACTIVEPROP_HEADERFILE
