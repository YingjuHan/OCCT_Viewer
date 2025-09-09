#ifndef _INTERACTIVESCALERULER_HEADERFILE
#define _INTERACTIVESCALERULER_HEADERFILE

#include <ViewBase/IInteractiveObject.h>
#include <ViewBase/InteractiveScaleRulerOwner.h>

#include <AIS_ScaleRuler.hxx>

namespace ViewBase
{

    //! Scale ruler presentation - auxiliary interactive object for displaying length.
    class InteractiveScaleRuler : public AIS_ScaleRuler, public IInteractiveObject
    {
        InteractiveScaleRuler() = delete;
        InteractiveScaleRuler(const InteractiveScaleRuler &) = delete;
        InteractiveScaleRuler &operator=(const InteractiveScaleRuler &) = delete;

    public:
        //! Public constructor
        Standard_EXPORT InteractiveScaleRuler(const Handle(ViewBase::InteractiveScaleRulerOwner) & theNewId,
                                              const Handle(Prs3d_TextAspect) & theTextStyle);

        //! Public destructor
        Standard_EXPORT virtual ~InteractiveScaleRuler();

    public:
        //! Return size of 2D objects to apply offset when arranging other 2D objects.
        virtual Graphic3d_Vec2i Get2dObjectSize() const override
        {
            if (myTransformPersistence.IsNull() || !myTransformPersistence->IsTrihedronOr2d())
            {
                return Graphic3d_Vec2i(0);
            }

            return Graphic3d_Vec2i((Graphic3d_Vec2d(myTransformPersistence->Offset2d()) / myDevicePixelRatio) + Graphic3d_Vec2d(BaseSize()));
        }

        //! Update presentation to new pixel scale ratio.
        Standard_EXPORT virtual bool OnPixelScaleChange(const Handle(AIS_InteractiveObject) & theObj,
                                                        float theRatio) override;

        //! Return name of this type.
        virtual const char *TypeName() const override { return "InteractiveScaleRuler"; }
    };

}

#endif // _INTERACTIVESCALERULER_HEADERFILE