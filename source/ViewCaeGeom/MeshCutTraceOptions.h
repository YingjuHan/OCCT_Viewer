#ifndef _MESHCUTTRACEOPTIONS_HEADERFILE
#define _MESHCUTTRACEOPTIONS_HEADERFILE
#include <Standard_Macro.hxx>
#include <cstdint>
#include <vector>
namespace ViewCaeGeom {
enum class MeshCutTracePolicy { Lowest, Negative, Positive, Region };
//! Per-cut source selection. Physical regions never inherit display recovery regions.
struct MeshCutTraceOptions {
    MeshCutTracePolicy Policy = MeshCutTracePolicy::Lowest;
    int64_t Region = 0;
    std::vector<int64_t> Regions;
    Standard_EXPORT bool IsValid(size_t elements) const;
};
}
#endif
