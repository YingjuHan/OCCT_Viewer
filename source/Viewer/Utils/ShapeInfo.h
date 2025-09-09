#ifndef _SHAPEINFO_HEADERFILE
#define _SHAPEINFO_HEADERFILE

#include "ColorTools.h"
#include "VecN.h"

#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <TopoDS_Shape.hxx>

namespace Viewer {

//! Shape information.
struct ShapeInfo {
    int shapeType   = TopAbs_SHAPE; // TopAbs_ShapeEnum
    int index       = -1;           // selection index
    int nbChildren  = 0;
    int orientation = TopAbs_EXTERNAL; // TopAbs_Orientation

    bool free     = false;
    bool locked   = false;
    bool modified = false;
    bool checked  = false;

    bool orientable = false;
    bool closed     = false;
    bool infinite   = false;
    bool convex     = false;

    int surfaceType = GeomAbs_OtherSurface; // GeomAbs_SurfaceType
    int curveType   = GeomAbs_OtherCurve;   // GeomAbs_CurveType
    //
    int nbTriangles    = 0; // number of triangles
    int nbTriNodes     = 0; // number of triangulation nodes
    int nbPolySegments = 0; // number of tessellation segments
    //
    Vec3                    point = Vec3::InvalidPoint(); // vertex position
    ColorTools::ColorString color;
    bool                    hidden = false;

    Location location; //!< shape location

public:
    //! Initialize fields.
    Standard_EXPORT void Init(const TopoDS_Shape& theShape, int theIndex);

    //! Explore subshapes.
    Standard_EXPORT void Explore(const TopoDS_Shape& theShape);
};

//! Cylinder shape construction parameters.
struct CylinderParams {
    Ax2    ax2;                 //!< coordinate system at center of bottom circle
    double radius = 0.0;        //!< cylinder radius
    double height = 0.0;        //!< cylinder height
    double angle  = 2.0 * M_PI; //!< cylinder section angle
};

//! Cone shape construction parameters.
struct ConeParams {
    Ax2    ax2;                    //!< coordinate system at center of bottom circle
    double radius    = 0.0;        //!< cone bottom circle radius
    double topRadius = 0.0;        //!< cone top circle radius
    double height    = 0.0;        //!< cone height
    double angle     = 2.0 * M_PI; //!< cone section angle
};

} // namespace

#endif // _SHAPEINFO_HEADERFILE