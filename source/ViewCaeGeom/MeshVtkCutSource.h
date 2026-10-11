#ifndef _MESHVTKCUTSOURCE_HEADERFILE
#define _MESHVTKCUTSOURCE_HEADERFILE

#include <ViewCaeGeom/MeshVtkCell.h>
#include <ViewCaeGeom/MeshCutViewManager.h>
#include <BVH_BoxSet.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pln.hxx>
#include <memory>

namespace ViewCaeGeom {
//! Single-threaded prepared source, shared by all cuts/portions within a manager epoch.
class MeshVtkCutSource {
public:
    struct Face {
        bool Planar = false;
        bool Internal = false;
        RWCae::MeshEntityId Owner = 0;
        gp_Pln Plane;
    };
    struct PrimitiveFace {
        TopoDS_Shape Shape;
        size_t FaceIndex;
    };
    struct Cell {
        std::unique_ptr<MeshVtkCell> Geometry;
        Bnd_Box Bounds;
        std::vector<Face> Faces;
        TopoDS_Shape Shape;
        std::vector<PrimitiveFace> PrimitiveFaces;
    };
    MeshVtkCutSource(uint64_t epoch, const MeshCutGeometryOptions& options) : myOptions(options) { Work.SourceEpoch = epoch; }
    void Prepare(const Handle(RWCae::MeshData)& mesh, MeshGeometry* source);
    std::vector<RWCae::MeshEntityId> Query(const MeshCutDefinition& cut, MeshCutPortion portion, bool sourceOnly);
    Cell& GetCell(RWCae::MeshEntityId index) { return myCells.at(index); }
    const TopoDS_Shape& Shape(RWCae::MeshEntityId index);
    double Tolerance() const { return myTolerance; }
    const Bnd_Box& Bounds() const { return myBounds; }
    MeshCutStatistics Work;
private:
    using BoxSet = BVH_BoxSet<double,3,RWCae::MeshEntityId>;
    bool myReady = false;
    double myTolerance = 0;
    Bnd_Box myBounds;
    std::vector<Cell> myCells;
    Handle(BoxSet) myBoxes;
    MeshCutGeometryOptions myOptions;
};
}
#endif
