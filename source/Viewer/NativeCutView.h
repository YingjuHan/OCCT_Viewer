#ifndef _NATIVECUTVIEW_HEADERFILE
#define _NATIVECUTVIEW_HEADERFILE

#include <ViewCae/InteractiveCaeCutPlane.h>
#include <RWMeshConv/MeshLoader.h>
#include <AIS_TextLabel.hxx>
#include <functional>

namespace Viewer {
enum class CutEditEvent { Begin, Update, Commit, Abort, Close };
struct CutFieldBinding {
    std::string Name;
    bool Nodal = true;
    int Component = 0;
    int Step = -1;
};
//! Native Viewer facade. Owns only its cut display/aids/overlays, not the caller's CAD objects.
//! All calls and callbacks execute on the Viewer thread. False means LastError() is set.
class Standard_EXPORT NativeCutView {
public:
    explicit NativeCutView(const Handle(RWCae::MeshData)& mesh);
    ~NativeCutView();
    NativeCutView(const NativeCutView&) = delete;
    NativeCutView& operator=(const NativeCutView&) = delete;
    const Handle(ViewCae::InteractiveCaeCutView)& Display() const { return myDisplay; }
    const std::string& LastError() const { return myError; }
    uint64_t Generation() const { return myGeneration; }
    bool IsDragging() const { return !myDragSnapshot.empty(); }
    const CutFieldBinding& FieldBinding() const { return myFieldBinding; }
    bool BindField(const CutFieldBinding& binding);
    bool SetDatum(const std::string& id, const ViewCaeGeom::MeshCutFrame& frame);
    bool SetReferenceAxes(const std::string& datum, bool show, double length);
    const std::string& ReferenceDatum() const { return myReferenceDatum; }
    bool ReferenceAxesVisible() const { return myReferenceAxesVisible; }
    double ReferenceAxesLength() const { return myReferenceAxesLength; }
    const std::map<std::string,ViewCaeGeom::MeshCutFrame>& Datums() const { return myDatums; }
    bool Edit(const ViewCaeGeom::MeshCutDefinition& definition, const std::string& datum = "");
    bool BeginDrag();
    bool EndDrag(bool commit);
    void SetOnEdit(const std::function<void(CutEditEvent,uint64_t)>& callback) { myOnEdit = callback; }
    bool Attach(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& viewport = nullptr);
    void Detach();
    void Close();
    bool Refresh(bool redraw = true);
    bool ShowHandle(const std::string& cut, bool gizmo);
    void ClearHandle();
    const Handle(ViewCae::InteractiveCaeCutPlane)& PlaneHandle() const { return myPlane; }
    const Handle(ViewCae::InteractiveCaeCutQuadric)& QuadricHandle() const { return myQuadric; }
    const Handle(AIS_Manipulator)& Manipulator() const { return myManipulator; }
    const std::array<uint64_t,5>& EventCounts() const { return myEventCounts; }
    bool AddAnnotation(const std::string& id, const ViewCae::MeshCutProbe& probe, const std::string& text);
    void ClearAnnotations();
    const std::map<std::string,Handle(AIS_TextLabel)>& Annotations() const { return myAnnotations; }
    //! CAD section is independent of viewport clipping; syncClip follows the selected plane.
    bool SetCadSection(const TopoDS_Shape& shape, const std::string& cut, bool syncClip);
    void ClearCadSection();
    bool SetGlobalClip(const Graphic3d_Vec4d& plane, bool enabled);
    const Handle(AIS_Shape)& CadSection() const { return myCadSection; }
    const Handle(Graphic3d_ClipPlane)& GlobalClip() const { return myGlobalClip; }
private:
    friend class NativeCutTimeline;
    bool attempt(const std::function<void()>& action);
    Handle(RWCae::MeshNVecProperty) resolveField(const CutFieldBinding& binding) const;
    void event(CutEditEvent value);
    void synchronized();
    Handle(ViewCae::InteractiveCaeCutView) myDisplay;
    Handle(AIS_InteractiveContext) myContext;
    Handle(V3d_View) myViewport;
    Handle(ViewCae::InteractiveCaeCutPlane) myPlane;
    Handle(ViewCae::InteractiveCaeCutQuadric) myQuadric;
    Handle(AIS_Manipulator) myManipulator;
    std::array<uint64_t,5> myEventCounts = {};
    std::map<std::string,ViewCaeGeom::MeshCutFrame> myDatums;
    std::string myReferenceDatum = "global";
    bool myReferenceAxesVisible = false;
    double myReferenceAxesLength = 1;
    std::map<std::string,Handle(AIS_TextLabel)> myAnnotations;
    Handle(AIS_Shape) myCadSection;
    TopoDS_Shape myCadSource;
    std::string myCadCut;
    bool mySyncClip = false;
    Handle(Graphic3d_ClipPlane) myGlobalClip;
    std::string myDragSnapshot;
    CutFieldBinding myFieldBinding;
    std::string myError;
    uint64_t myGeneration = 1;
    std::function<void(CutEditEvent,uint64_t)> myOnEdit;
};
}
#endif
