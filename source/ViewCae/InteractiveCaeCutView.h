#ifndef _INTERACTIVECAECUTVIEW_HEADERFILE
#define _INTERACTIVECAECUTVIEW_HEADERFILE

#include <ViewCae/MeshPresentationScalar.h>
#include <ViewCae/MeshPresentationVector.h>
#include <ViewCaeGeom/MeshCutViewManager.h>

#include <AIS_InteractiveObject.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_AspectMarker3d.hxx>
#include <AIS_ColorScale.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <ViewCaeGeom/MeshCutFrame.h>
#include <functional>
#include <AIS_DragAction.hxx>
#include <Select3D_SensitiveEntity.hxx>

namespace ViewCae {
class MeshCutGpuPresentation;
enum class MeshCutRenderBackend { Auto, Cpu };
struct MeshCutRenderStatistics {
    bool GpuPlane = false;
    std::string FallbackReason;
    uint64_t BodyBuilds = 0;
    uint64_t SliceReallocations = 0;
    uint64_t SliceEvaluations = 0;
    uint64_t SubmittedBodyBytes = 0;
    uint64_t SubmittedSliceBytes = 0;
    double SourceMs = 0, SliceMs = 0, FieldsMs = 0, PresentationMs = 0;
};
struct MeshCutDisplayStyle {
    Quantity_Color Color = Quantity_Color(Quantity_NOC_GRAY80);
    float Transparency = 0.0f;
    float LineWidth = 2.0f;
    InteractiveCaeMeshPrsMode Mode = InteractiveCaeMeshPrsMode::FlatShaded;
};

struct MeshCutContourOptions {
    bool AutomaticRange = true;
    double Minimum = 0;
    double Maximum = 1;
    bool ShowLegend = false;
    int Intervals = 12;
};
enum class MeshCutContourState { NoField, Empty, Constant, Variable };
struct MeshCutProbe {
    Graphic3d_Vec3d Position = Graphic3d_Vec3d(0.0);
    RWCae::MeshEntityId SourceElement = 0;
    std::vector<std::pair<RWCae::MeshEntityId,double>> SourceNodes;
    std::vector<double> Values; //!< Original physical raw components; no display recovery.
};
//! Selection modes: 1 section nodes, 2 section elements. Owners expire on synchronization.
class Standard_EXPORT MeshCutPickOwner : public SelectMgr_EntityOwner {
    DEFINE_STANDARD_RTTIEXT(MeshCutPickOwner, SelectMgr_EntityOwner)
public:
    MeshCutPickOwner(const Handle(SelectMgr_SelectableObject)& object, const std::string& id,
        int portion, RWCae::MeshEntityId entity, bool node, uint64_t generation)
        : SelectMgr_EntityOwner(object,5), CutId(id), Portion(portion), Entity(entity), Node(node), Generation(generation) {}
    std::string CutId;
    int Portion;
    RWCae::MeshEntityId Entity;
    bool Node;
    uint64_t Generation;
    size_t Branch = 0;
    bool Line = false;
    bool Source = false;
};

//! Native display object for enabled cut portions, or the uncut source when no cut is active.
//! After editing Cuts(), call Synchronize() and context->Redisplay(this, false).
class Standard_EXPORT InteractiveCaeCutView : public AIS_InteractiveObject {
    DEFINE_STANDARD_RTTIEXT(InteractiveCaeCutView, AIS_InteractiveObject)
public:
    Standard_EXPORT explicit InteractiveCaeCutView(const Handle(RWCae::MeshData)& theMesh);
    Standard_EXPORT ~InteractiveCaeCutView() override;
    void SetRenderBackend(MeshCutRenderBackend backend) { myRenderBackend = backend; }
    MeshCutRenderBackend RenderBackend() const { return myRenderBackend; }
    const MeshCutRenderStatistics& RenderStatistics() const { return myRenderStatistics; }
    Standard_EXPORT Handle(Graphic3d_ArrayOfTriangles) SliceArray() const;
    ViewCaeGeom::MeshCutViewManager& Cuts() { return myCuts; }
    Standard_EXPORT void SetField(const Handle(RWCae::MeshNVecProperty)& theField, int theRawComponent = 0);
    Standard_EXPORT void SetStyle(ViewCaeGeom::MeshCutPortion thePortion, const MeshCutDisplayStyle& theStyle);
    Standard_EXPORT void SetStyle(const std::string& cut, ViewCaeGeom::MeshCutPortion portion, const MeshCutDisplayStyle& style);
    Standard_EXPORT void ClearStyle(const std::string& cut, ViewCaeGeom::MeshCutPortion portion);
    Standard_EXPORT MeshCutDisplayStyle EffectiveStyle(const std::string& cut, ViewCaeGeom::MeshCutPortion portion) const;
    const std::map<std::pair<std::string,int>,MeshCutDisplayStyle>& StyleOverrides() const { return myStyleOverrides; }
    const MeshCutDisplayStyle& Style(ViewCaeGeom::MeshCutPortion portion) const { return myStyles.at((size_t)portion); }
    Standard_EXPORT void SetContourOptions(const MeshCutContourOptions& options);
    const MeshCutContourOptions& ContourOptions() const { return myContourOptions; }
    MeshCutContourState ContourState() const { return myContourState; }
    const Handle(AIS_ColorScale)& Legend() const { return myLegend; }
    const Handle(RWCae::MeshData)& SourceMesh() const { return myMesh; }
    const Handle(RWCae::MeshNVecProperty)& Field() const { return myField; }
    int Component() const { return myComponent; }
    Standard_EXPORT MeshCutProbe Probe(const std::string& cut, ViewCaeGeom::MeshCutPortion portion,
        RWCae::MeshEntityId entity, bool node);
    Standard_EXPORT MeshCutProbe Probe(const Handle(MeshCutPickOwner)& owner);
    //! Shell section line: endpoint 0/1, or -1 for the arithmetic endpoint mean.
    Standard_EXPORT MeshCutProbe ProbeLine(const std::string& cut, size_t line, int endpoint = -1);
    Standard_EXPORT std::vector<double> ProbeSource(RWCae::MeshEntityId entity, bool node,
        RWCae::MeshEntityId elementNode = 0) const;
    Standard_EXPORT void SetReferenceFrame(const ViewCaeGeom::MeshCutFrame& frame, bool show, double length);
    //! Vector scale follows the existing normalized mesh-vector convention.
    Standard_EXPORT void SetVectorField(const Handle(RWCae::MeshNVecProperty)& field, double scale = 1, double sampling = 1, bool orientation = false);
    const Handle(RWCae::MeshNVecProperty)& VectorField() const { return myVectorSource; }
    double VectorScale() const { return myVectorScale; }
    double VectorSampling() const { return myVectorSampling; }
    bool VectorOrientation() const { return myVectorOrientation; }
    const std::vector<Handle(MeshPresentationVector)>& VectorPresentations() const { return myVectorPresentations; }
    const ViewCaeGeom::MeshCutFrame& ReferenceFrame() const { return myReferenceFrame; }
    Standard_EXPORT void Synchronize();
    void SetOnDrag(const std::function<void(AIS_DragAction)>& callback) { myOnDrag = callback; }
    void SetOnSynchronize(const std::function<void()>& callback) { myOnSynchronize = callback; }
    Standard_EXPORT void NotifyDrag(AIS_DragAction action);

