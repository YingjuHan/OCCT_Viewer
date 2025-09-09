#ifndef BASEVIEWER_H
#define BASEVIEWER_H

#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

namespace ViewBase
{
    class BaseViewer
    {
    public:
        Standard_EXPORT static void SetupCubemapIBL(const Handle(V3d_View)& theView);
        // 设置立方体贴图

        Standard_EXPORT static void SetupLightSources(const Handle(V3d_Viewer)& theViewer);
        // 设置光源
    };
}

#endif // BASEVIEWER_H