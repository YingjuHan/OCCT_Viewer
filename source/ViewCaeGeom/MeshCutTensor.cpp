#include <ViewCaeGeom/MeshCutTensor.h>
#include "MeshCutFieldTools.h"
#include <RWCae/MeshNodeVecProperty.h>
#include <RWCae/MeshScalarProperty.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ViewCaeGeom {
bool MeshCutTensorOptions::IsValid() const
{
    return Measure >= MeshCutTensorMeasure::Components && Measure <= MeshCutTensorMeasure::Tresca
        && Layout >= MeshCutTensorLayout::XXYYZZXYXZYZ && Layout <= MeshCutTensorLayout::ColumnMajor;
}

bool MeshCutTensorOptions::Matches(int count) const
{
    return IsValid() && count == (Layout == MeshCutTensorLayout::RowMajor || Layout == MeshCutTensorLayout::ColumnMajor ? 9 : 6);
}

double CutTensorValue(const std::vector<double>& values, const MeshCutTensorOptions& options)
{
    if (!options.Matches((int)values.size()) || options.Measure == MeshCutTensorMeasure::Components) {
        throw std::invalid_argument("Tensor measure requires an explicit matching six/nine component layout");
    }
    double scale = 0; bool missing = false;
    for (auto value : values) {
        if (std::isinf(value)) { throw std::invalid_argument("Tensor input contains infinity"); }
        missing = missing || std::isnan(value);
        if (std::isfinite(value)) { scale = std::max(scale,std::abs(value)); }
    }
    if (missing) { return std::numeric_limits<double>::quiet_NaN(); }
    if (scale == 0) { return 0; }
    double a[3][3] = {};
    if (values.size() == 6) {
        for (int i = 0; i < 3; ++i) { a[i][i] = values[i]/scale; }
        a[0][1] = a[1][0] = values[3]/scale;
        const bool yzFirst = options.Layout == MeshCutTensorLayout::XXYYZZXYYZXZ;
        a[0][2] = a[2][0] = values[yzFirst ? 5 : 4]/scale;
        a[1][2] = a[2][1] = values[yzFirst ? 4 : 5]/scale;
    } else {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                a[i][j] = values[options.Layout == MeshCutTensorLayout::RowMajor ? 3*i+j : 3*j+i]/scale;
            }
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = i+1; j < 3; ++j) {
            if (std::abs(a[i][j]-a[j][i]) > 64*std::numeric_limits<double>::epsilon()) {
                throw std::invalid_argument("Tensor principal/invariant measures require a symmetric matrix");
            }
            a[i][j] = a[j][i] = (a[i][j]/2+a[j][i]/2)*(options.EngineeringShear ? 0.5 : 1);
        }
    }
    double result = 0;
    if (options.Measure == MeshCutTensorMeasure::Pressure) {
        result = -(a[0][0]/3+a[1][1]/3+a[2][2]/3);
    } else if (options.Measure == MeshCutTensorMeasure::Mises) {
        result = std::hypot(std::hypot(a[0][0]-a[1][1],a[1][1]-a[2][2]),a[2][2]-a[0][0])/std::sqrt(2.0);
        for (int i = 0; i < 3; ++i) {
            for (int j = i+1; j < 3; ++j) { result = std::hypot(result,std::sqrt(3.0)*a[i][j]); }
        }
    } else {
        // Jacobi rotations on the normalized symmetric matrix; repeated roots
        // require no determinant subtraction or cubic discriminant.
        for (int iteration = 0; iteration < 32; ++iteration) {
            int p = 0, q = 1;
            for (int i = 0; i < 3; ++i) {
                for (int j = i+1; j < 3; ++j) { if (std::abs(a[i][j]) > std::abs(a[p][q])) { p = i; q = j; } }
            }
            if (std::abs(a[p][q]) <= 8*std::numeric_limits<double>::epsilon()) { break; }
            const double angle = 0.5*std::atan2(2*a[p][q],a[q][q]-a[p][p]);
            const double c = std::cos(angle), s = std::sin(angle);
            const double pp = a[p][p], qq = a[q][q], pq = a[p][q];
            a[p][p] = c*c*pp-2*c*s*pq+s*s*qq;
            a[q][q] = s*s*pp+2*c*s*pq+c*c*qq; a[p][q] = a[q][p] = 0;
            for (int k = 0; k < 3; ++k) {
                if (k == p || k == q) { continue; }
                const double kp = a[k][p], kq = a[k][q];
                a[k][p] = a[p][k] = c*kp-s*kq; a[k][q] = a[q][k] = s*kp+c*kq;
            }
        }
        std::array<double,3> principal = {{a[0][0],a[1][1],a[2][2]}};
        std::sort(principal.begin(),principal.end(),std::greater<double>());
        result = options.Measure == MeshCutTensorMeasure::PrincipalMax ? principal[0]
            : options.Measure == MeshCutTensorMeasure::PrincipalMid ? principal[1]
            : options.Measure == MeshCutTensorMeasure::PrincipalMin ? principal[2] : principal[0]-principal[2];
    }
    result *= scale;
    if (!std::isfinite(result)) { throw std::overflow_error("Tensor measure exceeds the finite double range"); }
    return result;
}

