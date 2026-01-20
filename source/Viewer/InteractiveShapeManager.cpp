#include "InteractiveShapeManager.h"

#include <ViewBase/BaseDefaultDrawer.h>
#include <ViewBase/UnitManager.h>

#include <ViewShape/InteractiveShape.h>
#include <ViewShape/InteractiveShapeOwner.h>

#include <RWBase/EmulatedFileSystem.h>

#include <RWShapeConv/ShapeLoader.h>

#include "InteractiveContext.h"
#include "WasmViewer.h"

#include <AIS_InteractiveContext.hxx>
#include <BinTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepPreviewAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepTools.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GProp_GProps.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Sphere.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>

#include <iomanip>
#include <sstream>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveShapeManager
// ================================================================
InteractiveShapeManager::InteractiveShapeManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : GetMaterialPreset
// ================================================================
VisMaterial InteractiveShapeManager::GetMaterialPreset(const std::string& thePreset)
{
    Graphic3d_NameOfMaterial aMatName = Graphic3d_NameOfMaterial_DEFAULT;
    TCollection_AsciiString  aPreset(thePreset.c_str());
    aPreset.LowerCase();

    VisMaterial aPbrMat;
    if (aPreset == "default") {
        ViewBase::BaseDefaultDrawer* aDefDrawer = dynamic_cast<ViewBase::BaseDefaultDrawer*>(
            myWebViewer->GetContext()->DefaultDrawer().get());
        if (aDefDrawer != nullptr) {
            aPbrMat.Init(aDefDrawer->VisMaterial()->PbrMaterial());
        }
    } else if (!aPreset.IsEmpty() && Graphic3d_MaterialAspect::MaterialFromName(aPreset.ToCString(), aMatName)) {
        Graphic3d_MaterialAspect aMat = (Graphic3d_NameOfMaterial)aMatName;
        aPbrMat.Init(aMat.PBRMaterial());
        aPbrMat.name      = aPreset.ToCString();
        aPbrMat.isDefined = true;
    } else {
        Message::SendFail() << "Error: GetMaterialPreset(" << thePreset << ") unknown preset";
    }
    return aPbrMat;
}

// ================================================================
// Function : GetDefaultMaterial
// ================================================================
VisMaterial InteractiveShapeManager::GetDefaultMaterial()
{
    ViewBase::BaseDefaultDrawer* aDefDrawer = dynamic_cast<ViewBase::BaseDefaultDrawer*>(
        myWebViewer->GetContext()->DefaultDrawer().get());
    VisMaterial aResMat;
    if (aDefDrawer != nullptr) {
        aResMat.Init(aDefDrawer->VisMaterial()->PbrMaterial());
    }
    aResMat.isDefined = false;
    return aResMat;
}

// ================================================================
// Function : SetDefaultMaterial
// ================================================================
void InteractiveShapeManager::SetDefaultMaterial(const VisMaterial& theMat)
{
    if (theMat.baseColor.empty()) {
        Message::SendFail() << "Error: SetDefaultMaterial() called with invalid material";
        return;
    }

    ViewBase::BaseDefaultDrawer* aDefDrawer = dynamic_cast<ViewBase::BaseDefaultDrawer*>(
        myWebViewer->GetContext()->DefaultDrawer().get());
    if (aDefDrawer == nullptr) {
        return;
    }

    XCAFDoc_VisMaterialPBR aPbrMat = aDefDrawer->VisMaterial()->PbrMaterial();
    aPbrMat.IsDefined              = true;
    aPbrMat.Metallic               = theMat.metallic;
    aPbrMat.EmissiveFactor         = Graphic3d_Vec3(theMat.emissiveFactor);
    aPbrMat.RefractionIndex        = theMat.refractionIndex;
    aPbrMat.Roughness              = theMat.roughness;
    if (!ColorTools::ParseHexColor(theMat.baseColor, aPbrMat.BaseColor)) {
        return;
    }

    aDefDrawer->VisMaterial()->SetPbrMaterial(aPbrMat);
    for (AIS_DataMapOfIOStatus::Iterator anObjIter = myWebViewer->GetContext()->ObjectIterator(); anObjIter.More();
         anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)& anObj = anObjIter.Key();
        if (ViewShape::InteractiveShape* aShape = dynamic_cast<ViewShape::InteractiveShape*>(anObj.get())) {
            aShape->UpdateAspects();
            invalidateViewer(anObj);
        }
    }
}

// ================================================================
// Function : GetMaterial
// ================================================================
VisMaterial InteractiveShapeManager::GetMaterial(const ObjectId& theObjId)
{
    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    Handle(ViewShape::InteractiveShape)      aShape;
    if (!FindPrsOwnerAndObject(aShapeOwner, aShape, theObjId)) {
        Message::SendFail() << "Error: GetMaterial (" << theObjId << ") called for non-existing shape object";
        return VisMaterial();
    }

    if (aShapeOwner->BaseStyle().Material().IsNull()) {
        ViewBase::BaseDefaultDrawer* aDefDrawer = dynamic_cast<ViewBase::BaseDefaultDrawer*>(
            myWebViewer->GetContext()->DefaultDrawer().get());
        VisMaterial aResMat;
        if (aDefDrawer != nullptr) {
            aResMat.Init(aDefDrawer->VisMaterial()->PbrMaterial());
        }
        aResMat.isDefined = false;
        return aResMat;
    }

    VisMaterial aResMat;
    aResMat.Init(aShapeOwner->BaseStyle().Material()->PbrMaterial());
    aResMat.isDefined = true;
    return aResMat;
}

// ================================================================
// Function : SetMaterial
// ================================================================
void InteractiveShapeManager::SetMaterial(const ObjectId& theObjId, const VisMaterial& theMat)
{
    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    Handle(ViewShape::InteractiveShape)      aShape;
    if (!FindPrsOwnerAndObject(aShapeOwner, aShape, theObjId)) {
        Message::SendFail() << "Error: SetMaterial (" << theObjId << ") called for non-existing shape object";
        return;
    }

    if (theMat.baseColor.empty() || !theMat.isDefined) {
        if (aShapeOwner->BaseStyle().Material().IsNull()) {
            return;
        }

        aShapeOwner->ChangeBaseStyle().SetMaterial(Handle(XCAFDoc_VisMaterial)());
        aShape->SetToUpdate();
        invalidateViewer(aShape);
        return;
    }

    aShapeOwner->InitBaseMaterial();

    XCAFDoc_VisMaterialPBR aPbrMat = aShapeOwner->BaseStyle().Material()->PbrMaterial();
    aPbrMat.Metallic               = theMat.metallic;
    aPbrMat.EmissiveFactor         = Graphic3d_Vec3(theMat.emissiveFactor);
    aPbrMat.RefractionIndex        = theMat.refractionIndex;
    aPbrMat.Roughness              = theMat.roughness;
    if (!ColorTools::ParseHexColor(theMat.baseColor, aPbrMat.BaseColor)) {
        return;
    }

    aShapeOwner->BaseStyle().Material()->SetPbrMaterial(aPbrMat);
    aShape->UpdateAspects();
    invalidateViewer(aShape);
}

