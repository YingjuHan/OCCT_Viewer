## ConditionPromise.h

这个文件定义了一个条件承诺类，用于异步操作的承诺模式实现。

```cpp
#ifndef CONDITIONPROMISE_H_
#define CONDITIONPROMISE_H_

#include "IPromise.h"
#include <Standard_Condition.hxx>

namespace SCRWBase
{
    class ConditionPromise : public IPromise
    {
    public:
        ConditionPromise() : myCondition(false) {}

        Standard_EXPORT virtual void resolve() override { myCondition.Set(); }
        Standard_EXPORT virtual void reject(const std::string &theErr) override
        {
            myError = theErr;
            myCondition.Set();
        }

        const Standard_Condition &Condition() const { return myCondition; }

        Standard_Condition& ChangeCondition() { return myCondition; }

        const std::string &Error() const { return myError; }

    protected:
        Standard_Condition myCondition;
        std::string myError;
    };

}

#endif // CONDITIONPROMISE_H_
```

主要特点：
- 继承自IPromise接口，实现resolve()和reject()方法
- 使用OpenCASCADE的Standard_Condition作为同步机制
- resolve()方法设置条件为true，表示操作成功完成
- reject()方法设置条件为true并保存错误信息，表示操作失败
- 提供Condition()和ChangeCondition()方法访问内部条件变量
- 提供Error()方法获取错误信息

## EmulatedFileSystem.h / EmulatedFileSystem.cpp

模拟文件系统实现，主要用于将单个文件预加载到内存中（特别是为TypeScript层设计）。

头文件定义：
```cpp
#ifndef SCRWBase_EMULATEDFILESYSTEM_HEADERFILE
#define SCRWBase_EMULATEDFILESYSTEM_HEADERFILE

#include <NCollection_Buffer.hxx>
#include <OSD_FileSystem.hxx>

namespace SCRWBase
{
    //! Emulated file system with single file pre-loaded into memory (for TypeScript layer).
    class EmulatedFileSystem : public OSD_FileSystem
    {
    public:
        //! Create file system.
        Standard_EXPORT static Handle(SCRWBase::EmulatedFileSystem) Create(const TCollection_AsciiString &theFilePath);

    public:
        //! Constructor from memory buffer.
        Standard_EXPORT EmulatedFileSystem(const TCollection_AsciiString &theRootFile,
                                           const Handle(NCollection_Buffer) & theBuffer);

        //! Constructor from filename.
        Standard_EXPORT explicit EmulatedFileSystem(const TCollection_AsciiString &theRootFile);

        //! Make copy of filesystem.
        Standard_EXPORT Handle(EmulatedFileSystem) Copy();

        //! Preload data to the buffer if it is not loaded.
        //! @return status of preloading.
        Standard_EXPORT bool PreloadBuffer();

        //! @return buffer.
        const Handle(NCollection_Buffer) & Buffer() const { return myRootBuffer; }

        //! Opens stream for specified file URL for reading operations
        //! or returns previously created stream pointing to the same URL.
        virtual std::shared_ptr<std::istream> OpenIStream(const TCollection_AsciiString &theUrl,
                                                          const std::ios_base::openmode theParams,
                                                          const int64_t theOffset,
                                                          const std::shared_ptr<std::istream> &theOldStream) override;

        //! Opens output stream - will always fail.
        virtual std::shared_ptr<std::ostream> OpenOStream(const TCollection_AsciiString &theUrl,
                                                          const std::ios_base::openmode theMode) override;

        //! Returns TRUE if URL is equal myRootPath protocol.
        virtual Standard_Boolean IsSupportedPath(const TCollection_AsciiString &theUrl) const override;

        //! Returns TRUE if current input stream is opened for reading operations.
        virtual Standard_Boolean IsOpenIStream(const std::shared_ptr<std::istream> &theStream) const override;

        //! Returns TRUE if current output stream is opened for writing operations.
        virtual Standard_Boolean IsOpenOStream(const std::shared_ptr<std::ostream> &theStream) const override;

        //! Opens stream buffer for specified file URL.
        virtual std::shared_ptr<std::streambuf> OpenStreamBuffer(const TCollection_AsciiString &theUrl,
                                                                 const std::ios_base::openmode theMode,
                                                                 const int64_t theOffset,
                                                                 int64_t *theOutBufSize) override;

        //! Returns root path.
        const TCollection_AsciiString &RootPath() const { return myRootPath; }

    private:
        Handle(NCollection_Buffer) myRootBuffer;
        TCollection_AsciiString myRootPath;
    };

} // namespace

#endif // SCRWBase_EMULATEDFILESYSTEM_HEADERFILE
```

