#ifndef _BASESELECTION_HEADERFILE
#define _BASESELECTION_HEADERFILE

#include <AIS_Selection.hxx>

namespace ViewBase
{
    //! Selection controller extending AIS selection state.
    //! 选择控制器，扩展了AIS选择状态
    class BaseSelection : public AIS_Selection
    {
    public:
        //! Empty constructor.
        //! 空构造函数
        Standard_EXPORT BaseSelection();

        //! Destructor.
        //! 析构函数
        Standard_EXPORT virtual ~BaseSelection();

        //! Return last active selection scheme.
        //! 返回最后激活的选择方案
        AIS_SelectionScheme GetSelectionScheme() const { return mySelScheme; }

        //! Return last active selection scheme.
        //! 设置选择方案
        void SetSelectionScheme(AIS_SelectionScheme theScheme) { mySelScheme = theScheme; }

    private:
        //! Last active selection scheme.
        //! 最后激活的选择方案
        AIS_SelectionScheme mySelScheme = AIS_SelectionScheme_UNKNOWN;
    };

} // namespace

#endif // _BASESELECTION_HEADERFILE