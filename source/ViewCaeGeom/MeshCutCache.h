#ifndef _MESHCUTCACHE_HEADERFILE
#define _MESHCUTCACHE_HEADERFILE
#include <Standard_Macro.hxx>
#include <cstdint>
#include <memory>
#include <atomic>
namespace ViewCaeGeom {
struct MeshCutCachePolicy {
    uint64_t SourceBytes=4ULL*1024*1024*1024;
    uint64_t ResultBytes=16ULL*1024*1024*1024;
};
struct MeshCutCacheStatistics {
    uint64_t SourceReserved=0,ResultReserved=0,PeakSource=0,PeakResult=0,Evictions=0;
};
//! Conservative logical allocations are reserved before source/result construction.
//! A lease stays charged while exported geometry/presentations retain it.
class Standard_EXPORT MeshCutCache {
public:
    MeshCutCache();
    bool SetPolicy(const MeshCutCachePolicy& policy);
    MeshCutCachePolicy Policy() const;
    MeshCutCacheStatistics Statistics() const;
    std::shared_ptr<void> Reserve(uint64_t bytes,bool source);
    bool CanReserve(uint64_t bytes,bool source) const;
    void Evicted();
private:
    struct State;
    std::shared_ptr<State> myState;
    MeshCutCachePolicy myPolicy;
};
struct MeshCutCancellation { std::atomic<bool> Cancelled{false}; void Cancel() { Cancelled.store(true); } };
struct MeshCutFrameTimings { double SourceMs=0,IntersectionMs=0,FieldsMs=0,SliceMs=0,TraceMs=0; };
}
#endif