实现文件主要功能：
- Create()静态方法：创建文件系统实例，如果路径是s3://开头则预加载缓冲区
- 构造函数：支持从内存缓冲区或文件名创建
- Copy()方法：创建文件系统的副本
- PreloadBuffer()方法：将文件预加载到内存缓冲区
- 重写OSD_FileSystem的虚函数，实现文件系统操作
- 特别处理s3://协议路径，将其整个文件加载到内存

## FileSystemMap.h / FileSystemMap.cpp

文件系统映射实现，维护一个文件系统注册表，用于根据URL选择合适的文件系统。

头文件定义：
```cpp
#ifndef SCRWBase_FILESYSTEMMAP_HEADERFILE
#define SCRWBase_FILESYSTEMMAP_HEADERFILE

#include <NCollection_IndexedDataMap.hxx>
#include <OSD_FileSystem.hxx>

namespace SCRWBase
{
    //! File system implementation which tries to open stream using registered map of file systems (for TypeScript layer).
    class FileSystemMap : public OSD_FileSystem
    {
    public:
        typedef NCollection_IndexedDataMap<Handle(OSD_FileSystem), std::shared_ptr<std::istream>> Map;

        //! Constructor.
        Standard_EXPORT FileSystemMap();

        //! Copy constructor.
        Standard_EXPORT explicit FileSystemMap(const Handle(FileSystemMap) & theOther);

        //! Registers file system.
        //! @param[in] theFileSystem file system to register
        //! @param[in] theToRequirePreload require preload (has affect only for Emscripten pthread build)
        Standard_EXPORT void RegisterProtocol(const Handle(OSD_FileSystem) & theFileSystem, bool theToRequirePreload);

        //! Unregisters file system.
        Standard_EXPORT void RemoveProtocol(const Handle(OSD_FileSystem) & theFileSystem);

    public:
        //! Returns TRUE if URL defines a supported protocol.
        Standard_EXPORT virtual bool IsSupportedPath(const TCollection_AsciiString &theUrl) const override;

        //! Returns TRUE if current input stream is opened for reading operations.
        Standard_EXPORT virtual Standard_Boolean IsOpenIStream(
            const std::shared_ptr<std::istream> &theStream) const override;

        //! Returns TRUE if current output stream is opened for writing operations.
        Standard_EXPORT virtual Standard_Boolean IsOpenOStream(
            const std::shared_ptr<std::ostream> &theStream) const override;

        //! Opens input stream using one of registered protocols.
        Standard_EXPORT virtual std::shared_ptr<std::istream> OpenIStream(
            const TCollection_AsciiString &theUrl,
            const std::ios_base::openmode theMode,
            const int64_t theOffset,
            const std::shared_ptr<std::istream> &theOldStream) override;

        //! Opens output stream using one of registered protocols.
        Standard_EXPORT virtual std::shared_ptr<std::ostream> OpenOStream(const TCollection_AsciiString &theUrl,
                                                                          const std::ios_base::openmode theMode) override;

        //! Opens stream buffer using one of registered protocols.
        Standard_EXPORT virtual std::shared_ptr<std::streambuf> OpenStreamBuffer(const TCollection_AsciiString &theUrl,
                                                                                 const std::ios_base::openmode theMode,
                                                                                 const int64_t theOffset,
                                                                                 int64_t *theOutBufSize) override;

    protected:
        FileSystemMap::Map myProtocols;
    };

} // namespace

#endif // SCRWBase_FILESYSTEMMAP_HEADERFILE
```