    const std::vector<Handle(MeshPresentationScalar)>& Presentations() const { return myPresentations; }
    const std::vector<Handle(Graphic3d_ArrayOfSegments)>& LineArrays() const { return myLineArrays; }
    //! Last completed presentation synchronization, independent of lazy geometry evaluation.
    uint64_t SynchronizationCount() const { return mySynchronizationCount; }
    const std::map<std::string, uint64_t>& SynchronizedRevisions() const { return mySynchronizedRevisions; }

protected:
    Standard_EXPORT void Compute(const Handle(PrsMgr_PresentationManager)&,
        const Handle(Prs3d_Presentation)& thePrs, Standard_Integer theMode) override;
    Standard_EXPORT void ComputeSelection(const Handle(SelectMgr_Selection)&, Standard_Integer) override;

private:
    std::string planeFallback(const std::vector<ViewCaeGeom::MeshCutDefinition>& definitions) const;
    MeshCutRenderBackend myRenderBackend = MeshCutRenderBackend::Auto;
    MeshCutRenderStatistics myRenderStatistics;
    std::unique_ptr<MeshCutGpuPresentation> myGpuPresentation;
    Handle(MeshPresentationScalar) myBodyPresentation;
    uint64_t myBodySourceEpoch = 0, myBodyFieldEpoch = 0;
    int myBodyComponent = -1;
    ViewCaeGeom::MeshCutCellMode myBodyCellMode = ViewCaeGeom::MeshCutCellMode::Constant;
    std::array<std::vector<Handle(Select3D_SensitiveEntity)>,2> myBodySelection;
    uint64_t myBodySelectionEpoch = 0;
    ViewCaeGeom::MeshCutResult myBodyPickResult;
    MeshCutProbe probeResult(const ViewCaeGeom::MeshCutResult& cut, int portion, RWCae::MeshEntityId entity, bool node);
    MeshCutProbe probeLineResult(const ViewCaeGeom::MeshCutResult& cut, size_t line, int endpoint);
    ViewCaeGeom::MeshCutViewManager myCuts;
    Handle(RWCae::MeshData) myMesh;
    Handle(RWCae::MeshNVecProperty) myVectorField;
    Handle(RWCae::MeshNVecProperty) myVectorSource;
    double myVectorScale = 1;
    double myVectorSampling = 1;
    bool myVectorOrientation = false;
    Handle(RWCae::MeshNVecProperty) myField;
    int myComponent = 0;
    std::array<MeshCutDisplayStyle, 3> myStyles;
    std::map<std::pair<std::string,int>,MeshCutDisplayStyle> myStyleOverrides;
    MeshCutContourOptions myContourOptions;
    MeshCutContourState myContourState = MeshCutContourState::NoField;
    Handle(AIS_ColorScale) myLegend;
    ViewCaeGeom::MeshCutFrame myReferenceFrame;
    bool myShowReferenceFrame = false;
    double myReferenceLength = 1;
    std::vector<Handle(MeshPresentationScalar)> myPresentations;
    std::vector<Handle(MeshPresentationVector)> myVectorPresentations;
    std::vector<Handle(Graphic3d_ArrayOfSegments)> myLineArrays;
    std::vector<Handle(Graphic3d_AspectLine3d)> myLineAspects;
    std::vector<Handle(Graphic3d_ArrayOfPoints)> myPointArrays;
    std::vector<Handle(Graphic3d_AspectMarker3d)> myPointAspects;
    uint64_t mySynchronizationCount = 0;
    std::map<std::string, uint64_t> mySynchronizedRevisions;
    std::shared_ptr<ViewCaeGeom::MeshCutPresentation> myCutPresentation;
    std::vector<ViewCaeGeom::MeshCutResult> myPickResults;
    std::function<void(AIS_DragAction)> myOnDrag;
    std::function<void()> myOnSynchronize;
};
}
#endif
