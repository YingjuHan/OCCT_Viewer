#ifndef RWVTK_RESOURCELIMITS_HEADERFILE
#define RWVTK_RESOURCELIMITS_HEADERFILE
#include <cstdint>
#include <memory>
#include <istream>
#include <string>
#include <stdexcept>

namespace RWVtk {
//! Logical import budgets. All values are positive; limits cannot disable checks.
struct VtkResourceLimits {
    uint64_t MaxFileBytes = uint64_t(512) << 20;
    uint64_t MaxInputBytes = uint64_t(2) << 30;
    uint64_t MaxArrayBytes = uint64_t(256) << 20;
    uint64_t MaxDecodedBytes = uint64_t(1) << 30;
    uint64_t MaxArrays = 4096;
    uint64_t MaxComponents = 1024;
    uint64_t MaxNodes = 10000000;
    uint64_t MaxCells = 10000000;
    uint64_t MaxFiles = 4096;
    uint64_t MaxDepth = 64;
};
struct VtkResourceBudget {
    VtkResourceLimits Limits;
    uint64_t InputBytes = 0, DecodedBytes = 0, Arrays = 0, Nodes = 0, Cells = 0, Files = 0;
    explicit VtkResourceBudget(const VtkResourceLimits& limits) : Limits(limits) {}
    static void Consume(uint64_t& total, uint64_t amount, uint64_t limit, const char* name)
    {
        if (!limit || total > limit || amount > limit-total) { throw std::runtime_error(std::string("VTK resource limit: ")+name); }
        total += amount;
    }
    void Array(uint64_t bytes)
    {
        if (bytes > Limits.MaxArrayBytes) { throw std::runtime_error("VTK resource limit: array bytes"); }
        Consume(DecodedBytes,bytes,Limits.MaxDecodedBytes,"decoded bytes");
    }
    //! Check every chunk before growing the file buffer; valid for non-seekable streams.
    std::string Read(std::istream& stream)
    {
        Consume(Files,1,Limits.MaxFiles,"files");
        std::string text; char chunk[16384];
        while (stream.read(chunk,sizeof(chunk)) || stream.gcount()) {
            const auto bytes=(uint64_t)stream.gcount();
            if (text.size()>Limits.MaxFileBytes || bytes>Limits.MaxFileBytes-text.size()) { throw std::runtime_error("VTK resource limit: file bytes"); }
            Consume(InputBytes,bytes,Limits.MaxInputBytes,"input bytes");
            text.append(chunk,(size_t)bytes);
        }
        if (stream.bad()) { throw std::runtime_error("Cannot read VTK input stream"); }
        return text;
    }
};
}
#endif
