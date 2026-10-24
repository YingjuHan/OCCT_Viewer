
#ifndef _INTERACTIVELENGTHDIMENSION_HEADERFILE
#define _INTERACTIVELENGTHDIMENSION_HEADERFILE

#include <ViewBase/IInteractiveObject.h>

#include <ViewDim/InteractiveDimensionOwner.h>

#include <PrsDim_LengthDimension.hxx>

namespace ViewDim
{

    //! Length dimension presentation.
    class InteractiveLengthDimension : public PrsDim_LengthDimension, public ViewBase::IInteractiveObject
    {
        InteractiveLengthDimension() = delete;
        InteractiveLengthDimension(const InteractiveLengthDimension &) = delete;
        InteractiveLengthDimension &operator=(const InteractiveLengthDimension &) = delete;

    public:
        //! Public constructor.
        Standard_EXPORT explicit InteractiveLengthDimension(const Handle(ViewDim::InteractiveDimensionOwner) & theNewId);

        //! Public destructor.
        Standard_EXPORT virtual ~InteractiveLengthDimension();

        //! Return name of this type (shared between dimensions!).
        virtual const char *TypeName() const override { return "InteractiveDimension"; }

        //! Presentation builder.
        Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Standard_Integer theMode) Standard_OVERRIDE;
    };

}

#endif // _INTERACTIVELENGTHDIMENSION_HEADERFILE