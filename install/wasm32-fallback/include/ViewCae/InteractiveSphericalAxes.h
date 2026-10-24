#ifndef ViewCAE_INTERACTIVESPHERICALAXES_HEADERFILE
#define ViewCAE_INTERACTIVESPHERICALAXES_HEADERFILE

#include <ViewBase/IInteractiveObject.h>
#include <ViewBase/InteractiveOwner.h>

#include <RWBase/ValueConverter.h>

#include <Graphic3d_AspectLine3d.hxx>

namespace ViewCae {

//! Object for displaying a spherical coordinate system representation.
//! Has three axes (OX, OY, OZ), as well as two curved angular arrows
//! to mark the zero value and direction of two additional angles.
//! Also has longitude meridians and latitude parallels for visual reference.
//! The following elements of the 3D presentation are parameterized:
//! - Positions and directions of the angular arrows;
//! - The angle labels;
//! - Orientation of the longitude and latitude lines.
//! By default, the following configuration is used that corresponds to the way
//! a spherical coordinate system is defined in physics simulations:
//! - Angular1: Theta angle, has YPos orientation and starts from OZ axis;
//! - Angular2: Phi angle, has ZPos orientation and starts from OX axis.
class InteractiveSphericalAxes : public AIS_InteractiveObject, public ViewBase::IInteractiveObject {
public:
    //! Enumeration for supported axes.
    enum class AxisType {
        Angular1,  //!< Angular axis 1
        Angular2,  //!< Angular axis 2
        NbAngular, //!< Number of supported angular axes
        OX,        //!< Cartesian axis OX
        OY,        //!< Cartesian axis OY
        OZ,        //!< Cartesian axis OZ
        Unknown = NbAngular
    };

    //! Enumeration for angular arrow orientations.
    enum class ArrowOrientation {
        XPos, //!< The arrow in YOZ plane in counterclockwise direction
        XNeg, //!< The arrow in YOZ plane in clockwise direction
        YPos, //!< The arrow in XOZ plane in counterclockwise direction
        YNeg, //!< The arrow in XOZ plane in clockwise direction
        ZPos, //!< The arrow in XOY plane in counterclockwise direction
        ZNeg, //!< The arrow in XOY plane in clockwise direction
        Unknown
    };

    //! Structure for angular arrow parameters
    struct AngularAxesParams {
        ArrowOrientation           Orientation;
        AxisType                   ZeroAxis;
        TCollection_ExtendedString Label;
    };

public:
    //! Constructor.
    Standard_EXPORT explicit InteractiveSphericalAxes(const Handle(ViewBase::InteractiveOwner)& theOwner);

    //! Destructor.
    Standard_EXPORT virtual ~InteractiveSphericalAxes();

    //! Return name of this type.
    virtual const char* TypeName() const override { return "InteractiveSphericalAxes"; }

public:
    //! Return TRUE for supported display mode.
    virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const override { return theMode == 0; }

    //! Computes presentation.
    Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                         const Handle(Prs3d_Presentation)&         thePrs,
                                         const Standard_Integer                    theMode) override;

    //! Computes empty selection.
    Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSel,
                                                  const Standard_Integer             theMode) override;

