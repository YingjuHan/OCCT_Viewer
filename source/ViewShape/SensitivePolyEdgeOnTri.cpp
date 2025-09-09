
#include <ViewShape/SensitivePolyEdgeOnTri.h>

#include <ViewShape/InteractiveShapeOwner.h>

#include <BRep_Tool.hxx>

namespace ViewShape
{

    // ================================================================
    // Function : GetEdgeSensitive
    // ================================================================
    Handle(SensitivePolyEdgeOnTri) SensitivePolyEdgeOnTri::GetEdgeSensitive(const TopoDS_Edge &theEdge,
                                                                            const Handle(SelectMgr_EntityOwner) & theOwnerId)
    {
        TopLoc_Location aLoc;
        Handle(Poly_Triangulation) aTris;
        Handle(Poly_PolygonOnTriangulation) aPolyOnTri = BRep_Tool::PolygonOnTriangulation(theEdge, aTris, aLoc);
        const int aNbSegs = !aPolyOnTri.IsNull() ? aPolyOnTri->NbNodes() : 0;
        if (aNbSegs < 2)
        {
            return Handle(SensitivePolyEdgeOnTri)();
        }

        Handle(SensitivePolyEdgeOnTri) aSens = new SensitivePolyEdgeOnTri(theOwnerId, theEdge, aPolyOnTri, aTris, aLoc);
        return aSens;
    }

    // ================================================================
    // Function : SensitivePolyEdgeOnTri
    // ================================================================
    SensitivePolyEdgeOnTri::SensitivePolyEdgeOnTri(const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                                   const TopoDS_Edge &theEdge,
                                                   const Handle(Poly_PolygonOnTriangulation) & thePolyOnTri,
                                                   const Handle(Poly_Triangulation) & theTrg,
                                                   const TopLoc_Location &theInitLoc)
        : Select3D_SensitiveSet(theOwnerId),
          myEdge(theEdge),
          myPolyOnTri(thePolyOnTri),
          myTriangul(theTrg),
          myInitLocation(theInitLoc),
          myInvInitLocation(theInitLoc.Transformation().Inverted())
    {
        mySFactor = 3;

        const int aNbSegs = !myPolyOnTri.IsNull() ? myPolyOnTri->NbNodes() / m_nbSegNodes2 : 0;
        if (aNbSegs < 1)
        {
            return;
        }

        if (myTriangul->HasGeometry())
        {
            gp_XYZ aCenter(0.0, 0.0, 0.0);
            for (int aNodeIter = 0; aNodeIter < myPolyOnTri->NbNodes(); ++aNodeIter)
            {
                const int aTriIndex = myPolyOnTri->Node(aNodeIter);
                Standard_ASSERT(aTriIndex >= 1 && aTriIndex <= myTriangul->NbNodes(),
                                "Invalid PolygonOnTriangulation",
                                return);

                const gp_Pnt aNode = myTriangul->Node(aTriIndex);
                aCenter += aNode.XYZ();
            }
            myCDG3D = gp_Pnt(aCenter / aNbSegs);

            myBVHPrimIndexes = new NCollection_Shared<TColStd_Array1OfInteger>(0, aNbSegs - 1);
            for (int aSegIdx = 1; aSegIdx <= aNbSegs; ++aSegIdx)
            {
                myBVHPrimIndexes->SetValue(aSegIdx - 1, aSegIdx - 1);
            }
        }
        else if (myTriangul->HasCachedMinMax())
        {
            myCDG3D = gp_Pnt(
                0.5 * (myTriangul->CachedMinMax().CornerMin().XYZ() + myTriangul->CachedMinMax().CornerMax().XYZ()));
        }

        computeBoundingBox();
    }

