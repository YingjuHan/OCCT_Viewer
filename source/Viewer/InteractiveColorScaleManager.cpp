#include "InteractiveColorScaleManager.h"

#include <ViewBase/InteractiveViewCube.h>

#include <ViewCae/InteractiveCaeMesh.h>
#include <ViewCae/InteractiveColorScaleOwner.h>

#include "WasmViewer.h"
#include "InteractiveContext.h"

#include <AIS_InteractiveContext.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveColorScaleManager
// ================================================================
InteractiveColorScaleManager::InteractiveColorScaleManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : createColorScale
// ================================================================
void InteractiveColorScaleManager::createColorScale(const ObjectId& theObjId)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    Handle(ViewCae::InteractiveColorScale) aPrs = new ViewCae::InteractiveColorScale(
        new ViewCae::InteractiveColorScaleOwner(theObjId.c_str()));
    aPrs->SetZLayer(Graphic3d_ZLayerId_TopOSD);
    aPrs->OnPixelScaleChange(aPrs, myWebViewer->DevicePixelRatio());
    myWebViewer->GetContext()->SetTransformPersistence(aPrs,
                                                       new Graphic3d_TransformPers(Graphic3d_TMF_2d,
                                                                                   Aspect_TOTP_LEFT_LOWER));
    myWebViewer->GetContext()->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
}

// ================================================================
// Function : attachColorScale
// ================================================================
void InteractiveColorScaleManager::attachColorScale(const ObjectId&    theColorScaleId,
                                                    const ObjectId&    theMeshId,
                                                    const std::string& thePropertyId,
                                                    const int          theComponentIndex)
{
    Handle(ViewCae::InteractiveColorScale) aColorScale;
    Handle(ViewCae::InteractiveCaeMesh)    aMesh;
    if (!FindAisObject(aColorScale, theColorScaleId)) {
        Message::SendFail() << "Error: attachColorScale (" << theColorScaleId
                            << ") called for non-existing color scale object";
        return;
    } else if (!FindAisObject(aMesh, theMeshId)) {
        Message::SendFail() << "Error: attachColorScale (" << theMeshId << ") called for non-existing mesh object";
        return;
    }

    const Handle(RWCae::MeshNVecProperty)& aProperty = aMesh->GetPropertyByActiveTimeStep(thePropertyId.c_str());

    auto onPropertyUpdateCallback = [this](const Handle(ViewCae::MeshColorMapper)& theColorMapper) {
        if (ViewCae::MeshColorScaleMapper* aColorScaleMapper = dynamic_cast<ViewCae::MeshColorScaleMapper*>(
                theColorMapper.get())) {
            this->myWebViewer->Invalidate2dObjects(aColorScaleMapper->ColorScaleObject());
        } else {
            this->myWebViewer->Invalidate2dObjects();
        }
    };

    aColorScale->Attach(aMesh, RWCae::MeshPropertyComponent(aProperty, theComponentIndex), onPropertyUpdateCallback);
}

// ================================================================
// Function : getRelatedColorMapper
// ================================================================
Handle(ViewCae::MeshColorScaleMapper) InteractiveColorScaleManager::getRelatedColorMapper(
    const Handle(ViewCae::InteractiveColorScale)& theColorScaleObj) const
{
    for (AIS_DataMapOfIOStatus::Iterator anObjIter = myWebViewer->GetContext()->ObjectIterator(); anObjIter.More();
         anObjIter.Next()) {
        Handle(ViewCae::InteractiveCaeMesh) aMesh = Handle(ViewCae::InteractiveCaeMesh)::DownCast(anObjIter.Key());
        if (aMesh.IsNull()) {
            continue;
        }

        std::vector<Handle(ViewCae::MeshColorMapper)> aColorMappers = aMesh->ColorMappers();
        for (const Handle(ViewCae::MeshColorMapper)& aColorMapper : aColorMappers) {
            Handle(ViewCae::MeshColorScaleMapper) aColorScaleMapper = Handle(ViewCae::MeshColorScaleMapper)::DownCast(
                aColorMapper);
            if (!aColorScaleMapper.IsNull() && aColorScaleMapper->ColorScaleObject() == theColorScaleObj) {
                return aColorScaleMapper;
            }
        }
    }

    return nullptr;
}

