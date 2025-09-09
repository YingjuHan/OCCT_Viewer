

#include <ViewCae/InteractiveSphericalAxes.h>

#include <GeomAdaptor_Curve.hxx>
#include <Geom_Circle.hxx>
#include <Graphic3d_Text.hxx>
#include <Prs3d_Arrow.hxx>
#include <Prs3d_TextAspect.hxx>
#include <Prs3d_ToolCylinder.hxx>
#include <Prs3d_ToolDisk.hxx>
#include <Prs3d_ToolSphere.hxx>
#include <StdPrs_DeflectionCurve.hxx>
#include <gp_Quaternion.hxx>

//! Namespace for this file's constants.
namespace {

//! Radius of the latitude-longitude lines sphere.
static constexpr double ISA_Radius = 1.0;

//! X Axis color.
static const Quantity_Color ISA_XAxisColor{Quantity_NOC_RED};
//! Y Axis color.
static const Quantity_Color ISA_YAxisColor{Quantity_NOC_GREEN};
//! Z Axis color.
static const Quantity_Color ISA_ZAxisColor{Quantity_NOC_BLUE};

//! Number of facettes in computation of cylinders, spheres, etc.
static constexpr size_t ISA_Facettes = 11;

//! Controls the deviation allowed when computing primitives for meridian and parallel circles.
static constexpr double ISA_CircleDeviationCoefficient = 1e-4;

} // namespace

