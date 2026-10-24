#ifndef _INTERACTIVERADIUSDIMENSION_HEADERFILE
#define _INTERACTIVERADIUSDIMENSION_HEADERFILE

#include <ViewBase/IInteractiveObject.h>

#include <ViewDim/InteractiveDimensionOwner.h>

#include <PrsDim_RadiusDimension.hxx>

namespace ViewDim
{

    //! Radius dimension presentation.
    class InteractiveRadiusDimension : public PrsDim_RadiusDimension, public ViewBase::IInteractiveObject
    {
        InteractiveRadiusDimension() = delete;
        InteractiveRadiusDimension(const InteractiveRadiusDimension &) = delete;
        InteractiveRadiusDimension &operator=(const InteractiveRadiusDimension &) = delete;

    public:
        //! Public constructor.
        Standard_EXPORT explicit InteractiveRadiusDimension(const Handle(ViewDim::InteractiveDimensionOwner) & theNewId);

        //! Public destructor.
        Standard_EXPORT virtual ~InteractiveRadiusDimension();

        //! Return name of this type (shared between dimensions!).
        virtual const char *TypeName() const override { return "InteractiveDimension"; }

        //! Presentation builder.
        Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Standard_Integer theMode) Standard_OVERRIDE;
    };

}

#endif // _INTERACTIVERADIUSDIMENSION_HEADERFILE