#ifndef RWBase_EMULATEDFILESYSTEM_HEADERFILE
#define RWBase_EMULATEDFILESYSTEM_HEADERFILE

#include <NCollection_Buffer.hxx>
#include <OSD_FileSystem.hxx>

namespace RWBase
{
    //! 仅包含单文件缓存的模拟文件系统，供 TypeScript 层以内存方式访问。
    class EmulatedFileSystem : public OSD_FileSystem
    {
    public:
        //! 根据文件路径创建模拟文件系统实例。
        Standard_EXPORT static Handle(RWBase::EmulatedFileSystem) Create(const TCollection_AsciiString &theFilePath);

    public:
        //! 使用现有内存缓冲区构造，避免重复加载磁盘文件。
        Standard_EXPORT EmulatedFileSystem(const TCollection_AsciiString &theRootFile,
                                           const Handle(NCollection_Buffer) & theBuffer);

        //! 仅记录根文件路径的构造函数，缓冲区稍后再加载。
        Standard_EXPORT explicit EmulatedFileSystem(const TCollection_AsciiString &theRootFile);

        //! 复制当前文件系统，复用缓冲区或路径。
        Standard_EXPORT Handle(EmulatedFileSystem) Copy();

        //! 如果尚未加载，则将文件内容预读进缓冲区。
        //! @return true 表示缓冲区可用。
        Standard_EXPORT bool PreloadBuffer();

        //! 访问根文件对应的缓冲区句柄。
        const Handle(NCollection_Buffer) & Buffer() const { return myRootBuffer; }

        //! 打开只读输入流；若之前打开过相同 URL，则复用旧流。
        virtual std::shared_ptr<std::istream> OpenIStream(const TCollection_AsciiString &theUrl,
                                                          const std::ios_base::openmode theParams,
                                                          const int64_t theOffset,
                                                          const std::shared_ptr<std::istream> &theOldStream) override;

        //! 打开输出流，此实现始终返回失败。
        virtual std::shared_ptr<std::ostream> OpenOStream(const TCollection_AsciiString &theUrl,
                                                          const std::ios_base::openmode theMode) override;

        //! 判断 URL 是否与根路径协议一致。
        virtual Standard_Boolean IsSupportedPath(const TCollection_AsciiString &theUrl) const override;

        //! 检查输入流是否由该文件系统打开（始终返回 true）。
        virtual Standard_Boolean IsOpenIStream(const std::shared_ptr<std::istream> &theStream) const override;

        //! 检查输出流是否由该文件系统打开（始终返回 false）。
        virtual Standard_Boolean IsOpenOStream(const std::shared_ptr<std::ostream> &theStream) const override;

        //! 打开底层缓冲区，若已缓存则返回数组缓冲。
        virtual std::shared_ptr<std::streambuf> OpenStreamBuffer(const TCollection_AsciiString &theUrl,
                                                                 const std::ios_base::openmode theMode,
                                                                 const int64_t theOffset,
                                                                 int64_t *theOutBufSize) override;

        //! 获取根文件路径字符串。
        const TCollection_AsciiString &RootPath() const { return myRootPath; }

    private:
        Handle(NCollection_Buffer) myRootBuffer; //!< 持有整文件数据的共享缓冲。
        TCollection_AsciiString myRootPath; //!< 根文件路径或虚拟 URL。
    };

} // namespace

#endif // RWBase_EMULATEDFILESYSTEM_HEADERFILE
