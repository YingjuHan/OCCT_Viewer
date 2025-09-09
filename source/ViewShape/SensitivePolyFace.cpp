#include <ViewShape/SensitivePolyFace.h>

#include <ViewShape/InteractiveShapeOwner.h>

#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <Graphic3d_Vec2.hxx>
#include <Graphic3d_Vec3.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

namespace ViewShape
{

    // ================================================================
    // Function : SensitivePolyFace
    // ================================================================
    SensitivePolyFace::SensitivePolyFace(const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                         const TopoDS_Face &theFace,
                                         const Handle(Poly_Triangulation) & theTrg,
                                         const TopLoc_Location &theInitLoc)
        : Select3D_SensitiveTriangulation(theOwnerId, theTrg, theInitLoc),
          myFace(theFace)
    {
        mySFactor = 0;
    }

    // ================================================================
    // Function : ~SensitivePolyFace
    // ================================================================
    SensitivePolyFace::~SensitivePolyFace()
    {
        //
    }

    //! Find closest edge using extrema.
    static TopoDS_Edge findClosestSubEdgeExtrema(const TopoDS_Shape &theShape,
                                                 const gp_Pnt &thePickPoint,
                                                 const double thePickTol)
    {
        TopoDS_Edge anEdge;
        double aMinDist = Precision::Infinite();
        for (TopExp_Explorer anEdgeIter(theShape, TopAbs_EDGE); anEdgeIter.More(); anEdgeIter.Next())
        {
            const TopoDS_Edge &aCurEdge = TopoDS::Edge(anEdgeIter.Value());
            BRepExtrema_DistShapeShape aDist(BRepBuilderAPI_MakeVertex(thePickPoint), aCurEdge, Precision::Confusion());
            if (aDist.Value() < thePickTol && aDist.Value() < aMinDist)
            {
                aMinDist = aDist.Value();
                anEdge = aCurEdge;
            }
        }
        return anEdge;
    }

    //! Find nodes shared between segment and triangle.
    static int findSameNodesSegmentInTriangle(const Graphic3d_Vec3i &theTri, const Graphic3d_Vec2i &theSeg)
    {
        if (theSeg[0] == theSeg[1])
        {
            return 0;
        }

        int aNbSamePnts = 0;
        for (int aVertIter = 0; aVertIter < Graphic3d_Vec3i::Length() && aNbSamePnts < Graphic3d_Vec2i::Length();
             ++aVertIter)
        {
            if (theTri[aVertIter] == theSeg[0] || theTri[aVertIter] == theSeg[1])
            {
                ++aNbSamePnts;
            }
        }
        return aNbSamePnts;
    }

    //! Calculate distance from point to segment.
    static double calculatePntToSegDist(const gp_Pnt &thePoint, const gp_Pnt &theSegPnt1, const gp_Pnt &theSegPnt2)
    {
        double aDist = Min(thePoint.Distance(theSegPnt1), thePoint.Distance(theSegPnt2));
        gp_XYZ aDir = theSegPnt2.XYZ() - theSegPnt1.XYZ();

        const gp_XYZ aVecToPnt = thePoint.XYZ() - theSegPnt1.XYZ();
        const double aSegLen = aDir.Modulus();
        if (aSegLen > 0.0)
        {
            aDir /= aSegLen;
            const double aProj = aVecToPnt.Dot(aDir);
            if (aProj > 0.0 && aProj < aSegLen)
            {
                aDist = thePoint.Distance(theSegPnt1.XYZ() + aDir * aProj);
            }
        }
        return aDist;
    }

