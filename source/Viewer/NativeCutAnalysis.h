#ifndef _NATIVECUTANALYSIS_HEADERFILE
#define _NATIVECUTANALYSIS_HEADERFILE
#include <Viewer/NativeCutView.h>
#include <ViewCaeGeom/MeshCutPhysics.h>
#include <ViewCaeGeom/MeshCutAutomatic.h>
#include <ViewCae/InteractiveCaeMesh.h>

namespace Viewer {
enum class CutComponentResolution { DefinedFrame, NormalTangential, Datum };
struct NativeCutAnalysisRequest {
    std::string Cut;
    CutFieldBinding Field;
    bool Heat = false;
    bool Beam = false;
    std::array<int,9> Components = {{0,1,2,3,4,5,6,7,8}};
    ViewCaeGeom::MeshCutPhysicsOptions Options;
    bool Overlay = true;
    bool Enabled = true;
    bool Distributed = false;
    bool ComponentVectors = false;
    CutComponentResolution Resolution = CutComponentResolution::DefinedFrame;
    std::string Datum = "global";
    Graphic3d_Vec3d TangentY = Graphic3d_Vec3d(0,1,0);
    double ArrowScale = 0.1; //!< Model length units per SI resultant unit.
    CutFieldBinding DomainField; //!< Cell scalar labels, or a MeshGroupProperty group bit. Empty uses Options.Elements.
    double DomainValue = 0;
    bool DomainMask = false;
    std::string DomainSource = "explicit"; //!< explicit / element_set / display_snapshot.
};
//! Owned by NativeCutView. Recomputes requests from physical FE fields on edits.
class Standard_EXPORT NativeCutAnalysis {
public:
    explicit NativeCutAnalysis(NativeCutView& owner) : myOwner(&owner) {}
    ~NativeCutAnalysis();
    bool Set(const std::string& id, const NativeCutAnalysisRequest& request);
    bool SetJson(const std::string& id, const std::string& json);
    NativeCutAnalysisRequest ReadRequestJson(const std::string& json) const;
    static std::string RequestJson(const NativeCutAnalysisRequest& request);
    bool Remove(const std::string& id);
    bool SetEnabled(const std::string& id, bool enabled);
    bool SetCutEnabled(const std::string& cut, bool enabled);
    //! Capture the actual source-element visibility of an existing presentation; independent of its result field.
    bool CaptureDisplayGroup(const std::string& id, const Handle(ViewCae::InteractiveCaeMesh)& mesh,
        const std::string& presentation);
    bool Refresh();
    void Detach();
    void Close();
    const std::map<std::string,NativeCutAnalysisRequest>& Requests() const { return myRequests; }
    const std::map<std::string,ViewCaeGeom::MeshCutPhysicsReport>& Reports() const { return myReports; }
    const std::map<std::string,std::vector<Handle(AIS_Shape)>>& Overlays() const { return myOverlays; }
    //! Remeshing with explicit groups/sections needs a complete request mapping for that step.
    void SetForStep(int step, const std::string& id, const NativeCutAnalysisRequest& request) { mySteps[step][id] = request; }
    const std::map<int,std::map<std::string,NativeCutAnalysisRequest>>& StepRequests() const { return mySteps; }
    bool PrepareStep(const NativeCutAnalysis& source, int step, bool remesh);
    bool Automatic(const std::string& id, const CutFieldBinding& field,
        ViewCaeGeom::MeshCutAutomaticMapping mapping);
    struct AutomaticBinding { CutFieldBinding Field; ViewCaeGeom::MeshCutAutomaticMapping Mapping; };
    const std::map<std::string,AutomaticBinding>& AutomaticBindings() const { return myAutomatic; }
private:
    friend class NativeCutTimeline;
    friend class NativeCutSession;
    friend class NativeCutView;
    ViewCaeGeom::MeshCutPhysicsReport compute(const NativeCutAnalysisRequest& request) const;
    std::vector<Handle(AIS_Shape)> arrows(const NativeCutAnalysisRequest& request, const ViewCaeGeom::MeshCutPhysicsReport& report) const;
    void publish(const std::string& id, const NativeCutAnalysisRequest& request,
        const ViewCaeGeom::MeshCutPhysicsReport& report, const std::vector<Handle(AIS_Shape)>& overlays);
    NativeCutView* myOwner;
    std::map<std::string,NativeCutAnalysisRequest> myRequests;
    std::map<std::string,ViewCaeGeom::MeshCutPhysicsReport> myReports;
    std::map<std::string,std::vector<Handle(AIS_Shape)>> myOverlays;
    std::map<int,std::map<std::string,NativeCutAnalysisRequest>> mySteps;
    std::map<std::string,AutomaticBinding> myAutomatic;
};
}
#endif