    // ================================================================
    // Function : SensitivePolyEdgeOnTri
    // ================================================================
    SensitivePolyEdgeOnTri::SensitivePolyEdgeOnTri(const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                                   const TopoDS_Edge &theEdge,
                                                   const Handle(Poly_PolygonOnTriangulation) & thePolyOnTri,
                                                   const Handle(Poly_Triangulation) & theTrg,
                                                   const TopLoc_Location &theInitLoc,
                                                   const gp_Pnt &theCOG)
        : Select3D_SensitiveSet(theOwnerId),
          myEdge(theEdge),
          myPolyOnTri(thePolyOnTri),
          myTriangul(theTrg),
          myInitLocation(theInitLoc),
          myInvInitLocation(theInitLoc.Transformation().Inverted()),
          myCDG3D(theCOG)
    {
        mySFactor = 3;

        const int aNbSegs = !myPolyOnTri.IsNull() ? myPolyOnTri->NbNodes() / m_nbSegNodes2 : 0;
        if (aNbSegs < 1 || !theTrg->HasGeometry())
        {
            return;
        }

        myBVHPrimIndexes = new NCollection_Shared<TColStd_Array1OfInteger>(0, aNbSegs - 1);
        for (int aSegIdx = 1; aSegIdx <= aNbSegs; ++aSegIdx)
        {
            myBVHPrimIndexes->SetValue(aSegIdx - 1, aSegIdx - 1);
        }
    }

    // ================================================================
    // Function : ~SensitivePolyEdgeOnTri
    // ================================================================
    SensitivePolyEdgeOnTri::~SensitivePolyEdgeOnTri()
    {
        //
    }

    // ================================================================
    // Function : Box
    // ================================================================
    Select3D_BndBox3d SensitivePolyEdgeOnTri::Box(const Standard_Integer theIdx) const
    {
        const int aSegIdx = myBVHPrimIndexes->Value(theIdx);

        const int aTriNodeIdx[m_nbSegNodes2] = {myPolyOnTri->Node(aSegIdx * m_nbSegNodes2),
                                                myPolyOnTri->Node(aSegIdx * m_nbSegNodes2 + 1)};

        const gp_Pnt aPnt1 = myTriangul->Node(aTriNodeIdx[0]);
        const gp_Pnt aPnt2 = myTriangul->Node(aTriNodeIdx[1]);

        SelectMgr_Vec3 aMinPnt(Min(aPnt1.X(), aPnt2.X()), Min(aPnt1.Y(), aPnt2.Y()), Min(aPnt1.Z(), aPnt2.Z()));
        SelectMgr_Vec3 aMaxPnt(Max(aPnt1.X(), aPnt2.X()), Max(aPnt1.Y(), aPnt2.Y()), Max(aPnt1.Z(), aPnt2.Z()));
        return Select3D_BndBox3d(aMinPnt, aMaxPnt);
    }

