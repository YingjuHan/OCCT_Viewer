#ifndef INTERACTIVECLIPPINGOWNER_HEADERFILE
#define INTERACTIVECLIPPINGOWNER_HEADERFILE

#include <ViewBase/InteractiveOwner.h>

namespace ViewBase
{

    //! Class represents owner for @link InteractiveClipping @endlink object.
    class InteractiveClippingOwner : public InteractiveOwner
    {
    public:
        //! Constructor.
        Standard_EXPORT explicit InteractiveClippingOwner(const TCollection_AsciiString &theId);

        //! Return copy of this presentation; default implementation returns NULL.
        Standard_EXPORT virtual Handle(ViewBase::InteractiveOwner) Clone(const TCollection_AsciiString &theNewId) override;
    };

} // namespace

#endif // INTERACTIVECLIPPINGOWNER_HEADERFILE