// ================================================================
// Function : GetForcedColor
// ================================================================
ColorTools::ColorString InteractiveShapeManager::GetForcedColor(const ObjectId& theObjId) const
{
    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    return FindPrsOwner(aShapeOwner, theObjId) && aShapeOwner->HasForcedColor()
             ? Quantity_ColorRGBA::ColorToHex(Quantity_ColorRGBA(aShapeOwner->GetForcedColor())).ToCString()
             : ColorTools::ColorString();
}

// ================================================================
// Function : SetForcedColor
// ================================================================
void InteractiveShapeManager::SetForcedColor(const ObjectId& theObjId, const ColorTools::ColorString& theHexColor)
{
    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    Handle(ViewShape::InteractiveShape)      aShape;
    if (!FindPrsOwnerAndObject(aShapeOwner, aShape, theObjId)) {
        Message::SendFail() << "Error: SetForcedColor (" << theObjId << ") called for non-existing shape object";
        return;
    }

    if (theHexColor.empty()) {
        if (!aShapeOwner->HasForcedColor()) {
            return;
        }

        aShapeOwner->UnsetForcedColor();
    } else {
        Quantity_ColorRGBA aColor;
        if (!ColorTools::ParseHexColor(theHexColor, aColor)) {
            return;
        }

        if (aShapeOwner->HasForcedColor() && aShapeOwner->GetForcedColor().IsEqual(aColor.GetRGB())) {
            return;
        }
        aShapeOwner->SetForcedColor(aColor.GetRGB());
    }

    aShape->UpdateAspects();
    invalidateViewer(aShape);
}

// ================================================================
// Function : GetForcedTransparency
// ================================================================
float InteractiveShapeManager::GetForcedTransparency(const ObjectId& theObjId) const
{
    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    return FindPrsOwner(aShapeOwner, theObjId) && aShapeOwner->HasForcedTransparency()
             ? aShapeOwner->GetForcedTransparency()
             : -1.0f;
}

// ================================================================
// Function : SetForcedTransparency
// ================================================================
void InteractiveShapeManager::SetForcedTransparency(const ObjectId& theObjId, const float theTransp)
{
    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    Handle(ViewShape::InteractiveShape)      aShape;
    if (!FindPrsOwnerAndObject(aShapeOwner, aShape, theObjId)) {
        Message::SendFail() << "Error: SetForcedTransparency (" << theObjId << ") called for non-existing shape object";
        return;
    }

    if (theTransp < 0.0f || theTransp > 1.0f) {
        if (!aShapeOwner->HasForcedTransparency()) {
            return;
        }

        aShapeOwner->UnsetForcedTransparency();
    } else {
        if (aShapeOwner->HasForcedTransparency() && aShapeOwner->GetForcedTransparency() == theTransp) {
            return;
        }
        aShapeOwner->SetForcedTransparency(theTransp);
    }

    aShape->UpdateAspects();
    invalidateViewer(aShape);
}

// ================================================================
// Function : GetPresentationMode
// ================================================================
int InteractiveShapeManager::GetPresentationMode(const ObjectId& theObjId) const
{
    Handle(ViewShape::InteractiveShape) aShape;
    return FindAisObject(aShape, theObjId) ? (int)aShape->PresentationMode()
                                           : (int)ViewShape::InteractiveShapePrsMode::Shaded;
}

// ================================================================
// Function : SetPresentationMode
// ================================================================
void InteractiveShapeManager::SetPresentationMode(const ObjectId& theObjId, int theMode)
{
    Handle(ViewShape::InteractiveShape) aShape;
    if (!FindAisObject(aShape, theObjId)) {
        Message::SendFail() << "Error: SetPresentationMode (" << theObjId << ") called for non-existing shape object";
        return;
    }

    if (theMode < ViewShape::InteractiveShapePrsMode_LOWER || theMode > ViewShape::InteractiveShapePrsMode_UPPER) {
        Message::SendFail() << "Error: SetPresentationMode (" << theObjId << ", " << theMode
                            << ") called with unknown presentation mode";
        return;
    }

    if ((int)aShape->PresentationMode() == theMode) {
        return;
    }

    aShape->SetPresentationMode((ViewShape::InteractiveShapePrsMode)theMode);
    if (aShape->DisplayStatus() == PrsMgr_DisplayStatus_Displayed && aShape->MeshOwner()->IsLoaded()) {
        myWebViewer->GetContext()->Display(aShape, false);
        invalidateViewer(aShape);
    }
}

// ================================================================
// Function : GetSubshapeVertex
// ================================================================
Viewer::Vec3 InteractiveShapeManager::GetSubshapeVertex(const ObjectId& theObjId, int theVertId) const
{
    Handle(ViewShape::InteractiveShape) aShape;
    if (!FindAisObject(aShape, theObjId)) {
        Message::SendFail() << "Error: GetSubshapeVertex (" << theObjId << ", " << theVertId
                            << ") called for non-existing shape object";
        return Viewer::Vec3::InvalidPoint();
    }

    TopoDS_Shape aSubShape = aShape->MeshOwner()->GetSubshape(ViewShape::ShapeSelElemType::Vertex, theVertId);
    if (aSubShape.IsNull()) {
        Message::SendFail() << "Error: GetSubshapeVertex (" << theObjId << ", " << theVertId
                            << ") called with invalid vertex index";
        return Viewer::Vec3::InvalidPoint();
    }

    const gp_Pnt aPnt = BRep_Tool::Pnt(TopoDS::Vertex(aSubShape));
    return Viewer::Vec3(aPnt.XYZ());
}

// ================================================================
// Function : displayShape
// ================================================================
void InteractiveShapeManager::displayShape(const ObjectId& theObjId, const TopoDS_Shape& theShape)
{
    Handle(RWShape::ShapeData)              aShapeData = new RWShape::ShapeData(theObjId.c_str(), theShape);
    Handle(ViewShape::InteractiveShapeOwner) anOwner    = new ViewShape::InteractiveShapeOwner(aShapeData);

    Handle(AIS_InteractiveObject) aMeshPrs = anOwner->CreatePresentation();
    myWebViewer->GetContext()->Display(aMeshPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
}

// ================================================================
// Function : buildPoint
// ================================================================
void InteractiveShapeManager::buildPoint(const ObjectId& theObjId, const Vec3& thePnt)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);
    displayShape(theObjId, BRepBuilderAPI_MakeVertex(thePnt.XYZ()));
}

// ================================================================
// Function : buildLineSegment
// ================================================================
void InteractiveShapeManager::buildLineSegment(const ObjectId& theObjId, const Vec3& thePnt1, const Vec3& thePnt2)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    TopoDS_Shape aShape;
    if ((thePnt1.XYZ() - thePnt2.XYZ()).Modulus() < Precision::Confusion()) {
        aShape = BRepBuilderAPI_MakeVertex(thePnt1.XYZ());
    } else {
        BRepBuilderAPI_MakeEdge aBuilder(thePnt1.XYZ(), thePnt2.XYZ());
        aShape = aBuilder;
    }
    displayShape(theObjId, aShape);
}

