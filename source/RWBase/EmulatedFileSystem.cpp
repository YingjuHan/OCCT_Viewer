#include <RWBase/EmulatedFileSystem.h>

#include <Message.hxx>
#include <OSD_LocalFileSystem.hxx>
#include <Standard_ArrayStreamBuffer.hxx>

namespace RWBase
{

    //! 根据路径创建模拟文件系统，必要时会预加载远程文件。
    Handle(RWBase::EmulatedFileSystem) EmulatedFileSystem::Create(const TCollection_AsciiString &theFilePath)
    {
        Handle(RWBase::EmulatedFileSystem) aFileSystem = new RWBase::EmulatedFileSystem(theFilePath);
        // @c if (OSD_Path::IsRemoteProtocolPath (aFileName.ToCString()))
        if (theFilePath.StartsWith("s3://"))
        {
            // partial remote downloads are not implemented - it is better caching entire file
            if (!aFileSystem->PreloadBuffer())
            {
                Message::SendFail() << "Error: '" << theFilePath << "' cannot be opened";
                return Handle(RWBase::EmulatedFileSystem)();
            }
        }
        return aFileSystem;
    }

    //! 以已有缓冲区构造，可直接复用内容。
    EmulatedFileSystem::EmulatedFileSystem(const TCollection_AsciiString &theRootFile,
                                           const Handle(NCollection_Buffer) & theBuffer)
        : myRootBuffer(theBuffer),
          myRootPath(theRootFile)
    {
        //
    }

    //! 仅保存路径的构造函数，缓冲区稍后再加载。
    EmulatedFileSystem::EmulatedFileSystem(const TCollection_AsciiString &theRootFile) : myRootPath(theRootFile)
    {
        //
    }

    //! 复制当前文件系统，保留路径与数据。
    Handle(EmulatedFileSystem) EmulatedFileSystem::Copy()
    {
        if (!myRootBuffer.IsNull())
        {
            return new EmulatedFileSystem(myRootPath, myRootBuffer);
        }
        else
        {
            return new EmulatedFileSystem(myRootPath);
        }
    }

    //! 判断 URL 是否为当前模拟文件系统管理的路径。
    Standard_Boolean EmulatedFileSystem::IsSupportedPath(const TCollection_AsciiString &theUrl) const
    {
        return theUrl.IsEqual(myRootPath);
    }

    //! 模拟文件系统中输入流始终视为打开状态。
    Standard_Boolean EmulatedFileSystem::IsOpenIStream(const std::shared_ptr<std::istream> &theStream) const
    {
        (void)theStream;
        return true;
    }

    //! 不支持输出，因此永远返回 false。
    Standard_Boolean EmulatedFileSystem::IsOpenOStream(const std::shared_ptr<std::ostream> &theStream) const
    {
        (void)theStream;
        return false;
    }

    //! 将根文件读取至缓冲区，以便后续无磁盘 I/O。
    bool EmulatedFileSystem::PreloadBuffer()
    {
        if (!myRootBuffer.IsNull())
        {
            return true;
        }

        std::shared_ptr<std::istream> aStream = this->OpenIStream(myRootPath, std::ios::in | std::ios::binary, 0, nullptr);
        if (aStream == nullptr || !*aStream)
        {
            return false;
        }

        aStream->seekg(0, aStream->end);
        int64_t aLength = aStream->tellg();
        if (aLength < 0 || aLength > (int64_t)IntegerLast())
        {
            Message::SendFail() << "Error: '" << myRootPath << "' has invalid length=" << (double)aLength;
            return false;
        }

        aStream->seekg(0, aStream->beg);

        myRootBuffer = new NCollection_Buffer(NCollection_BaseAllocator::CommonBaseAllocator());
        if (!myRootBuffer->Allocate((int)aLength))
        {
            Message::SendFail() << "Error: can't allocate buffer with length=" << (int)aLength << " for '" << myRootPath
                                << "'";
            return false;
        }

        aStream->read((Standard_Character *)myRootBuffer->Data(), (int)aLength);
        return true;
    }

    //! 如果已有缓存则返回数组缓冲，否则委托默认文件系统。
    std::shared_ptr<std::streambuf> EmulatedFileSystem::OpenStreamBuffer(const TCollection_AsciiString &theUrl,
                                                                         const std::ios_base::openmode theMode,
                                                                         const int64_t theOffset,
                                                                         int64_t *theOutBufSize)
    {
        if (!myRootBuffer.IsNull())
        {
            std::shared_ptr<std::streambuf> aFileBuf;
            aFileBuf.reset(new Standard_ArrayStreamBuffer((Standard_Character *)myRootBuffer->Data(), myRootBuffer->Size()));
            return aFileBuf;
        }
        return OSD_FileSystem::DefaultFileSystem()->OpenStreamBuffer(theUrl, theMode, theOffset, theOutBufSize);
    }

    //! 当缓存存在时走父类实现，否则回退到默认文件系统。
    std::shared_ptr<std::istream> EmulatedFileSystem::OpenIStream(const TCollection_AsciiString &theUrl,
                                                                  const std::ios_base::openmode theParams,
                                                                  const int64_t theOffset,
                                                                  const std::shared_ptr<std::istream> &theOldStream)
    {
        if (!IsSupportedPath(theUrl))
        {
            return std::shared_ptr<std::istream>();
        }

        if (!myRootBuffer.IsNull())
        {
            return OSD_FileSystem::OpenIStream(theUrl, theParams, theOffset, theOldStream);
        }
        return OSD_FileSystem::DefaultFileSystem()->OpenIStream(theUrl, theParams, theOffset, nullptr);
    }

    //! 输出流不被支持，统一返回空指针。
    std::shared_ptr<std::ostream> EmulatedFileSystem::OpenOStream(const TCollection_AsciiString &, const std::ios_base::openmode)
    {
        return std::shared_ptr<std::ostream>();
    }

} // namespace
