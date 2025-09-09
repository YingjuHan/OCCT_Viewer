#include <ViewBase/InteractiveClipping.h>

namespace ViewBase
{

    //! Return a tail of a chain.
    static Handle(Graphic3d_ClipPlane) GetChainTail(const Handle(Graphic3d_ClipPlane) & theChain)
    {
        Graphic3d_ClipPlane *aPlnIter = theChain.get();
        for (; !aPlnIter->ChainNextPlane().IsNull();)
        {
            aPlnIter = aPlnIter->ChainNextPlane().get();
        }
        return aPlnIter;
    }

    // ================================================================
    // Function : InteractiveClipping
    // ================================================================
    InteractiveClipping::InteractiveClipping(const Handle(ViewBase::InteractiveClippingOwner) & theNewId)
    {
        theNewId->SetSelectablePointer(this);
        myOwner = theNewId;

        myManagedPlanes = new Graphic3d_SequenceOfHClipPlane();
        UpdateClippingBox();
    }

    // ================================================================
    // Function : ~InteractiveClipping
    // ================================================================
    InteractiveClipping::~InteractiveClipping()
    {
        //
    }

    // ================================================================
    // Function : SetContext
    // ================================================================
    void InteractiveClipping::SetContext(const Handle(AIS_InteractiveContext) & theCtx)
    {
        if (theCtx.IsNull() && HasInteractiveContext())
        {
            myManagedPlanes->Clear();
        }

        AIS_ClippingPlanes::SetContext(theCtx);

        if (!theCtx.IsNull())
        {
            UpdateClippingBox();
        }
    }

