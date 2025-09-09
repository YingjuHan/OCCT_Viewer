#include "RenderingParams.h"

#include "WasmViewer.h"

#include <Graphic3d_CubeMapPacked.hxx>
#include <Image_AlienPixMap.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {
// ================================================================
// Function : getRendParams
// ================================================================
const Graphic3d_RenderingParams& RenderingParams::getRendParams() const
{
    return myWebViewer->View()->Viewer()->DefaultRenderingParams();
}

// ================================================================
// Function : RenderingParams
// ================================================================
RenderingParams::RenderingParams(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : GetBackgroundColor
// ================================================================
ColorTools::ColorString RenderingParams::GetBackgroundColor() const
{
    Quantity_ColorRGBA      aColor(myWebViewer->BackgroundColor());
    TCollection_AsciiString aHex = Quantity_ColorRGBA::ColorToHex(aColor);
    return aHex.ToCString();
}

// ================================================================
// Function : SetBackgroundColor
// ================================================================
void RenderingParams::SetBackgroundColor(const ColorTools::ColorString& theHexColor)
{
    Quantity_ColorRGBA aColor;
    if (!ColorTools::ParseHexColor(theHexColor, aColor)) {
        return;
    }

    myWebViewer->SetBackgroundColor(aColor.GetRGB());
    for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
        if (!aViewIter->View()->IsSubviewComposer()) {
            aViewIter->SetBackgroundColor(aColor.GetRGB());
            aViewIter->SetBackgroundCubeMap(Handle(Graphic3d_CubeMap)(), false);
            aViewIter->Invalidate();
        }
    }
}

// ================================================================
// Function : SetBackgroundCubemap
// ================================================================
void RenderingParams::SetBackgroundCubemap(const std::string& theImagePath, bool theLightmapOnly)
{
    if (theImagePath.empty()) {
        for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
            aViewIter->SetBackgroundCubeMap(Handle(Graphic3d_CubeMapPacked)(), true, false);
            aViewIter->Invalidate();
        }
        return;
    }

    Handle(Graphic3d_CubeMapPacked) aCubemap;
    Handle(Image_AlienPixMap)       anImage = new Image_AlienPixMap();
    if (anImage->Load(theImagePath.c_str())) {
        aCubemap = new Graphic3d_CubeMapPacked(anImage);
        /// WebGL 2.0 ensures sRGB conformance within glGenerateMipmap()
        /// which leads to extremely SLOW generation (5 seconds instead of 0.035)
        aCubemap->SetColorMap(false);
    }

    for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
        aViewIter->SetBackgroundCubeMap(aCubemap, true, false);
        if (theLightmapOnly) {
            aViewIter->SetBgGradientStyle(Aspect_GFM_NONE, false);
        }
        aViewIter->Invalidate();
    }
}

// ================================================================
// Function : IsImageBasedLighting
// ================================================================
bool RenderingParams::IsImageBasedLighting() const
{
    return myWebViewer->View()->IsImageBasedLighting();
}

// ================================================================
// Function : SetImageBasedLighting
// ================================================================
void RenderingParams::SetImageBasedLighting(bool theToEnable)
{
    for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
        aViewIter->SetImageBasedLighting(theToEnable);
        aViewIter->Invalidate();
    }
}

// ================================================================
// Function : IsPbrEnabled
// ================================================================
bool RenderingParams::IsPbrEnabled() const
{
    return myWebViewer->View()->ShadingModel() == Graphic3d_TOSM_PBR;
}

// ================================================================
// Function : SetPbrEnabled
// ================================================================
void RenderingParams::SetPbrEnabled(bool theToEnable)
{
    if (IsPbrEnabled() != theToEnable) {
        Graphic3d_RenderingParams aParams = getRendParams();
        aParams.ShadingModel              = theToEnable ? Graphic3d_TOSM_PBR : Graphic3d_TOSM_FRAGMENT;
        myWebViewer->Viewer()->SetDefaultRenderingParams(aParams);
        for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
            aViewIter->SetShadingModel(theToEnable ? Graphic3d_TOSM_PBR : Graphic3d_TOSM_FRAGMENT);
            aViewIter->Invalidate();
        }
    }
}

// ================================================================
// Function : ResolutionScale
// ================================================================
float RenderingParams::RenderResolutionScale() const
{
    return getRendParams().RenderResolutionScale;
}

