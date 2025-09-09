#ifndef CONDITIONPROMISE_H_
#define CONDITIONPROMISE_H_

#include "IPromise.h"
#include <Standard_Condition.hxx>

namespace RWBase
{
    //! 利用 Standard_Condition 封装的 Promise，供跨线程同步使用。
    class ConditionPromise : public IPromise
    {
    public:
        //! 默认构造函数，创建处于未触发状态的条件变量。
        ConditionPromise() : myCondition(false) {}

        //! 标记 Promise 成功并唤醒所有等待者。
        Standard_EXPORT virtual void resolve() override { myCondition.Set(); }

        //! 记录错误信息并唤醒等待者，表示拒绝 Promise。
        Standard_EXPORT virtual void reject(const std::string &theErr) override
        {
            myError = theErr;
            myCondition.Set();
        }

        //! 访问内部条件对象的只读引用，便于外部等待。
        const Standard_Condition &Condition() const { return myCondition; }

        //! 返回可修改的条件对象，以便进行自定义等待/重置。
        Standard_Condition& ChangeCondition() { return myCondition; }

        //! 获取最近一次 reject 时保存的错误消息。
        const std::string &Error() const { return myError; }

    protected:
        Standard_Condition myCondition; //!< 控制 resolve/reject 通知的条件变量。
        std::string myError; //!< 保存拒绝 Promise 时的错误描述。
    };

}

#endif // CONDITIONPROMISE_H_