    // ================================================================
    // Function : Compute
    // ================================================================
    void InteractiveClipping::Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                      const Handle(Prs3d_Presentation) & thePrs,
                                      const Standard_Integer theMode)
    {
        myViewBoxTest.SetVoid();
        if (mySceneBox.IsVoid())
        {
            myViewBoxTest = GetViewSceneBox();
        }

        AIS_ClippingPlanes::Compute(thePrsMgr, thePrs, theMode);
    }

    // ================================================================
    // Function : OnBeforeRedraw
    // ================================================================
    AIS_RedrawProgressResult InteractiveClipping::ProcessRedraw(const Handle(AIS_InteractiveContext) & theCtx,
                                                                const Handle(V3d_View) & theView,
                                                                const AIS_RedrawProgress theStage)
    {
        AIS_RedrawProgressResult toUpdate = AIS_InteractiveObject::ProcessRedraw(theCtx, theView, theStage);
        if (toUpdate == AIS_RedrawProgressResult_NeedRedisplay)
        {
            return toUpdate;
        }

        if (mySceneBox.IsVoid() && !myViewBoxTest.IsVoid() && theStage == AIS_RedrawProgress_BeforeRedraw)
        {
            const Bnd_Box aNewBox = GetViewSceneBox();
            if (aNewBox.IsVoid())
            {
                return AIS_RedrawProgressResult_None;
            }
            if (!aNewBox.CornerMin().IsEqual(myViewBoxTest.CornerMin(), Precision::Confusion()) || !aNewBox.CornerMax().IsEqual(myViewBoxTest.CornerMax(), Precision::Confusion()))
            {
                return AIS_RedrawProgressResult_NeedRedisplay;
            }
        }

        return AIS_RedrawProgressResult_None;
    }

    // ================================================================
    // Function : SetClippingOn
    // ================================================================
    void InteractiveClipping::SetClippingOn(bool theToClip)
    {
        if (myIsClipping == theToClip)
        {
            return;
        }

        myIsClipping = theToClip;
        UpdateAspects();
        SynchronizeAspects();
    }

    // ================================================================
    // Function : SetCappingOn
    // ================================================================
    void InteractiveClipping::SetCappingOn(bool theCapping)
    {
        if (myIsCapping == theCapping)
        {
            return;
        }

        myIsCapping = theCapping;
        UpdateAspects();
        SynchronizeAspects();
    }

    // ================================================================
    // Function : SetUseObjectMaterial
    // ================================================================
    void InteractiveClipping::SetUseObjectMaterial(bool theToUse)
    {
        if (myIsObjMat == theToUse)
        {
            return;
        }

        myIsObjMat = theToUse;
        UpdateAspects();
        SynchronizeAspects();
    }

    // ================================================================
    // Function : UpdateAspects
    // ================================================================
    void InteractiveClipping::UpdateAspects()
    {
        for (Graphic3d_SequenceOfHClipPlane::Iterator aPlaneIter(myManagedPlanes); aPlaneIter.More(); aPlaneIter.Next())
        {
            const Handle(Graphic3d_ClipPlane) &aPlane = aPlaneIter.Value();
            aPlane->SetOn(myIsClipping);
            aPlane->SetCapping(myIsCapping);
            aPlane->SetUseObjectMaterial(myIsObjMat);

            for (Graphic3d_ClipPlane *aSubIter = aPlane.get(); aSubIter != nullptr;
                 aSubIter = aSubIter->ChainNextPlane().get())
            {
                aSubIter->SetCapping(myIsCapping);
                aSubIter->SetUseObjectMaterial(myIsObjMat);
            }
        }
    }

    // ================================================================
    // Function : GetViewSceneBox
    // ================================================================
    Bnd_Box InteractiveClipping::GetViewSceneBox() const
    {
        if (!mySceneBox.IsVoid())
        {
            return mySceneBox;
        }

        Bnd_Box aBox;
        if (!HasInteractiveContext())
        {
            aBox.Add(gp::Origin());
            return aBox;
        }

        for (Graphic3d_IndexedMapOfView::Iterator aViewIter(
                 InteractiveContext()->MainPrsMgr()->StructureManager()->DefinedViews());
             aViewIter.More();
             aViewIter.Next())
        {
            const Graphic3d_CView *aView = aViewIter.Value();
            if (myViewAffinity->IsVisible(aView->Identification()))
            {
                aBox = aView->MinMaxValues(false);
                break;
            }
        }
        if (aBox.IsVoid())
        {
            aBox.Add(gp::Origin());
        }
        return aBox;
    }

    // ================================================================
    // Function : RangeToRelative
    // ================================================================
    gp_XYZ InteractiveClipping::RangeToRelative(const gp_XYZ &theVec) const
    {
        Bnd_Box aViewBox = GetViewSceneBox();
        if (aViewBox.IsVoid())
        {
            aViewBox.Add(gp::Origin());
        }
        const gp_XYZ aRange = aViewBox.CornerMax().XYZ() - aViewBox.CornerMin().XYZ();

        gp_XYZ aPnt = theVec;
        for (int aCompIter = 1; aCompIter <= Graphic3d_Vec3d::Length(); ++aCompIter)
        {
            if (!Precision::IsInfinite(aPnt.Coord(aCompIter)) && Abs(aRange.Coord(aCompIter)) > Precision::Confusion())
            {
                aPnt.SetCoord(aCompIter, aPnt.Coord(aCompIter) / aRange.Coord(aCompIter));
            }
        }
        return aPnt;
    }

    // ================================================================
    // Function : RangeFromRelative
    // ================================================================
    gp_XYZ InteractiveClipping::RangeFromRelative(const gp_XYZ &theVec) const
    {
        Bnd_Box aViewBox = GetViewSceneBox();
        if (aViewBox.IsVoid())
        {
            aViewBox.Add(gp::Origin());
        }
        const gp_XYZ aRange = aViewBox.CornerMax().XYZ() - aViewBox.CornerMin().XYZ();

        gp_XYZ aPnt = theVec;
        for (int aCompIter = 1; aCompIter <= Graphic3d_Vec3d::Length(); ++aCompIter)
        {
            if (!Precision::IsInfinite(aPnt.Coord(aCompIter)) && Abs(aRange.Coord(aCompIter)) > Precision::Confusion())
            {
                aPnt.SetCoord(aCompIter, aPnt.Coord(aCompIter) * aRange.Coord(aCompIter));
            }
        }
        return aPnt;
    }

    // ================================================================
    // Function : PntToRelative
    // ================================================================
    gp_Pnt InteractiveClipping::PntToRelative(const gp_Pnt &thePnt) const
    {
        Bnd_Box aViewBox = GetViewSceneBox();
        if (aViewBox.IsVoid())
        {
            aViewBox.Add(gp::Origin());
        }
        const gp_XYZ aMin = aViewBox.CornerMin().XYZ();
        const gp_XYZ aMax = aViewBox.CornerMax().XYZ();
        const gp_XYZ aRange = aMax - aMin;

        gp_Pnt aPnt = thePnt;
        for (int aCompIter = 1; aCompIter <= Graphic3d_Vec3d::Length(); ++aCompIter)
        {
            if (Precision::IsInfinite(aPnt.Coord(aCompIter)))
            {
                continue;
            }

            aPnt.SetCoord(aCompIter, aPnt.Coord(aCompIter) - aMin.Coord(aCompIter));
            if (Abs(aRange.Coord(aCompIter)) > Precision::Confusion())
            {
                aPnt.SetCoord(aCompIter, aPnt.Coord(aCompIter) / aRange.Coord(aCompIter));
            }
        }
        return aPnt;
    }

    // ================================================================
    // Function : PntFromRelative
    // ================================================================
    gp_Pnt InteractiveClipping::PntFromRelative(const gp_Pnt &thePnt) const
    {
        Bnd_Box aViewBox = GetViewSceneBox();
        if (aViewBox.IsVoid())
        {
            aViewBox.Add(gp::Origin());
        }
        const gp_XYZ aMax = aViewBox.CornerMax().XYZ();
        const gp_XYZ aMin = aViewBox.CornerMin().XYZ();
        const gp_XYZ aRange = aMax - aMin;

        gp_Pnt aPnt = thePnt;
        for (int aCompIter = 1; aCompIter <= Graphic3d_Vec3d::Length(); ++aCompIter)
        {
            if (Precision::IsInfinite(aPnt.Coord(aCompIter)))
            {
                continue;
            }

            if (Abs(aRange.Coord(aCompIter)) > Precision::Confusion())
            {
                aPnt.SetCoord(aCompIter, aPnt.Coord(aCompIter) * aRange.Coord(aCompIter));
            }
            aPnt.SetCoord(aCompIter, aPnt.Coord(aCompIter) + aMin.Coord(aCompIter));
        }
        return aPnt;
    }

    // ================================================================
    // Function : GetClippingCorner
    // ================================================================
    gp_Pnt InteractiveClipping::GetClippingCorner(bool theIsRel) const
    {
        return !theIsRel ? myClipCorner : PntToRelative(myClipCorner);
    }

    // ================================================================
    // Function : SetClippingCorner
    // ================================================================
    void InteractiveClipping::SetClippingCorner(const gp_Pnt &thePnt, bool theIsRel)
    {
        const gp_Pnt aBack = myClipCorner;

        myClipCorner = !theIsRel ? thePnt : PntFromRelative(thePnt);
        if (Precision::IsInfinite(myClipCorner.X()))
        {
            myClipCorner.SetX(aBack.X());
        }
        if (Precision::IsInfinite(myClipCorner.Y()))
        {
            myClipCorner.SetY(aBack.Y());
        }
        if (Precision::IsInfinite(myClipCorner.Z()))
        {
            myClipCorner.SetZ(aBack.Z());
        }
        UpdateClippingBox();
    }

    // ================================================================
    // Function : GetClippingExtent
    // ================================================================
    gp_XYZ InteractiveClipping::GetClippingExtent(bool theIsRel) const
    {
        return !theIsRel ? myClipExtent : RangeToRelative(myClipExtent);
    }

    // ================================================================
    // Function : SetClippingExtent
    // ================================================================
    void InteractiveClipping::SetClippingExtent(const gp_XYZ &theExt, bool theIsRel)
    {
        myClipExtent = !theIsRel ? theExt : RangeFromRelative(theExt);
        UpdateClippingBox();
    }

    // ================================================================
    // Function : GetNbPlanesXYZ
    // ================================================================
    Graphic3d_Vec3i InteractiveClipping::GetNbPlanesXYZ() const
    {
        static constexpr int nbPlanes2 = 2;

        Graphic3d_Vec3i aVec3(0);

        const NCollection_Vec3<bool> isVoidXYZ = IsVoidXYZ();
        const NCollection_Vec3<bool> isInfiniteXYZ = IsInfiniteXYZ();
        for (int aCompIter = 0; aCompIter < Graphic3d_Vec3d::Length(); ++aCompIter)
        {
            if (isInfiniteXYZ[aCompIter])
            {
                aVec3[aCompIter] = myClipExtent.Coord(aCompIter + 1) >= 0.0 ? 1 : -1;
            }
            else if (!isVoidXYZ[aCompIter])
            {
                aVec3[aCompIter] = myClipExtent.Coord(aCompIter + 1) >= 0.0 ? nbPlanes2 : -nbPlanes2;
            }
        }
        return aVec3;
    }

    // ================================================================
    // Function : SetNbPlanesXYZ
    // ================================================================
    void InteractiveClipping::SetNbPlanesXYZ(const Graphic3d_Vec3i &theVec)
    {
        static constexpr int nbPlanes2 = 2;

        const Graphic3d_Vec3i anOld = GetNbPlanesXYZ();
        if (theVec == anOld)
        {
            return;
        }
        else if (theVec.cwiseAbs().maxComp() > nbPlanes2)
        {
            throw Standard_OutOfRange("InteractiveClipping::SetNbPlanesXYZ(), out of range");
        }

        const Bnd_Box aViewBox = GetViewSceneBox();

        gp_Pnt aNewCorner = myClipCorner;
        gp_XYZ aNewExtent = myClipExtent;
        gp_XYZ aNewFinExt = myClipFinExt;
        for (int aCompIter = 0; aCompIter < Graphic3d_Vec3d::Length(); ++aCompIter)
        {
            const double anOldExt = myClipExtent.Coord(aCompIter + 1);
            if (theVec[aCompIter] == anOld[aCompIter])
            {
                continue;
            }
            else if (Abs(theVec[aCompIter]) < nbPlanes2 && !Precision::IsInfinite(anOldExt) && Abs(anOldExt) > gp::Resolution())
            {
                // backup finite extent
                aNewFinExt.SetCoord(aCompIter + 1, anOldExt);
            }

            calculateDimExtent(theVec, aNewExtent, aCompIter, aViewBox);
        }

        myClipCorner = aNewCorner;
        myClipFinExt = aNewFinExt;
        myClipExtent = aNewExtent;
        UpdateClippingBox();
    }

    // ================================================================
    // Function : calculateDimExtent
    // ================================================================
    void InteractiveClipping::calculateDimExtent(const Graphic3d_Vec3i &theNbPlanesVec,
                                                 gp_XYZ &theExtent,
                                                 int theComp,
                                                 const Bnd_Box &theViewBox) const
    {
        static constexpr int nbPlanes2 = 2;
        static constexpr double sceneBoxGap = 0.01;

        switch (Abs(theNbPlanesVec[theComp]))
        {
        case 0:
            theExtent.SetCoord(theComp + 1, 0.0);
            break;
        case 1:
            theExtent.SetCoord(theComp + 1, theNbPlanesVec[theComp] > 0 ? RealLast() : -RealLast());
            break;
        case nbPlanes2:
        {
            const double anOldExt = myClipExtent.Coord(theComp + 1);
            double anExt = Abs(anOldExt);
            if (Precision::IsInfinite(anOldExt) || anOldExt <= gp::Resolution())
            {
                anExt = Abs(myClipFinExt.Coord(theComp + 1));
            }
            if ((Precision::IsInfinite(anExt) || anExt <= gp::Resolution()) && !theViewBox.IsVoid())
            {
                double aCorn = myClipCorner.Coord(theComp + 1);
                double aMin = theViewBox.CornerMin().Coord(theComp + 1);
                double aMax = theViewBox.CornerMax().Coord(theComp + 1);
                double aRange = aMax - aMin;
                // extra gap to avoid clipping artifacts at corners
                aMin += aRange * sceneBoxGap;
                aMax += aRange * sceneBoxGap;

                anExt = theNbPlanesVec[theComp] > 0 ? Max(aMax - aCorn, 0.0) : Max(aCorn - aMin, 0.0);
            }
            if (Precision::IsInfinite(anExt) || anExt <= gp::Resolution())
            {
                anExt = Precision::Confusion();
            }

            theExtent.SetCoord(theComp + 1, theNbPlanesVec[theComp] > 0 ? anExt : -anExt);
            break;
        }
        default:
            break;
        }
    }

    // ================================================================
    // Function : UpdateClippingBox
    // ================================================================
    void InteractiveClipping::UpdateClippingBox()
    {
        SetToUpdate();
        myManagedPlanes->Clear();

        const NCollection_Vec3<bool> isVoidXYZ = IsVoidXYZ();
        if (isVoidXYZ.x() && isVoidXYZ.y() && isVoidXYZ.z())
        {
            return;
        }

        auto appendClipPlane = [&](const Handle(Graphic3d_ClipPlane) & theClipPlane)
        {
            if (myToFlip || myManagedPlanes->IsEmpty())
            {
                myManagedPlanes->Append(theClipPlane);
            }
            else
            {
                GetChainTail(myManagedPlanes->First())->SetChainNextPlane(theClipPlane);
            }
        };

        const gp_Dir aDirs[3] = {!myToFlip ? gp::DX() : -gp::DX(),
                                 !myToFlip ? gp::DY() : -gp::DY(),
                                 !myToFlip ? gp::DZ() : -gp::DZ()};

        const NCollection_Vec3<bool> isInfiniteXYZ = IsInfiniteXYZ();
        for (int aComp = 0; aComp < Graphic3d_Vec3d::Length(); ++aComp)
        {
            if (isVoidXYZ[aComp])
            {
                continue;
            }

            const gp_Dir &aDir = aDirs[aComp];
            gp_Pnt anOrig = gp::Origin();
            anOrig.SetCoord(aComp + 1, myClipCorner.Coord(aComp + 1));
            if (isInfiniteXYZ[aComp])
            {
                gp_Pln aPln(anOrig, myClipExtent.Coord(aComp + 1) >= 0.0 ? -aDir : aDir);
                Handle(Graphic3d_ClipPlane) aClipPln = new Graphic3d_ClipPlane(aPln);
                appendClipPlane(aClipPln);
            }
            else
            {
                double aMin = myClipCorner.Coord(aComp + 1);
                double aMax = myClipExtent.Coord(aComp + 1) + aMin;
                if (aMin > aMax)
                {
                    std::swap(aMin, aMax);
                }

                anOrig.SetCoord(aComp + 1, aMin);
                gp_Pln aPln1(anOrig, -aDir);

                Handle(Graphic3d_ClipPlane) aClipPln1 = new Graphic3d_ClipPlane(aPln1);
                appendClipPlane(aClipPln1);

                anOrig.SetCoord(aComp + 1, aMax);
                gp_Pln aPln2(anOrig, aDir);

                Handle(Graphic3d_ClipPlane) aClipPln2 = new Graphic3d_ClipPlane(aPln2);
                appendClipPlane(aClipPln2);
            }
        }

        UpdateAspects();
    }

    // ================================================================
    // Function : GetClippingBox
    // ================================================================
    Graphic3d_BndBox3d InteractiveClipping::GetClippingBox(bool theIsRel) const
    {
        gp_XYZ aMin;
        gp_XYZ aMax;

        const NCollection_Vec3<bool> isVoidXYZ = IsVoidXYZ();
        for (int aComp = 1; aComp <= Graphic3d_Vec3d::Length(); ++aComp)
        {
            if (isVoidXYZ[aComp - 1])
            {
                aMin.SetCoord(aComp, myClipCorner.Coord(aComp));
                aMax.SetCoord(aComp, -RealLast());
            }
            else if (Precision::IsNegativeInfinite(myClipExtent.Coord(aComp)))
            {
                aMin.SetCoord(aComp, -RealLast());
                aMax.SetCoord(aComp, myClipCorner.Coord(aComp));
            }
            else if (Precision::IsPositiveInfinite(myClipExtent.Coord(aComp)))
            {
                aMin.SetCoord(aComp, myClipCorner.Coord(aComp));
                aMax.SetCoord(aComp, RealLast());
            }
            else if (myClipExtent.Coord(aComp) < 0.0)
            {
                aMin.SetCoord(aComp, myClipCorner.Coord(aComp) + myClipExtent.Coord(aComp));
                aMax.SetCoord(aComp, myClipCorner.Coord(aComp));
            }
            else
            {
                aMin.SetCoord(aComp, myClipCorner.Coord(aComp));
                aMax.SetCoord(aComp, myClipCorner.Coord(aComp) + myClipExtent.Coord(aComp));
            }
        }

        if (theIsRel)
        {
            aMin = PntToRelative(aMin).XYZ();
            aMax = PntToRelative(aMax).XYZ();
        }

        Graphic3d_BndBox3d aCfg(Graphic3d_Vec3d(aMin.X(), aMin.Y(), aMin.Z()),
                                Graphic3d_Vec3d(aMax.X(), aMax.Y(), aMax.Z()));
        return aCfg;
    }

    // ================================================================
    // Function : SetClippingBox
    // ================================================================
    void InteractiveClipping::SetClippingBox(const Graphic3d_BndBox3d &theBox, bool theIsRel)
    {
        gp_XYZ aMin(theBox.CornerMin().x(), theBox.CornerMin().y(), theBox.CornerMin().z());
        gp_XYZ aMax(theBox.CornerMax().x(), theBox.CornerMax().y(), theBox.CornerMax().z());
        if (theIsRel)
        {
            aMin = PntFromRelative(aMin).XYZ();
            aMax = PntFromRelative(aMax).XYZ();
        }

        gp_Pnt aNewCorn;
        gp_XYZ aNewExt;
        gp_XYZ aNewFinExt = myClipFinExt;
        for (int aComp = 1; aComp <= Graphic3d_Vec3d::Length(); ++aComp)
        {
            const double aCMin = aMin.Coord(aComp);
            const double aCMax = aMax.Coord(aComp);
            if (Precision::IsNegativeInfinite(aCMin))
            {
                if (!Precision::IsInfinite(aCMax))
                {
                    aNewCorn.SetCoord(aComp, aCMax);
                    aNewExt.SetCoord(aComp, -RealLast());
                }
                else
                {
                    aNewExt.SetCoord(aComp, 0.0);
                }
            }
            else if (Precision::IsPositiveInfinite(aCMax))
            {
                if (!Precision::IsInfinite(aCMin))
                {
                    aNewCorn.SetCoord(aComp, aCMin);
                    aNewExt.SetCoord(aComp, RealLast());
                }
                else
                {
                    aNewExt.SetCoord(aComp, 0.0);
                }
            }
            else if (!Precision::IsInfinite(aCMin) && !Precision::IsInfinite(aCMax) && aCMin <= aCMax)
            {
                aNewCorn.SetCoord(aComp, aCMin);
                aNewExt.SetCoord(aComp, aCMax - aCMin);
            }
            else
            {
                aNewCorn.SetCoord(aComp, 0.0);
                aNewExt.SetCoord(aComp, 0.0);
            }

            const double anOldExt = myClipExtent.Coord(aComp);
            const double aNewCExt = aNewExt.Coord(aComp);
            if ((Precision::IsInfinite(aNewCExt) || aNewCExt <= gp::Resolution()) && !Precision::IsInfinite(anOldExt) && Abs(anOldExt) > gp::Resolution())
            {
                // backup finite extent
                aNewFinExt.SetCoord(aComp, anOldExt);
            }
        }

        bool toUpdate = !aNewCorn.IsEqual(myClipCorner, gp::Resolution()) || !aNewExt.IsEqual(myClipExtent, gp::Resolution());

        myClipCorner = aNewCorn;
        myClipExtent = aNewExt;
        myClipFinExt = aNewFinExt;
        if (toUpdate)
        {
            UpdateClippingBox();
        }
    }

    // ================================================================
    // Function : FindDraggedMinMax
    // ================================================================
    void InteractiveClipping::FindDraggedMinMax(gp_Pnt &theNewMin, gp_Pnt &theNewMax)
    {
        const gp_Dir aDirs[3] = {!myToFlip ? gp::DX() : -gp::DX(),
                                 !myToFlip ? gp::DY() : -gp::DY(),
                                 !myToFlip ? gp::DZ() : -gp::DZ()};

        const NCollection_Vec3<bool> isVoidXYZ = IsVoidXYZ();
        const NCollection_Vec3<bool> isInfiniteXYZ = IsInfiniteXYZ();

        auto findMinMax = [&](const gp_Pln &theGPln, const int theComp)
        {
            const gp_Dir &aDir = aDirs[theComp];
            if (isVoidXYZ[theComp] || !theGPln.Axis().Direction().IsParallel(aDir, Precision::Angular()))
            {
                return;
            }

            const double aCPos = theGPln.Location().Coord(theComp + 1);
            if (isInfiniteXYZ[theComp])
            {
                theNewMin.SetCoord(theComp + 1, aCPos);
            }
            else if (theGPln.Axis().Direction().IsOpposite(aDir, Precision::Angular()))
            {
                theNewMin.SetCoord(theComp + 1, aCPos);
            }
            else
            {
                theNewMax.SetCoord(theComp + 1, aCPos);
            }
        };

        for (Graphic3d_SequenceOfHClipPlane::Iterator aPlaneIter(myManagedPlanes); aPlaneIter.More(); aPlaneIter.Next())
        {
            const Handle(Graphic3d_ClipPlane) &aPlane = aPlaneIter.Value();

            for (Graphic3d_ClipPlane *aSubIter = aPlane.get(); aSubIter != nullptr;
                 aSubIter = aSubIter->ChainNextPlane().get())
            {
                const gp_Pln &aGPln = aSubIter->ToPlane();
                for (int aComp = 0; aComp < Graphic3d_Vec3d::Length(); ++aComp)
                {
                    findMinMax(aGPln, aComp);
                }
            }
        }
    }

    // ================================================================
    // Function : SetDraggedMinMax
    // ================================================================
    void InteractiveClipping::SetDraggedMinMax(const gp_Pnt &theNewMin, const gp_Pnt &theNewMax)
    {
        const NCollection_Vec3<bool> isVoidXYZ = IsVoidXYZ();
        const NCollection_Vec3<bool> isInfiniteXYZ = IsInfiniteXYZ();
        const NCollection_Vec3<bool> isPosExtXYZ(myClipExtent.X() >= 0.0, myClipExtent.Y() >= 0.0, myClipExtent.Z() >= 0.0);
        for (int aComp = 0; aComp < Graphic3d_Vec3d::Length(); ++aComp)
        {
            if (isVoidXYZ[aComp])
            {
                continue;
            }

            const double aMin = theNewMin.Coord(aComp + 1);
            const double aMax = theNewMax.Coord(aComp + 1);
            if (isInfiniteXYZ[aComp])
            {
                myClipCorner.SetCoord(aComp + 1, aMin);
                continue;
            }

            if (myClipExtent.Coord(aComp + 1) >= 0.0)
            {
                myClipCorner.SetCoord(aComp + 1, aMin);
                myClipExtent.SetCoord(aComp + 1, aMax - aMin);
            }
            else
            {
                myClipCorner.SetCoord(aComp + 1, aMax);
                myClipExtent.SetCoord(aComp + 1, aMin - aMax);
            }
        }
    }

    // ================================================================
    // Function : ProcessDragging
    // ================================================================
    Standard_Boolean InteractiveClipping::ProcessDragging(const Handle(AIS_InteractiveContext) & theCtx,
                                                          const Handle(V3d_View) & theView,
                                                          const Handle(SelectMgr_EntityOwner) & theOwner,
                                                          const Graphic3d_Vec2i &theDragFrom,
                                                          const Graphic3d_Vec2i &theDragTo,
                                                          const AIS_DragAction theAction)
    {
        bool aRes = AIS_ClippingPlanes::ProcessDragging(theCtx, theView, theOwner, theDragFrom, theDragTo, theAction);

        // synchronize box configuration
        gp_Pnt aNewMin;
        gp_Pnt aNewMax;
        FindDraggedMinMax(aNewMin, aNewMax);
        SetDraggedMinMax(aNewMin, aNewMax);
        return aRes;
    }

}