实现文件主要功能：
- RegisterProtocol()方法：注册文件系统协议，支持预加载选项
- RemoveProtocol()方法：注销文件系统协议
- 重写OSD_FileSystem的虚函数，实现基于注册表的文件系统操作
- IsSupportedPath()方法：检查URL是否被任何注册的文件系统支持
- OpenIStream()方法：使用注册的协议之一打开输入流
- OpenOStream()方法：打开输出流（当前实现总是返回空指针）
- OpenStreamBuffer()方法：使用注册的协议之一打开流缓冲区

## IPromise.h

承诺接口定义，是一个纯虚基类。

```cpp
#ifndef IPromise_H_
#define IPromise_H_

#include <Standard_Macro.hxx>
#include <string>

namespace SCRWBase {
    extern "C" struct IPromise {
        virtual ~IPromise() {}

        Standard_EXPORT virtual void resolve() = 0;

        Standard_EXPORT virtual void reject(const std::string &theErr) = 0;
    };
}

#endif // IPromise_H_
```

功能：
- 定义了异步操作的承诺接口
- 包含resolve()和reject()两个纯虚方法
- resolve()表示操作成功完成
- reject()表示操作失败，并传递错误信息

## JsonHelper.h

RapidJSON库的辅助类，提供更安全的JSON访问方法。

```cpp
#ifndef JSONHELPER_HEADERFILE
#define JSONHELPER_HEADERFILE

#ifndef HAVE_RAPIDJSON
    #define HAVE_RAPIDJSON
#endif

#include <Standard_WarningsDisable.hxx> // NOLINT
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <Standard_WarningsRestore.hxx> // NOLINT

#include <sstream>

namespace SCRWBase {
//! Helper class for working with rapidjson.
class JsonHelper {
public:
    typedef rapidjson::Writer<rapidjson::OStreamWrapper> Writer;
    typedef rapidjson::Document::ValueType               Value;

    //! Find member of the object in a safe way.
    static const Value* FindMember(const Value& theObject, const char* theName)
    {
        if (!theObject.IsObject()) {
            return nullptr;
        }

        rapidjson::Document::ConstMemberIterator anIter = theObject.FindMember(theName);
        return anIter != theObject.MemberEnd() ? &anIter->value : nullptr;
    }

    //! Find bool member of the object in a safe way.
    static const Value* FindBoolMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsBool() ? aRes : nullptr;
    }

    //! Find int member of the object in a safe way.
    static const Value* FindIntMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsInt() ? aRes : nullptr;
    }

    //! Find double member of the object in a safe way.
    static const Value* FindDoubleMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsDouble() ? aRes : nullptr;
    }

    //! Find string member of the object in a safe way.
    static const Value* FindStringMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsString() ? aRes : nullptr;
    }

    //! Find object member of the object in a safe way.
    static const Value* FindObjectMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsObject() ? aRes : nullptr;
    }

    //! Find array member of the object in a safe way.
    static const Value* FindArrayMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsArray() ? aRes : nullptr;
    }

    //! Get Json from string.
    static bool ParseFromString(const std::string& theJsonStr, rapidjson::Document& theReader)
    {
        std::istringstream aStr(theJsonStr);

        rapidjson::IStreamWrapper aStrStream(aStr);
        rapidjson::ParseResult    aParseRes =
            theReader.ParseStream<rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseFullPrecisionFlag,
                                  rapidjson::UTF8<>,
                                  rapidjson::IStreamWrapper>(aStrStream);
        return !aParseRes.IsError();
    }
};

}

#endif // JSONHELPER_HEADERFILE
```

主要功能：
- 对RapidJSON库的包装，提供安全的JSON成员访问方法
- FindMember()系列函数：安全地查找JSON对象中的成员，包括特定类型的成员
- ParseFromString()方法：从字符串解析JSON
- 使用rapidjson::Document::ValueType作为值类型
- 包含类型检查，确保访问的成员是预期类型

## TaskManager.h / TaskManager.cpp

WebAssembly任务管理器，允许在主线程和异步线程中运行任务。

