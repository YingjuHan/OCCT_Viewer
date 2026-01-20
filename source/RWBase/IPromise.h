#ifndef IPromise_H_
#define IPromise_H_

#include <Standard_Macro.hxx>
#include <string>

namespace RWBase {
    //! 异步任务的最小 Promise 接口，支持显式成功或失败回调。
    extern "C" struct IPromise {
        //! 虚析构，确保派生类能正确释放资源。
        virtual ~IPromise() {}

        //! 将 Promise 标记为成功完成。
        Standard_EXPORT virtual void resolve() = 0;

        //! 将 Promise 标记为失败，并传递错误信息。
        Standard_EXPORT virtual void reject(const std::string &theErr) = 0;
    };
}

#endif // IPromise_H_