public:
    //! Returns the position of the center of coordinates.
    gp_XYZ GetPosition() { return myPosition; }

    //! Sets the position of the center of coordinates.
    void SetPosition(const gp_XYZ& thePos)
    {
        myPosition = thePos;
        updateLocalTransform();
    }

    //! Returns the scaling factor of the coordinate system.
    double GetScale() { return myScale; }

    //! Sets the scaling factor of the coordinate system.
    void SetScale(double theScale)
    {
        Standard_ProgramError_Raise_if(theScale < Precision::Confusion(),
                                       "InteractiveSphericalAxes::SetScale(): Attempt to set invalid scale.");
        myScale = theScale;
        updateLocalTransform();
    }

    //! Return the direction of the curved arrow for an angular axis defined with a rotation axis and
    //! direction (positive means counterclockwise). @sa ArrowOrientation.
    ArrowOrientation GetAngularArrowDir(const AxisType theAxis) const
    {
        Standard_ProgramError_Raise_if(theAxis >= AxisType::NbAngular,
                                       "InteractiveSphericalAxes::GetAngularArrowDir(): invalid angular axis.");
        return myAngularAxesParams[static_cast<int>(theAxis)].Orientation;
    }

    //! Set the direction of the curved arrow for an angular axis. @sa ArrowOrientation.
    void SetAngularArrowDir(const AxisType theAxis, const ArrowOrientation theDir)
    {
        Standard_ProgramError_Raise_if(theAxis >= AxisType::NbAngular,
                                       "InteractiveSphericalAxes::SetAngularArrowDir(): invalid angular axis.");
        if (myAngularAxesParams[static_cast<int>(theAxis)].Orientation == theDir) {
            return;
        }
        myAngularAxesParams[static_cast<int>(theAxis)].Orientation = theDir;
        SetToUpdate();
    }

    //! Return the principal coordinate axis, to which a given angular arrow is attached. @sa AxisType.
    AxisType GetAngularArrowAxis(const AxisType theAxis) const
    {
        Standard_ProgramError_Raise_if(theAxis >= AxisType::NbAngular,
                                       "InteractiveSphericalAxes::GetAngularArrowAxis(): invalid angular axis.");
        return myAngularAxesParams[static_cast<int>(theAxis)].ZeroAxis;
    }

    //! Set the principal coordinate axis, to which a given angular arrow is attached. @sa AxisType.
    void SetAngularArrowAxis(const AxisType theAxis, const AxisType theZeroAxis)
    {
        Standard_ProgramError_Raise_if(theAxis >= AxisType::NbAngular,
                                       "InteractiveSphericalAxes::SetAngularArrowAxis(): invalid angular axis.");
        if (myAngularAxesParams[static_cast<int>(theAxis)].ZeroAxis == theZeroAxis) {
            return;
        }
        myAngularAxesParams[static_cast<int>(theAxis)].ZeroAxis = theZeroAxis;
        SetToUpdate();
    }

    //! Return the text label for a given angular arrow.
    const TCollection_ExtendedString& GetAngularArrowLabel(const AxisType theAxis) const
    {
        Standard_ProgramError_Raise_if(theAxis >= AxisType::NbAngular,
                                       "InteractiveSphericalAxes::GetAngularArrowLabel(): invalid angular axis.");
        return myAngularAxesParams[static_cast<int>(theAxis)].Label;
    }

    //! Set the text label for a given angular arrow.
    void SetAngularArrowLabel(const AxisType theAxis, const TCollection_ExtendedString& theLabel)
    {
        Standard_ProgramError_Raise_if(theAxis >= AxisType::NbAngular,
                                       "InteractiveSphericalAxes::SetAngularArrowLabel(): invalid angular axis.");
        if (myAngularAxesParams[static_cast<int>(theAxis)].Label == theLabel) {
            return;
        }
        myAngularAxesParams[static_cast<int>(theAxis)].Label = theLabel;
        SetToUpdate();
    }

    //! Return the orientation of latitude and longitude circles. OZ orientation means latitude circles
    //! are orthogonal to Z axis and shifted along Z, while longitude circles are rotated about Z axis.
    //! OX and OY orientations work the same way but for X and Y axis, respectively.
    AxisType GetOrientation() const { return myCirclesOrientation; }

    //! Set the orientation of latitude and longitude circles.
    void SetOrientation(const AxisType theAxis)
    {
        Standard_ProgramError_Raise_if(theAxis < AxisType::OX,
                                       "InteractiveSphericalAxes::SetOrientation(): invalid cartesian axis.");
        if (myCirclesOrientation == theAxis) {
            return;
        }
        myCirclesOrientation = theAxis;
        SetToUpdate();
    }

private:
    InteractiveSphericalAxes()                                = delete;
    InteractiveSphericalAxes(const InteractiveSphericalAxes&) = delete;
    InteractiveSphericalAxes& operator=(const InteractiveSphericalAxes&) = delete;

private:
    //! Return line aspect for a longitude circle based on the orientation and step number
    Handle(Graphic3d_AspectLine3d) getLongAspect(const size_t theIter) const;

    //! Return line aspect for a latitude circle based on the orientation and step number
    Handle(Graphic3d_AspectLine3d) getLatAspect(const size_t theIter) const;

    //! Return a transformation for longitude circle based on the orientation and step number
    gp_Trsf getLongTrsf(const size_t theIter) const;

    //! Return a transformation for latitude circle based on the orientation and distance to the center
    gp_Trsf getLatTrsf(Standard_Real theCenter) const;

    //! Computes presentation for a circle made out of segments.
    void drawCircle(const Handle(Prs3d_Presentation)&     thePrs,
                    const double                          theRadius,
                    const gp_Trsf&                        theTrsf,
                    const Handle(Graphic3d_AspectLine3d)& theAspect);

    //! Computes presentation for all latitude and longitude lines.
    void drawLatLongLines(const Handle(Prs3d_Presentation)& thePrs);

    //! Draws text at specified position.
    void drawText(const Handle(Prs3d_Presentation)& thePrs,
                  const TCollection_ExtendedString& theString,
                  const gp_Pnt&                     thePos,
                  const float                       theHeight);

    //! Return the origin and direction of an angular axis.
    gp_Ax1 getAngularAxisPlacement(const AxisType theAxis) const;

    //! Computes presentation for all straight and curved arrows.
    void drawArrow(const Handle(Prs3d_Presentation)&         thePrs,
                   const AxisType                            theAxis,
                   const double                              theLength,
                   const Handle(Graphic3d_AspectFillArea3d)& theAspect);

    //! Computes a presentation for a small sphere in the center of coordinates.
    void drawCenterSphere(const Handle(Prs3d_Presentation)& thePrs);

    //! Updates local transformation as a method to implement support for setting center position and scale.
    void updateLocalTransform()
    {
        gp_Trsf aTrsf;
        aTrsf.SetTranslation(myPosition);
        aTrsf.SetScaleFactor(myScale);
        SetLocalTransformation(aTrsf);
    }

