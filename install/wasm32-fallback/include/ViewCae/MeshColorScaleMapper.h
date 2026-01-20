#ifndef MESHCOLORSCALEMAPPER_HEADERFILE
#define MESHCOLORSCALEMAPPER_HEADERFILE

#include <ViewCae/InteractiveColorScale.h>
#include <ViewCae/MeshColorMapper.h>

#include <RWCae/MeshGroupProperty.h>

namespace ViewCae {
//! Class for mapping mesh values to colors based on ViewCae::InteractiveColorScale object.
class MeshColorScaleMapper : public MeshColorMapper {
public:
    //! Constructor.
    Standard_EXPORT MeshColorScaleMapper(const Handle(InteractiveColorScale)& theColorScaleIO);

    //! Clear internal data.
    virtual void Clear() override { myColorScaleIO.Nullify(); }

    //! Return attached out-of-range transparency level.
    Standard_EXPORT virtual float ColorTextureTransparency() const override;

    //! Set an active mesh property.
    Standard_EXPORT virtual void SetMeshProperty(const Handle(RWCae::MeshNVecProperty)& theProperty,
                                                 int                                      theComponent,
                                                 bool                                     theToUpdate = true) override;

    //! Update the color scale object with values from the active mesh property.
    Standard_EXPORT virtual void SynchronizeValues();

    //! Return color scale object.
    const Handle(InteractiveColorScale)& ColorScaleObject() const { return myColorScaleIO; }

    //! Return visibility of color scale object.
    bool IsVisible() const { return myIsVisible; }

    //! Set visibility of color scale object.
    Standard_EXPORT void SetVisible(bool theIsVisible);

    //! Return visible values range.
    Standard_EXPORT virtual Bnd_Range ValuesRange() const override;

protected:
    Standard_EXPORT virtual void updateColorTexture() override;

    //! Compute color for each interval.
    Standard_EXPORT void computeIntervalColors(NCollection_Array1<Graphic3d_Vec4>& theColors, int theNbIntervals) const;

    //! Update the attached color scale for group property.
    Standard_EXPORT void setForGroupProp(const Handle(RWCae::MeshGroupProperty)& theProp);

    //! Update the attached color scale for scalar property.
    Standard_EXPORT void setForScalarProp(const Handle(RWCae::MeshScalarPropertyBase)& theProp);

protected:
    static constexpr float m_half       = 0.5f;
    static constexpr float m_oneAndHalf = 1.5f;
    static constexpr int   m_nbImgRows2 = 2;

protected:
    Handle(InteractiveColorScale) myColorScaleIO;
    bool                          myIsGroupPropAttached;
    bool                          myIsVisible;
};

} // namespace

#endif // MESHCOLORSCALEMAPPER_HEADERFILE