Handle(RWCae::MeshNVecProperty) DeriveCutTensorField(const Handle(RWCae::MeshNVecProperty)& field,
    const MeshCutTensorOptions& options)
{
    if (field.IsNull() || !field->IsInitialized() || field->ChangeComponents().Size() != field->NbRawComponents()
        || field->GetComponent(0).IsNull() || !options.Matches(field->NbComponents())
        || field->IsVector() || (!field->HasScalarComponents() && !field->HasNodeVecComponents())
        || options.Measure == MeshCutTensorMeasure::Components) {
        throw std::invalid_argument("Tensor display requires matching scalar or element-node tensor components");
    }
    const int count = field->NbValues();
    const auto source = field->GetComponent(0);
    for (int c = 0; c < field->NbComponents(); ++c) {
        const auto component = field->GetComponent(c);
        if (component.IsNull() || component->NbValues() != count || component->MapType() != field->MapType()
            || component->BaseType() != source->BaseType() || (count && !component->IsLoaded())
            || component->DisplayUnits() != source->DisplayUnits()) {
            throw std::invalid_argument("Tensor components require matching storage, tuples, mapping and units");
        }
    }
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty(field->Name(),1,field->MapType());
    if (field->HasNodeVecComponents()) { result->Init<RWCae::MeshNodeVecProperty>(); }
    else { result->Init<RWCae::MeshScalarProperty>(); }
    const auto target = result->GetComponent(0);
    if (field->HasNodeVecComponents()) { Handle(RWCae::MeshNodeVecProperty)::DownCast(target)->Init(count,0,false,false); }
    else { Handle(RWCae::MeshScalarProperty)::DownCast(target)->Init(count); }
    std::vector<double> values(field->NbComponents());
    for (int e = 0; e < count; ++e) {
        std::vector<int> indices;
        if (field->HasNodeVecComponents()) {
            Handle(RWCae::MeshNodeVecProperty)::DownCast(source)->SubIndices(e,indices);
            // Reject different trace keys instead of silently dropping extra data.
            std::sort(indices.begin(),indices.end());
            for (int c = 1; c < field->NbComponents(); ++c) {
                std::vector<int> keys; Handle(RWCae::MeshNodeVecProperty)::DownCast(field->GetComponent(c))->SubIndices(e,keys);
                std::sort(keys.begin(),keys.end());
                if (keys != indices) { throw std::invalid_argument("Tensor component trace keys do not match"); }
            }
        } else { indices.push_back(-1); }
        for (auto n : indices) {
            const RWCae::MeshPropertyEntityInfo info(e,n);
            for (int c = 0; c < field->NbComponents(); ++c) { values[c] = field->GetComponent(c)->Value(info); }
            target->SetValue(info,CutTensorValue(values,options));
        }
    }
    CutFieldTools::CopyMetadata(source,target);
    static const char* names[] = {"Components","Mises","Pressure","Maximum principal","Middle principal","Minimum principal","Tresca"};
    target->SetName(names[(int)options.Measure]); target->SetDescription(names[(int)options.Measure]); target->SetEnabled(true);
    result->SetDescription(field->Description()); result->SetTime(field->Time(),field->TimeStep());
    result->SetTimeStepDescription(field->TimeStepDescription()); result->SetTimeDisplayUnits(field->TimeDisplayUnits());
    result->UpdateComponents(); return result;
}
}
