#include <ViewBase/IInteractiveObject.h>
#include <ViewBase/InteractiveViewCubeOwner.h>

#include <AIS_ViewCube.hxx>

namespace ViewBase
{

    //! View Cube presentation - auxiliary interactive object for displaying
    //! and changing camera (V3d_View) orientation.
    //!
    //! This class extends AIS_ViewCube implementation with customized look
    //! and handling of pixel scale changes.
    class InteractiveViewCube : public AIS_ViewCube, public IInteractiveObject
    {
        InteractiveViewCube() = delete;
        InteractiveViewCube(const InteractiveViewCube &) = delete;
        InteractiveViewCube &operator=(const InteractiveViewCube &) = delete;

    public:
        //! Public constructor
        Standard_EXPORT InteractiveViewCube(const Handle(ViewBase::InteractiveViewCubeOwner) & theNewId,
                                            const Handle(Prs3d_TextAspect) & theTextStyle);

        //! Public destructor
        Standard_EXPORT virtual ~InteractiveViewCube();

    public:
        //! Return size of 2D objects to apply offset when arranging other 2D objects.
        virtual Graphic3d_Vec2i Get2dObjectSize() const override
        {
            if (myTransformPersistence.IsNull() || !myTransformPersistence->IsTrihedronOr2d())
            {
                return Graphic3d_Vec2i(0);
            }

            return Graphic3d_Vec2i((Graphic3d_Vec2d(myTransformPersistence->Offset2d())) + Graphic3d_Vec2d(mySize));
        }

        //! Return name of this type.
        virtual const char *TypeName() const override { return "InteractiveViewCube"; }
    };

}