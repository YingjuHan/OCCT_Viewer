
#include <ViewShape/SensitiveCompound.h>

#include <ViewShape/InteractiveShapeOwner.h>

#include <BRep_Tool.hxx>
#include <SelectMgr_SelectingVolumeManager.hxx>
#include <StdSelect_BRepSelectionTool.hxx>

namespace ViewShape
{

    // ================================================================
    // Function : SensitiveCompound
    // ================================================================
    SensitiveCompound::SensitiveCompound(const TopoDS_Shape &theShape,
                                         const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                         const std::vector<Handle(Select3D_SensitiveEntity)> &theEntities)
        : Select3D_SensitiveSet(theOwnerId),
          myShape(theShape),
          myEntities(theEntities),
          myEntityIndexes(theEntities.size())
    {
        for (size_t anEntityIter = 0; anEntityIter < myEntities.size(); ++anEntityIter)
        {
            const Handle(Select3D_SensitiveEntity) &anEntity = myEntities[anEntityIter];
            Select3D_BndBox3d aBndBox = anEntity->BoundingBox();
            myBndBox.Combine(aBndBox);
            myCenter.ChangeCoord() += anEntity->CenterOfGeometry().XYZ();
            myEntityIndexes[anEntityIter] = static_cast<int>(anEntityIter);
            mySFactor = Max(anEntity->SensitivityFactor(), mySFactor);
        }
        if (!myEntities.empty())
        {
            myCenter = myCenter.XYZ() / double(myEntities.size());
        }
    }

    // ================================================================
    // Function : ~SensitiveCompound
    // ================================================================
    SensitiveCompound::~SensitiveCompound()
    {
        //
    }

    // =======================================================================
    // Function : Center
    // =======================================================================
    Standard_Real SensitiveCompound::Center(const Standard_Integer theIdx, const Standard_Integer theAxis) const
    {
        const int aSensIdx = myEntityIndexes[theIdx];
        const gp_Pnt &aCenter = myEntities[aSensIdx]->CenterOfGeometry();
        return theAxis == 0 ? aCenter.X() : (theAxis == 1 ? aCenter.Y() : aCenter.Z());
    }

    // =======================================================================
    // Function : isActiveShapeType
    // =======================================================================
    bool SensitiveCompound::isActiveShapeType(int theSelMode) const
    {
        if (theSelMode == (int)InteractiveShapeSelMode::Global)
        {
            return true;
        }
        if (myShape.IsNull())
        {
            return true;
        }

        const ShapeSelElemType aSelType = ShapeSelElemTypeFromTopAbs(myShape.ShapeType());
        const InteractiveShapeSelMode aSelMode = ShapeElemTypeToSelMode(aSelType);
        return ((unsigned int)theSelMode & (unsigned int)aSelMode) != 0;
    }

    // =======================================================================
    // Function : isDisabledSubshape
    // =======================================================================
    bool SensitiveCompound::isDisabledSubshape(const Handle(Select3D_SensitiveEntity) & theSens) const
    {
        const SensitiveShape *aShapeEnt = dynamic_cast<SensitiveShape *>(theSens.get());
        if (aShapeEnt == nullptr || aShapeEnt->GetShapeIndex() == -1)
        {
            return false;
        }

        const InteractiveShapeOwner *anOwner = dynamic_cast<InteractiveShapeOwner *>(myOwnerId.get());
        if (anOwner == nullptr)
        {
            return false;
        }

        TopoDS_Shape aShape = aShapeEnt->GetShape();
        if (!aShape.IsNull() && anOwner->GetDisabledElems().Contains(ShapeSelElemTypeFromTopAbs(aShape.ShapeType()),
                                                                     aShapeEnt->GetShapeIndex()))
        {
            return true;
        }
        return false;
    }

