#ifndef MESHCOLORMAPPER_HEADERFILE
#define MESHCOLORMAPPER_HEADERFILE

#include <RWCae/MeshNVecProperty.h>

#include <Graphic3d_Texture2D.hxx>

#include <functional>

namespace ViewCae {
//! Base class for mapping CAE mesh values to colors.
class MeshColorMapper : public Standard_Transient {
public:
    //! Constructor.
    Standard_EXPORT MeshColorMapper();

    //! Destructor.
    Standard_EXPORT virtual ~MeshColorMapper();

    //! Clear internal data.
    virtual void Clear() {}

    //! Return active mesh property.
    const Handle(RWCae::MeshNVecProperty)& MeshProperty() const { return myMeshProperty; }

    //! Return component index of active mesh property.
    int MeshPropertyComp() const { return myMeshPropertyComponent; }

    //! Set an active mesh property.
    Standard_EXPORT virtual void SetMeshProperty(const Handle(RWCae::MeshNVecProperty)& theProperty,
                                                 int                                      theComponent,
                                                 bool                                     theToUpdate = true);

    //! Return attached color texture.
    const Handle(Graphic3d_Texture2D)& ColorTexture() const { return myColorTexture; }

    //! Return attached out-of-range transparency level.
    virtual float ColorTextureTransparency() const { return 0.0f; }

    //! Update colors in the attached texture.
    Standard_EXPORT void UpdateColorTexture();

    //! Set a callback that is called when the color texture is updated.
    void SetOnTextureUpdateCallback(const std::function<void()>& theCallback)
    {
        myOnTextureUpdateCallback = theCallback;
    }

    //! Set a callback that is called when attached property is updated.
    void SetOnPropertyUpdateCallback(const std::function<void(const Handle(MeshColorMapper)&)>& theCallback)
    {
        myOnPropertyUpdateCallback = theCallback;
    }

    //! Return visible values range.
    Standard_EXPORT virtual Bnd_Range ValuesRange() const;

protected:
    Standard_EXPORT virtual void updateColorTexture();

protected:
    Handle(Graphic3d_Texture2D)                         myColorTexture;
    Handle(RWCae::MeshNVecProperty)                   myMeshProperty;
    int                                                 myMeshPropertyComponent = 0;
    std::function<void()>                               myOnTextureUpdateCallback;
    std::function<void(const Handle(MeshColorMapper)&)> myOnPropertyUpdateCallback;
};

} // namespace

#endif // MESHCOLORMAPPER_HEADERFILE
