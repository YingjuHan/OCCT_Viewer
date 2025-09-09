#include <ViewCae/CaeMeshAnimation.h>

#include <ViewCae/InteractiveCaeMesh.h>

#include <RWCae/MeshTools.h>

namespace ViewCae {

// =======================================================================
// function : CaeMeshAnimation
// =======================================================================
CaeMeshAnimation::CaeMeshAnimation(InteractiveCaeMesh* theMeshIO)
    : AIS_Animation("CaeMeshAnimation"),
      myMeshIO(theMeshIO)
{
    constexpr double aDefaultDuration = 5.0;

    SetOwnDuration(aDefaultDuration);
}

// =======================================================================
// function : IsValid
// =======================================================================
bool CaeMeshAnimation::IsValid() const
{
    const std::vector<std::pair<int, double>>& aTimestamps = myMeshIO->MeshOwner()->AvailableTimeSteps();
    bool                                       hasNaNTime  = false;
    for (const std::pair<int, double>& aTimestamp : aTimestamps) {
        hasNaNTime = hasNaNTime || RWCae::IsNaN(aTimestamp.second);
    }
    return !hasNaNTime;
}

// =======================================================================
// function : StartTimer
// =======================================================================
void CaeMeshAnimation::StartTimer(const Standard_Real    theStartPts,
                                  const Standard_Real    thePlaySpeed,
                                  const Standard_Boolean theToUpdate,
                                  const Standard_Boolean theToStopTimer)
{
    if (!IsValid()) {
        return;
    }

    constexpr int aMinNbTimeSteps = 2;

    const std::vector<std::pair<int, double>>& aTimestamps = myMeshIO->MeshOwner()->AvailableTimeSteps();

    myTimestamps.clear();
    double aPrevTimestampValue = 0.0;
    for (const std::pair<int, double>& aTimestamp : aTimestamps) {
        double anOffset = Abs(aTimestamp.second - aPrevTimestampValue)
                        + (!myTimestamps.empty() ? myTimestamps.back().second : 0.0);
        myTimestamps.push_back(std::make_pair(aTimestamp.first, anOffset));
        aPrevTimestampValue = aTimestamp.second;
    }

    if (myTimestamps.size() < aMinNbTimeSteps) {
        return;
    }

    int    anActiveTimeStep  = myMeshIO->ActiveTimeStep();
    int    aTimeStepIndex    = 0;
    double anActiveTimestamp = myTimestamps.front().second;
    for (int i = 0; i < (int)myTimestamps.size(); i++) {
        const std::pair<int, double>& aTime = myTimestamps[i];
        if (anActiveTimeStep == aTime.first) {
            anActiveTimestamp = aTime.second;
            aTimeStepIndex    = i;
            break;
        }
    }

    double aStartTime = 0.0;
    if (theStartPts >= 0.0) {
        if (!myIsUniformSteps) {
            double aTimestampRange = timestampsRange();
            aStartTime = (anActiveTimestamp - myTimestamps.front().second) / aTimestampRange * myOwnDuration;
        } else {
            aStartTime = (double)aTimeStepIndex / myTimestamps.size() * myOwnDuration;
        }
    }

    myMeshIO->MeshOwner()->EnableBoxSelectionMode(true);
    myMeshIO->SetCacheTimeSteps(true, false);

    myIsRunning = true;
    myMeshIO->OnAnimationToggled(myIsRunning);

    AIS_Animation::StartTimer(aStartTime, thePlaySpeed, theToUpdate, theToStopTimer);
}

// =======================================================================
// function : Stop
// =======================================================================
void CaeMeshAnimation::Stop()
{
    if (!myIsUpdating || !myIsLoopPlayback) {
        if (!myMeshIO->MeshOwner()->ToUseGenGeom()) {
            int aCurTimeStep = myMeshIO->ActiveTimeStep();

            const Handle(RWCae::MeshData)& aMeshData = myMeshIO->GetMeshData();

            NCollection_Map<TCollection_AsciiString> aProps;
            myMeshIO->UsedProps(aCurTimeStep, aProps, aProps);
            for (NCollection_Map<TCollection_AsciiString>::Iterator anIter(aProps); anIter.More(); anIter.Next()) {
                const Handle(RWCae::MeshNVecProperty)& aProp = aMeshData->GetPropertyById(anIter.Key(), aCurTimeStep);
                RWCae::MeshTools::LoadMeshProperty(aMeshData, aProp);
            }
        } else {
            for (const Handle(MeshPresentationBase)& aPrs : myMeshIO->Presentations()) {
                Handle(MeshPresentation)::DownCast(aPrs)->SetToUpdate(true, true);
            }
            myMeshIO->SetDeferredUpdate();
        }

        myMeshIO->MeshOwner()->EnableBoxSelectionMode(false);
        myMeshIO->SetCacheTimeSteps(false, true);

        myIsRunning = false;
        myMeshIO->OnAnimationToggled(myIsRunning);
    }
    AIS_Animation::Stop();
}

// =======================================================================
// function : Continue
// =======================================================================
void CaeMeshAnimation::Continue()
{
    if (myState == AnimationState_Paused) {
        myState = AnimationState_Started;
    }
}

// =======================================================================
// function : UpdateTimer
// =======================================================================
Standard_Real CaeMeshAnimation::UpdateTimer()
{
    if (myWaitForComputing) {
        if (myMeshIO->HasNotLoadedGeom()) {
            myState = AnimationState_Paused;
            return myTimer->ElapsedTime();
        }
        myWaitForComputing = false;
        myTimer->Start();
    }

    myIsUpdating = true;
    double aRes  = AIS_Animation::UpdateTimer();
    myIsUpdating = false;
    return aRes;
}

// =======================================================================
// function : SetDuration
// =======================================================================
void CaeMeshAnimation::SetDuration(double theDur)
{
    if (IsStopped()) {
        SetOwnDuration(theDur);
        return;
    }

    myTimer->Pause();
    double aNormTime = myTimer->ElapsedTime() / myOwnDuration;
    myOwnDuration    = theDur;
    myTimer->Seek(myOwnDuration * aNormTime);
    myTimer->Start();
}

// =======================================================================
// function : update
// =======================================================================
void CaeMeshAnimation::update(const AIS_AnimationProgress& theProgress)
{
    int anActiveTimeStep = myTimestamps.front().first;

    if (!myIsUniformSteps) {
        double aTimestampRange = timestampsRange();
        double aCurTimestamp   = theProgress.LocalNormalized * aTimestampRange + myTimestamps.front().second;

        for (const std::pair<int, double>& aTime : myTimestamps) {
            if (aCurTimestamp < aTime.second - RealEpsilon()) {
                break;
            }
            anActiveTimeStep = aTime.first;
        }
    } else {
        int aCurTimeStepIndex = Min(int(theProgress.LocalNormalized * myTimestamps.size()),
                                    int(myTimestamps.size() - 1));
        anActiveTimeStep      = myTimestamps[aCurTimeStepIndex].first;
    }

    TCollection_AsciiString aNotLoadedProps = "";

    bool wasTimeStepChanged = false;

    if (anActiveTimeStep != myMeshIO->ActiveTimeStep()) {
        aNotLoadedProps = getNotLoadedProps(anActiveTimeStep);

        myTimer->Pause();

        if (aNotLoadedProps.IsEmpty()) {
            myWaitForComputing = true;
            myMeshIO->SetActiveTimeStep(anActiveTimeStep);
            wasTimeStepChanged = true;
        } else {
            if (myMeshIO->OnPropertiesRequestedCallback() != nullptr) {
                myMeshIO->OnPropertiesRequestedCallback()(anActiveTimeStep, aNotLoadedProps);
            }
            myState          = AnimationState_Paused;
            anActiveTimeStep = myMeshIO->ActiveTimeStep();
        }
    }

    if (myState == AnimationState_Stopped && myIsLoopPlayback) {
        myTimer->Seek(0.0);
        myTimer->Start();
        myState = AnimationState_Started;
    }

    if (myOnAnimationUpdate != nullptr) {
        myOnAnimationUpdate(anActiveTimeStep, wasTimeStepChanged, !IsStopped());
    }
}

// =======================================================================
// function : getNotLoadedProps
// =======================================================================
TCollection_AsciiString CaeMeshAnimation::getNotLoadedProps(int theTimeStep) const
{
    TCollection_AsciiString aRes = "";
    if (myMeshIO->MeshOwner()->ToUseGenGeom()) {
        return aRes;
    }

    NCollection_Map<TCollection_AsciiString> aComputedProps;
    NCollection_Map<TCollection_AsciiString> aNotComputedProps;
    myMeshIO->UsedProps(theTimeStep, aComputedProps, aNotComputedProps);

    for (NCollection_Map<TCollection_AsciiString>::Iterator anIter(aNotComputedProps); anIter.More(); anIter.Next()) {
        const Handle(RWCae::MeshNVecProperty)& aProp = myMeshIO->GetMeshData()->GetPropertyById(anIter.Key(),
                                                                                                  theTimeStep);
        if (!aProp.IsNull() && (aProp->IsLoaded() || aProp->CanBeLoaded())) {
            continue;
        }

        if (!aRes.IsEmpty()) {
            aRes += "\n";
        }
        aRes += anIter.Key();
    }

    return aRes;
}

} // namespace
