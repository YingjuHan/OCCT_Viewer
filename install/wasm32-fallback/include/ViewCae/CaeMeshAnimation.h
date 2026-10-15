#ifndef _CAEMESHANIMATION_HEADERFILE
#define _CAEMESHANIMATION_HEADERFILE

#include <AIS_Animation.hxx>

#include <functional>
#include <vector>

namespace ViewCae {
class InteractiveCaeMesh; // forward declared class due to cyclic dependency

//! Class that handles animation of mesh properties.
class CaeMeshAnimation : public AIS_Animation {
public:
    //! Constructor.
    Standard_EXPORT explicit CaeMeshAnimation(InteractiveCaeMesh* theMeshIO);

    //! Return true if this animation has valid time step input and can be started
    Standard_EXPORT bool IsValid() const;

    //! Start animation with internally defined timer instance.
    Standard_EXPORT virtual void StartTimer(const Standard_Real    theStartPts,
                                            const Standard_Real    thePlaySpeed,
                                            const Standard_Boolean theToUpdate,
                                            const Standard_Boolean theToStopTimer = Standard_False) override;

    //! Stop animation. This method changed status of the animation to Stopped.
    Standard_EXPORT virtual void Stop() override;

    //! Continue animation if it is paused.
    Standard_EXPORT void Continue();

    //! Check if animation is paused.
    bool IsPaused() const { return myState == AnimationState_Paused; }

    //! Check if animation is not stopped.
    bool IsRunning() const { return myIsRunning; }

    //! Update single frame of animation, update timer state.
    Standard_EXPORT virtual Standard_Real UpdateTimer() override;

    //! Check if animation should be restarted on the last update.
    bool IsLoopPlayback() const { return myIsLoopPlayback; }

    //! Define if animation should be restarted on the last update.
    void SetLoopPlayback(bool theIsLoopPlayback) { myIsLoopPlayback = theIsLoopPlayback; }

    //! Set animation duration, in seconds.
    Standard_EXPORT void SetDuration(double theDur);

    //! Check if animation steps duration should take into account timestamps values.
    bool IsUniformSteps() const { return myIsUniformSteps; }

    //! Define if animation steps duration should take into account timestamps values.
    void SetUniformSteps(bool theIsUniform) { myIsUniformSteps = theIsUniform; }

    //! Set a callback that is called on animation update.
    //! Parameters
    //! - current animation time step;
    //! - flag indicates that time step changed;
    //! - flag indicates that animation has not stopped.
    void SetOnAnimationUpdateCallback(const std::function<void(int, bool, bool)>& theCallback)
    {
        myOnAnimationUpdate = theCallback;
    }

protected:
    //! Update the own animation to specified position.
    Standard_EXPORT virtual void update(const AIS_AnimationProgress& theProgress) override;

    //! Return mesh timestamps range.
    double timestampsRange() const
    {
        return (myTimestamps.back().second - myTimestamps.front().second) * (1.0 + 1.0 / (myTimestamps.size() - 1));
    }

    //! Gather list of properties ids as a string that are used in calculation of primitives
    //! for specified time step but not loaded.
    Standard_EXPORT TCollection_AsciiString getNotLoadedProps(int theTimeStep) const;

protected:
    InteractiveCaeMesh*                 myMeshIO;
    std::vector<std::pair<int, double>> myTimestamps;

    std::function<void(int, bool, bool)> myOnAnimationUpdate;

    bool myWaitForComputing = false;
    bool myIsLoopPlayback   = true;
    bool myIsUpdating       = false;
    bool myIsUniformSteps   = false;
    bool myIsRunning        = false;
};

} // namespace

#endif // _CAEMESHANIMATION_HEADERFILE