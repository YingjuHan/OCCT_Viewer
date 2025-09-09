#ifndef BASEDEFAULT_DRAWER_H
#define BASEDEFAULT_DRAWER_H

#include <Prs3d_Drawer.hxx>
#include <XCAFDoc_VisMaterial.hxx>

namespace ViewBase
{
    class BaseDefaultDrawer : public Prs3d_Drawer
    {
    public:
        static constexpr int m_defTextHeight = 12;
        static constexpr int m_defDimArrowLen = 12;
        static constexpr int m_defFloatPrec = 6;

    public:
        static Aspect_TypeOfMarker MergedMarker(Aspect_TypeOfMarker theInner, Aspect_TypeOfMarker theOuter)
        {
            if (theOuter == Aspect_TOM_EMPTY)
            {
                return theInner;
            }
            else if (theInner == Aspect_TOM_EMPTY)
            {
                return theOuter;
            }

            Aspect_TypeOfMarker anInner = theInner;
            Aspect_TypeOfMarker anOuter = theOuter;

            if (anOuter != Aspect_TOM_O && anInner == Aspect_TOM_O)
            {
                std::swap(anInner, anOuter);
            }
            if (anOuter != Aspect_TOM_O)
            {
                return anOuter;
            }

            switch (theInner)
            {
            case Aspect_TOM_X:
                return Aspect_TOM_X;
            case Aspect_TOM_STAR:
                return Aspect_TOM_O_STAR;
            case Aspect_TOM_PLUS:
                return Aspect_TOM_O_PLUS;
            case Aspect_TOM_POINT:
                return Aspect_TOM_O_POINT;
            default:
                break;
            }

            return anOuter;
        }

        //! Split marker type.
        //! 分割标记类型
        static void SplitMarker(Aspect_TypeOfMarker theType, Aspect_TypeOfMarker &theInner, Aspect_TypeOfMarker &theOuter)
        {
            switch (theType)
            {
            case Aspect_TOM_O_POINT:
            {
                theInner = Aspect_TOM_POINT;
                theOuter = Aspect_TOM_O;
                return;
            }
            case Aspect_TOM_O_PLUS:
            {
                theInner = Aspect_TOM_PLUS;
                theOuter = Aspect_TOM_O;
                return;
            }
            case Aspect_TOM_O_STAR:
            {
                theInner = Aspect_TOM_STAR;
                theOuter = Aspect_TOM_O;
                return;
            }
            case Aspect_TOM_O_X:
            {
                theInner = Aspect_TOM_X;
                theOuter = Aspect_TOM_O;
                return;
            }
            default:
                break;
            }
            theInner = theType;
            theOuter = Aspect_TOM_EMPTY;
        }

    public:
        //! Empty constructor.
        //! 空构造函数
        Standard_EXPORT BaseDefaultDrawer();

        //! Destructor.
        //! 析构函数
        Standard_EXPORT virtual ~BaseDefaultDrawer();

        //! Setup units.
        //! 设置单位
        Standard_EXPORT void UpdateUnits();

        //! Return default visualization material.
        //! 返回默认可视化材质
        const Handle(XCAFDoc_VisMaterial) & VisMaterial() const { return myVisMaterial; }

    protected:
        Handle(XCAFDoc_VisMaterial) myVisMaterial;
    };
}

#endif // BASEDEFAULT_DRAWER_H