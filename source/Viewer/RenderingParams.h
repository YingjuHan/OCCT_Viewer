#ifndef _RENDERINGPARAMS_HEADERFILE
#define _RENDERINGPARAMS_HEADERFILE

#include "IManager.h"
#include "Utils/ColorTools.h"

#include <Graphic3d_RenderingParams.hxx>

namespace Viewer {
//! Class provides public interface for managing Rendering Parameters @sa Graphic3d_RenderingParams.
class RenderingParams : public IManager {
public:
    //! Main constructor
    Standard_EXPORT explicit RenderingParams(WasmViewer* theViewer);

    //! Get background color.
    Standard_EXPORT ColorTools::ColorString GetBackgroundColor() const;

    //! Set background color.
    Standard_EXPORT void SetBackgroundColor(const ColorTools::ColorString& theHexColor);

    //! Set cubemap background.
    //! @param[in] theImagePath image path
    //! @param[in] theLightmapOnly apply only light map
    Standard_EXPORT void SetBackgroundCubemap(const std::string& theImagePath, bool theLightmapOnly);

    //! Returns TRUE if IBL (Image Based Lighting) from background cubemap is enabled.
    Standard_EXPORT bool IsImageBasedLighting() const;

    //! Set image base lighting.
    //! @param[in] theToEnable enable/disable image base Lighting
    Standard_EXPORT void SetImageBasedLighting(bool theToEnable);

    //! Return PBR enabled status.
    Standard_EXPORT bool IsPbrEnabled() const;

    //! Set PBR enabled state.
    Standard_EXPORT void SetPbrEnabled(bool theToEnable);

    //! Return rendering resolution scale within [0.25, 2.0] range; 1.0 means 100% (native) resolution.
    Standard_EXPORT float RenderResolutionScale() const;

    //! Set rendering resolution scale.
    //! It is better using integer scale factors for better quality.
    Standard_EXPORT void SetRenderResolutionScale(float theScale);

    //! Number of samples for MSAA buffer.
    Standard_EXPORT int NbMsaaSamples() const;

    //! Number of samples for MSAA buffer.
    Standard_EXPORT void SetNbMsaaSamples(int theNbSamples);

    //! Show stats on/off.
    Standard_EXPORT bool ToShowStats() const;

    //! Show stats on/off.
    Standard_EXPORT void SetShowStats(bool theToShow);

    //! Cast shadows on/off.
    Standard_EXPORT bool IsShadowEnabled() const;

    //! Cast shadows on/off.
    Standard_EXPORT void SetShadowEnabled(bool theToCast);

    //! Return shadow map resolution.
    Standard_EXPORT int ShadowMapResolution() const;

    //! Set shadow map resolution.
    Standard_EXPORT void SetShadowMapResolution(int theRes);

    //! Return shadow map bias.
    Standard_EXPORT float ShadowMapBias() const;

    //! Return shadow map bias.
    Standard_EXPORT void SetShadowMapBias(float theBias);

    //! Return transparency method (Graphic3d_RenderTransparentMethod).
    Standard_EXPORT int GetTransparencyMethod() const;

    //! Set transparency method (Graphic3d_RenderTransparentMethod).
    Standard_EXPORT void SetTransparencyMethod(int theMethod);

    //! Return number of depth peeling OIT layers.
    Standard_EXPORT int GetNbOitDepthPeelingLayers() const;

    //! Set number of depth peeling OIT layers..
    Standard_EXPORT void SetNbOitDepthPeelingLayers(int theNbLayers);

private:
    //! Return rendering parameters.
    Standard_EXPORT const Graphic3d_RenderingParams& getRendParams() const;
};

} // namespace

#endif // _RENDERINGPARAMS_HEADERFILE