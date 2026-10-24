#ifndef _MESHCUTFIELDTOOLS_HEADERFILE
#define _MESHCUTFIELDTOOLS_HEADERFILE

#include <RWCae/MeshData.h>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ViewCaeGeom { namespace CutFieldTools {
//! Validate every component, including cached vector magnitudes, before mapping.
inline void Validate(const Handle(RWCae::MeshData)& mesh, const Handle(RWCae::MeshNVecProperty)& field)
{
    if (mesh.IsNull() || field.IsNull() || !field->IsInitialized() || field->NbComponents() <= 0
        || field->ChangeComponents().Size() != field->NbRawComponents()) {
        throw std::invalid_argument("Cut field requires initialized components");
    }
    const size_t count = field->IsNodal() ? mesh->NbNodes() : mesh->NbIndexedElems();
    if (count > (size_t)std::numeric_limits<int>::max() || field->NbValues() != (int)count) {
        throw std::invalid_argument("Cut field tuple count does not match the source mesh");
    }
    for (int c = 0; c < field->NbRawComponents(); ++c) {
        const auto component = field->GetRawComponent(c);
        if (component.IsNull() || component->MapType() != field->MapType()
            || component->NbValues() != (int)count || (count && !component->IsLoaded())
            || (component->BaseType() != RWCae::MeshScalarPropertyBaseType::Scalar
                && component->BaseType() != RWCae::MeshScalarPropertyBaseType::NodeVec)
            || component->BaseType() != field->GetRawComponent(0)->BaseType()
            || (field->IsNodal() && component->BaseType() != RWCae::MeshScalarPropertyBaseType::Scalar)) {
            throw std::invalid_argument("Cut field components require matching storage, mapping and tuple counts");
        }
    }
}

//! Increment a finite mean without overflowing the sum or opposite-sign difference.
inline double Mean(double mean, double value, size_t count)
{
    if (count == 1) { return value; }
    const double fraction = 1.0/count;
    return std::signbit(mean) == std::signbit(value)
        ? mean+(value-mean)*fraction : mean*(1-fraction)+value*fraction;
}

//! Compare spreads as a percentage without overflowing a finite source range.
inline bool AcceptSpread(double low, double high, double minimum, double maximum, double percent)
{
    if (low == high || percent == 100) { return true; }
    if (percent == 0) { return false; }
    double range = maximum-minimum, spread = high-low, factor = 100;
    if (!std::isfinite(range)) {
        range = maximum/2-minimum/2;
        if (!std::isfinite(spread)) { spread = high/2-low/2; }
        else { factor = 50; }
    }
    const double scaled = spread*factor;
    const double relative = std::isfinite(scaled) ? scaled/range : (spread/range)*factor;
    return relative <= percent;
}

inline void CopyMetadata(const Handle(RWCae::MeshScalarPropertyBase)& source,
    const Handle(RWCae::MeshScalarPropertyBase)& target)
{
    target->SetName(source->Name()); target->SetDescription(source->Description());
    target->SetDisplayUnits(source->DisplayUnits()); target->SetEnabled(source->IsEnabled());
}
} }
#endif