private:
    //! Initializes the specified aspect and assigns it a color.
    void initColorMatAspect(Handle(Graphic3d_AspectFillArea3d)& theAspect, const Quantity_Color& theColor);

    //! Initializes the specified aspect and assigns it a color.
    void initColorCircleAspect(Handle(Graphic3d_AspectLine3d)& theAspect, const Quantity_Color& theColor);

private:
    gp_XYZ                             myPosition{0.0, 0.0, 0.0}; //!< Position of the center of coordinates.
    double                             myScale = 1.0;             //!< Scale of the axes presentation.
    Handle(Graphic3d_AspectFillArea3d) myAxesXAspect;             //!< Aspect for drawing OX axis.
    Handle(Graphic3d_AspectFillArea3d) myAxesYAspect;             //!< Aspect for drawing OY axis.
    Handle(Graphic3d_AspectFillArea3d) myAxesZAspect;             //!< Aspect for drawing OZ axis.
    Handle(Graphic3d_AspectFillArea3d) myAngularAxesAspect;       //!< Aspect for drawing angular coordinate arrows.
    Handle(Graphic3d_AspectFillArea3d) myCenterSphereAspect;      //!< Aspect for drawing center sphere.
    Handle(Graphic3d_AspectLine3d)     myCircleDefaultAspect;     //!< Default circle aspect.
    Handle(Graphic3d_AspectLine3d)     myCircleXAspect;           //!< Aspect for drawing YOZ plane circle.
    Handle(Graphic3d_AspectLine3d)     myCircleYAspect;           //!< Aspect for drawing XOZ plane circle.
    Handle(Graphic3d_AspectLine3d)     myCircleZAspect;           //!< Aspect for drawing XOY plane circle.
    AxisType                           myCirclesOrientation{AxisType::OZ}; //!< Axis of latitude/longitude circles
    //! Parameters of angular axes
    AngularAxesParams myAngularAxesParams[static_cast<int>(AxisType::NbAngular)] =
        {{ArrowOrientation::YPos, AxisType::OZ, "Theta"}, {ArrowOrientation::ZPos, AxisType::OX, "Phi"}};
};

using AxisType = InteractiveSphericalAxes::AxisType;

//! Bidirectional mapper between AxisType enumeration items and strings.
class AxisTypeConverter : public RWBase::ValueConverter<AxisType, AxisTypeConverter> {
private:
    //! Default constructor
    AxisTypeConverter()
    {
        valueMap().Bind(ViewCae::AxisType::Angular1, "angular1");
        valueMap().Bind(ViewCae::AxisType::Angular2, "angular2");
        valueMap().Bind(ViewCae::AxisType::OX, "ox");
        valueMap().Bind(ViewCae::AxisType::OY, "oy");
        valueMap().Bind(ViewCae::AxisType::OZ, "oz");
        valueMap().Bind(ViewCae::AxisType::Unknown, "");
    }

    friend class RWBase::ValueConverter<AxisType, AxisTypeConverter>;
};

using ArrowOrientation = InteractiveSphericalAxes::ArrowOrientation;

//! Bidirectional mapper between ArrowOrientation enumeration items and strings.
class ArrowOrientationConverter : public RWBase::ValueConverter<ArrowOrientation, ArrowOrientationConverter> {
private:
    //! Default constructor
    ArrowOrientationConverter()
    {
        valueMap().Bind(ViewCae::ArrowOrientation::XNeg, "-x");
        valueMap().Bind(ViewCae::ArrowOrientation::XPos, "+x");
        valueMap().Bind(ViewCae::ArrowOrientation::YNeg, "-y");
        valueMap().Bind(ViewCae::ArrowOrientation::YPos, "+y");
        valueMap().Bind(ViewCae::ArrowOrientation::ZNeg, "-z");
        valueMap().Bind(ViewCae::ArrowOrientation::ZPos, "+z");
        valueMap().Bind(ViewCae::ArrowOrientation::Unknown, "");
    }

    friend class RWBase::ValueConverter<ArrowOrientation, ArrowOrientationConverter>;
};

} // namespace

#endif // ViewCAE_INTERACTIVESPHERICALAXES_HEADERFILE