namespace ViewCae {

// =======================================================================
// function : InteractiveSphericalAxes
// =======================================================================
InteractiveSphericalAxes::InteractiveSphericalAxes(const Handle(ViewBase::InteractiveOwner)& theOwner)
{
    // Initialize aspects
    initColorMatAspect(myAxesXAspect, ISA_XAxisColor);
    initColorMatAspect(myAxesYAspect, ISA_YAxisColor);
    initColorMatAspect(myAxesZAspect, ISA_ZAxisColor);
    initColorMatAspect(myAngularAxesAspect, {Quantity_NOC_GRAY20});
    initColorMatAspect(myCenterSphereAspect, {Quantity_NOC_WHITE});

    initColorCircleAspect(myCircleDefaultAspect, {Quantity_NOC_GRAY50});
    initColorCircleAspect(myCircleXAspect, ISA_XAxisColor);
    initColorCircleAspect(myCircleYAspect, ISA_YAxisColor);
    initColorCircleAspect(myCircleZAspect, ISA_ZAxisColor);

    theOwner->SetSelectablePointer(this);
    myOwner = theOwner;
}

// =======================================================================
// function : ~InteractiveSphericalAxes
// =======================================================================
InteractiveSphericalAxes::~InteractiveSphericalAxes()
{
    //
}

// =======================================================================
// function : initColorMatAspect
// =======================================================================
void InteractiveSphericalAxes::initColorMatAspect(Handle(Graphic3d_AspectFillArea3d)& theAspect,
                                                  const Quantity_Color&               theColor)
{
    theAspect = new Graphic3d_AspectFillArea3d(Aspect_IS_SOLID,
                                               Quantity_Color(Quantity_NOC_BLACK),
                                               Quantity_Color(Quantity_NOC_BLACK),
                                               Aspect_TOL_EMPTY,
                                               1.0,
                                               Graphic3d_MaterialAspect(Graphic3d_NameOfMaterial_Charcoal),
                                               Graphic3d_MaterialAspect(Graphic3d_NameOfMaterial_Charcoal));

    theAspect->ChangeFrontMaterial().SetColor(theColor);
}

// =======================================================================
// function : initColorCircleAspect
// =======================================================================
void InteractiveSphericalAxes::initColorCircleAspect(Handle(Graphic3d_AspectLine3d)& theAspect,
                                                     const Quantity_Color&           theColor)
{
    theAspect = new Graphic3d_AspectLine3d(theColor, Aspect_TOL_SOLID, 2.0);
}

// =======================================================================
// function : drawCircle
// =======================================================================
void InteractiveSphericalAxes::drawCircle(const Handle(Prs3d_Presentation)&     thePrs,
                                          const double                          theRadius,
                                          const gp_Trsf&                        theTrsf,
                                          const Handle(Graphic3d_AspectLine3d)& theAspect)
{
    gp_Ax2 aLocOrigin;
    switch (myCirclesOrientation) {
        case AxisType::OX: aLocOrigin = gp::ZOX(); break;
        case AxisType::OY: aLocOrigin = gp::XOY(); break;
        case AxisType::OZ: aLocOrigin = gp::YOZ(); break;
        default: break;
    }
    Handle(Geom_Circle) aGeomCircle = new Geom_Circle(aLocOrigin.Transformed(theTrsf), theRadius);
    GeomAdaptor_Curve   aCurve(aGeomCircle);

    Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
    StdPrs_DeflectionCurve::Add(thePrs, aCurve, ISA_CircleDeviationCoefficient, myDrawer->MaximalParameterValue());

    // Override the group aspect previously given by StdPrs_DeflectionCurve::Add().
    aGroup->SetGroupPrimitivesAspect(theAspect);
}

// =======================================================================
// function : drawText
// =======================================================================
void InteractiveSphericalAxes::drawText(const Handle(Prs3d_Presentation)& thePrs,
                                        const TCollection_ExtendedString& theString,
                                        const gp_Pnt&                     thePos,
                                        const float                       theHeight)
{
    Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();

    Handle(Graphic3d_Text) aText = new Graphic3d_Text(theHeight);
    aText->SetText(theString);
    aText->SetPosition(thePos);
    aText->SetHorizontalAlignment(Graphic3d_HTA_CENTER);
    aText->SetVerticalAlignment(Graphic3d_VTA_CENTER);

    aGroup->SetGroupPrimitivesAspect(myDrawer->TextAspect()->Aspect());
    aGroup->AddText(aText);
}

//! Rudimentary point/vertex transform for creating curved angle arrows.
//! Not generic code. Only behaves correctly for the places it is used in.
//! The code projects input point onto a cartesian axis governed by @p theProjDir
//! argument, then adds a component governed by rotation axis and rotates the
//! point by rotation angle.
//! @param[in] theInPoint input point
//! @param[in] theProjDir direction onto which the point is projected;
//!                       must be either (0, 0, 1) or (0, 1, 0).
//! @param[in] theRotAxis rotation axis; must be either (0, 1, 0) or (0, 0, 1).
static gp_Pnt curveTransformPoint(const gp_Pnt& theInPoint, const gp_Dir& theProjDir, const gp_Dir& theRotAxis)
{
    // gp_XYZ aliases for shorter math
    const gp_XYZ anInPoint{theInPoint.XYZ()};
    const gp_XYZ aProjDir{theProjDir.XYZ()};
    const gp_XYZ aRotAxis{theRotAxis.XYZ()};

    gp_XYZ aResultPoint{aProjDir.Multiplied(anInPoint)};
    aResultPoint = aResultPoint + aRotAxis.Multiplied(anInPoint);

    // The rotation angle is one of the components of the original point.
    // The expression below chooses the right component.
    Standard_Real aRotAngle = theRotAxis.Crossed(theProjDir).Dot(anInPoint);

    gp_Trsf aTrsf;
    aTrsf.SetRotation(gp_Ax1(gp::Origin(), theRotAxis), aRotAngle);
    aTrsf.Transforms(aResultPoint);
    return gp_Pnt(aResultPoint);
}

//! Transform a direction by a start and end point of corresponding vector.
//! The reason for this is that the intensity of rotation depends on the place the direction is used at.
static gp_Dir curveTransformDir(const gp_XYZ& theStart,
                                const gp_XYZ& theEnd,
                                const gp_Dir& theProjDir,
                                const gp_Dir& theRotAxis)
{
    gp_Pnt aStartPoint = curveTransformPoint(theStart, theProjDir, theRotAxis);
    gp_Pnt anEndPoint  = curveTransformPoint(theEnd, theProjDir, theRotAxis);
    return gp_Vec(aStartPoint, anEndPoint);
}

//! Generate primitives for the curved arrows that indicate angle coordinates.
//! There are two parts to it: the arrow tube and the arrow tip.
//! Each vertex of the tube gets transformed and the tip's position and direction gets calculated separately.
static Handle(Graphic3d_ArrayOfTriangles) getCurvedArrowTris(const gp_Ax1&          theAxis,
                                                             const gp_Dir&          theProjDir,
                                                             const Standard_Real    theTubeRadius,
                                                             const Standard_Real    theAxisLength,
                                                             const Standard_Real    theConeRadius,
                                                             const Standard_Real    theConeLength,
                                                             const Standard_Integer theNbFacettes,
                                                             const Standard_Integer theNbCrossSections)
{
    const Standard_Real aTubeLength = Max(0.0, theAxisLength - theConeLength);

    // Get number of triangles.
    const size_t aNbTrisTube = Prs3d_ToolCylinder::TrianglesNb(theNbFacettes, theNbCrossSections);
    const size_t aNbTrisCone = Prs3d_ToolDisk::TrianglesNb(theNbFacettes, 1)
                             + Prs3d_ToolCylinder::TrianglesNb(theNbFacettes, 1);
    const size_t aNbTrisTotal = aNbTrisTube + aNbTrisCone;
    if (aNbTrisTotal == 0) {
        return Handle(Graphic3d_ArrayOfTriangles)();
    }

    // Get number of vertices.
    const size_t aNbVertsTube = Prs3d_ToolCylinder::VerticesNb(theNbFacettes, theNbCrossSections);
    const size_t aNbVertsCone = Prs3d_ToolDisk::VerticesNb(theNbFacettes, 1)
                              + Prs3d_ToolCylinder::VerticesNb(theNbFacettes, 1);
    const size_t aNbVertsTotal = aNbVertsTube + aNbVertsCone;
    if (aNbVertsTotal == 0) {
        return Handle(Graphic3d_ArrayOfTriangles)();
    }

    // Choose appropriate variables for applying arrow curvature transform.
    const gp_Dir aRotAxis = theProjDir ^ theAxis.Direction();

    Handle(Graphic3d_ArrayOfTriangles) aResultArray = new Graphic3d_ArrayOfTriangles(static_cast<int>(aNbVertsTotal),
                                                                                     static_cast<int>(aNbTrisTotal * 3),
                                                                                     Graphic3d_ArrayFlags_VertexNormal);

    // Generate primitives for arrow tube. We need some crossections along the length to manifest curvature later.
    if (aNbTrisTube != 0) {
        gp_Ax3  aSystem(theAxis.Location(), theAxis.Direction());
        gp_Trsf aTrsf;
        aTrsf.SetTransformation(aSystem, gp_Ax3());

        Prs3d_ToolCylinder aTool(theTubeRadius,
                                 theTubeRadius,
                                 aTubeLength + theTubeRadius,
                                 theNbFacettes,
                                 theNbCrossSections);
        aTool.FillArray(aResultArray, aTrsf);
    }

    // Generate primitives for arrow tip.
    if (aNbTrisCone != 0) {
        gp_Pnt aConeOrigin = theAxis.Location().Translated(gp_Vec(theAxis.Direction().X() * aTubeLength,
                                                                  theAxis.Direction().Y() * aTubeLength,
                                                                  theAxis.Direction().Z() * aTubeLength));

        // Origin and direction in curved space.
        gp_Pnt aConeOriginCurved = curveTransformPoint(aConeOrigin, theProjDir, aRotAxis);
        gp_Dir aConeDirCurved    = curveTransformDir(aConeOrigin.XYZ(),
                                                  aConeOrigin.XYZ() + theAxis.Direction().XYZ() * theTubeRadius,
                                                  theProjDir,
                                                  aRotAxis);

        gp_Ax3  aSystem(aConeOriginCurved, aConeDirCurved);
        gp_Trsf aTrsf;
        aTrsf.SetTransformation(aSystem, gp_Ax3());
        Prs3d_ToolDisk aToolDisk(0.0, theConeRadius, theNbFacettes, 1);
        aToolDisk.FillArray(aResultArray, aTrsf);
        Prs3d_ToolCylinder aToolCyl(theConeRadius, 0.0, theConeLength, theNbFacettes, 1);
        aToolCyl.FillArray(aResultArray, aTrsf);
    }

    // Apply a transform to each vertex of the tube to curve it.
    for (size_t anIter = 1; anIter <= aNbVertsTube; anIter++) {
        const gp_Pnt anOrigVertex = aResultArray->Vertice(static_cast<int>(anIter));
        aResultArray->SetVertice(static_cast<int>(anIter), curveTransformPoint(anOrigVertex, theProjDir, aRotAxis));
        // Ignore the need for normal transformation.
    }

    return aResultArray;
}

//! Convert AxisType enum into gp_Dir
static gp_Dir axisToDir(const InteractiveSphericalAxes::AxisType theAxis)
{
    switch (theAxis) {
        case InteractiveSphericalAxes::AxisType::OX: return gp::DX();
        case InteractiveSphericalAxes::AxisType::OY: return gp::DY();
        case InteractiveSphericalAxes::AxisType::OZ: return gp::DZ();
        default: return gp_Dir(0, 0, 0);
    }
}

//! Convert AxisType enum into corresponding rotation direction
static gp_Dir orientationToDir(const InteractiveSphericalAxes::ArrowOrientation theDir)
{
    switch (theDir) {
        case InteractiveSphericalAxes::ArrowOrientation::XPos: return gp::DX();
        case InteractiveSphericalAxes::ArrowOrientation::XNeg: return -gp::DX();
        case InteractiveSphericalAxes::ArrowOrientation::YPos: return gp::DY();
        case InteractiveSphericalAxes::ArrowOrientation::YNeg: return -gp::DY();
        case InteractiveSphericalAxes::ArrowOrientation::ZPos: return gp::DZ();
        case InteractiveSphericalAxes::ArrowOrientation::ZNeg: return -gp::DZ();
        default: return gp_Dir(0, 0, 0);
    }
}

// =======================================================================
// function : getAngularAxisPlacement
// =======================================================================
gp_Ax1 InteractiveSphericalAxes::getAngularAxisPlacement(const AxisType theAxis) const
{
    Standard_ProgramError_Raise_if(theAxis >= AxisType::NbAngular,
                                   "InteractiveSphericalAxes::getAngularAxisPlacement(): invalid angular axis.");

    static constexpr double aStraightAxisLength = 1.2 * ISA_Radius;
    static constexpr double anAngleArrowPos     = 0.9 * aStraightAxisLength;

    gp_Dir anAttachmentDir = axisToDir(GetAngularArrowAxis(theAxis));
    gp_Dir aRotationDir    = orientationToDir(GetAngularArrowDir(theAxis));
    gp_Ax1 anAxisPlacement(anAttachmentDir.XYZ() * anAngleArrowPos, aRotationDir ^ anAttachmentDir);
    return anAxisPlacement;
}

// =======================================================================
// function : drawArrow
// =======================================================================
void InteractiveSphericalAxes::drawArrow(const Handle(Prs3d_Presentation)&         thePrs,
                                         const AxisType                            theAxis,
                                         const double                              theLength,
                                         const Handle(Graphic3d_AspectFillArea3d)& theAspect)
{
    static constexpr double aTubeRadius = 0.005 * ISA_Radius;
    static constexpr double aConeRadius = 0.030 * ISA_Radius;
    static constexpr double aConeLength = 0.100 * ISA_Radius;

    Handle(Graphic3d_ArrayOfTriangles) anArrowTris;
    if (theAxis > AxisType::NbAngular) {
        gp_Ax1 anAxis(gp::Origin(), axisToDir(theAxis));
        anArrowTris = Prs3d_Arrow::DrawShaded(anAxis, aTubeRadius, theLength, aConeRadius, aConeLength, ISA_Facettes);
    } else if (theAxis < AxisType::NbAngular) {
        gp_Ax1 anAxis   = getAngularAxisPlacement(theAxis);
        gp_Dir aProjDir = axisToDir(GetAngularArrowAxis(theAxis));
        anArrowTris     = getCurvedArrowTris(anAxis,
                                         aProjDir,
                                         aTubeRadius,
                                         theLength,
                                         aConeRadius,
                                         aConeLength,
                                         ISA_Facettes,
                                         ISA_Facettes);
    }

    if (anArrowTris.IsNull())
        return;

    Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
    aGroup->SetGroupPrimitivesAspect(theAspect);
    aGroup->AddPrimitiveArray(anArrowTris);
}

// =======================================================================
// function : drawCenterSphere
// =======================================================================
void InteractiveSphericalAxes::drawCenterSphere(const Handle(Prs3d_Presentation)& thePrs)
{
    Handle(Graphic3d_ArrayOfTriangles) aSphereTris =
        Prs3d_ToolSphere::Create(0.0075 * ISA_Radius, ISA_Facettes, ISA_Facettes, gp_Trsf());
    Handle(Graphic3d_Group) aSphGroup = thePrs->NewGroup();
    aSphGroup->SetGroupPrimitivesAspect(myCenterSphereAspect);
    aSphGroup->AddPrimitiveArray(aSphereTris);
}

// =======================================================================
// function : getLongAspect
// =======================================================================
Handle(Graphic3d_AspectLine3d) InteractiveSphericalAxes::getLongAspect(const size_t theIter) const
{
    switch (theIter) {
        case 0:
            switch (myCirclesOrientation) {
                case AxisType::OX: return myCircleYAspect;
                case AxisType::OY: return myCircleZAspect;
                case AxisType::OZ: return myCircleXAspect;
                default: return myCircleDefaultAspect;
            }
            break;
        case 2:
            switch (myCirclesOrientation) {
                case AxisType::OX: return myCircleZAspect;
                case AxisType::OY: return myCircleXAspect;
                case AxisType::OZ: return myCircleYAspect;
                default: return myCircleDefaultAspect;
            }
            break;
        default: return myCircleDefaultAspect;
    }
}

// =======================================================================
// function : getLatAspect
// =======================================================================
Handle(Graphic3d_AspectLine3d) InteractiveSphericalAxes::getLatAspect(const size_t theIter) const
{
    if (theIter == 3) {
        switch (myCirclesOrientation) {
            case AxisType::OX: return myCircleXAspect;
            case AxisType::OY: return myCircleYAspect;
            case AxisType::OZ: return myCircleZAspect;
            default: return myCircleDefaultAspect;
        }
    }
    return myCircleDefaultAspect;
}

// =======================================================================
// function : getLongTrsf
// =======================================================================
gp_Trsf InteractiveSphericalAxes::getLongTrsf(const size_t theIter) const
{
    gp_Ax1 aRotAxis;
    switch (myCirclesOrientation) {
        case AxisType::OX: aRotAxis = gp::OX(); break;
        case AxisType::OY: aRotAxis = gp::OY(); break;
        case AxisType::OZ: aRotAxis = gp::OZ(); break;
        default: break;
    }
    gp_Trsf aRotTrsf;
    aRotTrsf.SetRotation(aRotAxis, theIter * M_PI_4);
    return aRotTrsf;
}

// =======================================================================
// function : getLatTrsf
// =======================================================================
gp_Trsf InteractiveSphericalAxes::getLatTrsf(const Standard_Real theCenter) const
{
    gp_Trsf aTrsf;
    switch (myCirclesOrientation) {
        case AxisType::OX: aTrsf.SetTransformation(gp_Quaternion(gp::DZ(), M_PI_2), {theCenter, 0.0, 0.0}); break;
        case AxisType::OY: aTrsf.SetTransformation(gp_Quaternion(gp::DX(), M_PI_2), {0.0, theCenter, 0.0}); break;
        case AxisType::OZ: aTrsf.SetTransformation(gp_Quaternion(gp::DY(), M_PI_2), {0.0, 0.0, theCenter}); break;
        default: break;
    }
    return aTrsf;
}

// =======================================================================
// function : drawLatLongLines
// =======================================================================
void InteractiveSphericalAxes::drawLatLongLines(const Handle(Prs3d_Presentation)& thePrs)
{
    // Draw longitude meridians, coloring them if they are perpendicular to some axis.
    for (size_t anIter = 0; anIter < 4; anIter++) {
        drawCircle(thePrs, ISA_Radius, getLongTrsf(anIter), getLongAspect(anIter));
    }

    // Draw latitude parallels
    for (size_t anIter = 1; anIter <= 5; anIter++) {
        Standard_Real aLatitudeAngle   = anIter * M_PI / 6.0;
        Standard_Real aCenterCoord     = ISA_Radius * std::cos(aLatitudeAngle);
        Standard_Real aLatCircleRadius = ISA_Radius * std::sin(aLatitudeAngle);
        drawCircle(thePrs, aLatCircleRadius, getLatTrsf(aCenterCoord), getLatAspect(anIter));
    }
}

// =======================================================================
// function : Compute
// =======================================================================
void InteractiveSphericalAxes::Compute(const Handle(PrsMgr_PresentationManager)&,
                                       const Handle(Prs3d_Presentation)& thePrs,
                                       const Standard_Integer            theMode)
{
    if (!AcceptDisplayMode(theMode)) {
        return;
    }

    updateLocalTransform();

    static constexpr double anAxisLength = 1.2 * ISA_Radius;

    // Draw cartesian coordinate axes
    drawArrow(thePrs, AxisType::OX, anAxisLength, myAxesXAspect);
    drawArrow(thePrs, AxisType::OY, anAxisLength, myAxesYAspect);
    drawArrow(thePrs, AxisType::OZ, anAxisLength, myAxesZAspect);

    const double anAngleArrowLength = 0.3 * anAxisLength;

    // Draw curved arrows to indicate spherical angle coordinate directions.
    const Quantity_Color anAngleAxisColor(Quantity_NOC_GRAY20);
    drawArrow(thePrs, AxisType::Angular1, anAngleArrowLength, myAngularAxesAspect);
    drawArrow(thePrs, AxisType::Angular2, anAngleArrowLength, myAngularAxesAspect);

    // Create a small sphere on axis origin to cover awkward geometry
    drawCenterSphere(thePrs);

    // Draw latitude and longitude rings.
    // Longitude meridians are 45 degrees apart.
    // Latitude parallels are 30 degrees apart.
    drawLatLongLines(thePrs);

    // Draw cartesian axes labels.
    drawText(thePrs, "X", {anAxisLength + 0.05, 0.0, 0.0}, 1.0f);
    drawText(thePrs, "Y", {0.0, anAxisLength + 0.05, 0.0}, 1.0f);
    drawText(thePrs, "Z", {0.0, 0.0, anAxisLength + 0.05}, 1.0f);

    // Draw angular axes labels.
    auto drawLocatedText = [this, &thePrs, anAngleArrowLength](const AxisType theAxis) {
        const gp_Ax1 anAxisPlacement = getAngularAxisPlacement(theAxis);
        const gp_Pnt anAnchor        = anAxisPlacement.Location();
        drawText(thePrs,
                 GetAngularArrowLabel(theAxis),
                 anAnchor.Translated(gp_Vec(anAxisPlacement.Direction()).Scaled(anAngleArrowLength)),
                 0.2f);
    };

    drawLocatedText(AxisType::Angular1);
    drawLocatedText(AxisType::Angular2);
}

// =======================================================================
// function : ComputeSelection
// =======================================================================
void InteractiveSphericalAxes::ComputeSelection(const Handle(SelectMgr_Selection)&, const Standard_Integer)
{
    //
}

} // namespace