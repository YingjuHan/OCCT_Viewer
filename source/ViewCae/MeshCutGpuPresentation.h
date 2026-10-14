#ifndef _MESHCUTGPUPRESENTATION_HEADERFILE
#define _MESHCUTGPUPRESENTATION_HEADERFILE

#include <ViewCae/InteractiveCaeCutView.h>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_Group.hxx>
#include <Graphic3d_ShaderProgram.hxx>

namespace ViewCae {
//! Viewer-thread resident structures. Parent AIS recomputation only reconnects
//! them; it never recreates the body's OpenGL primitive arrays.
class MeshCutGpuPresentation {
public:
    ~MeshCutGpuPresentation();
    void Update(const Handle(MeshPresentationScalar)& body,
        const ViewCaeGeom::MeshCutResult& section,
        const ViewCaeGeom::MeshCutDefinition& definition,
        const std::array<MeshCutDisplayStyle,3>& styles,
        const Handle(Graphic3d_Texture2D)& texture, int component,
        ViewCaeGeom::MeshCutViewManager& cuts, MeshCutRenderStatistics& statistics);
    void Attach(const Handle(Prs3d_Presentation)& parent,const Handle(Graphic3d_StructureManager)& manager);
    void Reset();
    bool IsAttached() const { return !myBodyStructure.IsNull(); }
    bool AcceptBodyPoint(const gp_Pnt& point) const;
    const Handle(Graphic3d_ArrayOfTriangles)& SliceArray() const { return mySlice; }
private:
    void updateStructures();
    Handle(MeshPresentationScalar) myBody;
    std::vector<Handle(Graphic3d_ArrayOfTriangles)> myBodyArrays;
    Graphic3d_Vec3d myOrigin = Graphic3d_Vec3d(0);
    uint64_t mySourceEpoch = 0;
    uint64_t myBodyEpoch = 0;
    Bnd_Box myBounds;
    Bnd_Box mySliceBounds;
    Handle(Graphic3d_ArrayOfTriangles) mySlice;
    Handle(Graphic3d_AspectFillArea3d) myBodyAspect, mySliceAspect;
    Handle(Graphic3d_ShaderProgram) myBodyShader, mySliceShader;
    Handle(Graphic3d_Structure) myBodyStructure, myParent;
    Handle(Graphic3d_StructureManager) myManager;
    Handle(Graphic3d_Group) mySliceGroup;
    Handle(Graphic3d_ClipPlane) myClip;
    bool myBodyChanged = false, mySliceChanged = false;
    bool myShowBody = false, myShowSlice = false;
    bool myBodyConnected = false;
    Graphic3d_Vec4d myPlane;
    unsigned myMask = 0;
    std::shared_ptr<void> myBodyLease, mySliceLease;
};
}
#endif