    // =======================================================================
    // Function : Matches
    // =======================================================================
    Standard_Boolean SensitiveCompound::Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                                SelectBasics_PickResult &thePickResult)
    {
        myDetectedMap.Clear();

        InteractiveShapeOwner *anOwner = dynamic_cast<InteractiveShapeOwner *>(myOwnerId.get());

        bool toCheckAllInside = false;
        if (!theMgr.IsOverlapAllowed() && theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point)
        {
            // check for InteractiveShapeSelMode::Global
            toCheckAllInside = myShape.IsNull() && anOwner != nullptr && anOwner->GetSelectionMode() != 0;
        }

        if (anOwner != nullptr && anOwner->GetSelectionMode() != (int)InteractiveShapeSelMode::Global)
        {
            if (!isActiveShapeType(anOwner->GetSelectionMode()))
            {
                return false;
            }

            if (mySubshapeIndex != -1 && !myShape.IsNull() && anOwner->GetDisabledElems().Contains(ShapeSelElemTypeFromTopAbs(myShape.ShapeType()), mySubshapeIndex))
            {
                return false;
            }
        }

        myTmpDetectedIdx = -1;
        myTmpPick.Invalidate();
        myTmpSens = 0;
        if (!Select3D_SensitiveSet::matches(theMgr, thePickResult, toCheckAllInside))
        {
            return false;
        }
        if (myTmpDetectedIdx != -1)
        {
            myDetectedIdx = myTmpDetectedIdx; // pick-up manually sorted result
            thePickResult.SetDistToGeomCenter(myTmpPick.DistToGeomCenter());
        }

        myOwnerId->SetPriority((int)SensPriority::Global);
        if (theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point)
        {
            return true;
        }

        Handle(Select3D_SensitiveEntity) anEntity = GetLastDetected();
        if (anEntity.IsNull())
        {
            return false;
        }

        if (const SensitiveShape *aSensSubShape = dynamic_cast<SensitiveShape *>(anEntity.get()))
        {
            myOwnerId->SetPriority(aSensSubShape->GetPriority());
        }

        if (anOwner != nullptr)
        {
            int aSelPrior = Max(Min(anOwner->Priority() + anOwner->PriorityModifier(),
                                    ViewBase::MaxOwnerSelectionPriority),
                                    ViewBase::MinOwnerSelectionPriority);
            anOwner->SetPriority(aSelPrior);
        }

        // update picked point (similar to SelectMgr_ViewerSelector::updatePoint3d())
        updatePickedPoint(thePickResult, anEntity);
        return true;
    }

    // =======================================================================
    // Function : updatePickedPoint
    // =======================================================================
    void SensitiveCompound::updatePickedPoint(SelectBasics_PickResult &thePickResult,
                                              const Handle(Select3D_SensitiveEntity) & theEntity)
    {
        // update picked point (similar to SelectMgr_ViewerSelector::updatePoint3d())
        if (!theEntity->HasInitLocation() || !thePickResult.HasPickedPoint() || theEntity->InvInitLocation().Form() == gp_Identity)
        {
            return;
        }

        const gp_GTrsf anInvTrsf = theEntity->InvInitLocation();
        const gp_GTrsf anInvInvTrsd = anInvTrsf.Inverted();

        gp_XYZ aPnt = thePickResult.PickedPoint().XYZ();
        anInvInvTrsd.Transforms(aPnt);
        thePickResult.SetPickedPoint(aPnt);

        Graphic3d_Vec3 aNorm = thePickResult.SurfaceNormal();
        const float aNormLen2 = aNorm.SquareModulus();
        if (aNormLen2 > ShortRealEpsilon())
        {
            aNorm *= 1.0f / sqrtf(aNormLen2);

            Graphic3d_Mat4d aMat4;
            anInvInvTrsd.GetMat4(aMat4);
            const Graphic3d_Vec4d aNormRes = aMat4 * Graphic3d_Vec4d(Graphic3d_Vec3d(aNorm), 0.0);
            thePickResult.SetSurfaceNormal(Graphic3d_Vec3(aNormRes.xyz()));
        }
    }

    // =======================================================================
    // Function : checkDetectedElem
    // =======================================================================
    bool SensitiveCompound::checkDetectedElem(const Handle(Select3D_SensitiveEntity) & theSens,
                                              const SelectBasics_PickResult &thePickResult) const
    {
        AIS_InteractiveObject *aPrs = static_cast<AIS_InteractiveObject *>(myOwnerId->Selectable().get());

        if (theSens->SensitivityFactor() == myTmpSens)
        {
            if (myTmpDetectedIdx != -1 && thePickResult.Depth() >= myTmpPick.Depth())
            {
                return false;
            }
        }
        else if (myTmpDetectedIdx != -1)
        {
            using TolViewerSelector = ViewBase::InteractiveOwner::TolViewerSelector;
            TolViewerSelector *aMainSel = static_cast<TolViewerSelector *>(aPrs->InteractiveContext()->MainSelector().get());

            const int aSensFact = Max(theSens->SensitivityFactor(), myTmpSens);
            double aTol = aMainSel->GetCamScale() * aSensFact;
            if (!aMainSel->GetManager().Camera()->IsOrthographic())
            {
                const double aPntDepth = myTmpPick.SurfaceNormal().SquareModulus() > 0.0f ? myTmpPick.Depth()
                                                                                          : thePickResult.Depth();
                const gp_Pnt aPnt = aMainSel->GetManager().DetectedPoint(aPntDepth);
                const double aDistFromEye = (aPnt.XYZ() - aMainSel->GetCamEye().XYZ()).Dot(aMainSel->GetCamDir().XYZ());

                aTol *= Abs(aDistFromEye);
            }

            if (Abs(thePickResult.Depth() - myTmpPick.Depth()) <= aTol)
            {
                if (theSens->SensitivityFactor() < myTmpSens)
                { // aSens->Priority()
                    return false;
                }
            }
            else if (thePickResult.Depth() > myTmpPick.Depth())
            {
                return false;
            }
        }

        return true;
    }

    // =======================================================================
    // Function : overlapsElement
    // =======================================================================
    Standard_Boolean SensitiveCompound::overlapsElement(SelectBasics_PickResult &thePickResult,
                                                        SelectBasics_SelectingVolumeManager &theMgr,
                                                        Standard_Integer theElemIdx,
                                                        Standard_Boolean theIsFullInside)
    {
        const int aSensIdx = myEntityIndexes[theElemIdx];
        const Handle(Select3D_SensitiveEntity) &aSens = myEntities[aSensIdx];

        AIS_InteractiveObject *aPrs = static_cast<AIS_InteractiveObject *>(myOwnerId->Selectable().get());

        bool isNewScaleFactor = theMgr.GetActiveSelectionType() == SelectMgr_SelectionType_Point && mySFactor != aSens->SensitivityFactor();
        if (theIsFullInside)
        {
            if (isDisabledSubshape(aSens))
            {
                return false;
            }
        }
        else if (isNewScaleFactor || (aSens->HasInitLocation() && aSens->InvInitLocation().Form() != gp_Identity))
        {
            // this doesn't look good - SelectMgr_ViewerSelector::computeFrustum() should be reused somehow
            /* @code
              const SelectMgr_SelectingVolumeManager* anObjMgr =
                dynamic_cast<SelectMgr_SelectingVolumeManager*>(&theMgr);
              SelectMgr_SelectingVolumeManager aTmpMgr =
               anObjMgr->ScaleAndTransform(aScaleFactor, aSens->InvInitLocation(),
                 Handle(SelectMgr_FrustumBuilder)());
              aTmpMgr.SetViewClipping(*anObjMgr);
            @endcode */
            int aScaleFactor = theMgr.GetActiveSelectionType() == SelectMgr_SelectionType_Point ? aSens->SensitivityFactor()
                                                                                                : 1;

            gp_GTrsf anInvTrsf = aSens->HasInitLocation() ? aSens->InvInitLocation() * aPrs->InversedTransformation()
                                                          : aPrs->InversedTransformation();
            if (aScaleFactor != 1 || anInvTrsf.Form() != gp_Identity)
            {
                SelectMgr_SelectingVolumeManager aTmpMgr =
                    aPrs->InteractiveContext()->MainSelector()->GetManager().ScaleAndTransform(
                        aScaleFactor,
                        anInvTrsf,
                        Handle(SelectMgr_FrustumBuilder)());
                if (!aSens->Matches(aTmpMgr, thePickResult))
                {
                    return false;
                }
            }
            else if (!aSens->Matches(theMgr, thePickResult))
            {
                return false;
            }
        }
        else if (!aSens->Matches(theMgr, thePickResult))
        {
            return false;
        }

        if (theMgr.GetActiveSelectionType() == SelectMgr_SelectionType_Point)
        {
            // handle sorting of sensitives
            if (!checkDetectedElem(aSens, thePickResult))
            {
                return false;
            }

            myTmpDetectedIdx = theElemIdx;
            myTmpPick = thePickResult;
            myTmpSens = aSens->SensitivityFactor();
        }
        else
        {
            myDetectedMap.Add(aSensIdx);
        }
        return true;
    }

    // =======================================================================
    // Function : elementIsInside
    // =======================================================================
    Standard_Boolean SensitiveCompound::elementIsInside(SelectBasics_SelectingVolumeManager &theMgr,
                                                        Standard_Integer theElemIdx,
                                                        Standard_Boolean theIsFullInside)
    {
        const int aSensIdx = myEntityIndexes[theElemIdx];
        const Handle(Select3D_SensitiveEntity) &aSens = myEntities[aSensIdx];

        SelectBasics_PickResult aMatchResult;
        if (theIsFullInside)
        {
            if (isDisabledSubshape(aSens))
            {
                return false;
            }
        }
        else if (aSens->HasInitLocation() && aSens->InvInitLocation().Form() != gp_Identity)
        {
            // this doesn't look good - SelectMgr_ViewerSelector::computeFrustum() should be reused somehow
            const SelectMgr_SelectingVolumeManager *anObjMgr = dynamic_cast<SelectMgr_SelectingVolumeManager *>(&theMgr);

            SelectMgr_SelectingVolumeManager aTmpMgr = anObjMgr->ScaleAndTransform(1,
                                                                                   aSens->InvInitLocation(),
                                                                                   Handle(SelectMgr_FrustumBuilder)());
            aTmpMgr.SetViewClipping(*anObjMgr);
            if (!aSens->Matches(aTmpMgr, aMatchResult))
            {
                return false;
            }
        }
        else if (!aSens->Matches(theMgr, aMatchResult))
        {
            return false;
        }

        if (theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point)
        {
            myDetectedMap.Add(aSensIdx);
        }
        return true;
    }

    // =======================================================================
    // Function : GetConnected
    // =======================================================================
    Handle(Select3D_SensitiveEntity) SensitiveCompound::GetConnected()
    {
        std::vector<Handle(Select3D_SensitiveEntity)> anEntities(myEntities.size());
        for (size_t anEntityIter = 0; anEntityIter < myEntities.size(); ++anEntityIter)
        {
            anEntities[anEntityIter] = myEntities[anEntityIter]->GetConnected();
        }

        Handle(SensitiveCompound) aNewEntity = new SensitiveCompound(myShape, myOwnerId, anEntities);
        aNewEntity->SetShapeIndex(mySubshapeIndex);
        return aNewEntity;
    }

    // =============================================================================
    // Function : GetLastDetected
    // =============================================================================
    Handle(Select3D_SensitiveEntity) SensitiveCompound::GetLastDetected() const
    {
        if (myDetectedIdx >= 0 && myDetectedIdx < (int)myEntities.size())
        {
            const int aSensIdx = myEntityIndexes[myDetectedIdx];
            return myEntities[aSensIdx];
        }
        return Handle(Select3D_SensitiveEntity)();
    }

    // =======================================================================
    // Function : Set
    // =======================================================================
    void SensitiveCompound::Set(const Handle(SelectMgr_EntityOwner) & theOwnerId)
    {
        Select3D_SensitiveEntity::Set(theOwnerId);
        for (const Handle(Select3D_SensitiveEntity) &anEntityIter : myEntities)
        {
            anEntityIter->Set(theOwnerId);
        }
    }

    // =======================================================================
    // Function : BoundingBox
    // =======================================================================
    Select3D_BndBox3d SensitiveCompound::BoundingBox()
    {
        if (myBndBox.IsValid())
        {
            return myBndBox;
        }

        for (const Handle(Select3D_SensitiveEntity) &anEntityIter : myEntities)
        {
            myBndBox.Combine(anEntityIter->BoundingBox());
        }

        return myBndBox;
    }

} // namespace