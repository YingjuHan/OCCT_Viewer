#include <RWBase/FileSystemMap.h>

#include <RWBase/EmulatedFileSystem.h>

#include <Message.hxx>
#include <OSD_LocalFileSystem.hxx>
#include <Standard_NullObject.hxx>

namespace RWBase
{

    //! 默认构造，初始化空映射。
    FileSystemMap::FileSystemMap()
    {
        //
    }

    //! 复制构造函数，共享另一实例的协议映射。
    FileSystemMap::FileSystemMap(const Handle(FileSystemMap) & theOther) : myProtocols(theOther->myProtocols)
    {
        //
    }

    //! 注册协议并按需预加载缓存。
    void FileSystemMap::RegisterProtocol(const Handle(OSD_FileSystem) & theFileSystem, bool theToRequireCaching)
    {
        if (theFileSystem.IsNull())
        {
            throw Standard_NullObject("FileSystemMap::RegisterProtocol() - null argument");
        }

        const std::shared_ptr<std::istream> *aStream = myProtocols.Seek(theFileSystem);
        if (aStream != nullptr)
        {
            return;
        }

#if defined(__EMSCRIPTEN_PTHREADS__)
        if (theToRequireCaching)
        {
            Handle(RWBase::EmulatedFileSystem) aFileSystem = Handle(RWBase::EmulatedFileSystem)::DownCast(
                theFileSystem);
            if (!aFileSystem.IsNull())
            {
                if (!aFileSystem->PreloadBuffer())
                {
                    Message::SendFail() << "Error: can't preload buffer";
                }
            }
        }
#else
        (void)theToRequireCaching;
#endif
        myProtocols.Add(theFileSystem, NULL);
    }

    //! 根据句柄移除指定协议。
    void FileSystemMap::RemoveProtocol(const Handle(OSD_FileSystem) & theFileSystem)
    {
        myProtocols.RemoveKey(theFileSystem);
    }

    //! 判断 URL 是否被任何注册协议识别。
    Standard_Boolean FileSystemMap::IsSupportedPath(const TCollection_AsciiString &theUrl) const
    {
        for (FileSystemMap::Map::Iterator aProtIter(myProtocols); aProtIter.More(); aProtIter.Next())
        {
            if (aProtIter.Key()->IsSupportedPath(theUrl))
            {
                return true;
            }
        }
        return false;
    }

    //! 检查给定输出流是否来自任一子文件系统。
    Standard_Boolean FileSystemMap::IsOpenOStream(const std::shared_ptr<std::ostream> &theStream) const
    {
        std::shared_ptr<OSD_OStreamBuffer> aFileStream = std::dynamic_pointer_cast<OSD_OStreamBuffer>(theStream);
        if (aFileStream.get() == NULL)
        {
            return false;
        }
        for (FileSystemMap::Map::Iterator aProtIter(myProtocols); aProtIter.More(); aProtIter.Next())
        {
            const Handle(OSD_FileSystem) &aFS = aProtIter.Key();
            if (aFS->IsSupportedPath(TCollection_AsciiString(aFileStream->Url().c_str())))
            {
                if (aFS->IsOpenOStream(theStream))
                {
                    return true;
                }
            }
        }
        return false;
    }

    //! 检查给定输入流是否来自任一子文件系统。
    Standard_Boolean FileSystemMap::IsOpenIStream(const std::shared_ptr<std::istream> &theStream) const
    {
        std::shared_ptr<OSD_IStreamBuffer> aFileStream = std::dynamic_pointer_cast<OSD_IStreamBuffer>(theStream);
        if (aFileStream.get() == NULL)
        {
            return false;
        }
        for (FileSystemMap::Map::Iterator aProtIter(myProtocols); aProtIter.More(); aProtIter.Next())
        {
            const Handle(OSD_FileSystem) &aFS = aProtIter.Key();
            if (aFS->IsSupportedPath(TCollection_AsciiString(aFileStream->Url().c_str())))
            {
                if (aFS->IsOpenIStream(theStream))
                {
                    return true;
                }
            }
        }
        return false;
    }

    //! 根据 URL 找到合适协议并打开输入流，同时缓存结果。
    std::shared_ptr<std::istream> FileSystemMap::OpenIStream(const TCollection_AsciiString &theUrl,
                                                             const std::ios_base::openmode theMode,
                                                             const int64_t theOffset,
                                                             const std::shared_ptr<std::istream> &theOldStream)
    {
        for (FileSystemMap::Map::Iterator aProtIter(myProtocols); aProtIter.More(); aProtIter.Next())
        {
            const Handle(OSD_FileSystem) &aFileSystem = aProtIter.Key();
            if (aFileSystem->IsSupportedPath(theUrl))
            {
                std::shared_ptr<std::istream> &aStream = aProtIter.ChangeValue();

                if (aStream == nullptr)
                {
                    std::shared_ptr<std::istream> anOpenedStream =
                        aFileSystem->OpenIStream(theUrl, theMode, theOffset, theOldStream);
                    aStream = std::move(anOpenedStream);
                }

                if (!(aStream)->good())
                {
                    (aStream)->clear();
                }
                if (theOffset >= 0)
                {
                    (aStream)->seekg((std::streamoff)theOffset, std::ios_base::beg);
                }
                return (aStream);
            }
        }
        return std::shared_ptr<std::istream>();
    }

    //! 输出流未实现，始终返回空指针。
    std::shared_ptr<std::ostream> FileSystemMap::OpenOStream(const TCollection_AsciiString &theUrl,
                                                             const std::ios_base::openmode theMode)
    {
        (void)theUrl;
        (void)theMode;
        return std::shared_ptr<std::ostream>();
    }

    //! 调用匹配协议的 streambuf 打开接口，若失败继续尝试其它协议。
    std::shared_ptr<std::streambuf> FileSystemMap::OpenStreamBuffer(const TCollection_AsciiString &theUrl,
                                                                    const std::ios_base::openmode theMode,
                                                                    const int64_t theOffset,
                                                                    int64_t *theOutBufSize)
    {
        for (FileSystemMap::Map::Iterator aProtIter(myProtocols); aProtIter.More(); aProtIter.Next())
        {
            const Handle(OSD_FileSystem) &aFileSystem = aProtIter.Key();
            if (aFileSystem->IsSupportedPath(theUrl))
            {
                std::shared_ptr<std::streambuf> aBuf =
                    aFileSystem->OpenStreamBuffer(theUrl, theMode, theOffset, theOutBufSize);
                if (aBuf.get() != NULL)
                {
                    return aBuf;
                }
            }
        }
        return std::shared_ptr<std::streambuf>();
    }

} // namespace RWBase
