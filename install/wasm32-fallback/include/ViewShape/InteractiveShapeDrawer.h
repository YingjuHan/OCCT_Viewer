#ifndef INTERACTIVESHAPEDRAWER_HEADERFILE
#define INTERACTIVESHAPEDRAWER_HEADERFILE

#include <Prs3d_Drawer.hxx>
#include <XCAFDoc_VisMaterial.hxx>

namespace ViewShape
{
    //! Class defining presentation attributes for `InteractiveShape` sub-shapes.
    class InteractiveShapeDrawer : public Prs3d_Drawer
    {
    public:
        //! Default constructor.
        Standard_EXPORT InteractiveShapeDrawer();

        //! Destructor..
        Standard_EXPORT virtual ~InteractiveShapeDrawer();

        //! Return transparent material.
        const Handle(XCAFDoc_VisMaterial) & TransparentMat() const { return myTransparentMat; }

        //! Return selected material.
        const Handle(XCAFDoc_VisMaterial) & SelectedMat() const { return mySelectedMat; }

        //! Return color definition.
        const Quantity_ColorRGBA &SemitransparentColor() const { return myTransparentMat->PbrMaterial().BaseColor; }

        //! Set color definition.
        void SetSemitransparentColor(const Quantity_ColorRGBA &theColor);

        //! Set selection highlighting color.
        void SetSelectedColor(const Quantity_ColorRGBA &theColor);

    protected:
        Handle(XCAFDoc_VisMaterial) myTransparentMat;
        Handle(XCAFDoc_VisMaterial) mySelectedMat;
    };

} // namespace

#endif // INTERACTIVESHAPEDRAWER_HEADERFILE