头文件定义：
```cpp
#ifndef SCRWBase_TASKMANAGER_HEADERFILE
#define SCRWBase_TASKMANAGER_HEADERFILE

#include <Message.hxx>
#include <OSD_Thread.hxx>
#include <Standard_Condition.hxx>
#include <Standard_Mutex.hxx>

#include <deque>
#include <functional>

namespace SCRWBase
{
    //! Wasm task manager allows to run tasks in main thread and asynchronously.
    class TaskManager : public Standard_Transient
    {
    public:
        //! Emscripten main loop controller.
        class MainEmscriptenLoop
        {
        public:
            //! Interface for TaskManager::OnRunMain().
            Standard_EXPORT void operator()() const;

        private:
            //! Set up main-loop runner (runOnce callback)
            Standard_EXPORT static void setupRun();

            //! Callback to be executed in main loop.
            Standard_EXPORT static void runOnce();

        private:
            Standard_EXPORT static bool myIsRunning; //!< callback initialization state
        };

    public:
        //! Return global instance.
        Standard_EXPORT static const Handle(TaskManager) & Instance();

        //! Set global instance.
        Standard_EXPORT static void SetInstance(const Handle(TaskManager) & theInstance);

    public:
        //! Run task asynchronously
        Standard_EXPORT virtual void RunAsync(const std::function<void()> &theFunc);

        //! Run task in main thread
        Standard_EXPORT virtual void RunMain(const std::function<void()> &theFunc);

        //! Run tasks in parallel mode if allowed
        Standard_EXPORT virtual void RunInParallel(int theStartIdx,
                                                   int theEndIdx,
                                                   const std::function<void(int theThreadIndex, int theObjIndex)> &theFunc,
                                                   bool theIsForceSingleThread = false);

        //! Add callback on run main event
        virtual void OnRunMain(std::function<void()> theFunc) { myOnRunMain = theFunc; }

        //! Run one task from main queue or several tasks in singlethread mode (return false is no one executed)
        Standard_EXPORT virtual bool RunOnce();

        //! Return number of threads to launch
        int NbDefaultThreadsToLaunch() const { return myDefaultThreadsToLaunch; }

        //! Set Default number of threads to launch,
        //! if < 0 then will be set max (1, maxNumber of logical processors - 1).
        //! @param[in] theNumberOfThreads default number of threads to set
        Standard_EXPORT void SetDefaultThreadsToLaunch(int theNumberOfThreads);

        //! Return multithreading mode status
        bool IsMultithreadingMode() const { return myIsMultitreadingOn; }

        //! Set multithreading mode
        Standard_EXPORT void SetMultithreadingMode(bool theIsMultithreadingOn);

        //! Destructor.
        Standard_EXPORT ~TaskManager();

    protected:
        //! Empty constructor.
        Standard_EXPORT TaskManager();

        //! Create and init a new task manager object.
        Standard_EXPORT static TaskManager *create();

        //! Return global instance.
        Standard_EXPORT static Handle(TaskManager) & instance();

    private:
        //! Represents async queue.
        class TaskQueue
        {
        public:
            //! Add function to the queue.
            Standard_EXPORT void Add(const std::function<void()> &theFunc);

            //! Try to extract front element of queue and call it.
            Standard_EXPORT bool RunOnce();

            //! Run tasks in a loop for a given period of time.
            Standard_EXPORT bool RunMany(double theThreshold);

        private:
            Standard_Mutex myMutex;
            std::deque<std::function<void()>> myQueue;
        };

    private:
        static Standard_Address run(Standard_Address theData);

        // default time threshold
        static constexpr double m_defaultTimeThreshold = 0.025;

        //! Release resources.
        Standard_EXPORT void release() noexcept;

    private:
        TaskQueue myMainQueue;
        TaskQueue myBackgroundQueue;
        OSD_Thread myBackgroundThread;
        Standard_Condition myWaitCondition;
        std::function<void()> myOnRunMain;
        double myTimeThreshold = m_defaultTimeThreshold;
        int myDefaultThreadsToLaunch = 1;
        bool myIsRunning = true;
        bool myIsMultitreadingOn = false;
    };

}

#endif // SCRWBase_TASKMANAGER_HEADERFILE
```

