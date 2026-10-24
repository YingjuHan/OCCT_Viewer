#ifndef _NATIVECUTDIGEST_HEADERFILE
#define _NATIVECUTDIGEST_HEADERFILE
#include <Standard_Macro.hxx>
#include <string>
#include <cstdint>
#include <array>
namespace Viewer {
//! Portable streaming SHA-256 for immutable session source identity.
class Standard_EXPORT NativeCutDigest {
public:
    void Update(const void* data,size_t size);
    std::string Finish();
    static std::string Text(const std::string& text);
    static std::string File(const std::string& path,uint64_t maximumBytes);
private:
    void block(const unsigned char* data);
    std::array<uint32_t,8> myState={{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19}};
    std::array<unsigned char,64> myBuffer{};
    uint64_t myBytes=0; size_t myUsed=0; bool myFinished=false;
};
}
#endif
