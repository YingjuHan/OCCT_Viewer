#ifndef _IINTERACTIVEOBJECT_HEADERFILE
#define _IINTERACTIVEOBJECT_HEADERFILE

#include <ViewBase/InteractiveOwner.h>

#include <AIS_Animation.hxx>
#include <AIS_InteractiveObject.hxx>
#include <Aspect_VKey.hxx>
#include <TColgp_SequenceOfPnt.hxx>

#include <functional>

namespace ViewBase
{

    //! Common interface expanding AIS_InteractiveObject base interface
    //! with auxiliary for updating presentation on viewer changes.
    //! 通用接口，扩展了AIS_InteractiveObject基础接口，用于在查看器变化时更新显示
    class IInteractiveObject
    {
    public:
        //! Find interactive object by id.
        //! 通过ID查找交互对象
        Standard_EXPORT static Handle(AIS_InteractiveObject) FindAisObjectById(const AIS_InteractiveContext &theCtx,
                                                                               const TCollection_AsciiString &theObjId);

        //! Find object by id.
        //! 通过ID查找对象
        template <class T>
        static bool FindObjectById(opencascade::handle<T> &theObject,
                                   const AIS_InteractiveContext &theCtx,
                                   const TCollection_AsciiString &theObjId)
        {
            Handle(AIS_InteractiveObject) anObj = FindAisObjectById(theCtx, theObjId);
            if (!anObj.IsNull())
            {
                theObject = opencascade::handle<T>::DownCast(anObj);
                return !theObject.IsNull();
            }
            return false;
        }

        //! Find object by it's owner.
        //! 通过所有者查找对象
        Standard_EXPORT static Handle(AIS_InteractiveObject) FindAisObjectByOwner(const AIS_InteractiveContext &theCtx,
                                                                                  const Handle(InteractiveOwner) & theOwner);

        //! Find object by it's owner.
        //! 通过所有者查找对象
        template <class T>
        static bool FindAisObjectByOwner(opencascade::handle<T> &theObject,
                                         const AIS_InteractiveContext &theCtx,
                                         const Handle(InteractiveOwner) & theOwner)
        {
            Handle(AIS_InteractiveObject) anObj = FindAisObjectByOwner(theCtx, theOwner);
            if (!anObj.IsNull())
            {
                theObject = opencascade::handle<T>::DownCast(anObj);
                return !theObject.IsNull();
            }
            return false;
        }

    public:
        //! Invalidate V3d_View's, where object is displayed.
        //! 使对象显示的V3d_View失效
        Standard_EXPORT static void InvalidateViewer(const Handle(AIS_InteractiveObject) & theObj);

        //! Find single View where object is visible.
        //! 查找对象可见的单个视图
        Standard_EXPORT static Handle(V3d_View) GetVisibleInView(const Handle(AIS_InteractiveObject) & theObj);

        //! Find any View where object is visible.
        //! 查找对象可见的任意视图
        Standard_EXPORT static Handle(V3d_View) GetVisibleInAnyView(const Handle(AIS_InteractiveObject) & theObj);

        //! Return picking axis for specified mouse position.
        //! 返回指定鼠标位置的拾取轴
        Standard_EXPORT static gp_Ax1 GetPickLine(const Handle(V3d_View) & theView, const Graphic3d_Vec2i &thePos);

        //! Return on-screen coordinates for point defined within Graphic3d_TMF_2d persistence.
        //! 返回在Graphic3d_TMF_2d持久性中定义的点的屏幕坐标
        Standard_EXPORT static Graphic3d_Vec2i GetPointOnScreen2d(const Handle(AIS_InteractiveObject) & theObj,
                                                                  const Handle(V3d_View) & theView,
                                                                  const Graphic3d_Vec2i &thePos);

    public:
        //! Destructor.
        //! 析构函数
        Standard_EXPORT virtual ~IInteractiveObject();

        //! Callback called by Viewer on pixel scale ratio changes,
        //! so that to recompute presentation accordingly.
        //! @param[in] theObj object to be drawn (e.g. 'this')
        //! @param[in] theRatio new scale ratio
        //! @return TRUE if object presentation should be recomputed
        //! 当像素缩放比例变化时由查看器调用的回调函数，
        //! 以便相应地重新计算显示。
        //! @param[in] theObj 要绘制的对象（例如'this'）
        //! @param[in] theRatio 新的缩放比例
        //! @return 如果需要重新计算对象显示则返回TRUE
        Standard_EXPORT virtual bool OnPixelScaleChange(const Handle(AIS_InteractiveObject) & theObj, float theRatio);

