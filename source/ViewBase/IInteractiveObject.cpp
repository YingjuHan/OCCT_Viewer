#include <ViewBase/IInteractiveObject.h>

#include <AIS_InteractiveContext.hxx>
#include <Message.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

namespace ViewBase
{

    Handle(AIS_InteractiveObject) IInteractiveObject::FindAisObjectById(const AIS_InteractiveContext &theCtx,
                                                                        const TCollection_AsciiString &theObjId)
    {
        /* selectable might be NULL in broken cases
        @code Handle(InteractiveOwner) anOwner = InteractiveOwner::FindPrsOwnerById(theCtx, theObjId);
        if (anOwner.IsNull()) {
            return Handle(AIS_InteractiveObject)();
        }
        return Handle(AIS_InteractiveObject)::DownCast(anOwner->Selectable()); @endcode */
        /* 在损坏的情况下可选择对象可能为NULL
        @code Handle(InteractiveOwner) anOwner = InteractiveOwner::FindPrsOwnerById(theCtx, theObjId);
        if (anOwner.IsNull()) {
            return Handle(AIS_InteractiveObject)();
        }
        return Handle(AIS_InteractiveObject)::DownCast(anOwner->Selectable()); @endcode */

        if (theObjId.IsEmpty())
        {
            return Handle(AIS_InteractiveObject)();
        }

        for (AIS_DataMapOfIOStatus::Iterator anObjIter = theCtx.ObjectIterator(); anObjIter.More(); anObjIter.Next())
        {
            const Handle(AIS_InteractiveObject) &anObj = anObjIter.Key();

            const InteractiveOwner *anOwner = dynamic_cast<InteractiveOwner *>(anObj->GetOwner().get());
            const InteractiveOwner *aChild = anOwner != nullptr ? anOwner->FindChildById(theObjId) : nullptr;
            if (aChild != nullptr)
            {
                return anObj;
            }
        }

        return Handle(AIS_InteractiveObject)();
    }

    Handle(AIS_InteractiveObject) IInteractiveObject::FindAisObjectByOwner(const AIS_InteractiveContext &theCtx,
                                                                           const Handle(InteractiveOwner) & theOwner)
    {
        if (theOwner.IsNull())
        {
            return Handle(AIS_InteractiveObject)();
        }

        for (AIS_DataMapOfIOStatus::Iterator anObjIter = theCtx.ObjectIterator(); anObjIter.More(); anObjIter.Next())
        {
            const Handle(AIS_InteractiveObject) &anObj = anObjIter.Key();
            if (anObj->GetOwner() == theOwner)
            {
                return anObj;
            }
            else if (theOwner->Parent() != nullptr && anObj->GetOwner() == theOwner->Parent())
            {
                return anObj;
            }
        }
        return Handle(AIS_InteractiveObject)();
    }

    IInteractiveObject::IInteractiveObject() : myDevicePixelRatio(1.0f), myIsEnabled(true), myIsThickLine2d(true)
    {
        //
    }

    IInteractiveObject::~IInteractiveObject()
    {
        //
    }

    bool IInteractiveObject::OnPixelScaleChange(const Handle(AIS_InteractiveObject) & theObj, float theRatio)
    {
        myDevicePixelRatio = theRatio;

        if (!theObj->TransformPersistence().IsNull() && theObj->TransformPersistence()->IsTrihedronOr2d() && !theObj->TransformPersistence()->IsDensityIndependent())
        {
            Graphic3d_Vec2d aGap = Graphic3d_Vec2d(myViewOffset) * theRatio;
            theObj->TransformPersistence()->SetOffset2d(Graphic3d_Vec2i(aGap));
        }
        return false;
    }

    bool IInteractiveObject::HandleMouseClick(const Handle(AIS_InteractiveObject) & theObj,
                                              const Graphic3d_Vec2i &,
                                              Aspect_VKeyMouse,
                                              Aspect_VKeyFlags,
                                              bool)
    {
        if (myCallback)
        {
            myCallback(theObj);
            return true;
        }

        return false;
    }

    void IInteractiveObject::InvalidateViewer(const Handle(AIS_InteractiveObject) & theObj)
    {
        if (theObj.IsNull())
        {
            return;
        }

        Handle(Graphic3d_ViewAffinity) anAffinity =
            theObj->InteractiveContext()->CurrentViewer()->StructureManager()->ObjectAffinity(theObj);
        if (anAffinity.IsNull())
        {
            return;
        }

        for (const Handle(V3d_View) &aViewIter : theObj->InteractiveContext()->CurrentViewer()->DefinedViews())
        {
            if (anAffinity->IsVisible(aViewIter->View()->Identification()))
            {
                aViewIter->Invalidate();
                if (aViewIter->ParentView() != nullptr)
                {
                    aViewIter->ParentView()->InvalidateImmediate();
                }
            }
        }
    }