// ================================================================
// Function : buildBox
// ================================================================
void InteractiveShapeManager::buildBox(const ObjectId& theObjId, const Ax2& theAx2, const Vec3& theDims)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    gp_Ax2 anAx2  = theAx2.GetAx2();
    gp_Pnt anOrig = anAx2.Location().XYZ();
    if (theDims.X() < 0) {
        anOrig.SetX(anOrig.X() + theDims.X());
    }
    if (theDims.Y() < 0) {
        anOrig.SetY(anOrig.Y() + theDims.Y());
    }
    if (theDims.Z() < 0) {
        anOrig.SetZ(anOrig.Z() + theDims.Z());
    }
    anAx2.SetLocation(anOrig);

    BRepPreviewAPI_MakeBox aPreview;
    aPreview.Init(anAx2, Abs(theDims.X()), Abs(theDims.Y()), Abs(theDims.Z()));
    TopoDS_Shape aShape = aPreview;

    BRepMesh_IncrementalMesh aMesher; // tolerances do not matter for a box
    aMesher.SetShape(aShape);
    aMesher.Perform();

    displayShape(theObjId, aShape);
}

//! Return default linear deflection parameter.
static double DefaultMeshDeflection(double theSizeA, double theSizeB = 0.0, double theSizeC = 0.0)
{
    static constexpr double sizeFactor = 10;
    return Max(Abs(theSizeA), Max(Abs(theSizeB), Abs(theSizeC))) / sizeFactor;
}

// ================================================================
// Function : buildSphere
// ================================================================
void InteractiveShapeManager::buildSphere(const ObjectId& theObjId,
                                          const Ax2&      theAx2,
                                          const double    theRadius,
                                          const double    theDevAngle)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    TopoDS_Shape aShape;
    if (Abs(theRadius) <= Precision::Confusion()) {
        aShape = BRepBuilderAPI_MakeVertex(theAx2.location.XYZ());
    } else {
        BRepPrimAPI_MakeSphere aSphere(theAx2.GetAx2(), Abs(theRadius));
        aShape = aSphere;

        BRepMesh_IncrementalMesh aMesher;
        aMesher.ChangeParameters().Deflection = DefaultMeshDeflection(theRadius);
        aMesher.ChangeParameters().MinSize    = Precision::Confusion();
        aMesher.ChangeParameters().Angle      = theDevAngle;
        aMesher.SetShape(aShape);
        aMesher.Perform();
    }

    displayShape(theObjId, aShape);
}

// ================================================================
// Function : buildCylinder
// ================================================================
void InteractiveShapeManager::buildCylinder(const ObjectId&       theObjId,
                                            const CylinderParams& theParams,
                                            const double          theDevAngle)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    gp_Ax2 anAx2  = theParams.ax2.GetAx2();
    gp_Pnt anOrig = anAx2.Location().XYZ();
    if (theParams.height < 0) {
        anOrig.SetZ(anOrig.Z() + theParams.height);
    }
    anAx2.SetLocation(anOrig);

    TopoDS_Shape aShape;
    try {
        BRepPrimAPI_MakeCylinder aCyl(anAx2, Abs(theParams.radius), Abs(theParams.height), theParams.angle);
        aShape = aCyl;

        BRepMesh_IncrementalMesh aMesher;
        aMesher.ChangeParameters().Deflection = DefaultMeshDeflection(theParams.radius, theParams.height);
        aMesher.ChangeParameters().MinSize    = Precision::Confusion();
        aMesher.ChangeParameters().Angle      = theDevAngle;
        aMesher.SetShape(aShape);
        aMesher.Perform();
    } catch (const Standard_Failure& theErr) {
        Message::SendFail() << "buildCylinder() - construction FAILED:\n" << theErr;
        return;
    }

    displayShape(theObjId, aShape);
}

// ================================================================
// Function : buildCone
// ================================================================
void InteractiveShapeManager::buildCone(const ObjectId& theObjId, const ConeParams& theParams, const double theDevAngle)
{
    if (Abs(Abs(theParams.radius) - Abs(theParams.topRadius)) < Precision::Confusion()
        || Abs(theParams.height) < Precision::Confusion()) {
        CylinderParams aParams;
        aParams.ax2    = theParams.ax2;
        aParams.height = theParams.height;
        aParams.radius = theParams.radius;
        aParams.angle  = theParams.angle;
        buildCylinder(theObjId, aParams, theDevAngle);
        return;
    }

    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    gp_Ax2 anAx2  = theParams.ax2.GetAx2();
    gp_Pnt anOrig = anAx2.Location().XYZ();
    if (theParams.height < 0) {
        anOrig.SetZ(anOrig.Z() + theParams.height);
    }
    anAx2.SetLocation(anOrig);

    TopoDS_Shape aShape;
    try {
        BRepPrimAPI_MakeCone aCone(anAx2,
                                   Abs(theParams.radius),
                                   Abs(theParams.topRadius),
                                   Abs(theParams.height),
                                   theParams.angle);
        aShape = aCone;

        BRepMesh_IncrementalMesh aMesher;
        aMesher.ChangeParameters().Deflection = DefaultMeshDeflection(theParams.radius,
                                                                      theParams.topRadius,
                                                                      theParams.height);
        aMesher.ChangeParameters().MinSize    = Precision::Confusion();
        aMesher.ChangeParameters().Angle      = theDevAngle;
        aMesher.SetShape(aShape);
        aMesher.Perform();
    } catch (const Standard_Failure& theErr) {
        Message::SendFail() << "buildCone() - construction FAILED:\n" << theErr;
        return;
    }

    displayShape(theObjId, aShape);
}

// ================================================================
// Function : probeFileFormat
// ================================================================
std::string InteractiveShapeManager::probeFileFormat(const std::string& theName,
                                                     uintptr_t          theBuffer,
                                                     int                theDataLen,
                                                     bool               theToFree)
{
    std::string aFormat = RWShapeConv::ShapeLoader::ProbeFileFormat(theName, (Standard_CString)theBuffer, theDataLen);
    if (theToFree) {
        free((void*)theBuffer);
    }
    return aFormat;
}

// ================================================================
// Function : openModelShape
// ================================================================
bool InteractiveShapeManager::openModelShape(const ObjectId& theObjId, const std::string& theFilePath)
{
    return OpenShape(myWebViewer->GetContext(), theObjId, theFilePath);
}

// ================================================================
// Function : OpenShape
// ================================================================
bool InteractiveShapeManager::OpenShape(const Handle(AIS_InteractiveContext)& theCtx,
                                        const ObjectId&                       theObjId,
                                        const std::string&                    theFilePath)
{
    InteractiveContext::RemoveObject(theCtx, theObjId);

    Handle(OSD_FileSystem) aFileSystem = RWBase::EmulatedFileSystem::Create(theFilePath.c_str());
    if (aFileSystem.IsNull()) {
        return false;
    }

    const Handle(ViewBase::UnitManager)& aUnitMgr = ViewBase::UnitManager::Instance();

    RWShapeConv::ShapeLoader aLoader;
    aLoader.SetSystemLengthUnitScale(aUnitMgr->SystemToSI(ViewBase::QuantityType::Length, 1.0));

    NCollection_List<Handle(RWShape::ShapeData)> aShapeList;
    aLoader.OpenShape(aShapeList, theObjId.c_str(), aFileSystem, theFilePath);
    if (aShapeList.IsEmpty()) {
        return false;
    }

    for (const Handle(RWShape::ShapeData)& aShapeIter : aShapeList) {
        if (aShapeIter.IsNull()) {
            continue;
        }

        Handle(ViewShape::InteractiveShapeOwner) anOwner = new ViewShape::InteractiveShapeOwner(aShapeIter);
        Handle(AIS_InteractiveObject)             aPrs    = anOwner->CreatePresentation();
        theCtx->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
    }
    return true;
}

