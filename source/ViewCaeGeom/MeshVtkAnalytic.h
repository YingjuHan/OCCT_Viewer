#ifndef _MESHVTKANALYTIC_HEADERFILE
#define _MESHVTKANALYTIC_HEADERFILE

#include <Bnd_Box.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <ViewCaeGeom/MeshCellKernel.h>

namespace ViewCaeGeom {
//! Polynomial/rational parameter conversion through degree six. Unsupported charts
//! or normalized positive weights <=1e-12 return null; invalid weights are rejected.
Handle(Geom_BezierSurface) VtkAnalyticSurface(MeshCellKernel* cell, Bnd_Box& bounds, double& residual);
Handle(Geom_BezierCurve) VtkAnalyticCurve(MeshCellKernel* cell, Bnd_Box& bounds, double& residual);
}
#endif