    Handle(V3d_View) IInteractiveObject::GetVisibleInView(const Handle(AIS_InteractiveObject) & theObj)
    {
        if (theObj.IsNull() || !theObj->HasInteractiveContext())
        {
            return Handle(V3d_View)();
        }

        Handle(Graphic3d_ViewAffinity) anAffinity =
            theObj->InteractiveContext()->CurrentViewer()->StructureManager()->ObjectAffinity(theObj);
        int aNbVisible = 0;
        Handle(V3d_View) aView;
        for (const Handle(V3d_View) &aViewIter : theObj->InteractiveContext()->CurrentViewer()->DefinedViews())
        {
            if (anAffinity->IsVisible(aViewIter->View()->Identification()))
            {
                ++aNbVisible;
                if (aNbVisible == 1)
                {
                    aView = aViewIter;
                }
            }
        }
        return aNbVisible == 1 ? aView : Handle(V3d_View)();
    }

    Handle(V3d_View) IInteractiveObject::GetVisibleInAnyView(const Handle(AIS_InteractiveObject) & theObj)
    {
        if (theObj.IsNull() || !theObj->HasInteractiveContext())
        {
            return Handle(V3d_View)();
        }

        Handle(V3d_View) aView = GetVisibleInView(theObj);
        if (!aView.IsNull())
        {
            return aView;
        }

        return theObj->InteractiveContext()->CurrentViewer()->ActiveViews().First();
    }

    gp_Ax1 IInteractiveObject::GetPickLine(const Handle(V3d_View) & theView, const Graphic3d_Vec2i &thePos)
    {
        Graphic3d_Vec3d aXYZ;
        Graphic3d_Vec3d aVxyz;
        theView->ConvertWithProj(thePos.x(), thePos.y(), aXYZ.x(), aXYZ.y(), aXYZ.z(), aVxyz.x(), aVxyz.y(), aVxyz.z());
        return gp_Ax1(gp_Pnt(aXYZ.x(), aXYZ.y(), aXYZ.z()), gp_Dir(aVxyz.x(), aVxyz.y(), aVxyz.z()));
    }

    Graphic3d_Vec2i IInteractiveObject::GetPointOnScreen2d(const Handle(AIS_InteractiveObject) & theObj,
                                                           const Handle(V3d_View) & theView,
                                                           const Graphic3d_Vec2i &thePos)
    {
        static constexpr int halfDiv2 = 2;

        const Handle(Graphic3d_TransformPers) &aPers = theObj->TransformPersistence();
        if (aPers.IsNull() || !aPers->IsTrihedronOr2d())
        {
            return thePos;
        }

        Handle(V3d_View) aView = theView;
        if (aView.IsNull())
        {
            aView = GetVisibleInView(theObj);
        }
        if (aView.IsNull())
        {
            return thePos;
        }

        const Aspect_TypeOfTriedronPosition aCorner = aPers->Corner2d();

        const Graphic3d_Vec2d aWinSize(aView->Window()->Dimensions());
        Graphic3d_Vec2d anOffset = aPers->IsDensityIndependent()
                                       ? Graphic3d_Vec2d(aPers->Offset2d()) * aView->RenderingParams().ResolutionRatio()
                                       : Graphic3d_Vec2d(aPers->Offset2d());
        if ((aCorner & Aspect_TOTP_LEFT) != 0)
        {
            //
        }
        else if ((aCorner & Aspect_TOTP_RIGHT) != 0)
        {
            anOffset.x() = aWinSize.x() - anOffset.x() - 1;
        }
        else
        {
            anOffset.x() = aWinSize.x() / halfDiv2;
        }

        if ((aCorner & Aspect_TOTP_TOP) != 0)
        {
            //
        }
        else if ((aCorner & Aspect_TOTP_BOTTOM) != 0)
        {
            anOffset.y() = aWinSize.y() - anOffset.y() - 1;
        }
        else
        {
            anOffset.y() = aWinSize.y() / halfDiv2;
        }

        return Graphic3d_Vec2i(Graphic3d_Vec2d(anOffset.x() + thePos.x(), anOffset.y() - thePos.y()));
    }

} // namespace ViewBase