//! Collect all shapes in context and put them into compound.
static TopoDS_Compound getAllShapesInContext(const Handle(AIS_InteractiveContext)& theCtx)
{
    TopoDS_Compound aComp;
    BRep_Builder().MakeCompound(aComp);
    for (ViewBase::InteractiveOwner::Iterator anOwnIter(*theCtx); anOwnIter.More(); anOwnIter.Next()) {
        const ViewShape::InteractiveShapeOwner* aShapeOwner = dynamic_cast<ViewShape::InteractiveShapeOwner*>(
            anOwnIter.Current());
        if (aShapeOwner == nullptr) {
            continue;
        }

        for (const TopoDS_Shape& aRootIter : aShapeOwner->ShapeData()->Shapes()) {
            BRep_Builder().Add(aComp, aRootIter);
        }
    }
    return aComp;
}

// ================================================================
// Function : saveModelShape
// ================================================================
bool InteractiveShapeManager::saveModelShape(const ObjectId&                         theObjId,
                                             const Viewer::SelectionShapeElems& theElems,
                                             const std::string&                      theFile) const
{
    ViewShape::ShapeSelElems anElems;
    for (int aTypeIter = 0; aTypeIter < ViewShape::ShapeSelElemType_NB; ++aTypeIter) {
        const ViewShape::ShapeSelElemType aType = (ViewShape::ShapeSelElemType)aTypeIter;
        anElems.ChangeIndices(aType)             = IdsMapFromString(theElems.GetElems(aType));
    }

    TopoDS_Shape aShape;
    if (theObjId == "*") {
        if (theElems.IsValid()) {
            Message::SendFail() << "[ERR_EXPORT_SHAPE] saveModelShape(" << theFile << "," << theObjId
                                << ") is called with non-empty subshape reference";
            return false;
        }

        aShape = getAllShapesInContext(myWebViewer->GetContext());
    } else {
        Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
        if (!FindPrsOwner(aShapeOwner, theObjId)) {
            Message::SendFail() << "[ERR_EXPORT_SHAPE] saveModelShape(" << theFile << "," << theObjId
                                << ") is called for non-existing object";
            return false;
        }

        if (!theElems.IsValid()) {
            aShape = aShapeOwner->ShapeData()->GetSingleShape();
        } else {
            aShape = aShapeOwner->GetSubshapesByIndexMap(anElems);
        }
    }

    if (aShape.IsNull()) {
        Message::SendFail() << "[ERR_EXPORT_SHAPE] saveModelShape(" << theFile << "," << theObjId << ") is called for NULL shape";
        return false;
    }

    if (!BinTools::Write(aShape, theFile.c_str())) {
        Message::SendFail() << "[ERR_EXPORT_SHAPE] saveModelShape(" << theFile << "," << theObjId << ") failed to save file";
        return false;
    }
    return true;
}