// ================================================================
// Function : getColorScaleParams
// ================================================================
ColorScaleParams InteractiveColorScaleManager::getColorScaleParams(const ObjectId& theObjId)
{
    ColorScaleParams aParams;

    Handle(ViewCae::InteractiveColorScale) aColorScale;
    if (!FindAisObject(aColorScale, theObjId)) {
        Message::SendFail() << "Error: getColorScaleRange (" << theObjId
                            << ") called for non-existing color scale object";
        return aParams;
    }

    aParams.InitFromPresentation(aColorScale);
    Handle(ViewCae::MeshColorScaleMapper) aRelatedColorMapper = getRelatedColorMapper(aColorScale);
    if (!aRelatedColorMapper.IsNull()) {
        aParams.propertyId        = !aRelatedColorMapper->MeshProperty().IsNull()
                                      ? aRelatedColorMapper->MeshProperty()->Id().ToCString()
                                      : "";
        aParams.propertyComponent = aRelatedColorMapper->MeshPropertyComp();
    }

    return aParams;
}

// ================================================================
// Function : setColorScaleParams
// ================================================================
void InteractiveColorScaleManager::setColorScaleParams(const ObjectId& theObjId, const ColorScaleParams& theParams)
{
    Handle(ViewCae::InteractiveColorScale) aColorScale;
    if (!FindAisObject(aColorScale, theObjId)) {
        Message::SendFail() << "Error: setColorScaleRange (" << theObjId
                            << ") called for non-existing color scale object";
        return;
    }

    theParams.UpdatePresentation(aColorScale);

    // invalidate viewer
    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aColorScale);
}

// ================================================================
// Function : getMeshAttachedColorScales
// ================================================================
std::vector<ObjectId> InteractiveColorScaleManager::getMeshAttachedColorScales(const ObjectId& theMeshObjId)
{
    std::vector<ObjectId> aRes;

    Handle(ViewCae::InteractiveCaeMesh) aMesh;
    if (!FindAisObject(aMesh, theMeshObjId)) {
        Message::SendFail() << "Error: getMeshAttachedColorScales (" << theMeshObjId
                            << ") called for non-existing mesh object";
        return aRes;
    }

    std::vector<Handle(ViewCae::MeshColorMapper)> aColorMappers = aMesh->ColorMappers();
    for (const Handle(ViewCae::MeshColorMapper)& aColorMapper : aColorMappers) {
        Handle(ViewCae::MeshColorScaleMapper) aColorScaleMapper = Handle(ViewCae::MeshColorScaleMapper)::DownCast(
            aColorMapper);
        if (aColorScaleMapper.IsNull() || aColorScaleMapper->ColorScaleObject().IsNull()) {
            continue;
        }

        aRes.push_back(aColorScaleMapper->ColorScaleObject()->MeshOwner()->Id().ToCString());
    }

    return aRes;
}

// ================================================================
// Function : UpdateColorScales
// ================================================================
void InteractiveColorScaleManager::UpdateColorScales(const Handle(AIS_InteractiveContext)& theCtx,
                                                     const Handle(AIS_InteractiveObject)&  theUpdatedObj,
                                                     const ViewOffsets&                    theViewOffsets,
                                                     bool                                  theIsDragged)
{
    NCollection_DataMap<Aspect_TypeOfTriedronPosition, Graphic3d_Vec2i> aViewOffsets = theViewOffsets;
    Handle(ViewCae::InteractiveColorScale) anUpdateObj = Handle(ViewCae::InteractiveColorScale)::DownCast(
        theUpdatedObj);
    MapOfOrientedColorScales anOrientedColorScales;
    computeOrientedColorScales(theCtx, anUpdateObj, aViewOffsets, anOrientedColorScales);

    if (anOrientedColorScales.IsEmpty()) {
        return;
    }

    for (MapOfOrientedColorScales::Iterator anIter(anOrientedColorScales); anIter.More(); anIter.Next()) {
        const ListOfColorScales& aColorScales = anIter.Value();

        Graphic3d_Vec2i aWinSize =
            ViewBase::IInteractiveObject::GetVisibleInAnyView(aColorScales.First())->Window()->Dimensions();

        MapOfSortedColorScales aSortedColorScales;
        computeSortedColorScales(anUpdateObj, aViewOffsets, anIter, aSortedColorScales);

        sortColorScales(theIsDragged ? anUpdateObj : nullptr, theCtx, anIter.Key(), aSortedColorScales, aWinSize);
    }
}

