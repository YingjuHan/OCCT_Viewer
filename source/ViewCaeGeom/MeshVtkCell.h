#ifndef _MESHVTKCELL_HEADERFILE
#define _MESHVTKCELL_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>
#include <ViewCaeGeom/MeshCutViewManager.h>
#include <vtkGenericCell.h>
#include <vtkIdList.h>
#include <vtkSmartPointer.h>
#include <Geom_BezierSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <array>

namespace ViewCaeGeom {
//! Original VTK shape functions, with explicit bounded geometric tessellation.
class MeshVtkCell {
public:
    struct Face {
        int Type = 0;
        std::vector<RWCae::MeshEntityId> Nodes;
        std::vector<std::vector<RWCae::MeshEntityId>> Edges;
        bool StraightSided = true;
        size_t FirstTriangle = 0;
        size_t NbTriangles = 0;
        Handle(Geom_BezierSurface) Surface;
    };
    MeshVtkCell(const Handle(RWCae::MeshData)& theMesh, RWCae::MeshEntityId theIndex,
        MeshGeometry* theSource, double theTolerance,
        const MeshCutGeometryOptions& theOptions = MeshCutGeometryOptions(), int theMinimumDepth = 0,
        bool thePrepareBoundary = true);
    //! Parameter-space simplices and original FE evaluation, without inverse mapping.
    std::vector<std::array<Graphic3d_Vec3d,4>> ParameterSimplices() const;
    MeshInterpolatedNode EvaluateParameter(const Graphic3d_Vec3d& theParameter,
        Graphic3d_Vec3d& thePosition) const;
    Standard_EXPORT MeshInterpolatedNode Origin(const Graphic3d_Vec3d& thePosition) const;
    //! Original node IDs on faces, surface edges or curve endpoints.
    std::vector<std::vector<RWCae::MeshEntityId>> BoundaryNodes() const;
    int Dimension() const { return myCell->GetCellDimension(); }
    std::vector<std::array<Graphic3d_Vec3d,3>> Triangles;
    std::vector<std::array<Graphic3d_Vec3d,2>> Segments;
    std::vector<Graphic3d_Vec3d> Points;
    std::vector<Face> Faces;
    int FaceDepth = 0;
    Graphic3d_Vec3d InteriorPoint() const;
    Handle(Geom_BezierCurve) Curve;
    Handle(Geom_BezierSurface) Surface;
    std::vector<Handle(Geom_BezierCurve)> BoundaryCurves;
    Bnd_Box AnalyticBounds;
    double AnalyticResidual = 0;
    bool SourceApproximate = false;
private:
    void tessellate(vtkCell* theCell, double theTolerance);
    vtkSmartPointer<vtkGenericCell> myCell;
    vtkSmartPointer<vtkIdList> myMvcFaces;
    std::vector<RWCae::MeshEntityId> myNodes;
    double myTolerance;
    MeshCutGeometryOptions myOptions;
    int myMinimumDepth;
};
}
#endif