// ================================================================
// Function : cloneSubshapes
// ================================================================
ObjectId InteractiveShapeManager::cloneSubshapes(const ObjectId&                         theObjIdOld,
                                                 const Viewer::SelectionShapeElems& theElems,
                                                 const ObjectId&                         theObjIdNew) const
{
    Handle(ViewShape::InteractiveShapeOwner) anOldOwner;
    FindPrsOwner(anOldOwner, theObjIdOld);
    if (anOldOwner.IsNull()) {
        Message::SendFail() << "Error: cloneSubshapes (" << theObjIdOld << ", " << theObjIdNew
                            << ") called for non-existing object";
        return ObjectId();
    }
    if (theObjIdOld == theObjIdNew) {
        Message::SendFail() << "Error: cloneSubshapes (" << theObjIdOld << ", " << theObjIdNew
                            << ") called with invalid new id";
        return ObjectId();
    }

    ViewShape::ShapeSelElems anElems;
    for (int aTypeIter = 0; aTypeIter < ViewShape::ShapeSelElemType_NB; ++aTypeIter) {
        const ViewShape::ShapeSelElemType aType = (ViewShape::ShapeSelElemType)aTypeIter;
        anElems.ChangeIndices(aType)             = IdsMapFromString(theElems.GetElems(aType));
    }
    if (!anElems.IsValid()) {
        Message::SendFail() << "Error: cloneSubshapes (" << theObjIdOld << ", " << theObjIdNew
                            << ") called with empty subshape list";
        return ObjectId();
    }

    TopoDS_Shape aShape = anOldOwner->GetSubshapesByIndexMap(anElems);
    if (aShape.IsNull()) {
        Message::SendFail() << "Error: cloneSubshapes (" << theObjIdOld << ", " << theObjIdNew
                            << ") called with invalid subshape list";
        return ObjectId();
    }

    ObjectId aNewId = theObjIdNew;
    if (aNewId.empty()) {
        aNewId = InteractiveContext::GenerateId(myWebViewer->GetContext(), theObjIdOld);
    } else {
        InteractiveContext::RemoveObject(myWebViewer->GetContext(), aNewId);
    }

    Handle(RWShape::ShapeData)              aNewData = new RWShape::ShapeData(aNewId.c_str(), aShape);
    Handle(ViewShape::InteractiveShapeOwner) aClone   = new ViewShape::InteractiveShapeOwner(aNewData);
    Handle(AIS_InteractiveObject)             anAisPrs = aClone->CreatePresentation();
    if (anAisPrs.IsNull()) {
        Message::SendFail() << "Error: cloneSubshapes (" << theObjIdOld << ", " << theObjIdNew
                            << ") called for non-displayable object";
        return ObjectId();
    }

    if (ViewBase::IInteractiveObject* anSCPrs = dynamic_cast<ViewBase::IInteractiveObject*>(anAisPrs.get())) {
        anSCPrs->OnPixelScaleChange(anAisPrs, myWebViewer->DevicePixelRatio());
    }

    myWebViewer->GetContext()->Display(anAisPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
    return aNewId;
}

// ================================================================
// Function : explodeShape
// ================================================================
std::string InteractiveShapeManager::explodeShape(const ObjectId&    theObjId,
                                                  const int          theExplodeDepth,
                                                  const bool         theIsAutoNaming,
                                                  const bool         theToGenId,
                                                  const bool         theToPropagateId,
                                                  const std::string& theNewShapeIds)
{
    std::string anExplodedObjs = theNewShapeIds;
    ExplodeShape(myWebViewer->GetContext(),
                 theObjId,
                 theExplodeDepth,
                 theIsAutoNaming,
                 theToGenId,
                 theToPropagateId,
                 anExplodedObjs);
    return anExplodedObjs;
}

//! Apply input ids instead of generated ones.
static bool applyIdsToExploded(const NCollection_List<Handle(RWShape::ShapeData)>& theShapeList,
                               const std::string&                                    theResObjList)
{
    std::stringstream aListStream(theResObjList);
    std::string       aName;
    int               aNbNames = 0;
    for (const Handle(RWShape::ShapeData)& aShapeIter : theShapeList) {
        if (aShapeIter.IsNull()) {
            continue;
        }

        if (!std::getline(aListStream, aName, IManager::THE_LIST_SEPARATOR)) {
            Message::SendFail() << "Error: unexpected size of ids (" << aNbNames << ") to explode ("
                                << theShapeList.Size() << " shapes)";
            return false;
        }

        ++aNbNames;
        aShapeIter->SetId(aName.c_str());
    }

    if (std::getline(aListStream, aName, IManager::THE_LIST_SEPARATOR)) {
        Message::SendFail() << "Error: unexpected size of ids to explode (more than " << theShapeList.Size()
                            << " shapes)";
        return false;
    }
    return true;
}

// ================================================================
// Function : ExplodeShape
// ================================================================
bool InteractiveShapeManager::ExplodeShape(const Handle(AIS_InteractiveContext)& theCtx,
                                           const ObjectId&                       theObjId,
                                           const int                             theExplodeDepth,
                                           const bool                            theIsAutoNaming,
                                           const bool                            theToGenId,
                                           const bool                            theToPropagateId,
                                           std::string&                          theResObjList)
{
    Handle(ViewShape::InteractiveShape) aPrevShape;
    if (!ViewBase::IInteractiveObject::FindObjectById(aPrevShape, *theCtx, theObjId.c_str())) {
        Message::SendFail() << "Error: explode of NULL object";
        return false;
    }
    const bool isPrevVisible = aPrevShape->DisplayStatus() == PrsMgr_DisplayStatus_Displayed;

    RWShapeConv::ShapeLoader::ExplodeParams anExpParams;
    anExpParams.explodeDepth  = theExplodeDepth;
    anExpParams.isAutoNaming  = theIsAutoNaming;
    anExpParams.toGenerateId  = theToGenId;
    anExpParams.toPropagateId = theToPropagateId;

    const Handle(ViewShape::InteractiveShapeOwner)& aPrevShapeOwner = aPrevShape->MeshOwner();
    NCollection_List<Handle(RWShape::ShapeData)>   aShapeList;
    RWShapeConv::ShapeLoader::ExplodeShape(aShapeList, aPrevShapeOwner->ShapeData(), anExpParams);
    if (aShapeList.IsEmpty()) {
        return false;
    }

    // apply input ids instead of generated ones
    const bool hasResNames = !theResObjList.empty();
    if (hasResNames && !applyIdsToExploded(aShapeList, theResObjList)) {
        return false;
    }

    ViewBase::IInteractiveObject::InvalidateViewer(aPrevShape);
    InteractiveContext::RemoveObject(theCtx, aPrevShape);
    for (const Handle(RWShape::ShapeData)& aShapeIter : aShapeList) {
        if (aShapeIter.IsNull()) {
            continue;
        }

        Handle(ViewShape::InteractiveShapeOwner) anOwner = new ViewShape::InteractiveShapeOwner(aShapeIter);
        anOwner->SetPresentationMode(aPrevShapeOwner->PresentationMode());
        anOwner->SetSelectionMode(aPrevShapeOwner->GetSelectionMode());

        Handle(AIS_InteractiveObject) aNewPrs = anOwner->CreatePresentation();
        if (isPrevVisible) {
            theCtx->Display(aNewPrs, false);
        } else {
            theCtx->Display(aNewPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
        }
        if (hasResNames) {
            continue;
        }

        if (!theResObjList.empty()) {
            theResObjList += THE_LIST_SEPARATOR;
        }
        theResObjList += aShapeIter->Id().ToCString();
    }
    return true;
}

// ================================================================
// Function : GetSubshapeDump
// ================================================================
std::string InteractiveShapeManager::GetSubshapeDump(const ObjectId& theObjId,
                                                     const int       theElemType,
                                                     const int       theIndex) const
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: GetSubshapeDump(" << theObjId << ", " << theElemType
                            << ") is called with invalid subshape type";
        return std::string();
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: GetSubshapeDump(" << theObjId << ", " << theElemType
                            << ") is called for non-existing object";
        return std::string();
    }

    const TopoDS_Shape aShape = aShapeOwner->GetSubshape(anElemType, theIndex);
    if (aShape.IsNull()) {
        return std::string();
    }

    std::stringstream aString;
    BRepTools::Write(aShape, aString);
    return aString.str();
}

namespace {
std::string jsonPoint(const gp_Pnt& thePnt)
{
    std::ostringstream aJson;
    aJson << std::setprecision(17);
    aJson << "{\"x\":" << thePnt.X() << ",\"y\":" << thePnt.Y() << ",\"z\":" << thePnt.Z() << "}";
    return aJson.str();
}

bool resolveShapeForMeasurement(TopoDS_Shape&                                    theOut,
                                const Handle(ViewShape::InteractiveShapeOwner)& theOwner,
                                const int                                       theSelMode,
                                const int                                       theEntityId)
{
    if (theOwner.IsNull() || theOwner->ShapeData().IsNull()) {
        return false;
    }

    if (theSelMode == (int)ViewShape::InteractiveShapeSelMode::Global || theSelMode == 0 || theEntityId < 0) {
        theOut = theOwner->ShapeData()->GetSingleShape();
        return !theOut.IsNull();
    }

    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theSelMode)) {
        return false;
    }

    theOut = theOwner->GetSubshape(anElemType, theEntityId);
    return !theOut.IsNull();
}
} // namespace

// ================================================================
// Function : MeasureDistanceJson
// ================================================================
std::string InteractiveShapeManager::MeasureDistanceJson(const ObjectId& theObjId1,
                                                         const int       theSelMode1,
                                                         const int       theEntityId1,
                                                         const ObjectId& theObjId2,
                                                         const int       theSelMode2,
                                                         const int       theEntityId2) const
{
    Handle(ViewShape::InteractiveShapeOwner) aOwner1;
    if (!FindPrsOwner(aOwner1, theObjId1)) {
        Message::SendFail() << "Error: MeasureDistanceJson() called for non-existing object '" << theObjId1 << "'";
        return std::string();
    }

    Handle(ViewShape::InteractiveShapeOwner) aOwner2;
    if (!FindPrsOwner(aOwner2, theObjId2)) {
        Message::SendFail() << "Error: MeasureDistanceJson() called for non-existing object '" << theObjId2 << "'";
        return std::string();
    }

    TopoDS_Shape aSh1;
    TopoDS_Shape aSh2;
    if (!resolveShapeForMeasurement(aSh1, aOwner1, theSelMode1, theEntityId1)) {
        Message::SendFail() << "Error: MeasureDistanceJson() failed to resolve subshape (" << theObjId1 << ", "
                            << theSelMode1 << ", " << theEntityId1 << ")";
        return std::string();
    }
    if (!resolveShapeForMeasurement(aSh2, aOwner2, theSelMode2, theEntityId2)) {
        Message::SendFail() << "Error: MeasureDistanceJson() failed to resolve subshape (" << theObjId2 << ", "
                            << theSelMode2 << ", " << theEntityId2 << ")";
        return std::string();
    }

    BRepExtrema_DistShapeShape aDist(aSh1, aSh2, Extrema_ExtFlag_MIN);
    aDist.Perform();
    if (!aDist.IsDone() || aDist.NbSolution() < 1) {
        Message::SendFail() << "Error: MeasureDistanceJson() failed to compute distance";
        return std::string();
    }

    const gp_Pnt aPnt1 = aDist.PointOnShape1(1);
    const gp_Pnt aPnt2 = aDist.PointOnShape2(1);

    std::ostringstream aJson;
    aJson << std::setprecision(17);
    aJson << "{\"distance\":" << aDist.Value() << ",\"pnt1\":" << jsonPoint(aPnt1) << ",\"pnt2\":" << jsonPoint(aPnt2)
          << "}";
    return aJson.str();
}