// ================================================================
// Function : computeOrientedColorScales
// ================================================================
void InteractiveColorScaleManager::computeOrientedColorScales(
    const Handle(AIS_InteractiveContext)&          theCtx,
    const Handle(ViewCae::InteractiveColorScale)& theUpdatedObj,
    ViewOffsets&                                   theViewOffsets,
    MapOfOrientedColorScales&                      theRes)
{
    for (AIS_DataMapOfIOStatus::Iterator anObjIter = theCtx->ObjectIterator(); anObjIter.More(); anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)&   anObj = anObjIter.Key();
        const Handle(Graphic3d_TransformPers)& aPers = anObj->TransformPersistence();
        if (aPers.IsNull() || !aPers->IsTrihedronOr2d() || anObj->DisplayStatus() != PrsMgr_DisplayStatus_Displayed
            || ViewBase::IInteractiveObject::GetVisibleInAnyView(anObj).IsNull()) {
            continue;
        }

        Aspect_TypeOfTriedronPosition aPos = aPers->Corner2d();
        if (Handle(ViewCae::InteractiveColorScale) aColorScale = Handle(ViewCae::InteractiveColorScale)::DownCast(
                anObj)) {
            if (aColorScale->DisplayMode() == 1) {
                continue;
            }

            // for vertical colorscales only
            aPos = (aPos & Aspect_TOTP_LEFT) != 0  ? Aspect_TOTP_LEFT
                 : (aPos & Aspect_TOTP_RIGHT) != 0 ? Aspect_TOTP_RIGHT
                                                   : Aspect_TOTP_CENTER;
            if (aPos == Aspect_TOTP_CENTER) {
                continue;
            }

            ListOfColorScales* aColorScales = theRes.ChangeSeek(aPos);
            if (aColorScales == nullptr) {
                aColorScales = theRes.Bound(aPos, ListOfColorScales());
            }
            if (aColorScale == theUpdatedObj) {
                aColorScales->Append(aColorScale);
            } else {
                aColorScales->Prepend(aColorScale);
            }
        } else if (ViewBase::IInteractiveObject* anIObj = dynamic_cast<ViewBase::IInteractiveObject*>(anObj.get())) {
            Graphic3d_Vec2i anObjOffset = anIObj->Get2dObjectSize();
            if (anObjOffset == Graphic3d_Vec2i(0)) {
                continue;
            }

            Graphic3d_Vec2i* anOffset = theViewOffsets.ChangeSeek(aPos);
            if (anOffset == nullptr) {
                anOffset = theViewOffsets.Bound(aPos, Graphic3d_Vec2i(0));
            }
            anObjOffset = anObjOffset.cwiseMax(*anOffset);
            anOffset->SetValues(anObjOffset.x(), anObjOffset.y());
        }
    }
}

// ================================================================
// Function : computeSortedColorScales
// ================================================================
void InteractiveColorScaleManager::computeSortedColorScales(
    const Handle(ViewCae::InteractiveColorScale)& theUpdatedObj,
    const ViewOffsets&                             theViewOffsets,
    const MapOfOrientedColorScales::Iterator&      theColorScales,
    MapOfSortedColorScales&                        theRes)
{
    Aspect_TypeOfTriedronPosition aPos = theColorScales.Key();

    int aSide = aPos == Aspect_TOTP_LEFT || aPos == Aspect_TOTP_RIGHT ? 0 : 1;
    int anUp  = (aSide + 1) % 2;

    Aspect_TypeOfTriedronPosition aTopAsp = aSide == 0 ? Aspect_TOTP_TOP : Aspect_TOTP_LEFT;
    Aspect_TypeOfTriedronPosition aBotAsp = aSide == 0 ? Aspect_TOTP_BOTTOM : Aspect_TOTP_RIGHT;

    int             aMinSideOffset   = getOffset(aPos, theViewOffsets)[aSide];
    int             aMinTopOffset    = getOffset(aTopAsp, theViewOffsets)[anUp];
    int             aMinBotOffset    = getOffset(aBotAsp, theViewOffsets)[anUp];
    Graphic3d_Vec2i aCornerTopOffset = getOffset((Aspect_TypeOfTriedronPosition)(aPos | aTopAsp), theViewOffsets);
    Graphic3d_Vec2i aCornerBotOffset = getOffset((Aspect_TypeOfTriedronPosition)(aPos | aBotAsp), theViewOffsets);

    auto updateOffset = [aSide, &theRes](const Handle(ViewCae::InteractiveColorScale)& theObj, int& theOffset) {
        int aMin = theObj->Get2dViewOffset()[aSide] + theObj->GetXPosition();
        int aMax = aMin + theObj->GetBreadth();
        for (MapOfSortedColorScales::Iterator aSortedIter(theRes); aSortedIter.More(); aSortedIter.Next()) {
            if (aMin < aSortedIter.Key().z() + aMax - aMin && aMax > aSortedIter.Key().z()) {
                theOffset = aSortedIter.Key().z();
                break;
            }
        }
    };

    const ListOfColorScales& aColorScales = theColorScales.Value();
    for (const Handle(ViewCae::InteractiveColorScale)& anObj : aColorScales) {
        Graphic3d_Vec3i anOffsets(0);
        anOffsets.z() = Max(anObj->Get2dViewOffset()[aSide], aMinSideOffset);
        if (anObj == theUpdatedObj) {
            updateOffset(anObj, anOffsets.z());
        }
        anOffsets.x() = Max(anOffsets.z() < aCornerTopOffset[aSide] ? aCornerTopOffset[anUp] : aMinTopOffset,
                            aMinTopOffset);
        anOffsets.y() = Max(anOffsets.z() < aCornerBotOffset[aSide] ? aCornerBotOffset[anUp] : aMinBotOffset,
                            aMinBotOffset);
        VectorOfColorScales* aRangedColorScales = theRes.ChangeSeek(anOffsets);
        if (aRangedColorScales == nullptr) {
            aRangedColorScales = theRes.Bound(anOffsets, VectorOfColorScales());
        }
        aRangedColorScales->push_back(anObj);
    }
}