    // ================================================================
    // Function : Matches
    // ================================================================
    Standard_Boolean SensitivePolyEdgeOnTri::Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                                     SelectBasics_PickResult &thePickResult)
    {
        const InteractiveShapeOwner *anOwner = dynamic_cast<InteractiveShapeOwner *>(myOwnerId.get());
        if (anOwner != nullptr && anOwner->GetSelectionMode() != (int)InteractiveShapeSelMode::Global)
        {
            if (mySubshapeIndex != -1 && anOwner->GetDisabledElems().Contains(ShapeSelElemType::Edge, mySubshapeIndex))
            {
                return false;
            }
        }

        if (myTriangul->HasGeometry())
        {
            return Select3D_SensitiveSet::Matches(theMgr, thePickResult);
        }

        Select3D_BndBox3d aBox = BoundingBox();
        if (!aBox.IsValid())
        {
            return false;
        }

        if (!theMgr.IsOverlapAllowed())
        { // check for inclusion
            bool isInside = true;
            return theMgr.OverlapsBox(aBox.CornerMin(), aBox.CornerMax(), &isInside) && isInside;
        }
        if (!theMgr.OverlapsBox(aBox.CornerMin(), aBox.CornerMax(), thePickResult))
        { // check for overlap
            return false;
        }
        thePickResult.SetDistToGeomCenter(theMgr.DistToGeometryCenter(myCDG3D));
        return true;
    }

    // ================================================================
    // Function : Center
    // ================================================================
    Standard_Real SensitivePolyEdgeOnTri::Center(const Standard_Integer theIdx, const Standard_Integer theAxis) const
    {
        const Select3D_BndBox3d &aBox = Box(theIdx);
        const SelectMgr_Vec3 aCenter = (aBox.CornerMin() + aBox.CornerMax()) * 0.5;
        return aCenter[theAxis];
    }

    // ================================================================
    // Function : Swap
    // ================================================================
    void SensitivePolyEdgeOnTri::Swap(const Standard_Integer theIdx1, const Standard_Integer theIdx2)
    {
        const int anElemIdx1 = myBVHPrimIndexes->Value(theIdx1);
        const int anElemIdx2 = myBVHPrimIndexes->Value(theIdx2);

        myBVHPrimIndexes->ChangeValue(theIdx1) = anElemIdx2;
        myBVHPrimIndexes->ChangeValue(theIdx2) = anElemIdx1;
    }

    // ================================================================
    // Function : LastDetectedSegment
    // ================================================================
    bool SensitivePolyEdgeOnTri::LastDetectedSegment(Graphic3d_Vec2i &theSeg) const
    {
        const int anIndex = LastDetectedSegmentIndex();
        if (anIndex != -1)
        {
            theSeg.SetValues(myPolyOnTri->Node(anIndex * m_nbSegNodes2), myPolyOnTri->Node(anIndex * m_nbSegNodes2) + 1);
            return true;
        }
        return false;
    }

    // ================================================================
    // Function : LastDetectedSegment
    // ================================================================
    bool SensitivePolyEdgeOnTri::LastDetectedSegment(Graphic3d_Vec2i &theSeg, gp_Pnt theSegNodes[2]) const
    {
        if (!LastDetectedSegment(theSeg))
        {
            return false;
        }

        theSegNodes[0] = myTriangul->Node(theSeg[0]).Transformed(myInitLocation.Transformation());
        theSegNodes[1] = myTriangul->Node(theSeg[1]).Transformed(myInitLocation.Transformation());
        return true;
    }

    // ================================================================
    // Function : overlapsElement
    // ================================================================
    Standard_Boolean SensitivePolyEdgeOnTri::overlapsElement(SelectBasics_PickResult &thePickResult,
                                                             SelectBasics_SelectingVolumeManager &theMgr,
                                                             Standard_Integer theElemIdx,
                                                             Standard_Boolean theIsFullInside)
    {
        if (myBVHPrimIndexes.IsNull())
        {
            return Standard_False;
        }
        else if (theIsFullInside)
        {
            return Standard_True;
        }

        const int aSegIdx = myBVHPrimIndexes->Value(theElemIdx);

        const int aTriNodeIdx[m_nbSegNodes2] = {myPolyOnTri->Node(aSegIdx * m_nbSegNodes2),
                                                myPolyOnTri->Node(aSegIdx * m_nbSegNodes2 + 1)};

        const gp_Pnt aPnt1 = myTriangul->Node(aTriNodeIdx[0]);
        const gp_Pnt aPnt2 = myTriangul->Node(aTriNodeIdx[1]);
        return theMgr.OverlapsSegment(aPnt1, aPnt2, thePickResult);
    }

    // ================================================================
    // Function : elementIsInside
    // ================================================================
    Standard_Boolean SensitivePolyEdgeOnTri::elementIsInside(SelectBasics_SelectingVolumeManager &theMgr,
                                                             Standard_Integer theElemIdx,
                                                             Standard_Boolean theIsFullInside)
    {
        if (theIsFullInside)
        {
            return Standard_True;
        }

        const int aSegIdx = myBVHPrimIndexes->Value(theElemIdx);

        const int aTriNodeIdx[m_nbSegNodes2] = {myPolyOnTri->Node(aSegIdx * m_nbSegNodes2),
                                                myPolyOnTri->Node(aSegIdx * m_nbSegNodes2 + 1)};

        const gp_Pnt aPnt1 = myTriangul->Node(aTriNodeIdx[0]);
        const gp_Pnt aPnt2 = myTriangul->Node(aTriNodeIdx[1]);
        if (theMgr.GetActiveSelectionType() == SelectMgr_SelectionType_Polyline)
        {
            SelectBasics_PickResult aDummy;
            return theMgr.OverlapsSegment(aPnt1, aPnt2, aDummy);
        }
        return theMgr.OverlapsPoint(aPnt1) && theMgr.OverlapsPoint(aPnt2);
    }

    // ================================================================
    // Function : distanceToCOG
    // ================================================================
    Standard_Real SensitivePolyEdgeOnTri::distanceToCOG(SelectBasics_SelectingVolumeManager &theMgr)
    {
        return theMgr.DistToGeometryCenter(myCDG3D);
    }

    // ================================================================
    // Function : GetConnected
    // ================================================================
    Handle(Select3D_SensitiveEntity) SensitivePolyEdgeOnTri::GetConnected()
    {
        Handle(SensitivePolyEdgeOnTri) aNewEntity =
            new SensitivePolyEdgeOnTri(myOwnerId, myEdge, myPolyOnTri, myTriangul, myInitLocation, myCDG3D);
        aNewEntity->SetShapeIndex(mySubshapeIndex);
        return aNewEntity;
    }

    // ================================================================
    // Function : applyTransformation
    // ================================================================
    Select3D_BndBox3d SensitivePolyEdgeOnTri::applyTransformation()
    {
        if (!HasInitLocation())
        {
            return myBndBox;
        }

        Select3D_BndBox3d aBox;
        for (int aX = 0; aX <= 1; ++aX)
        {
            for (int aY = 0; aY <= 1; ++aY)
            {
                for (int aZ = 0; aZ <= 1; ++aZ)
                {
                    gp_Pnt aVertex = gp_Pnt(aX == 0 ? myBndBox.CornerMin().x() : myBndBox.CornerMax().x(),
                                            aY == 0 ? myBndBox.CornerMin().y() : myBndBox.CornerMax().y(),
                                            aZ == 0 ? myBndBox.CornerMin().z() : myBndBox.CornerMax().z());
                    aVertex.Transform(myInitLocation.Transformation());
                    aBox.Add(Select3D_Vec3(aVertex.X(), aVertex.Y(), aVertex.Z()));
                }
            }
        }

        return aBox;
    }

    // ================================================================
    // Function : BoundingBox
    // ================================================================
    Select3D_BndBox3d SensitivePolyEdgeOnTri::BoundingBox()
    {
        if (!myBndBox.IsValid())
        {
            computeBoundingBox();
        }
        return applyTransformation();
    }

    // ================================================================
    // Function : computeBoundingBox
    // ================================================================
    void SensitivePolyEdgeOnTri::computeBoundingBox()
    {
        myBndBox.Clear();

        if (myTriangul->HasCachedMinMax())
        {
            Bnd_Box aCachedBox = myTriangul->CachedMinMax();
            myBndBox.Add(
                SelectMgr_Vec3(aCachedBox.CornerMin().X(), aCachedBox.CornerMin().Y(), aCachedBox.CornerMin().Z()));
            myBndBox.Add(
                SelectMgr_Vec3(aCachedBox.CornerMax().X(), aCachedBox.CornerMax().Y(), aCachedBox.CornerMax().Z()));
            return;
        }
        else if (myTriangul->HasGeometry())
        {
            for (int aNodeIdx = 1; aNodeIdx <= myPolyOnTri->NbNodes(); ++aNodeIdx)
            {
                const gp_Pnt aNode = myTriangul->Node(myPolyOnTri->Node(aNodeIdx));
                myBndBox.Add(SelectMgr_Vec3(aNode.X(), aNode.Y(), aNode.Z()));
            }
        }
    }

} // namespace ViewShape