// ================================================================
// Function : GetCircleParamsJson
// ================================================================
std::string InteractiveShapeManager::GetCircleParamsJson(const ObjectId& theObjId,
                                                         const int       theSelMode,
                                                         const int       theEntityId) const
{
    if (theEntityId < 0) {
        Message::SendFail() << "Error: GetCircleParamsJson() called with invalid entityId";
        return std::string();
    }

    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theSelMode)) {
        Message::SendFail() << "Error: GetCircleParamsJson(" << theObjId << ", " << theSelMode << ") invalid selMode";
        return std::string();
    }

    if (anElemType != ViewShape::ShapeSelElemType::Edge
        && anElemType != ViewShape::ShapeSelElemType::Face) {
        Message::SendFail() << "Error: GetCircleParamsJson() supports Edge or Face only";
        return std::string();
    }

    Handle(ViewShape::InteractiveShapeOwner) aOwner;
    if (!FindPrsOwner(aOwner, theObjId)) {
        Message::SendFail() << "Error: GetCircleParamsJson() called for non-existing object '" << theObjId << "'";
        return std::string();
    }

    const TopoDS_Shape aSub = aOwner->GetSubshape(anElemType, theEntityId);
    auto toJson = [](const gp_Ax2& theAx2, const double theRad) -> std::string {
        const gp_Pnt aLoc = theAx2.Location();
        const gp_Dir aDir = theAx2.Direction();
        const gp_Dir aXDir = theAx2.XDirection();
        const gp_Pnt anAnchor = aLoc.Translated(gp_Vec(aXDir) * theRad);

        std::ostringstream aJson;
        aJson << std::setprecision(17);
        aJson << "{\"ax2\":{\"location\":" << jsonPoint(aLoc)
              << ",\"direction\":" << jsonPoint(gp_Pnt(aDir.X(), aDir.Y(), aDir.Z()))
              << ",\"xDirection\":" << jsonPoint(gp_Pnt(aXDir.X(), aXDir.Y(), aXDir.Z()))
              << "},\"radius\":" << theRad
              << ",\"anchor\":" << jsonPoint(anAnchor) << "}";
        return aJson.str();
    };

    if (anElemType == ViewShape::ShapeSelElemType::Edge) {
        if (aSub.IsNull() || aSub.ShapeType() != TopAbs_EDGE) {
            Message::SendFail() << "Error: GetCircleParamsJson() failed to resolve Edge subshape";
            return std::string();
        }

        const TopoDS_Edge anEdge = TopoDS::Edge(aSub);
        BRepAdaptor_Curve aCurve(anEdge);
        if (aCurve.GetType() != GeomAbs_Circle) {
            Message::SendFail() << "Error: GetCircleParamsJson() selected edge is not a circle";
            return std::string();
        }

        const gp_Circ aCirc = aCurve.Circle();
        return toJson(aCirc.Position(), aCirc.Radius());
    }

    if (aSub.IsNull() || aSub.ShapeType() != TopAbs_FACE) {
        Message::SendFail() << "Error: GetCircleParamsJson() failed to resolve Face subshape";
        return std::string();
    }

    const TopoDS_Face aFace = TopoDS::Face(aSub);
    BRepAdaptor_Surface aSurf(aFace, true);
    if (aSurf.GetType() == GeomAbs_Cylinder) {
        const gp_Cylinder aCyl = aSurf.Cylinder();
        return toJson(aCyl.Position().Ax2(), aCyl.Radius());
    }
    if (aSurf.GetType() == GeomAbs_Sphere) {
        const gp_Sphere aSphere = aSurf.Sphere();
        return toJson(aSphere.Position().Ax2(), aSphere.Radius());
    }
    if (aSurf.GetType() == GeomAbs_Plane) {
        for (TopExp_Explorer anExp(aFace, TopAbs_EDGE); anExp.More(); anExp.Next()) {
            const TopoDS_Edge anEdge = TopoDS::Edge(anExp.Current());
            BRepAdaptor_Curve aCurve(anEdge);
            if (aCurve.GetType() == GeomAbs_Circle) {
                const gp_Circ aCirc = aCurve.Circle();
                return toJson(aCirc.Position(), aCirc.Radius());
            }
        }
    }

    Message::SendFail() << "Error: GetCircleParamsJson() face is not a circle/cylinder/sphere";
    return std::string();
}

// ================================================================
// Function : GetMassPropsJson
// ================================================================
std::string InteractiveShapeManager::GetMassPropsJson(const ObjectId& theObjId) const
{
    Handle(ViewShape::InteractiveShapeOwner) aOwner;
    if (!FindPrsOwner(aOwner, theObjId)) {
        Message::SendFail() << "Error: GetMassPropsJson() called for non-existing object '" << theObjId << "'";
        return std::string();
    }
    if (aOwner->ShapeData().IsNull()) {
        Message::SendFail() << "Error: GetMassPropsJson() called for object without shape data '" << theObjId << "'";
        return std::string();
    }

    const TopoDS_Shape aShape = aOwner->ShapeData()->GetSingleShape();
    if (aShape.IsNull()) {
        Message::SendFail() << "Error: GetMassPropsJson() failed to resolve shape '" << theObjId << "'";
        return std::string();
    }

    GProp_GProps aSurfProps;
    BRepGProp::SurfaceProperties(aShape, aSurfProps);
    const double aArea = aSurfProps.Mass();
    const gp_Pnt aSurfCenter = aSurfProps.CentreOfMass();

    bool hasSolid = false;
    bool allClosed = true;
    for (TopExp_Explorer anExp(aShape, TopAbs_SOLID); anExp.More(); anExp.Next()) {
        hasSolid = true;
        const TopoDS_Solid& aSolid = TopoDS::Solid(anExp.Value());
        if (!aSolid.Closed()) {
            allClosed = false;
            break;
        }
    }

    bool volumeValid = hasSolid && allClosed;
    double aVolume = 0.0;
    gp_Pnt aVolCenter = aSurfCenter;
    if (volumeValid) {
        GProp_GProps aVolProps;
        BRepGProp::VolumeProperties(aShape, aVolProps);
        aVolume = aVolProps.Mass();
        aVolCenter = aVolProps.CentreOfMass();
        if (aVolume <= 0.0) {
            volumeValid = false;
        }
    }

    if (!volumeValid) {
        Message::SendWarning() << "Warning: GetMassPropsJson() volume is unavailable for non-closed shape '"
                               << theObjId << "'";
    }

    const gp_Pnt aCentroid = volumeValid ? aVolCenter : aSurfCenter;
    const char* aSource = volumeValid ? "volume" : "surface";

    std::ostringstream aJson;
    aJson << std::setprecision(17);
    aJson << "{\"area\":" << aArea << ",\"volume\":";
    if (volumeValid) {
        aJson << aVolume;
    } else {
        aJson << "null";
    }
    aJson << ",\"centroid\":" << jsonPoint(aCentroid)
          << ",\"centroidSource\":\"" << aSource << "\""
          << ",\"volumeValid\":" << (volumeValid ? "true" : "false") << "}";
    return aJson.str();
}