// ================================================================
// Function : SetRenderResolutionScale
// ================================================================
void RenderingParams::SetRenderResolutionScale(float theScale)
{
    Graphic3d_RenderingParams aParams = getRendParams();
    if (aParams.RenderResolutionScale != theScale) {
        aParams.RenderResolutionScale = theScale;
        myWebViewer->Viewer()->SetDefaultRenderingParams(aParams);
        for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
            aViewIter->ChangeRenderingParams().RenderResolutionScale = !aViewIter->View()->IsSubviewComposer()
                                                                         ? theScale
                                                                         : 1.0f;
            aViewIter->Invalidate();
        }
    }
}

// ================================================================
// Function : NbMsaaSamples
// ================================================================
int RenderingParams::NbMsaaSamples() const
{
    return getRendParams().NbMsaaSamples;
}

// ================================================================
// Function : SetNbMsaaSamples
// ================================================================
void RenderingParams::SetNbMsaaSamples(int theNbSamples)
{
    Graphic3d_RenderingParams aParams = getRendParams();
    if (aParams.NbMsaaSamples != theNbSamples) {
        aParams.NbMsaaSamples = theNbSamples;
        myWebViewer->Viewer()->SetDefaultRenderingParams(aParams);
        for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
            aViewIter->ChangeRenderingParams().NbMsaaSamples = !aViewIter->View()->IsSubviewComposer() ? theNbSamples
                                                                                                       : 0;
            aViewIter->Invalidate();
        }
    }
}

// ================================================================
// Function : ToShowStats
// ================================================================
bool RenderingParams::ToShowStats() const
{
    return getRendParams().ToShowStats;
}

// ================================================================
// Function : SetShowStats
// ================================================================
void RenderingParams::SetShowStats(bool theToShow)
{
    Graphic3d_RenderingParams aParams = getRendParams();
    if (aParams.ToShowStats != theToShow) {
        aParams.ToShowStats = theToShow;
        myWebViewer->Viewer()->SetDefaultRenderingParams(aParams);
        for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
            aViewIter->ChangeRenderingParams().ToShowStats = theToShow;
            aViewIter->Invalidate();
        }
    }
}

// ================================================================
// Function : IsShadowEnabled
// ================================================================
bool RenderingParams::IsShadowEnabled() const
{
    return getRendParams().IsShadowEnabled;
}

// ================================================================
// Function : SetShadowEnabled
// ================================================================
void RenderingParams::SetShadowEnabled(bool theToCast)
{
    Graphic3d_RenderingParams aParams = getRendParams();
    if (aParams.IsShadowEnabled == theToCast) {
        return;
    }

    aParams.IsShadowEnabled = theToCast;
    myWebViewer->Viewer()->SetDefaultRenderingParams(aParams);
    for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
        aViewIter->ChangeRenderingParams().IsShadowEnabled = !aViewIter->View()->IsSubviewComposer() && theToCast;
        aViewIter->Invalidate();
    }

    // disable topmost immediate layers as they will not work with MSAA in WebGL,
    // but enable it for shadows (to prevent depth->shadow buffer clearing)
    const bool toUseImmediateLayer = theToCast;

    Graphic3d_ZLayerSettings aLayer = myWebViewer->Viewer()->ZLayerSettings(Graphic3d_ZLayerId_Topmost);
    aLayer.SetImmediate(toUseImmediateLayer);
    myWebViewer->Viewer()->SetZLayerSettings(Graphic3d_ZLayerId_Topmost, aLayer);
}

// ================================================================
// Function : ShadowMapResolution
// ================================================================
int RenderingParams::ShadowMapResolution() const
{
    return getRendParams().ShadowMapResolution;
}

// ================================================================
// Function : SetShadowMapResolution
// ================================================================
void RenderingParams::SetShadowMapResolution(int theRes)
{
    Graphic3d_RenderingParams aParams = getRendParams();
    if (aParams.ShadowMapResolution != theRes) {
        aParams.ShadowMapResolution = theRes;
        myWebViewer->Viewer()->SetDefaultRenderingParams(aParams);
        for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
            aViewIter->ChangeRenderingParams().ShadowMapResolution = theRes;
            if (aViewIter->RenderingParams().IsShadowEnabled) {
                aViewIter->Invalidate();
            }
        }
    }
}

// ================================================================
// Function : ShadowMapBias
// ================================================================
float RenderingParams::ShadowMapBias() const
{
    return getRendParams().ShadowMapBias;
}

