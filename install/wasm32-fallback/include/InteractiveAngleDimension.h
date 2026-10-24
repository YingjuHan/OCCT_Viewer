#ifndef _INTERACTIVEANGLEDIMENSION_HEADERFILE
#define _INTERACTIVEANGLEDIMENSION_HEADERFILE

#include <ViewBase/IInteractiveObject.h>

#include <ViewDim/InteractiveDimensionOwner.h>

#include <PrsDim_AngleDimension.hxx>

namespace ViewDim
{

    //! Angle dimension presentation.
    class InteractiveAngleDimension : public PrsDim_AngleDimension, public ViewBase::IInteractiveObject
    {
        InteractiveAngleDimension() = delete;
        InteractiveAngleDimension(const InteractiveAngleDimension &) = delete;
        InteractiveAngleDimension &operator=(const InteractiveAngleDimension &) = delete;

    public:
        //! Public constructor.
        Standard_EXPORT explicit InteractiveAngleDimension(const Handle(ViewDim::InteractiveDimensionOwner) & theNewId);

        //! Public destructor.
        Standard_EXPORT virtual ~InteractiveAngleDimension();

        //! Return name of this type (shared between dimensions!).
        virtual const char *TypeName() const override { return "InteractiveDimension"; }

        //! Presentation builder.
        Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Standard_Integer theMode) Standard_OVERRIDE;
    };

}

#endif // _INTERACTIVEANGLEDIMENSION_HEADERFILE