// ================================================================
// Function : getOffset
// ================================================================
Graphic3d_Vec2i InteractiveColorScaleManager::getOffset(Aspect_TypeOfTriedronPosition thePos,
                                                        const ViewOffsets&            theViewOffsets)
{
    Graphic3d_Vec2i aRes(0);
    theViewOffsets.Find(thePos, aRes);
    return aRes;
}

// ================================================================
// Function : sortColorScales
// ================================================================
void InteractiveColorScaleManager::sortColorScales(const Handle(ViewCae::InteractiveColorScale)& theUpdatedObj,
                                                   const Handle(AIS_InteractiveContext)&          theCtx,
                                                   Aspect_TypeOfTriedronPosition                  thePos,
                                                   const MapOfSortedColorScales&                  theSorted,
                                                   const Graphic3d_Vec2i&                         theWinSize)
{
    const int aSide = thePos == Aspect_TOTP_LEFT || thePos == Aspect_TOTP_RIGHT ? 0 : 1;
    const int anUp  = (aSide + 1) % 2;
    for (MapOfSortedColorScales::Iterator aSortedIter(theSorted); aSortedIter.More(); aSortedIter.Next()) {
        VectorOfColorScales& aCSVector = aSortedIter.ChangeValue();

        auto sortByIds = [](const Handle(ViewCae::InteractiveColorScale)& theObj1,
                            const Handle(ViewCae::InteractiveColorScale)& theObj2) -> bool {
            return theObj1->MeshOwner()->Id() < theObj2->MeshOwner()->Id();
        };
        std::sort(aCSVector.begin(), aCSVector.end(), sortByIds);
        int i = 0;
        for (const Handle(ViewCae::InteractiveColorScale)& anObj : aCSVector) {
            double aSize = (double(theWinSize[anUp]) / anObj->DevicePixelRatio() - aSortedIter.Key().x()
                            - aSortedIter.Key().y())
                         / aCSVector.size();

            Graphic3d_Vec2d anOffset(anObj->Get2dViewOffset());
            anOffset[aSide] = aSortedIter.Key().z();
            anOffset[anUp]  = aSortedIter.Key().y() + i * aSize;
            anObj->Set2dViewOffset(Graphic3d_Vec2i(anOffset));
            anObj->SetBaseHeight((float)aSize);

            anObj->TransformPersistence()->SetOffset2d(Graphic3d_Vec2i(anOffset));
            theCtx->RecomputePrsOnly(anObj, false, true);
            if (anObj != theUpdatedObj) {
                theCtx->RecomputeSelectionOnly(anObj);
            }
            ++i;
        }
    }
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(InteractiveColorScaleManager)
{
    emscripten::class_<InteractiveColorScaleManager, emscripten::base<IManager>>("InteractiveColorScaleManager")
        .function("createColorScale", &InteractiveColorScaleManager::createColorScale)
        .function("attachColorScale", &InteractiveColorScaleManager::attachColorScale)
        .function("getColorScaleParams", &InteractiveColorScaleManager::getColorScaleParams)
        .function("setColorScaleParams", &InteractiveColorScaleManager::setColorScaleParams)
        .function("getMeshAttachedColorScales", &InteractiveColorScaleManager::getMeshAttachedColorScales);
}
#endif

} // namespace

