#ifndef _MESHCUTTENSOR_HEADERFILE
#define _MESHCUTTENSOR_HEADERFILE
#include <RWCae/MeshNVecProperty.h>
#include <vector>

namespace ViewCaeGeom {
enum class MeshCutTensorMeasure { Components, Mises, Pressure, PrincipalMax, PrincipalMid, PrincipalMin, Tresca };
enum class MeshCutTensorLayout { XXYYZZXYXZYZ, XXYYZZXYYZXZ, RowMajor, ColumnMajor };

//! Explicit display conventions; never reinterpret the original physical field.
struct MeshCutTensorOptions {
    MeshCutTensorMeasure Measure = MeshCutTensorMeasure::Components;
    MeshCutTensorLayout Layout = MeshCutTensorLayout::XXYYZZXYXZYZ;
    bool EngineeringShear = false; //!< Off-diagonal tensor entries are half the supplied engineering shear.
    Standard_EXPORT bool IsValid() const;
    Standard_EXPORT bool Matches(int theComponents) const;
};

//! Symmetric-tensor measure, evaluated with normalized intermediates.
//! Any NaN yields NaN; infinity, nonsymmetry and an unrepresentable result throw.
Standard_EXPORT double CutTensorValue(const std::vector<double>& theComponents, const MeshCutTensorOptions& theOptions);
//! Derive one scalar per original tuple (including separate element-node traces).
Standard_EXPORT Handle(RWCae::MeshNVecProperty) DeriveCutTensorField(
    const Handle(RWCae::MeshNVecProperty)& theField, const MeshCutTensorOptions& theOptions);
}
#endif