        //! Return TRUE if object is in enabled state; TRUE by default.
        //! 如果对象处于启用状态则返回TRUE；默认为TRUE
        virtual bool IsEnabled() const { return myIsEnabled; }

        //! Set object enabled state.
        //! 设置对象启用状态
        virtual void SetEnabled(bool theToEnable) { myIsEnabled = theToEnable; }

        //! Update presentation aspects.
        //! 更新显示方面
        virtual void UpdateAspects() {}

        //! Handle or ignore mouse click.
        //! InteractiveOwner::HandleMouseClick() redirects to this callback.
        //! Default implementation redirects to callback set via SetCallback().
        //! @param[in] theObj object to be drawn (e.g. 'this')
        //! @param[in] thePoint on-screen mouse cursor position
        //! @param[in] theButton pressed mouse button
        //! @param[in] theModifiers pressed modifiers
        //! @param[in] theIsDoubleClick flag indicating double-click event
        //! @return TRUE if object responds to mouse click
        //! 处理或忽略鼠标点击。
        //! InteractiveOwner::HandleMouseClick()重定向到此回调函数。
        //! 默认实现重定向到通过SetCallback()设置的回调函数。
        //! @param[in] theObj 要绘制的对象（例如'this'）
        //! @param[in] thePoint 屏幕上的鼠标光标位置
        //! @param[in] theButton 按下的鼠标按钮
        //! @param[in] theModifiers 按下的修饰键
        //! @param[in] theIsDoubleClick 表示双击事件的标志
        //! @return 如果对象响应鼠标点击则返回TRUE
        Standard_EXPORT virtual bool HandleMouseClick(const Handle(AIS_InteractiveObject) & theObj,
                                                      const Graphic3d_Vec2i &thePoint,
                                                      Aspect_VKeyMouse theButton,
                                                      Aspect_VKeyFlags theModifiers,
                                                      bool theIsDoubleClick);

        //! Return callback function to be called on mouse button click.
        //! 返回在鼠标按钮点击时要调用的回调函数
        const std::function<void(const Handle(AIS_InteractiveObject) &)> &GetCallback() const { return myCallback; }

        //! Set callback function to be called on mouse button click.
        //! 设置在鼠标按钮点击时要调用的回调函数
        void SetCallback(const std::function<void(const Handle(AIS_InteractiveObject) &)> &theCallback)
        {
            myCallback = theCallback;
        }

        //! Return transform persistence offset in unscaled pixels.
        //! 返回未缩放像素中的变换持久性偏移
        const Graphic3d_Vec2i &Get2dViewOffset() const { return myViewOffset; }

        //! Set transform persistence offset in unscaled pixels.
        //! 设置未缩放像素中的变换持久性偏移
        void Set2dViewOffset(const Graphic3d_Vec2i &theOffset) { myViewOffset = theOffset; }

        //! Return device pixel ratio for scaling 2D elements; 1.0 by default.
        //! Property is updated on calling OnPixelScaleChange().
        //! 返回用于缩放2D元素的设备像素比率；默认为1.0
        //! 在调用OnPixelScaleChange()时更新此属性
        Standard_ShortReal DevicePixelRatio() const { return myDevicePixelRatio; }

        //! Return size of 2D objects to apply offset when arranging other 2D objects.
        //! 返回2D对象的大小，用于在排列其他2D对象时应用偏移
        virtual Graphic3d_Vec2i Get2dObjectSize() const { return Graphic3d_Vec2i(0); }

        //! Return name of this type.
        //! 返回此类型的名称
        virtual const char *TypeName() const = 0;

    protected:
        //! Empty constructor.
        //! 空构造函数
        Standard_EXPORT IInteractiveObject();

    protected:
        std::function<void(const Handle(AIS_InteractiveObject) &)> myCallback;
        Graphic3d_Vec2i myViewOffset; //!< transform persistence offset in unscaled pixels
                                      //!< 未缩放像素中的变换持久性偏移
        Standard_ShortReal myDevicePixelRatio;
        Standard_Boolean myIsEnabled;     //!< enabled state
                                          //!< 启用状态
        Standard_Boolean myIsThickLine2d; //!< draw 2D line with thickness in 3D
                                          //!< 在3D中绘制带厚度的2D线
    };

}

#endif // _IINTERACTIVEOBJECT_HEADERFILE