// ================================================================
// Function : GetSubshapeInfo
// ================================================================
ShapeInfo InteractiveShapeManager::GetSubshapeInfo(const ObjectId& theObjId,
                                                   const int       theElemType,
                                                   const int       theIndex) const
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: GetSubshapeInfo(" << theObjId << ", " << theElemType
                            << ") is called with invalid subshape type";
        return ShapeInfo();
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: GetSubshapeInfo(" << theObjId << ", " << theElemType
                            << ") is called for non-existing object";
        return ShapeInfo();
    }

    const TopoDS_Shape aShape = aShapeOwner->GetSubshape(anElemType, theIndex);
    if (aShape.IsNull()) {
        return ShapeInfo();
    }

    ShapeInfo aShapeInfo;
    aShapeInfo.Init(aShape, theIndex);

    Quantity_ColorRGBA aColor;
    if (aShapeOwner->GetSubshapeColor(aColor, aShape)) {
        aShapeInfo.color = Quantity_ColorRGBA::ColorToHex(aColor).ToCString();
    }

    aShapeInfo.hidden = aShapeOwner->IsSubshapeHidden(aShape);

    return aShapeInfo;
}

// ================================================================
// Function : GetSubshapesInfo
// ================================================================
std::vector<ShapeInfo> InteractiveShapeManager::GetSubshapesInfo(const ObjectId&    theObjId,
                                                                 const int          theElemType,
                                                                 const std::string& theIndices) const
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: GetSubshapesInfo(" << theObjId << ", " << theElemType
                            << ") is called with invalid subshape type";
        return std::vector<ShapeInfo>();
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: GetSubshapesInfo(" << theObjId << ", " << theElemType
                            << ") is called for non-existing object";
        return std::vector<ShapeInfo>();
    }

    if (theIndices.empty()) {
        return std::vector<ShapeInfo>();
    }

    Handle(TColStd_HPackedMapOfInteger) anIndexMap = IdsMapFromString(theIndices);
    if (anIndexMap.IsNull() || anIndexMap->Map().IsEmpty()) {
        Message::SendFail() << "Error: GetSubshapesInfo(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ") is called with invalid indices";
        return std::vector<ShapeInfo>();
    }

    std::vector<RWShape::EnumeratedShape> aShapes;
    if (!aShapeOwner->GetSubshapes(aShapes, anElemType, anIndexMap)) {
        return std::vector<ShapeInfo>();
    }

    std::vector<ShapeInfo> anInfos;
    anInfos.reserve(aShapes.size());

    TColStd_PackedMapOfInteger::Iterator anIndexIter(anIndexMap->Map());
    for (const RWShape::EnumeratedShape& aShapeIter : aShapes) {
        anInfos.push_back(ShapeInfo());
        ShapeInfo& aShapeInfo = anInfos.back();
        aShapeInfo.Init(aShapeIter.Shape, aShapeIter.Index);

        Quantity_ColorRGBA aColor;
        if (aShapeOwner->GetSubshapeColor(aColor, aShapeIter.Shape)) {
            aShapeInfo.color = Quantity_ColorRGBA::ColorToHex(aColor).ToCString();
        }

        aShapeInfo.hidden = aShapeOwner->IsSubshapeHidden(aShapeIter.Shape);
    }

    return anInfos;
}

// ================================================================
// Function : GetSubshapeColor
// ================================================================
ColorTools::ColorString InteractiveShapeManager::GetSubshapeColor(const ObjectId& theObjId,
                                                                  const int       theElemType,
                                                                  const int       theIndex) const
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: GetSubshapeColor(" << theObjId << ", " << theElemType
                            << ") is called with invalid subshape type";
        return ColorTools::ColorString();
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: GetSubshapeColor(" << theObjId << ", " << theElemType
                            << ") is called for non-existing object";
        return ColorTools::ColorString();
    }

    Quantity_ColorRGBA aColor;
    if (!aShapeOwner->GetSubshapeColor(aColor, anElemType, theIndex)) {
        return ColorTools::ColorString();
    }

    return Quantity_ColorRGBA::ColorToHex(aColor).ToCString();
}

// ================================================================
// Function : SetSubshapeColor
// ================================================================
void InteractiveShapeManager::SetSubshapeColor(const ObjectId&                theObjId,
                                               const int                      theElemType,
                                               const std::string&             theIndices,
                                               const ColorTools::ColorString& theHexColor)
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: SetSubshapeColor(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ", " << theHexColor << ") is called with invalid subshape type";
        return;
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: SetSubshapeColor(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ", " << theHexColor << ") is called for non-existing object";
        return;
    }

    Handle(TColStd_HPackedMapOfInteger) anIndexMap;
    if (theIndices != "*") {
        anIndexMap = IdsMapFromString(theIndices);
        if (!anIndexMap.IsNull() && anIndexMap->Map().IsEmpty()) {
            Message::SendFail() << "Error: SetSubshapeColor(" << theObjId << ", " << theElemType << ", " << theIndices
                                << ", " << theHexColor << ") is called with invalid indices";
            return;
        }
        if (anIndexMap.IsNull()) {
            anIndexMap = new TColStd_HPackedMapOfInteger();
        }
    }

    Handle(ViewShape::InteractiveShape) aPrsForColor = Handle(ViewShape::InteractiveShape)::DownCast(
        aShapeOwner->Selectable());
    if (aPrsForColor.IsNull()) {
        return;
    }

    Quantity_ColorRGBA aColor;
    if (!theHexColor.empty() && !ColorTools::ParseHexColor(theHexColor, aColor)) {
        return;
    }

    if (aShapeOwner->SetSubshapeColor(!theHexColor.empty() ? &aColor : nullptr, anElemType, anIndexMap)) {
        aPrsForColor->SetToUpdate();
        invalidateViewer(aPrsForColor);
    }
}

// ================================================================
// Function : IsSubshapeHidden
// ================================================================
bool InteractiveShapeManager::IsSubshapeHidden(const ObjectId& theObjId,
                                               const int       theElemType,
                                               const int       theIndex) const
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: IsSubshapeHidden(" << theObjId << ", " << theElemType
                            << ") is called with invalid subshape type";
        return false;
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: IsSubshapeHidden(" << theObjId << ", " << theElemType
                            << ") is called for non-existing object";
        return false;
    }

    return aShapeOwner->IsSubshapeHidden(anElemType, theIndex);
}

// ================================================================
// Function : SetSubshapeHidden
// ================================================================
void InteractiveShapeManager::SetSubshapeHidden(const ObjectId&    theObjId,
                                                const int          theElemType,
                                                const std::string& theIndices,
                                                const bool         theIsHidden,
                                                const bool         theToInvertOthers)
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: SetSubshapeHidden(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ", " << int(theIsHidden) << ") is called with invalid subshape type";
        return;
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: SetSubshapeHidden(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ", " << int(theIsHidden) << ") is called for non-existing object";
        return;
    }

    Handle(TColStd_HPackedMapOfInteger) anIndexMap;
    if (theIndices != "*") {
        anIndexMap = IdsMapFromString(theIndices);
        if (!anIndexMap.IsNull() && anIndexMap->Map().IsEmpty()) {
            Message::SendFail() << "Error: SetSubshapeHidden(" << theObjId << ", " << theElemType << ", " << theIndices
                                << ", " << int(theIsHidden) << ") is called with invalid indices";
            return;
        }
        if (anIndexMap.IsNull()) {
            anIndexMap = new TColStd_HPackedMapOfInteger();
        }
    }

    Handle(ViewShape::InteractiveShape) aPrsForHidden = Handle(ViewShape::InteractiveShape)::DownCast(
        aShapeOwner->Selectable());
    if (aPrsForHidden.IsNull()) {
        return;
    }

    if (aShapeOwner->SetSubshapeHidden(anElemType, anIndexMap, theIsHidden, theToInvertOthers)) {
        aPrsForHidden->SetToUpdate();
        invalidateViewer(aPrsForHidden);
    }
}

