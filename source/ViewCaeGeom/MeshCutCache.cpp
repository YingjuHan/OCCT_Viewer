#include <ViewCaeGeom/MeshCutCache.h>
#include <stdexcept>
#include <string>
#include <algorithm>
namespace ViewCaeGeom {
struct MeshCutCache::State { MeshCutCacheStatistics Statistics; };
MeshCutCache::MeshCutCache():myState(std::make_shared<State>()) {}
bool MeshCutCache::SetPolicy(const MeshCutCachePolicy& policy) { if(!policy.SourceBytes||!policy.ResultBytes||policy.SourceBytes<myState->Statistics.SourceReserved||policy.ResultBytes<myState->Statistics.ResultReserved) { return false; } myPolicy=policy; return true; }
MeshCutCachePolicy MeshCutCache::Policy() const { return myPolicy; }
MeshCutCacheStatistics MeshCutCache::Statistics() const { return myState->Statistics; }
bool MeshCutCache::CanReserve(uint64_t bytes,bool source) const { const auto used=source?myState->Statistics.SourceReserved:myState->Statistics.ResultReserved,limit=source?myPolicy.SourceBytes:myPolicy.ResultBytes; return bytes<=limit && used<=limit-bytes; }
std::shared_ptr<void> MeshCutCache::Reserve(uint64_t bytes,bool source)
{
    if(!CanReserve(bytes,source)) {
        const auto used=source?myState->Statistics.SourceReserved:myState->Statistics.ResultReserved;
        const auto limit=source?myPolicy.SourceBytes:myPolicy.ResultBytes;
        throw std::length_error(std::string(source?"Cut source cache budget exceeded before preparation":"Cut result cache budget exceeded before allocation")
            +" (requested "+std::to_string(bytes)+", retained "+std::to_string(used)+", limit "+std::to_string(limit)+")");
    }
    struct Reservation {
        std::shared_ptr<State> Owner; uint64_t Bytes=0; bool Source=false,Charged=false;
        ~Reservation() { if(Charged) { (Source?Owner->Statistics.SourceReserved:Owner->Statistics.ResultReserved)-=Bytes; } }
    };
    auto lease=std::make_shared<Reservation>(); lease->Owner=myState; lease->Bytes=bytes; lease->Source=source;
    auto& s=myState->Statistics; auto& used=source?s.SourceReserved:s.ResultReserved; auto& peak=source?s.PeakSource:s.PeakResult;
    used+=bytes; peak=std::max(peak,used); lease->Charged=true; return lease;
}
void MeshCutCache::Evicted() { ++myState->Statistics.Evictions; }
}
