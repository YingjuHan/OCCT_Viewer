
#include <RWCaeTest/ScvNearfieldPhaseMesh.h>

#include <ViewCae/InteractiveCaeMesh.h>
#include <RWCae/MeshTools.h>
#include <RWCaeTest/DrawableMeshData.h>

#include <array>

namespace RWCaeTest {

// =======================================================================
// function : ScvNearfieldPhaseTest
// =======================================================================
ScvNearfieldPhaseMesh::ScvNearfieldPhaseMesh()
{
    AddKey(this, int(), "-phase") = [](ScvNearfieldPhaseMesh* theCmd, int theVal) -> bool {
        theCmd->m_phases.push_back(theVal);
        return true;
    };
    AddKey(this, Graphic3d_Vec3i(), "-phaserange") = [](ScvNearfieldPhaseMesh* theCmd, Graphic3d_Vec3i theVal) -> bool {
        int aStart = theVal.x();
        int anEnd  = theVal.y();
        int aStep  = theVal.z();
        if (aStart >= anEnd || aStep <= 0)
            return false;

        for (int anIter = aStart; anIter <= anEnd; anIter += aStep) {
            theCmd->m_phases.push_back(anIter);
        }
        return true;
    };
    AddKey(this, "", "-real") = [](ScvNearfieldPhaseMesh* theCmd, Standard_CString theVal) -> bool {
        theCmd->m_realPropName = theVal;
        return true;
    };
    AddKey(this, "", "-imag") = [](ScvNearfieldPhaseMesh* theCmd, Standard_CString theVal) -> bool {
        theCmd->m_imagPropName = theVal;
        return true;
    };
    AddKey(this, "", "-out") = [](ScvNearfieldPhaseMesh* theCmd, Standard_CString theVal) -> bool {
        theCmd->m_outMeshName = theVal;
        return true;
    };
    AddKey(this, int(), "-timestep") = [](ScvNearfieldPhaseMesh* theCmd, int theVal) -> bool {
        theCmd->m_inputTimeStep = theVal;
        return true;
    };
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvNearfieldPhaseMesh::ParseArgument(int& theArgIter)
{
    if (ViewerCommand::ParseArgument(theArgIter)) {
        return true;
    }
    const char* anArg = Argument(theArgIter);
    if (m_prsName.IsEmpty() && anArg[0] != '\0') {
        m_prsName = anArg;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvNearfieldPhaseMesh::ParseAllArguments()
{
    if (!ViewBaseTest::ViewerCommand::ParseAllArguments()) {
        return false;
    }
    if (m_outMeshName.IsEmpty()) {
        Message::SendFail() << "Error: no -out argument specified.";
        return false;
    }
    if (m_phases.empty()) {
        Message::SendFail() << "Error: no target phases specified.";
        return false;
    } else {
        std::sort(m_phases.begin(), m_phases.end());
    }
    return true;
}

// =======================================================================
// function : copyMeshBlocks
// =======================================================================
void ScvNearfieldPhaseMesh::copyMeshBlocks(const Handle(RWCae::MeshDataBlock)& theSrc,
                                           Handle(RWCae::MeshDataBlock)&       theDst)
{
    std::vector<std::vector<TCollection_AsciiString>> aMeshBlockPaths;
    std::vector<TCollection_AsciiString>              aDFSPath;
    dfsMeshBlockTree(theSrc, aDFSPath, aMeshBlockPaths);
    theDst = new RWCae::MeshDataBlock("Root");
    for (const std::vector<TCollection_AsciiString>& aPath : aMeshBlockPaths) {
        Handle(RWCae::MeshDataBlock) aBlockOld = theSrc->GetChild(aPath);
        Handle(RWCae::MeshDataBlock) aBlockNew = theDst;
        for (size_t anIter = 1; anIter < aPath.size(); anIter++) {
            const TCollection_AsciiString& aPathPart = aPath[anIter];
            aBlockNew                                = aBlockNew->AppendChild(aPathPart);
        }
        aBlockNew->Range().AppendRange(aBlockOld->Range());
    }
}

// =======================================================================
// function : dfsMeshBlockTree
// =======================================================================
void ScvNearfieldPhaseMesh::dfsMeshBlockTree(const Handle(RWCae::MeshDataBlock)&              theBlock,
                                             std::vector<TCollection_AsciiString>&              thePath,
                                             std::vector<std::vector<TCollection_AsciiString>>& theOutput)
{
    thePath.push_back(theBlock->Name());
    bool isLeafBlock = true;
    for (const Handle(RWCae::MeshDataBlock)& aChildBlock : theBlock->Children()) {
        dfsMeshBlockTree(aChildBlock, thePath, theOutput);
        isLeafBlock = false;
    }
    if (isLeafBlock) {
        theOutput.push_back(thePath);
    }
    thePath.pop_back();
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvNearfieldPhaseMesh::Perform()
{
    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!ViewBase::IInteractiveObject::FindObjectById(aMesh, *m_context, m_prsName)) {
        *myDI << "Error: mesh object not found\n";
        return false;
    }

    // Retrieve properties.
    Handle(RWCae::MeshData)         aMeshData  = aMesh->GetMeshData();
    Handle(RWCae::MeshNVecProperty) aRealProp  = aMeshData->GetPropertyById(m_realPropName, m_inputTimeStep);
    Handle(RWCae::MeshNVecProperty) anImagProp = aMeshData->GetPropertyById(m_imagPropName, m_inputTimeStep);
    if (aRealProp.IsNull() || anImagProp.IsNull()) {
        Message::SendFail() << "Failed retrieving properties '" << m_realPropName << "' or '" << m_imagPropName
                            << "' with time step " << m_inputTimeStep;
        return false;
    }

    // Validate properties.
    if (aRealProp->NbComponents() == 3 && anImagProp->NbComponents() == 3) {
        //
    } else {
        Message::SendFail() << "Error: Wrong number of components in props.";
        return false;
    }
    if (aRealProp->IsElemental() || anImagProp->IsElemental()) {
        Message::SendFail() << "Error: Elemental properties are not supported.";
        return false;
    }

    const size_t aSize = aRealProp->NbValues();
    // Id of animated property
    const TCollection_AsciiString aNearFieldId = "nf_efield";

    // Create new mesh data to put the results of the computation in.
    // Copy over all MeshData elements.
    RWCae::MeshTools::MeshDataList aCopyList;
    aCopyList.push_back({aMeshData, {}});
    Handle(RWCae::MeshData) aNewMeshData = RWCae::MeshTools::MergeMeshData(aCopyList, false);
    aNewMeshData->SetId(m_outMeshName);

    // Copy block data.
    Handle(RWCae::MeshDataBlock) aBlocks;
    copyMeshBlocks(aMeshData->Blocks(), aBlocks);
    aNewMeshData->SetBlocks(aBlocks);

    // Gather all magnitude value ranges.
    Bnd_Range anAllTimeStepsRange;

    // Setup multithreading data
    const int aNbThreads = RWBase::TaskManager::Instance()->NbDefaultThreadsToLaunch();
    // Store start and end of data for each thread.
    std::vector<std::pair<int, int>> aThreadElemBounds;
    for (int anIter = 0; anIter < aNbThreads; anIter++) {
        // Thread array partitioning code.
        const auto getChunkEndBoundary = [aSize, aNbThreads](int theChunkIndex) -> int {
            if (theChunkIndex == aNbThreads - 1) {
                return static_cast<int>(aSize - 1);
            }
            return static_cast<int>((aSize / aNbThreads) * (theChunkIndex + 1) - 1);
        };
        const int aStart = (anIter == 0) ? 0 : getChunkEndBoundary(anIter - 1) + 1;
        const int anEnd  = getChunkEndBoundary(anIter);
        aThreadElemBounds.push_back({aStart, anEnd});
    }

    // Functor for multithreaded execution.
    NFPhasePropCalcFunctor aThreadContext;
    aThreadContext.ThreadElemBounds = &aThreadElemBounds;
    aThreadContext.RealProp         = aRealProp;
    aThreadContext.ImagProp         = anImagProp;

    // Create a single time step for every phase.
    for (size_t aPhaseIter = 0; aPhaseIter < m_phases.size(); aPhaseIter++) {
        // Use per-thread ranges to avoid contention.
        std::vector<std::array<Bnd_Range, 3>> aThreadPropRanges(aNbThreads);
        // Calculate phase parameters.
        const double aPhaseAngleDeg = m_phases.at(aPhaseIter);
        const double aPhaseAngleRad = aPhaseAngleDeg * M_PI / 180.0;
        aThreadContext.CosTheta     = std::cos(aPhaseAngleRad);
        aThreadContext.SinTheta     = std::sin(aPhaseAngleRad);

        // Create new property for putting the results of computation.
        Handle(RWCae::MeshNVecProperty) aProp =
            new RWCae::MeshNVecProperty("Near-field E-field",
                                          3,
                                          RWCae::MeshPropertyBase::MappingType::Nodal,
                                          RWCae::MeshNVecPropertyType::Vector);
        aProp->SetId(aNearFieldId);
        aProp->Init<RWCae::MeshScalarProperty>();
        for (int aCompIter = 0; aCompIter < aProp->NbComponents(); ++aCompIter) {
            Handle(RWCae::MeshScalarProperty) aComp = Handle(RWCae::MeshScalarProperty)::DownCast(
                aProp->GetComponent(aCompIter));
            aComp->Init(static_cast<int>(aSize));
        }
        aProp->GetComponent(0)->SetName("X");
        aProp->GetComponent(1)->SetName("Y");
        aProp->GetComponent(2)->SetName("Z");
        aProp->SetTime(aPhaseAngleDeg, static_cast<int>(aPhaseIter + 1));
        aProp->SetTimeDisplayUnits("deg");
        aProp->SetTimeStepDescription("Phase");

        aThreadContext.OutProp          = aProp;
        aThreadContext.ThreadPropRanges = &aThreadPropRanges;
        RWBase::TaskManager::Instance()->RunInParallel(0, static_cast<int>(aThreadElemBounds.size()), aThreadContext);

        // Merge multithreaded value ranges.
        for (int aComp = 0; aComp < 3; aComp++) {
            Bnd_Range aMergedRange;
            for (int anIter = 0; anIter < aNbThreads; anIter++) {
                aMergedRange.Add(aThreadPropRanges[anIter][aComp]);
            }
            aProp->GetComponent(aComp)->SetValueRange(aMergedRange);
        }

        // Add the property.
        aProp->UpdateComponents(true);
        aNewMeshData->ChangeProperties().Append(aProp);
        // Collect all value ranges of vector magnitudes from all time steps.
        anAllTimeStepsRange.Add(aProp->GetRawComponent(0)->ValueRange());
    }

    // Set the same range for all time steps
    for (size_t aPhaseIter = 0; aPhaseIter < m_phases.size(); aPhaseIter++) {
        Handle(RWCae::MeshNVecProperty) aProp = aNewMeshData->GetPropertyById(aNearFieldId,
                                                                                static_cast<int>(aPhaseIter + 1));
        aProp->ChangeComponents()[0]->SetValueRange(anAllTimeStepsRange);
    }

    // Create output mesh.
    Handle(RWCaeTest::DrawableMeshData) aDrawable = new RWCaeTest::DrawableMeshData(aNewMeshData);
    Draw::Set(m_outMeshName.ToCString(), aDrawable);
    aDrawable->Display(PrsMgr_DisplayStatus_Erased);
    *myDI << m_outMeshName;
    return true;
}

void ScvNearfieldPhaseMesh::NFPhasePropCalcFunctor::operator()(int, int theElemIndex)
{
    std::array<Bnd_Range, 3>&  aValueRanges = (*ThreadPropRanges)[theElemIndex];
    const std::pair<int, int>& aBounds      = (*ThreadElemBounds)[theElemIndex];
    const int                  aStart       = aBounds.first;
    const int                  anEnd        = aBounds.second;
    for (int anIter = aStart; anIter <= anEnd; anIter++) {
        RWCae::MeshPropertyEntityInfo anEntInfo(static_cast<int>(anIter));
        double                          aValueReal[3] = {0.0, 0.0, 0.0};
        double                          aValueImag[3] = {0.0, 0.0, 0.0};
        RealProp->GetVector(anEntInfo, aValueReal);
        ImagProp->GetVector(anEntInfo, aValueImag);

        // Perform value calculations.
        for (size_t aComp = 0; aComp < 3; aComp++) {
            const double aRes = aValueReal[aComp] * CosTheta - aValueImag[aComp] * SinTheta;
            OutProp->GetComponent(static_cast<int>(aComp))->SetValue(anEntInfo, aRes, false);
            // Add value to per-thread range.
            aValueRanges[aComp].Add(aRes);
        }
    }
}

}
