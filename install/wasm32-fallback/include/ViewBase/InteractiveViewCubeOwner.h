#ifndef _INTERACTIVE_VIEW_CUBE_OWNER_H_
#define _INTERACTIVE_VIEW_CUBE_OWNER_H_

#include <ViewBase/InteractiveOwner.h>

namespace ViewBase
{
    class InteractiveViewCubeOwner : public InteractiveOwner
    {
    public:
        Standard_EXPORT explicit InteractiveViewCubeOwner(const TCollection_AsciiString &theId);

        Standard_EXPORT virtual Handle(ViewBase::InteractiveOwner) Clone(const TCollection_AsciiString &theId);
    };
}

#endif