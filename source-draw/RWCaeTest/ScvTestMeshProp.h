#ifndef RWCAETEST_SCVTESTMESHPROP_HEADERFILE
#define RWCAETEST_SCVTESTMESHPROP_HEADERFILE

#include <RWCae/MeshAdaptor.h>

#include <DrawBase/DrawCommand.h>

namespace RWCaeTest {

//! Command for generating test CAE mesh property.
class ScvTestMeshProp : public DrawBase::DrawCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvtestmeshprop";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvtestmeshprop prsName [{-nodal|-nodalVector|-nodalFromElem|-elemental|-nodevec} propId [-time min max]]
                        [-sort] [-byLoc]
Generate test mesh property.
[-nodal]         scalar nodal property.
[-nodalVector]   vector nodal property.
[-nodalFromElem] nodal property converted from elemental property with the same id.
[-elemental]     scalar elemental property.
[-nodevec]       scalar elemental property with values in elements nodes.
[-time]          define property time range.
[-sort]          sort existing properties by time/id.
[-byLoc]         toggle filling property values by nodes/elements location or indices (default).
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvTestMeshProp();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process arguments.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    struct PropInfo {
        TCollection_AsciiString Id;
        std::vector<int>        TimeSteps;

        PropInfo() : Id("") {}
    };

    //! Create nodal scalar property.
    void AddNodalProp(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo);

    //! Create nodal vector property.
    void AddNodalVectorProp(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo);

    //! Create elemental scalar property.
    void AddElementalProp(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo);

    //! Create nodevec scalar property.
    void AddNodeVecProp(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo);

    //! Create nodal scalar property from elemental property.
    void AddNodalPropFromElem(const Handle(RWCae::MeshData)& theMesh, const PropInfo& theInfo);

    //! Parse property arguments.
    bool ParsePropInfo(int& theArgIter, PropInfo& theInfo);

    //! Return element's center.
    bool ElementCenter(const Handle(RWCae::MeshData)&         theMeshData,
                       const RWCae::MeshData::IndexedElement& theElem,
                       Graphic3d_Vec3&                          theCenter);

protected:
    //! Fill nodal property with elemental values.
    template<class Elem_T>
    Standard_EXPORT void genNodalPropT(const RWCae::MeshAdaptor<Elem_T>&      theAdaptor,
                                       const Handle(RWCae::MeshNVecProperty)& thePropIn,
                                       const Handle(RWCae::MeshNVecProperty)& thePropOut,
                                       NCollection_Array1<int>&                 theNodesNbValues) const;

private:
    enum class Type { Nodal, NodalVector, Elemental, NodeVec, NodalFromElem };

    TCollection_AsciiString m_prsName;

    std::vector<std::pair<Type, PropInfo>> m_propsToGen;

    bool m_toSort = false;
    bool m_byLoc  = false;
};

}

#endif // RWCAETEST_SCVTESTMESHPROP_HEADERFILE