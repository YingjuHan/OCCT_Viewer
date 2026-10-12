#ifndef _MESHCUTRECOVERY_HEADERFILE
#define _MESHCUTRECOVERY_HEADERFILE
#include <RWCae/MeshData.h>
#include <ViewCaeGeom/MeshCutTensor.h>
#include <cstdint>
#include <vector>

namespace ViewCaeGeom {
struct MeshCutRecoveryOptions {
    double ThresholdPercent = 100;
    bool InvariantsBeforeAveraging = false;
    MeshCutTensorOptions Tensor;
    std::vector<int64_t> Regions; //!< Empty means one region; otherwise one ID per original element.
    Standard_EXPORT bool IsValid(size_t theElementCount) const;
};
//! Display recovery only; never modify the supplied physical field.
Standard_EXPORT Handle(RWCae::MeshNVecProperty) RecoverCutField(const Handle(RWCae::MeshData)& theMesh,
    const Handle(RWCae::MeshNVecProperty)& theField, const MeshCutRecoveryOptions& theOptions);
}
#endif
