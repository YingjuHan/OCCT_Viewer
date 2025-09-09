#ifndef INTERACTIVECOLORSCALEOWNER_HEADERFILE
#define INTERACTIVECOLORSCALEOWNER_HEADERFILE

#include <ViewCae/MeshColorScaleMapper.h>

#include <ViewBase/InteractiveOwner.h>

namespace ViewCae {
//! Class represents owner for ViewCae::InteractiveColorScale interactive object.
class InteractiveColorScaleOwner : public ViewBase::InteractiveOwner {
public:
    //! Constructor.
    Standard_EXPORT InteractiveColorScaleOwner(const TCollection_AsciiString& theId);

    //! Return copy of this presentation; default implementation returns NULL.
    Standard_EXPORT virtual Handle(ViewBase::InteractiveOwner) Clone(const TCollection_AsciiString& theNewId) override;

    //! Return the corresponding color mapper object.
    const Handle(MeshColorScaleMapper)& ColorMapper() const { return myColorMapper; }

    //! Set the corresponding color mapper object.
    void SetColorMapper(const Handle(MeshColorScaleMapper)& theColorMapper) { myColorMapper = theColorMapper; }

    //! Check if selection mode is acceptable; 0 is selectable, 1 otherwise.
    virtual bool AcceptSelectionMode(const int theMode) const override { return theMode == 0 || theMode == 1; }

protected:
    Handle(MeshColorScaleMapper) myColorMapper;
};

} // namespace

#endif // INTERACTIVECOLORSCALEOWNER_HEADERFILE