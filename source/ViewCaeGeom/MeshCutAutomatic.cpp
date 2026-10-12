#include <ViewCaeGeom/MeshCutAutomatic.h>
#include <cmath>
#include <stdexcept>
#include <limits>

namespace ViewCaeGeom {
MeshCutAutomaticResult MeshCutAutomatic::Add(MeshCutViewManager& manager, const std::string& id,
    const MeshCutAutomaticMapping& mapping)
{
    const auto field = mapping.Field;
    if (field.IsNull() || !field->IsInitialized() || !field->HasScalarComponents()
        || mapping.Component < 0 || mapping.Component >= field->NbRawComponents()
        || !std::isfinite(mapping.Threshold) || mapping.Visibility > 7 || (mapping.Sign != 1 && mapping.Sign != -1)
        || (mapping.Kind != MeshCutAutomaticKind::CrackDistance && mapping.Kind != MeshCutAutomaticKind::OptimizationDensity)) {
        throw std::invalid_argument("Automatic cut requires an explicit finite scalar mapping/sign/threshold");
    }
    if (mapping.Kind == MeshCutAutomaticKind::OptimizationDensity && (!std::isfinite(mapping.Minimum)
        || !std::isfinite(mapping.Maximum) || mapping.Minimum >= mapping.Maximum
        || mapping.Threshold < mapping.Minimum || mapping.Threshold > mapping.Maximum)) {
        throw std::invalid_argument("Density threshold requires an explicit valid source range");
    }
    MeshCutAutomaticResult result; result.SourceMinimum = std::numeric_limits<double>::infinity(); result.SourceMaximum = -result.SourceMinimum;
    const auto values = field->GetRawComponent(mapping.Component);
    for (int n = 0; n < values->NbValues(); ++n) {
        const double value = values->Value(RWCae::MeshPropertyEntityInfo(n));
        if (!std::isfinite(value) || (mapping.Kind == MeshCutAutomaticKind::OptimizationDensity
            && (value < mapping.Minimum || value > mapping.Maximum))) { throw std::invalid_argument("Automatic source has missing/nonfinite/out-of-range values"); }
        result.SourceMinimum = std::min(result.SourceMinimum,value); result.SourceMaximum = std::max(result.SourceMaximum,value);
    }
    if (!std::isfinite(result.SourceMinimum)) { throw std::invalid_argument("Automatic source is empty"); }
    const bool positiveKept = mapping.Kind == MeshCutAutomaticKind::OptimizationDensity || mapping.Sign == 1;
    if (mapping.Threshold < result.SourceMinimum) { result.State = positiveKept ? MeshCutAutomaticState::Full : MeshCutAutomaticState::Empty; }
    else if (mapping.Threshold > result.SourceMaximum) { result.State = positiveKept ? MeshCutAutomaticState::Empty : MeshCutAutomaticState::Full; }
    unsigned visibility = mapping.Visibility;
    if (mapping.Kind == MeshCutAutomaticKind::CrackDistance && mapping.Sign == -1) { visibility = (visibility&2)|((visibility&1)<<2)|((visibility&4)>>2); }
    if (!manager.AddIsosurface(id,field,mapping.Component,mapping.Threshold)) { throw std::invalid_argument("Automatic cut source tuple/topology mapping or ID is invalid"); }
    if (!manager.SetVisibility(id,visibility)) { manager.Remove(id); throw std::invalid_argument("Automatic visibility mapping failed"); }
    return result;
}
}