// ================================================================
// Function : GetConnectedSubshapes
// ================================================================
std::string InteractiveShapeManager::GetConnectedSubshapes(const Viewer::ObjectId& theObjId,
                                                           const int                    theElemType,
                                                           const std::string&           theIndices) const
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: GetConnectedSubshapes(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ") is called with invalid subshape type";
        return std::string();
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: GetConnectedSubshapes(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ") is called for non-existing object";
        return std::string();
    }

    Handle(TColStd_HPackedMapOfInteger) anIndexMap = IdsMapFromString(theIndices);

    Handle(TColStd_HPackedMapOfInteger) aResMap = aShapeOwner->GetConnectedSubshapes(anElemType, anIndexMap);

    std::string aResStr;
    IdsMapToString(aResStr, aResMap);
    return aResStr;
}

// ================================================================
// Function : ClearSelectionGroups
// ================================================================
void InteractiveShapeManager::ClearSelectionGroups(const ObjectId& theObjId)
{
    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: ClearSelectionGroup(" << theObjId << ") is called for non-existing object";
        return;
    }

    aShapeOwner->SetShapeGroups(std::vector<TopoDS_Compound>());
}

// ================================================================
// Function : AddSelectionGroup
// ================================================================
int InteractiveShapeManager::AddSelectionGroup(const ObjectId&    theObjId,
                                               const int          theElemType,
                                               const std::string& theIndices)
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)
        || anElemType == ViewShape::ShapeSelElemType::Group) {
        Message::SendFail() << "Error: AddSelectionGroup(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ") is called with invalid subshape type";
        return -1;
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theObjId)) {
        Message::SendFail() << "Error: AddSelectionGroup(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ") is called for non-existing object";
        return -1;
    }

    Handle(TColStd_HPackedMapOfInteger) anIndexMap = IdsMapFromString(theIndices);
    if (anIndexMap.IsNull() || anIndexMap->Map().IsEmpty()) {
        Message::SendFail() << "Error: AddSelectionGroup(" << theObjId << ", " << theElemType << ", " << theIndices
                            << ") is called with invalid indices";
        return -1;
    }

    const TopAbs_ShapeEnum aShapeType = ViewShape::ShapeSelElemTypeToTopAbs(anElemType);

    std::vector<RWShape::EnumeratedShape> aShapes;
    if (!aShapeOwner->ShapeData()->GetSubshapes(aShapes, aShapeType, anIndexMap)) {
        return -1;
    }

    TopoDS_Compound aComp;
    BRep_Builder::MakeCompound(aComp);
    for (const RWShape::EnumeratedShape& aShapeIter : aShapes) {
        BRep_Builder::Add(aComp, aShapeIter.Shape);
    }

    aShapeOwner->AddShapeGroup(aComp);
    return (int)aShapeOwner->ShapeGroups().size() - 1;
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(VisMaterial)
{
    emscripten::value_object<VisMaterial>("VisMaterial")
        .field("isDefined", &VisMaterial::isDefined)
        .field("baseColor", &VisMaterial::baseColor)
        .field("emissiveFactor", &VisMaterial::emissiveFactor)
        .field("metallic", &VisMaterial::metallic)
        .field("roughness", &VisMaterial::roughness)
        .field("refractionIndex", &VisMaterial::refractionIndex);
}
EMSCRIPTEN_BINDINGS(InteractiveShapeManager)
{
    emscripten::register_vector<ShapeInfo>("ShapeInfoVector");

    emscripten::class_<InteractiveShapeManager, emscripten::base<IManager>>("InteractiveShapeManager")
        .function("probeFileFormat", &InteractiveShapeManager::probeFileFormat)
        .function("openModelShape", &InteractiveShapeManager::openModelShape)
        .function("saveModelShape", &InteractiveShapeManager::saveModelShape)
        .function("cloneSubshapes", &InteractiveShapeManager::cloneSubshapes)
        .function("explodeShape", &InteractiveShapeManager::explodeShape)
        //
        .function("buildPoint", &InteractiveShapeManager::buildPoint)
        .function("buildLineSegment", &InteractiveShapeManager::buildLineSegment)
        .function("buildBox", &InteractiveShapeManager::buildBox)
        .function("buildSphere", &InteractiveShapeManager::buildSphere)
        .function("buildCylinder", &InteractiveShapeManager::buildCylinder)
        .function("buildCone", &InteractiveShapeManager::buildCone)
        //
        .function("getMaterialPreset", &InteractiveShapeManager::GetMaterialPreset)
        .function("getDefaultMaterial", &InteractiveShapeManager::GetDefaultMaterial)
        .function("setDefaultMaterial", &InteractiveShapeManager::SetDefaultMaterial)
        .function("getMaterial", &InteractiveShapeManager::GetMaterial)
        .function("setMaterial", &InteractiveShapeManager::SetMaterial)
        .function("getPresentationMode", &InteractiveShapeManager::GetPresentationMode)
        .function("setPresentationMode", &InteractiveShapeManager::SetPresentationMode)
        .function("getForcedColor", &InteractiveShapeManager::GetForcedColor)
        .function("setForcedColor", &InteractiveShapeManager::SetForcedColor)
        .function("getForcedTransparency", &InteractiveShapeManager::GetForcedTransparency)
        .function("setForcedTransparency", &InteractiveShapeManager::SetForcedTransparency)
        .function("getSubshapeVertex", &InteractiveShapeManager::GetSubshapeVertex)
        .function("getSubshapeInfo", &InteractiveShapeManager::GetSubshapeInfo)
        .function("getSubshapesInfo", &InteractiveShapeManager::GetSubshapesInfo)
        .function("getSubshapeColor", &InteractiveShapeManager::GetSubshapeColor)
        .function("setSubshapeColor", &InteractiveShapeManager::SetSubshapeColor)
        .function("isSubshapeHidden", &InteractiveShapeManager::IsSubshapeHidden)
        .function("setSubshapeHidden", &InteractiveShapeManager::SetSubshapeHidden)
        .function("getConnectedSubshapes", &InteractiveShapeManager::GetConnectedSubshapes)
        .function("getSubshapeDump", &InteractiveShapeManager::GetSubshapeDump)
        .function("measureDistanceJson", &InteractiveShapeManager::MeasureDistanceJson)
        .function("getCircleParamsJson", &InteractiveShapeManager::GetCircleParamsJson)
        .function("getMassPropsJson", &InteractiveShapeManager::GetMassPropsJson)
        //
        .function("clearSelectionGroups", &InteractiveShapeManager::ClearSelectionGroups)
        .function("addSelectionGroup", &InteractiveShapeManager::AddSelectionGroup);
}
#endif

} // namespace