// ================================================================
// Function : SetShadowMapBias
// ================================================================
void RenderingParams::SetShadowMapBias(float theBias)
{
    Graphic3d_RenderingParams aParams = getRendParams();
    if (aParams.ShadowMapBias != theBias) {
        aParams.ShadowMapBias = theBias;
        myWebViewer->Viewer()->SetDefaultRenderingParams(aParams);
        for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
            aViewIter->ChangeRenderingParams().ShadowMapBias = theBias;
            if (aViewIter->RenderingParams().IsShadowEnabled) {
                aViewIter->Invalidate();
            }
        }
    }
}

// ================================================================
// Function : GetTransparencyMethod
// ================================================================
int RenderingParams::GetTransparencyMethod() const
{
    return (int)getRendParams().TransparencyMethod;
}

// ================================================================
// Function : SetTransparencyMethod
// ================================================================
void RenderingParams::SetTransparencyMethod(int theMethod)
{
    Graphic3d_RenderingParams aParams = getRendParams();
    if ((int)aParams.TransparencyMethod == theMethod) {
        return;
    }
    if (theMethod != Graphic3d_RTM_BLEND_UNORDERED && theMethod != Graphic3d_RTM_BLEND_OIT
        && theMethod != Graphic3d_RTM_DEPTH_PEELING_OIT) {
        Message::SendFail() << "Error: SetTransparencyMethod (" << theMethod << ") called with unknown method";
        return;
    }

    aParams.TransparencyMethod = (Graphic3d_RenderTransparentMethod)theMethod;
    myWebViewer->Viewer()->SetDefaultRenderingParams(aParams);
    for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
        aViewIter->ChangeRenderingParams().TransparencyMethod = aParams.TransparencyMethod;
        aViewIter->Invalidate();
    }
}

// ================================================================
// Function : GetNbOitDepthPeelingLayers
// ================================================================
int RenderingParams::GetNbOitDepthPeelingLayers() const
{
    return (int)getRendParams().NbOitDepthPeelingLayers;
}

// ================================================================
// Function : SetNbOitDepthPeelingLayers
// ================================================================
void RenderingParams::SetNbOitDepthPeelingLayers(int theNbLayers)
{
    Graphic3d_RenderingParams aParams = getRendParams();
    if ((int)aParams.NbOitDepthPeelingLayers == theNbLayers) {
        return;
    }

    aParams.NbOitDepthPeelingLayers = theNbLayers;
    myWebViewer->Viewer()->SetDefaultRenderingParams(aParams);
    for (const Handle(V3d_View)& aViewIter : myWebViewer->Viewer()->DefinedViews()) {
        aViewIter->ChangeRenderingParams().NbOitDepthPeelingLayers = aParams.NbOitDepthPeelingLayers;
        if (aParams.TransparencyMethod == Graphic3d_RTM_DEPTH_PEELING_OIT) {
            aViewIter->Invalidate();
        }
    }
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(RenderingParams)
{
    emscripten::class_<RenderingParams, emscripten::base<IManager>>("RenderingParams")
        .function("setBackgroundCubemap", &RenderingParams::SetBackgroundCubemap)
        .property("backgroundColor", &RenderingParams::GetBackgroundColor, &RenderingParams::SetBackgroundColor)
        .property("isImageBasedLighting",
                  &RenderingParams::IsImageBasedLighting,
                  &RenderingParams::SetImageBasedLighting)
        .property("isPbrEnabled", &RenderingParams::IsPbrEnabled, &RenderingParams::SetPbrEnabled)
        .property("renderResolutionScale",
                  &RenderingParams::RenderResolutionScale,
                  &RenderingParams::SetRenderResolutionScale)
        .property("nbMsaaSamples", &RenderingParams::NbMsaaSamples, &RenderingParams::SetNbMsaaSamples)
        .property("toShowStats", &RenderingParams::ToShowStats, &RenderingParams::SetShowStats)
        .property("isShadowEnabled", &RenderingParams::IsShadowEnabled, &RenderingParams::SetShadowEnabled)
        .property("shadowMapResolution",
                  &RenderingParams::ShadowMapResolution,
                  &RenderingParams::SetShadowMapResolution)
        .property("shadowMapBias", &RenderingParams::ShadowMapBias, &RenderingParams::SetShadowMapBias)
        .property("transparencyMethod",
                  &RenderingParams::GetTransparencyMethod,
                  &RenderingParams::SetTransparencyMethod)
        .property("nbOitDepthPeelingLayers",
                  &RenderingParams::GetNbOitDepthPeelingLayers,
                  &RenderingParams::SetNbOitDepthPeelingLayers);
}
#endif

} // namespace

