#ifndef RWBase_TASKMANAGER_HEADERFILE
#define RWBase_TASKMANAGER_HEADERFILE

#include <Message.hxx>
#include <OSD_Thread.hxx>
#include <Standard_Condition.hxx>
#include <Standard_Mutex.hxx>

#include <deque>
#include <functional>

namespace RWBase
{
    //! Wasm 场景下使用的任务管理器，负责协调主线程与异步后台任务。
    class TaskManager : public Standard_Transient
    {
    public:
        //! Emscripten 主循环控制器。
        class MainEmscriptenLoop
        {
        public:
            //! TaskManager::OnRunMain() 所需的回调包装。
            Standard_EXPORT void operator()() const;

        private:
            //! 初始化主循环执行器（注册 runOnce 回调）。
            Standard_EXPORT static void setupRun();

            //! 主循环中每帧调用的回调。
            Standard_EXPORT static void runOnce();

        private:
            Standard_EXPORT static bool myIsRunning; //!< 标记回调是否已注册。
        };

    public:
        //! 获取全局 TaskManager 实例。
        Standard_EXPORT static const Handle(TaskManager) & Instance();

        //! 覆盖全局实例，可用于注入自定义实现。
        Standard_EXPORT static void SetInstance(const Handle(TaskManager) & theInstance);

    public:
        //! 在后台线程异步执行任务。
        Standard_EXPORT virtual void RunAsync(const std::function<void()> &theFunc);

        //! 将任务投递到主线程执行。
        Standard_EXPORT virtual void RunMain(const std::function<void()> &theFunc);

        //! 在允许多线程的情况下并行执行分块任务。
        Standard_EXPORT virtual void RunInParallel(int theStartIdx,
                                                   int theEndIdx,
                                                   const std::function<void(int theThreadIndex, int theObjIndex)> &theFunc,
                                                   bool theIsForceSingleThread = false);

        //! 注册主线程唤醒回调，用于驱动事件循环。
        virtual void OnRunMain(std::function<void()> theFunc) { myOnRunMain = theFunc; }

        //! 从队列中执行一次任务；若为单线程模式则会在时间片内尽量多跑任务。
        Standard_EXPORT virtual bool RunOnce();

        //! 获取默认可用线程数。
        int NbDefaultThreadsToLaunch() const { return myDefaultThreadsToLaunch; }

        //! 设置默认线程数；传入 < 0 时会自动使用 max(1, CPU 逻辑核 - 1)。
        //! @param[in] theNumberOfThreads 希望的默认线程数
        Standard_EXPORT void SetDefaultThreadsToLaunch(int theNumberOfThreads);

        //! 查询是否允许多线程模式。
        bool IsMultithreadingMode() const { return myIsMultitreadingOn; }

        //! 切换多线程模式开关。
        Standard_EXPORT void SetMultithreadingMode(bool theIsMultithreadingOn);

        //! 析构函数，确保后台线程正确退出。
        Standard_EXPORT ~TaskManager();

    protected:
        //! 受保护的默认构造。
        Standard_EXPORT TaskManager();

        //! 工厂方法：创建并初始化任务管理器。
        Standard_EXPORT static TaskManager *create();

        //! 获取全局实例的可修改引用。
        Standard_EXPORT static Handle(TaskManager) & instance();

    private:
        //! 简单的任务队列实现。
        class TaskQueue
        {
        public:
            //! 将任务函数推入队列尾部。
            Standard_EXPORT void Add(const std::function<void()> &theFunc);

            //! 取出队首任务并执行，若为空返回 false。
            Standard_EXPORT bool RunOnce();

            //! 在指定时间阈值内循环执行队列任务。
            Standard_EXPORT bool RunMany(double theThreshold);

        private:
            Standard_Mutex myMutex; //!< 保护队列的互斥量。
            std::deque<std::function<void()>> myQueue; //!< 待执行任务列表。
        };

    private:
        //! 后台线程入口函数。
        static Standard_Address run(Standard_Address theData);

        static constexpr double m_defaultTimeThreshold = 0.025; //!< 单线程模式下的默认时间片（秒）。

        //! 释放资源并终止后台执行。
        Standard_EXPORT void release() noexcept;

    private:
        TaskQueue myMainQueue; //!< 主线程待执行任务队列。
        TaskQueue myBackgroundQueue; //!< 后台线程队列。
        OSD_Thread myBackgroundThread; //!< 后台线程对象。
        Standard_Condition myWaitCondition; //!< 后台线程的唤醒条件。
        std::function<void()> myOnRunMain; //!< 外部提供的主线程唤醒回调。
        double myTimeThreshold = m_defaultTimeThreshold; //!< 单线程批处理的时间上限。
        int myDefaultThreadsToLaunch = 1; //!< 默认允许的线程数。
        bool myIsRunning = true; //!< 指示 TaskManager 是否仍在运行。
        bool myIsMultitreadingOn = false; //!< 是否启用多线程模式。
    };

}

#endif // RWBase_TASKMANAGER_HEADERFILE
