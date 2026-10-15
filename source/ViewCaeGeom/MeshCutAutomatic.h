#ifndef _MESHCUTAUTOMATIC_HEADERFILE
#define _MESHCUTAUTOMATIC_HEADERFILE
#include <ViewCaeGeom/MeshCutViewManager.h>

namespace ViewCaeGeom {
enum class MeshCutAutomaticKind { CrackDistance, OptimizationDensity };
enum class MeshCutAutomaticState { Empty, Full, Crossing };
struct MeshCutAutomaticMapping {
    MeshCutAutomaticKind Kind = MeshCutAutomaticKind::CrackDistance;
    Handle(RWCae::MeshNVecProperty) Field;
    int Component = 0;
    double Threshold = 0;
    int Sign = 1; //!< Crack: explicitly map which sign is the positive side.
    double Minimum = 0, Maximum = 1; //!< Density: explicit admissible source range.
    unsigned Visibility = 2;
};
struct MeshCutAutomaticResult {
    MeshCutAutomaticState State = MeshCutAutomaticState::Crossing;
    double SourceMinimum = 0, SourceMaximum = 0;
};
class MeshCutAutomatic {
public:
    //! Complete mapping validation precedes adding a cut. No name-based field inference.
    Standard_EXPORT static MeshCutAutomaticResult Add(MeshCutViewManager& manager,
        const std::string& id, const MeshCutAutomaticMapping& mapping);
};
}
#endif
