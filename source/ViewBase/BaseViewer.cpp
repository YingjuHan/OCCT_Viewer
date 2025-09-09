#include "BaseViewer.h"

#include <Graphic3d_CubeMapPacked.hxx>
#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>

namespace ViewBase
{
#include "cubemaps/CubeMapRoom.h"

    void BaseViewer::SetupCubemapIBL(const Handle(V3d_View)& theView)
    {
        Handle(Image_PixMap) aCubeMapImage = new Image_PixMap();
        const Standard_Byte *aBytes = THE_CUBEMAPROOM_IMAGE_RGBA.pixel_data;
        unsigned int aWidth = THE_CUBEMAPROOM_IMAGE_RGBA.width;
        unsigned int aHeight = THE_CUBEMAPROOM_IMAGE_RGBA.height;
        aCubeMapImage->InitWrapper(Image_Format_RGBA, const_cast<Standard_Byte *>(aBytes), aWidth, aHeight);
        aCubeMapImage->SetTopDown(true);
        Handle(Graphic3d_CubeMapPacked) aCubeMap = new Graphic3d_CubeMapPacked(aCubeMapImage);
        aCubeMap->SetColorMap(false);
        theView->SetBackgroundCubeMap(aCubeMap, true, false);
        theView->SetBgGradientStyle(Aspect_GFM_NONE, false);
    }

    void BaseViewer::SetupLightSources(const Handle(V3d_Viewer)& theViewer)
    {
        theViewer->SetDefaultShadingModel(Graphic3d_TOSM_FRAGMENT);
        for (; !theViewer->DefinedLights().IsEmpty();)
        {
            Handle(Graphic3d_CLight) aLight = theViewer->DefinedLights().First();
            theViewer->DelLight(aLight);
        }

        const double aThetaDeg = -77, aPhiDeg = -153;
        const double aTheta = aThetaDeg * M_PI / 180.0, aPhi = (aPhiDeg + 90.0) * M_PI / 180.0;
        gp_Dir aDir(Cos(aPhi) * Cos(aTheta), Cos(aTheta) * Sin(aPhi), Sin(aTheta));

        Handle(V3d_DirectionalLight) aDirLight = new V3d_DirectionalLight(V3d_Zneg, Quantity_NOC_WHITE);
        aDirLight->SetName("DirLight");
        aDirLight->SetHeadlight(true);
        aDirLight->SetCastShadows(true);
        aDirLight->SetDirection(aDir);
        aDirLight->SetIntensity(3.0f);
        theViewer->AddLight(aDirLight);
        theViewer->SetLightOn(aDirLight);

        Handle(V3d_AmbientLight) anAmbientLight = new V3d_AmbientLight(Quantity_NOC_WHITE);
        anAmbientLight->SetName("AmbientLight");
        anAmbientLight->SetIntensity(1.0f);
        theViewer->AddLight(anAmbientLight);
        theViewer->SetLightOn(anAmbientLight);
    }
}