实现文件主要功能：
- TaskQueue内部类：表示异步队列，包含添加任务、运行一次任务、运行多个任务的方法
- Instance()静态方法：返回全局实例
- RunAsync()方法：异步运行任务
- RunMain()方法：在主线程中运行任务
- RunInParallel()方法：在允许的情况下并行运行任务
- RunOnce()方法：从主队列运行一个任务或在单线程模式下运行多个任务
- SetDefaultThreadsToLaunch()方法：设置默认启动的线程数
- SetMultithreadingMode()方法：设置多线程模式
- 特别优化了WebAssembly环境（包括Emscripten pthreads）的支持
- MainEmscriptenLoop内部类：Emscripten主循环控制器

## ValueConverter.h

枚举值和字符串之间的双向映射器模板类。

```cpp
#ifndef _VALUECONVERTER_HEADERFILE
#define _VALUECONVERTER_HEADERFILE

#include <NCollection_DoubleMap.hxx>
#include <TCollection_AsciiString.hxx>

namespace SCRWBase
{
    //! Bidirectional mapper between enumeration items and strings.
    //! This template class should not be instantiated directly. Instead, a specialized converter class
    //! (ConverterType template parameter) should be derived from this base to fill the internal map
    //! for a specific enum type:
    //! @code
    //! class SomeConverter : public ValueConverter<SomeEnum, SomeConverter> {...}
    //! @endcode
    template <typename ValueType, typename ConverterType>
    class ValueConverter
    {
    public:
        //! Hash value, for Map interface.
        static int HashCode(const ValueType theType, const int theUpper)
        {
            unsigned int aHashCode = (unsigned int)::HashCode((int)theType, theUpper);
            return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
        }

        //! Matching two instances, for Map interface.
        static bool IsEqual(const ValueType theType1, const ValueType theType2) { return theType1 == theType2; }

        typedef NCollection_DoubleMap<ValueType, TCollection_AsciiString, ValueConverter<ValueType, ConverterType>>
            MapOfValues;

        //! Return string description by value.
        static const char *GetString(const ValueType theType) { return instance()->getString(theType); }

        //! Return value by string description.
        static ValueType GetValue(const char *theString) { return instance()->getValue(theString); }

    protected:
        //! Empty constructor made protected to suppress direct creation of this class' instances.
        ValueConverter() {}

        //! Return internal "enum <--> string" map
        MapOfValues &valueMap() { return myMap; }

    private:
        //! Return an instance of specialized converter class
        static ConverterType *instance()
        {
            static ConverterType *anInstance = nullptr;
            if (anInstance == nullptr)
            {
                anInstance = new ConverterType();
            }
            return anInstance;
        }

        //! Return string description by value.
        const char *getString(const ValueType theType) const
        {
            const TCollection_AsciiString *aResult = myMap.Seek1(theType);
            return (aResult == nullptr ? "" : aResult->ToCString());
        }

        //! Return value by string description.
        ValueType getValue(const char *theString) const
        {
            const ValueType *aResult = myMap.Seek2(theString);
            return (aResult == nullptr ? ValueType::Unknown : *aResult);
        }

    private:
        MapOfValues myMap; //!< Bidirectional "enum <--> string" map
    };

} // namespace

#endif // _VALUECONVERTER_HEADERFILE
```

主要功能：
- 模板类，用于在枚举值和字符串描述之间进行双向转换
- 使用NCollection_DoubleMap实现双向映射
- GetString()静态方法：根据值返回字符串描述
- GetValue()静态方法：根据字符串描述返回值
- HashCode()和IsEqual()方法：为映射接口提供哈希和比较功能
- 设计为基类，需要派生类来填充具体的映射关系
- 使用单例模式确保每个ConverterType只有一个实例

## CMakeLists.txt

CMake构建配置文件，定义了SCRWBase库的构建规则。

主要功能：
- 定义SCRWBase项目
- 指定源文件（.cpp）和头文件（.h）
- 使用source_group组织源文件和头文件
- 创建库（静态或动态取决于OCCTViewer_LIBTYPE）
- 链接OCCT核心库（TKernel、TKMath、TKService等）
- 配置安装目标，安装库文件和头文件