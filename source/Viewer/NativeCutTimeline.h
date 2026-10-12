#ifndef _NATIVECUTTIMELINE_HEADERFILE
#define _NATIVECUTTIMELINE_HEADERFILE
#include <Viewer/NativeCutView.h>

namespace Viewer {
enum class CutFrameConfiguration { Reference, Updated };
struct CutMaterialFrame {
    CutFrameConfiguration Configuration = CutFrameConfiguration::Reference;
    //! Reference uses key -1; updated geometry uses an explicit key for every requested step.
    std::map<int,std::array<RWCae::MeshEntityId,3>> Anchors;
};
//! Series controller. A complete new display is prepared before replacing the live one.
//! Tick takes elapsed wall-clock seconds on the Viewer thread; failed frames pause playback.
class Standard_EXPORT NativeCutTimeline {
public:
    explicit NativeCutTimeline(NativeCutView& owner);
    bool SwitchStep(int step);
    bool BindDeformation(const CutFieldBinding& binding, double normalizedScale);
    bool SetMaterialFrame(const std::string& datum, const CutMaterialFrame& frame);
    bool BindCutFrame(const std::string& cut, const std::string& datum);
    bool Play(double secondsPerStep, bool loop = true, bool restart = false);
    //! Real source timestamps, advanced at the requested time-unit/second speed.
    bool PlayTime(double speed = 1, bool loop = true, bool restart = false);
    void Pause() { myPlaying = false; }
    bool Tick(double elapsed);
    bool IsPlaying() const { return myPlaying; }
    int Step() const { return myStep; }
    size_t StepCount() const { return mySource->SeriesSteps().size(); }
    const Handle(RWCae::MeshData)& Source() const { return mySource; }
    const CutFieldBinding& DeformationBinding() const { return myDeformation; }
    double DeformationScale() const { return myScale; }
    const std::map<std::string,CutMaterialFrame>& MaterialFrames() const { return myMaterialFrames; }
    const std::map<std::string,std::string>& CutFrames() const { return myCutFrames; }
    //! Explicit remesh mappings. Missing region mappings reject a topology-changing step.
    void SetRecoveryForStep(int step, const ViewCaeGeom::MeshCutRecoveryOptions& options) { myRecoverySteps[step] = options; }
    void SetTraceForStep(int step, const std::string& cut, const ViewCaeGeom::MeshCutTraceOptions& display,
        const ViewCaeGeom::MeshCutTraceOptions& physical) { myTraceSteps[step][cut] = {display,physical}; }
private:
    Handle(RWCae::MeshNVecProperty) resolve(const Handle(RWCae::MeshData)& mesh, const CutFieldBinding& binding) const;
    ViewCaeGeom::MeshCutFrame materialFrame(const CutMaterialFrame& binding, int step,
        const Handle(RWCae::MeshData)& mesh, const Handle(RWCae::MeshNVecProperty)& deformation, double effectiveScale) const;
    bool prepare(int step, const CutFieldBinding& deformation, double scale);
    NativeCutView& myOwner;
    Handle(RWCae::MeshData) mySource;
    int myStep = -1;
    CutFieldBinding myDeformation;
    double myScale = 0;
    std::map<std::string,CutMaterialFrame> myMaterialFrames;
    std::map<std::string,std::string> myCutFrames;
    std::map<int,ViewCaeGeom::MeshCutRecoveryOptions> myRecoverySteps;
    std::map<int,std::map<std::string,std::pair<ViewCaeGeom::MeshCutTraceOptions,ViewCaeGeom::MeshCutTraceOptions>>> myTraceSteps;
    bool myPlaying = false;
    bool myLoop = true;
    bool myUseTimes = false;
    double myPeriod = 1;
    double myElapsed = 0;
};
}
#endif
