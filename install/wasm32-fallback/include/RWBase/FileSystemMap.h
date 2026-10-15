#ifndef RWBase_FILESYSTEMMAP_HEADERFILE
#define RWBase_FILESYSTEMMAP_HEADERFILE

#include <NCollection_IndexedDataMap.hxx>
#include <OSD_FileSystem.hxx>

namespace RWBase
{
    //! 组合多个文件系统的路由器，实现按协议分发流的功能。
    class FileSystemMap : public OSD_FileSystem
    {
    public:
        typedef NCollection_IndexedDataMap<Handle(OSD_FileSystem), std::shared_ptr<std::istream>> Map;

        //! 默认构造，初始化空的协议映射表。
        Standard_EXPORT FileSystemMap();

        //! 复制构造，复用另一映射表中的协议和流。
        Standard_EXPORT explicit FileSystemMap(const Handle(FileSystemMap) & theOther);

        //! 注册新的子文件系统。
        //! @param[in] theFileSystem 待注册的协议实现
        //! @param[in] theToRequirePreload 是否要求预加载（仅在 Emscripten pthreads 构建生效）
        Standard_EXPORT void RegisterProtocol(const Handle(OSD_FileSystem) & theFileSystem, bool theToRequirePreload);

        //! 根据句柄注销已注册的文件系统。
        Standard_EXPORT void RemoveProtocol(const Handle(OSD_FileSystem) & theFileSystem);

    public:
        //! 判断 URL 是否由任一已注册协议支持。
        Standard_EXPORT virtual bool IsSupportedPath(const TCollection_AsciiString &theUrl) const override;

        //! 判断输入流是否由映射表中的任一文件系统打开。
        Standard_EXPORT virtual Standard_Boolean IsOpenIStream(
            const std::shared_ptr<std::istream> &theStream) const override;

        //! 判断输出流是否由映射表中的任一文件系统打开。
        Standard_EXPORT virtual Standard_Boolean IsOpenOStream(
            const std::shared_ptr<std::ostream> &theStream) const override;

        //! 依据 URL 协议打开输入流并缓存句柄。
        Standard_EXPORT virtual std::shared_ptr<std::istream> OpenIStream(
            const TCollection_AsciiString &theUrl,
            const std::ios_base::openmode theMode,
            const int64_t theOffset,
            const std::shared_ptr<std::istream> &theOldStream) override;

        //! 依据 URL 协议打开输出流（当前实现返回空，因为只读）。
        Standard_EXPORT virtual std::shared_ptr<std::ostream> OpenOStream(const TCollection_AsciiString &theUrl,
                                                                          const std::ios_base::openmode theMode) override;

        //! 依据 URL 协议打开底层 streambuf 以便高效读取。
        Standard_EXPORT virtual std::shared_ptr<std::streambuf> OpenStreamBuffer(const TCollection_AsciiString &theUrl,
                                                                                 const std::ios_base::openmode theMode,
                                                                                 const int64_t theOffset,
                                                                                 int64_t *theOutBufSize) override;

    protected:
        FileSystemMap::Map myProtocols; //!< 记录协议文件系统与其共享输入流之间的映射。
    };

} // namespace

#endif // RWBase_FILESYSTEMMAP_HEADERFILE