    //! Find closest edge using from detected triangle.
    static TopoDS_Edge findClosestSubEdgeTriangle(const TopoDS_Shape &theDetShape,
                                                  const gp_Pnt &thePickPoint,
                                                  const double thePickTol,
                                                  const Handle(Poly_Triangulation) & theTris,
                                                  const Graphic3d_Vec3i &theDetTri)
    {
        TopoDS_Edge anEdge;
        double aMinDist = Precision::Infinite();

        auto updateMinDist = [&anEdge, &aMinDist, thePickTol](const TopoDS_Edge &theCurEdge, double theCurMinDist)
        {
            if (theCurMinDist < thePickTol && theCurMinDist < aMinDist)
            {
                aMinDist = theCurMinDist;
                anEdge = theCurEdge;
            }
        };

        for (TopExp_Explorer anEdgeIter(theDetShape, TopAbs_EDGE); anEdgeIter.More(); anEdgeIter.Next())
        {
            const TopoDS_Edge &aCurEdge = TopoDS::Edge(anEdgeIter.Value());
            const Handle(Poly_PolygonOnTriangulation) &aPolyOnTri = BRep_Tool::PolygonOnTriangulation(aCurEdge,
                                                                                                      theTris,
                                                                                                      aCurEdge.Location());
            if (aPolyOnTri.IsNull())
            {
                continue;
            }

            const TopLoc_Location &aTrsf = aCurEdge.Location();
            for (int aNodeIter = 2; aNodeIter <= aPolyOnTri->NbNodes(); ++aNodeIter)
            {
                const Graphic3d_Vec2i aCurNodes(aPolyOnTri->Node(aNodeIter - 1), aPolyOnTri->Node(aNodeIter));
                const int aNbSamePnts = findSameNodesSegmentInTriangle(theDetTri, aCurNodes);

                // detect edge if 2 vertices of the triangle belong to the polygon in the triangulation
                if (aNbSamePnts == 2)
                {
                    const gp_Pnt aSegPoints[2] = {theTris->Node(aCurNodes[0]).Transformed(aTrsf),
                                                  theTris->Node(aCurNodes[1]).Transformed(aTrsf)};

                    // @c BRepExtrema_DistShapeShape anExtrema (BRepBuilderAPI_MakeVertex(thePickPoint),
                    // @c   aCurEdge, Precision::Confusion());
                    // @c double aDist = anExtrema.Value();
                    double aDist = calculatePntToSegDist(thePickPoint, aSegPoints[0], aSegPoints[1]);
                    updateMinDist(aCurEdge, aDist);
                    break;
                }
                else if (aNbSamePnts == 1)
                {
                    Standard_ASSERT(aCurNodes[0] >= 1 && aCurNodes[0] <= theTris->NbNodes(),
                                    "Invalid PolygonOnTriangulation",
                                    break);

                    const gp_Pnt aSegPnt = theTris->Node(aCurNodes[0]).Transformed(aTrsf);
                    const double aDist = thePickPoint.Distance(aSegPnt);
                    updateMinDist(aCurEdge, aDist);
                }
            }
        }
        return anEdge;
    }

    // ================================================================
    // Function : DetectClosestEdge
    // ================================================================
    bool SensitivePolyFace::DetectClosestEdge(const TopoDS_Shape &theDetShape,
                                              const gp_Pnt &thePickPoint,
                                              const double thePickTol,
                                              TopoDS_Edge &theResEdge) const
    {
        if (theDetShape.IsNull())
        {
            return false;
        }

        TopoDS_Edge anEdge;
        if (theDetShape.ShapeType() != TopAbs_WIRE)
        {
            Poly_Triangle aDetTriang;
            if (!LastDetectedTriangle(aDetTriang))
            {
                return false;
            }

            Graphic3d_Vec3i aDetTriElem(-1);
            aDetTriang.Get(aDetTriElem.x(), aDetTriElem.y(), aDetTriElem.z());

            anEdge = findClosestSubEdgeTriangle(theDetShape, thePickPoint, thePickTol, Triangulation(), aDetTriElem);
        }

        if (anEdge.IsNull())
        {
            // Picking a narrow triangle might prevent detecting boundary segment.
            // So fallback to a slow brute-force iteration.
            // Another issue could be picking a triangle in the middle of a Face,
            // close to an Edge on another Face - which this iteration will lost.
            anEdge = findClosestSubEdgeExtrema(theDetShape, thePickPoint, thePickTol);
        }

        if (!anEdge.IsNull())
        {
            theResEdge = anEdge;
            return true;
        }
        return false;
    }

    // ================================================================
    // Function : Matches
    // ================================================================
    Standard_Boolean SensitivePolyFace::Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                                SelectBasics_PickResult &thePickResult)
    {
        const InteractiveShapeOwner *anOwner = dynamic_cast<InteractiveShapeOwner *>(myOwnerId.get());
        if (anOwner != nullptr && anOwner->GetSelectionMode() != (int)InteractiveShapeSelMode::Global)
        {
            if (mySubshapeIndex != -1 && anOwner->GetDisabledElems().Contains(ShapeSelElemType::Face, mySubshapeIndex))
            {
                return false;
            }
        }

        return Select3D_SensitiveTriangulation::Matches(theMgr, thePickResult);
    }

} // namespace ViewShape