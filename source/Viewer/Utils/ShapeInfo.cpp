#include "ShapeInfo.h"

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : Init
// ================================================================
void ShapeInfo::Init(const TopoDS_Shape& theShape, int theIndex)
{
    static constexpr int gNbSegNodes2 = 2;

    index       = theIndex;
    shapeType   = theShape.ShapeType();
    nbChildren  = theShape.NbChildren();
    orientation = theShape.Orientation();

    free     = theShape.Free();
    locked   = theShape.Locked();
    modified = theShape.Modified();
    checked  = theShape.Checked();

    orientable = theShape.Orientable();
    closed     = theShape.Closed();
    infinite   = theShape.Infinite();
    convex     = theShape.Convex();

    location = Location(theShape.Location());

    TopLoc_Location aLoc;
    switch (theShape.ShapeType()) {
        case TopAbs_FACE: {
            BRepAdaptor_Surface aSurf(TopoDS::Face(theShape));
            surfaceType = aSurf.GetType();

            if (const Handle(Poly_Triangulation)& aTri = BRep_Tool::Triangulation(TopoDS::Face(theShape), aLoc)) {
                nbTriangles = aTri->NbTriangles();
                nbTriNodes  = aTri->NbNodes();
            }
            break;
        }
        case TopAbs_EDGE: {
            const TopoDS_Edge& anEdge = TopoDS::Edge(theShape);

            curveType = BRep_Tool::IsGeometric(anEdge) ? BRepAdaptor_Curve(anEdge).GetType() : GeomAbs_OtherCurve;

            Handle(Poly_PolygonOnTriangulation) aPolOnTri;
            Handle(Poly_Triangulation)          aTri;
            BRep_Tool::PolygonOnTriangulation(anEdge, aPolOnTri, aTri, aLoc);
            if (const Handle(Poly_Polygon3D)& aPol = BRep_Tool::Polygon3D(anEdge, aLoc)) {
                nbPolySegments = aPol->NbNodes() / gNbSegNodes2;
            } else if (!aPolOnTri.IsNull()) {
                nbPolySegments = aPolOnTri->NbNodes() / gNbSegNodes2;
            }
            break;
        }
        case TopAbs_VERTEX: {
            point = Vec3(BRep_Tool::Pnt(TopoDS::Vertex(theShape)).XYZ());
            break;
        }
        default: {
            Explore(theShape);
            break;
        }
    }
}

// ================================================================
// Function : Explore
// ================================================================
void ShapeInfo::Explore(const TopoDS_Shape& theShape)
{
    static constexpr int gNbSegNodes2 = 2;

    TopLoc_Location aLoc;
    for (TopExp_Explorer aFaceIter(theShape, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next()) {
        const TopoDS_Face& aFace = TopoDS::Face(aFaceIter.Value());
        if (const Handle(Poly_Triangulation)& aTri = BRep_Tool::Triangulation(aFace, aLoc)) {
            nbTriangles += aTri->NbTriangles();
            nbTriNodes += aTri->NbNodes();
        }
    }

    for (TopExp_Explorer anEdgeIter(theShape, TopAbs_EDGE); anEdgeIter.More(); anEdgeIter.Next()) {
        const TopoDS_Edge& anEdge = TopoDS::Edge(anEdgeIter.Value());

        Handle(Poly_PolygonOnTriangulation) aPolOnTri;
        Handle(Poly_Triangulation)          aTri;
        BRep_Tool::PolygonOnTriangulation(anEdge, aPolOnTri, aTri, aLoc);
        if (const Handle(Poly_Polygon3D)& aPol = BRep_Tool::Polygon3D(anEdge, aLoc)) {
            nbPolySegments += aPol->NbNodes() / gNbSegNodes2;
        } else if (!aPolOnTri.IsNull()) {
            nbPolySegments += aPolOnTri->NbNodes() / gNbSegNodes2;
        }
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(ShapeInfo)
{
    emscripten::value_object<ShapeInfo>("ShapeInfo")
        .field("shapeType", &ShapeInfo::shapeType)
        .field("index", &ShapeInfo::index)
        .field("nbChildren", &ShapeInfo::nbChildren)
        .field("orientation", &ShapeInfo::orientation)
        //
        .field("free", &ShapeInfo::free)
        .field("locked", &ShapeInfo::locked)
        .field("modified", &ShapeInfo::modified)
        .field("checked", &ShapeInfo::checked)
        .field("orientable", &ShapeInfo::orientable)
        .field("closed", &ShapeInfo::closed)
        .field("infinite", &ShapeInfo::infinite)
        .field("convex", &ShapeInfo::convex)
        //
        .field("surfaceType", &ShapeInfo::surfaceType)
        .field("curveType", &ShapeInfo::curveType)
        //
        .field("nbTriangles", &ShapeInfo::nbTriangles)
        .field("nbTriNodes", &ShapeInfo::nbTriNodes)
        .field("nbPolySegments", &ShapeInfo::nbPolySegments)
        //
        .field("point", &ShapeInfo::point)
        .field("color", &ShapeInfo::color)
        //
        .field("location", &ShapeInfo::location);
    emscripten::value_object<CylinderParams>("CylinderParams")
        .field("ax2", &CylinderParams::ax2)
        .field("radius", &CylinderParams::radius)
        .field("height", &CylinderParams::height)
        .field("angle", &CylinderParams::angle);
    emscripten::value_object<ConeParams>("ConeParams")
        .field("ax2", &ConeParams::ax2)
        .field("radius", &ConeParams::radius)
        .field("topRadius", &ConeParams::topRadius)
        .field("height", &ConeParams::height)
        .field("angle", &ConeParams::angle);
}
#endif

} // namespace

