#ifndef _MESHVTKANALYTIC_HEADERFILE
#define _MESHVTKANALYTIC_HEADERFILE

#include <Bnd_Box.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <vtkCell.h>

namespace ViewCaeGeom {
//! Exact polynomial/rational parameter conversion; unsupported orders return null.
Handle(Geom_BezierSurface) VtkAnalyticSurface(vtkCell* cell, Bnd_Box& bounds, double& residual);
Handle(Geom_BezierCurve) VtkAnalyticCurve(vtkCell* cell, Bnd_Box& bounds, double& residual);
}
#endif
