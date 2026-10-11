#ifndef _INTERACTIVECAECUTVIEW_HEADERFILE
#define _INTERACTIVECAECUTVIEW_HEADERFILE

#include <ViewCae/MeshPresentationScalar.h>
#include <ViewCaeGeom/MeshCutViewManager.h>

#include <AIS_InteractiveObject.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_AspectMarker3d.hxx>

namespace ViewCae {
struct MeshCutDisplayStyle {
    Quantity_Color Color = Quantity_Color(Quantity_NOC_GRAY80);
    float Transparency = 0.0f;
    float LineWidth = 2.0f;
    InteractiveCaeMeshPrsMode Mode = InteractiveCaeMeshPrsMode::FlatShaded;
};

//! Native display object for enabled cut portions, or the uncut source when no cut is active.
//! After editing Cuts(), call Synchronize() and context->Redisplay(this, false).
class InteractiveCaeCutView : public AIS_InteractiveObject {
    DEFINE_STANDARD_RTTIEXT(InteractiveCaeCutView, AIS_InteractiveObject)
public:
    Standard_EXPORT explicit InteractiveCaeCutView(const Handle(RWCae::MeshData)& theMesh);
    ViewCaeGeom::MeshCutViewManager& Cuts() { return myCuts; }
    Standard_EXPORT void SetField(const Handle(RWCae::MeshNVecProperty)& theField, int theRawComponent = 0);
    Standard_EXPORT void SetStyle(ViewCaeGeom::MeshCutPortion thePortion, const MeshCutDisplayStyle& theStyle);
    Standard_EXPORT void Synchronize();

    const std::vector<Handle(MeshPresentationScalar)>& Presentations() const { return myPresentations; }
    const std::vector<Handle(Graphic3d_ArrayOfSegments)>& LineArrays() const { return myLineArrays; }
    //! Last completed presentation synchronization, independent of lazy geometry evaluation.
    uint64_t SynchronizationCount() const { return mySynchronizationCount; }
    const std::map<std::string, uint64_t>& SynchronizedRevisions() const { return mySynchronizedRevisions; }

protected:
    Standard_EXPORT void Compute(const Handle(PrsMgr_PresentationManager)&,
        const Handle(Prs3d_Presentation)& thePrs, Standard_Integer theMode) override;
    void ComputeSelection(const Handle(SelectMgr_Selection)&, Standard_Integer) override {}

private:
    ViewCaeGeom::MeshCutViewManager myCuts;
    Handle(RWCae::MeshNVecProperty) myField;
    int myComponent = 0;
    std::array<MeshCutDisplayStyle, 3> myStyles;
    std::vector<Handle(MeshPresentationScalar)> myPresentations;
    std::vector<Handle(Graphic3d_ArrayOfSegments)> myLineArrays;
    std::vector<Handle(Graphic3d_AspectLine3d)> myLineAspects;
    std::vector<Handle(Graphic3d_ArrayOfPoints)> myPointArrays;
    std::vector<Handle(Graphic3d_AspectMarker3d)> myPointAspects;
    uint64_t mySynchronizationCount = 0;
    std::map<std::string, uint64_t> mySynchronizedRevisions;
};
}
#endif
