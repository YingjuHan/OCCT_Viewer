#include <ViewCaeGeom/MeshCutRecovery.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>

namespace ViewCaeGeom {
bool MeshCutRecoveryOptions::IsValid(size_t count) const
{
    return std::isfinite(ThresholdPercent) && ThresholdPercent >= 0 && ThresholdPercent <= 100
        && (Regions.empty() || Regions.size() == count);
}

Handle(RWCae::MeshNVecProperty) RecoverCutField(const Handle(RWCae::MeshData)& mesh,
    const Handle(RWCae::MeshNVecProperty)& field, const MeshCutRecoveryOptions& options)
{
    if (mesh.IsNull() || field.IsNull() || !field->IsInitialized() || !field->IsElemental()
        || (!field->HasScalarComponents() && !field->HasNodeVecComponents())
        || !options.IsValid(mesh->NbIndexedElems()) || field->NbValues() != (int)mesh->NbIndexedElems()) {
        throw std::invalid_argument("Recovery requires matching original elemental components and region IDs");
    }
    const bool before = options.InvariantsBeforeAveraging;
    const int count = before ? field->NbRawComponents() : field->NbComponents();
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty(field->Name(),count,
        RWCae::MeshPropertyBase::MappingType::Elemental,before ? RWCae::MeshNVecPropertyType::ScalarSet : field->Type());
    result->Init<RWCae::MeshNodeVecProperty>();
    std::vector<std::vector<RWCae::MeshEntityId>> incident(mesh->NbNodes()), nodes(mesh->NbIndexedElems());
    for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
        mesh->ElementNodes(e,nodes[e]);
        auto unique = nodes[e]; std::sort(unique.begin(),unique.end()); unique.erase(std::unique(unique.begin(),unique.end()),unique.end());
        for (auto n : unique) { incident.at(n).push_back(e); }
    }
    for (int c = 0; c < count; ++c) {
        const auto source = before ? field->GetRawComponent(c) : field->GetComponent(c);
        if (source->NbValues() != (int)mesh->NbIndexedElems()) {
            throw std::invalid_argument("Recovery component tuple count does not match the source mesh");
        }
        auto valueAt = [&](RWCae::MeshEntityId e, RWCae::MeshEntityId n) {
            return source->Value(RWCae::MeshPropertyEntityInfo(e,field->HasNodeVecComponents() ? (int)n : -1));
        };
        const auto target = Handle(RWCae::MeshNodeVecProperty)::DownCast(before ? result->GetRawComponent(c) : result->GetComponent(c));
        target->Init(mesh->NbIndexedElems(),0,false,false);
        target->SetName(source->Name()); target->SetDescription(source->Description());
        target->SetDisplayUnits(source->DisplayUnits()); target->SetEnabled(source->IsEnabled());
        double minimum = INFINITY, maximum = -INFINITY;
        for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
            for (auto n : nodes[e]) {
                const double value = valueAt(e,n);
                if (std::isinf(value)) { throw std::invalid_argument("Recovery field contains infinity"); }
                if (std::isfinite(value)) { minimum = std::min(minimum,value); maximum = std::max(maximum,value); }
            }
            target->ReserveValueMap(e,nodes[e].size());
        }
        const double threshold = minimum <= maximum ? (maximum-minimum)*(options.ThresholdPercent/100) : 0;
        for (RWCae::MeshEntityId n = 0; n < incident.size(); ++n) {
            std::map<int64_t,std::vector<RWCae::MeshEntityId>> regions;
            for (auto e : incident[n]) { regions[options.Regions.empty() ? 0 : options.Regions[e]].push_back(e); }
            for (const auto& region : regions) {
                double low = INFINITY, high = -INFINITY, sum = 0; size_t finite = 0;
                for (auto e : region.second) {
                    const double value = valueAt(e,n);
                    if (std::isfinite(value)) { low = std::min(low,value); high = std::max(high,value); sum += value; ++finite; }
                }
                const bool average = finite && high-low <= threshold;
                for (auto e : region.second) {
                    const double original = valueAt(e,n);
                    const double value = average && std::isfinite(original) ? sum/finite : original;
                    target->SetValue(RWCae::MeshPropertyEntityInfo(e,n),value);
                }
            }
        }
    }
    result->SetDescription(field->Description()); result->SetTime(field->Time(),field->TimeStep());
    result->SetTimeStepDescription(field->TimeStepDescription()); result->SetTimeDisplayUnits(field->TimeDisplayUnits());
    if (!before && result->IsVector()) { result->GetRawComponent(0)->SetDisplayUnits(field->GetRawComponent(0)->DisplayUnits()); }
    result->UpdateComponents(); return result;
}
}
