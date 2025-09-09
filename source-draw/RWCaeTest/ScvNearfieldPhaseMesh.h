#ifndef RWCAETEST_SCVNEARFIELDPHASETEST_HEADERFILE
#define RWCAETEST_SCVNEARFIELDPHASETEST_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>
#include <RWCae/MeshDataBlock.h>
#include <RWCae/MeshNVecProperty.h>

namespace RWCaeTest {

//! Command for creating mesh data with time steps for near-field E-field phase animation.
class ScvNearfieldPhaseMesh : public ViewBaseTest::ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvnearfieldphasemesh";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvnearfieldphasemesh prsName -real realPropName -imag imagPropName
  -out outPrsName [-phase angle_deg] [-phaserange start end step]
  [-timestep]=-1
Creates a new mesh object named outPrsName containing near-field mesh
animation data in its time steps. Number of added time steps depends
on the number of phases specified in the command arguments.
  -real realPropName           name of real property in source mesh
  -imag imagPropName           name of imaginary property in source mesh
  -out outPrsName              name of output mesh
  [-phase angle_deg]           add an integer phase angle in degrees to the output
  [-phaserange start end step] add a range of phase angle steps to the output
  [-timestep]=-1               the time step to select from the source mesh
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvNearfieldPhaseMesh();

    //! Process all arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    //! Replicates source mesh block data tree structure to the destination argument.
    void copyMeshBlocks(const Handle(RWCae::MeshDataBlock)& theSrc, Handle(RWCae::MeshDataBlock)& theDst);

    //! Depth-first search for collecting all paths to leaf blocks in mesh block tree structure.
    void dfsMeshBlockTree(const Handle(RWCae::MeshDataBlock)&              theBlock,
                          std::vector<TCollection_AsciiString>&              thePath,
                          std::vector<std::vector<TCollection_AsciiString>>& theOutput);

    //! Functor for multithreaded execution.
    struct NFPhasePropCalcFunctor {
        //! Calculates near-field phase animation step depending on the Theta phase
        //! (specified by cosine and sine values for phase angle).
        void operator()(int theThreadIndex, int theElemIndex);

        std::vector<std::array<Bnd_Range, 3>>* ThreadPropRanges =
            nullptr;                                                  //!< Per-thread range of values of a property.
        std::vector<std::pair<int, int>>* ThreadElemBounds = nullptr; //!< Per-thread chunks of an array for processing.
        Handle(RWCae::MeshNVecProperty) RealProp;                   //!< Real part property.
        Handle(RWCae::MeshNVecProperty) ImagProp;                   //!< Imaginary part property.
        Handle(RWCae::MeshNVecProperty) OutProp;                    //!< Output property.
        double                            CosTheta = 0;               //!< Cosine of phase angle.
        double                            SinTheta = 0;               //!< Sine of phase angle.
    };

private:
    std::vector<double>     m_phases;             //!< The target phases for calculation.
    TCollection_AsciiString m_prsName;            //!< The source mesh presentation name.
    TCollection_AsciiString m_realPropName;       //!< The name of the source real part property.
    TCollection_AsciiString m_imagPropName;       //!< The name of the source imaginary part property.
    TCollection_AsciiString m_outMeshName;        //!< The name of output mesh.
    int                     m_inputTimeStep = -1; //!< The source timestep.
};

}

#endif // RWCAETEST_SCVNEARFIELDPHASETEST_HEADERFILE