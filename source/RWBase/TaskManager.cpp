#include <RWBase/TaskManager.h>

#include <OSD_Timer.hxx>
#include <OSD_ThreadPool.hxx>
#include <OSD_Parallel.hxx>

#ifdef __EMSCRIPTEN__
#include <emscripten/threading.h>
#endif

namespace RWBase
{
    //! 将任务插入队尾，必要时加锁保护。
    void TaskManager::TaskQueue::Add(const std::function<void()> &theFunc)
    {
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
        Standard_Mutex::Sentry aSentry(myMutex);
#endif
        myQueue.push_back(theFunc);
    }

    //! 执行队首任务，若队列为空返回 false。
    bool TaskManager::TaskQueue::RunOnce()
    {
        if (myQueue.empty())
        {
            return false;
        }

        std::function<void()> aFunc;
        {
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
            Standard_Mutex::Sentry aSentry(myMutex);
#endif
            aFunc = myQueue.front();
            myQueue.pop_front();
        }
        aFunc();
        return true;
    }

    //! 在限定时间内批量执行队列任务。
    bool TaskManager::TaskQueue::RunMany(double theThreshold)
    {
        if (myQueue.empty())
        {
            return false;
        }

        OSD_Timer aTimer;
        aTimer.Start();
        while (!myQueue.empty() && aTimer.ElapsedTime() < theThreshold)
        {
            RunOnce();
        }
        return true;
    }

    //! 取得 TaskManager 单例，必要时自动创建。
    const Handle(TaskManager) & TaskManager::Instance()
    {
        Handle(TaskManager) &anInstance = instance();
        if (anInstance.IsNull())
        {
            anInstance = TaskManager::create();
        }
        return anInstance;
    }

    //! 外部设定单例实例。
    void TaskManager::SetInstance(const Handle(TaskManager) & theInstance)
    {
        instance() = theInstance;
    }

    //! 返回单例引用，供内部使用。
    Handle(TaskManager) & TaskManager::instance()
    {
        static Handle(TaskManager) anInstance;
        return anInstance;
    }

    //! 创建 TaskManager 并在需要时启动后台线程。
    TaskManager *TaskManager::create()
    {
        TaskManager *aRes = new TaskManager();
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
        aRes->myIsMultitreadingOn = true;
        aRes->myBackgroundThread.Run(aRes);
#endif
        return aRes;
    }

    //! 默认构造函数，初始化线程与等待条件。
    TaskManager::TaskManager() : myBackgroundThread(run), myWaitCondition(false)
    {
        SetDefaultThreadsToLaunch(-1);
    }

    //! 析构时释放后台资源。
    TaskManager::~TaskManager()
    {
        release();
    }

    //! 通知后台线程退出并等待其结束。
    void TaskManager::release() noexcept
    {
        myIsRunning = false;
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
        myWaitCondition.Set();
        myBackgroundThread.Wait();
#endif
    }

    //! 配置默认的线程池线程数量。
    void TaskManager::SetDefaultThreadsToLaunch(int theNumberOfThreads)
    {
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
        if (theNumberOfThreads < 0)
        {
            // The idea to have NbLogicalProcessors() - 1 is
            // keeping foreground GUI thread in browser more responsive until calculations will be ended.
            myDefaultThreadsToLaunch = Max(1, OSD_Parallel::NbLogicalProcessors() - 1);
        }
        else
        {
            myDefaultThreadsToLaunch = theNumberOfThreads;
        }
        OSD_ThreadPool::DefaultPool()->SetNbDefaultThreadsToLaunch(myDefaultThreadsToLaunch);
#else
        (void)theNumberOfThreads;
        myDefaultThreadsToLaunch = 1;
#endif
    }

    //! 开启或关闭多线程模式。
    void TaskManager::SetMultithreadingMode(bool theIsMultithreadingOn)
    {
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
        myIsMultitreadingOn = theIsMultithreadingOn;
#else
        (void)theIsMultithreadingOn;
#endif
    }

    //! 若允许多线程则放入后台队列，否则退化为主线程调用。
    void TaskManager::RunAsync(const std::function<void()> &theFunc)
    {
        if (myIsMultitreadingOn)
        {
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
            myBackgroundQueue.Add(theFunc);
            myWaitCondition.Set();
            return;
#endif
        }

        RunMain(theFunc);
    }

    //! 将任务排入主线程队列，如有回调立即触发。
    void TaskManager::RunMain(const std::function<void()> &theFunc)
    {
        myMainQueue.Add(theFunc);
        if (myOnRunMain)
        {
            myOnRunMain();
        }
    }

    //! 使用线程池并行执行区间任务，或在主线程串行执行。
    void TaskManager::RunInParallel(int theStartIdx,
                                    int theEndIdx,
                                    const std::function<void(int theThreadIndex, int theObjIndex)> &theFunc,
                                    bool theIsForceSingleThread)
    {
        if (myIsMultitreadingOn && !theIsForceSingleThread)
        {
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
            const Handle(OSD_ThreadPool) &aThreadPool = OSD_ThreadPool::DefaultPool();
            const int aNbThreads = Min(theEndIdx - theStartIdx, NbDefaultThreadsToLaunch());
            OSD_ThreadPool::Launcher aLauncher(*aThreadPool, aNbThreads);
            aLauncher.Perform(theStartIdx, theEndIdx, theFunc);
            return;
#endif
        }

        for (int i = theStartIdx; i < theEndIdx; i++)
        {
            theFunc(0, i);
        }
    }

    //! 单步运行：多线程模式执行一个任务，单线程模式在时间片内尽量多跑任务。
    bool TaskManager::RunOnce()
    {
        if (myIsMultitreadingOn)
        {
            return myMainQueue.RunOnce();
        }
        else
        {
            return myMainQueue.RunMany(myTimeThreshold);
        }
    }

    //! 后台线程主循环，不断消费后台队列。
    Standard_Address TaskManager::run(Standard_Address theData)
    {
#if defined(__EMSCRIPTEN_PTHREADS__) || !defined(__EMSCRIPTEN__)
        TaskManager *aMngr = static_cast<TaskManager *>(theData);
        if (!aMngr)
        {
            return NULL;
        }
        while (aMngr->myIsRunning)
        {
            if (!aMngr->myBackgroundQueue.RunOnce())
            {
                aMngr->myWaitCondition.Wait();
                aMngr->myWaitCondition.Reset();
            }
        }
#else
        (void)theData;
#endif
        return NULL;
    }

    bool TaskManager::MainEmscriptenLoop::myIsRunning = false;

    //! 配置 Emscripten 主循环，使之调用 TaskManager::RunOnce。
    void TaskManager::MainEmscriptenLoop::operator()() const
    {
        if (myIsRunning)
        {
            return;
        }

#if defined(__EMSCRIPTEN_PTHREADS__)
        emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_V, setupRun);
#else
        setupRun();
#endif
    }

    //! 注册 runOnce 回调到 Emscripten 主循环。
    void TaskManager::MainEmscriptenLoop::setupRun()
    {
#if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(runOnce, -1, 0);
#endif
        myIsRunning = true;
    }

    //! 主循环回调：执行任务，完成后退出循环。
    void TaskManager::MainEmscriptenLoop::runOnce()
    {
        if (!TaskManager::Instance()->RunOnce())
        {
#if defined(__EMSCRIPTEN__)
            emscripten_cancel_main_loop();
#endif
            myIsRunning = false;
        }
    }

}
