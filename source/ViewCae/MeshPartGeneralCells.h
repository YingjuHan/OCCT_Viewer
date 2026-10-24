#ifndef _MESHPARTGENERALCELLS_HEADERFILE
#define _MESHPARTGENERALCELLS_HEADERFILE

#include <ViewCae/MeshPart.h>
#include <array>

namespace ViewCae {
//! Ordinary scalar Viewer primitives for original external VTK cells.
class MeshPartGeneralCells : public MeshPart {
public:
    struct Vertex {
        Graphic3d_Vec3d Position;
        ViewCaeGeom::MeshInterpolatedNode Weights;
    };
    struct Primitive {
        std::array<Vertex, 3> Vertices;
        RWCae::MeshEntityId Element = 0;
        int Dimension = 0;
        int Size = 0;
    };
    Standard_EXPORT static std::vector<Primitive> BuildPrimitives(
        const Handle(ViewCaeGeom::MeshGeometry)& geometry);
    Standard_EXPORT static double Value(const Primitive& primitive, int vertex,
        const RWCae::MeshPropertyComponent& field);
protected:
    void clear(bool) override { myArrays.clear(); myDimensions.clear(); }
    void update(const Handle(MeshPresentation)& presentation) override;
    void compute(const Handle(AIS_InteractiveContext)&,
        const Handle(InteractiveCaeMeshDrawer)& drawer,
        const Handle(Prs3d_Presentation)& presentation,
        const Handle(Prs3d_Presentation)&, float) const override;
    bool isDefined() const override { return !myArrays.empty(); }
private:
    std::vector<Handle(Graphic3d_ArrayOfPrimitives)> myArrays;
    std::vector<int> myDimensions;
};
}
#endif
