#include <ViewCaeGeom/MeshCutSlicing.h>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace {
using namespace ViewCaeGeom;
void finite(const Graphic3d_Vec3d& v)
{
    for (int i = 0; i < 3; ++i) { if (!std::isfinite(v[i])) { throw std::invalid_argument("Slice coordinates must be finite"); } }
}
Graphic3d_Vec3d normalized(const Graphic3d_Vec3d& v)
{
    finite(v); const double norm = std::hypot(v.x(),v.y(),v.z());
    if (!std::isfinite(norm) || norm <= 1e-12) { throw std::invalid_argument("Degenerate slicing direction"); }
    return v/norm;
}
void validate(const MeshCutDefinition& plane, double start, double end, size_t count)
{
    if (plane.Shape != MeshCutShape::Plane || plane.Id.empty() || !std::isfinite(plane.Plane.w())
        || !std::isfinite(start) || !std::isfinite(end) || count == 0 || count > 10000) {
        throw std::invalid_argument("Slicing requires a plane, finite range and 1..10000 samples");
    }
    normalized(Graphic3d_Vec3d(plane.Plane.x(),plane.Plane.y(),plane.Plane.z()));
    if (count == 1 && start != end) { throw std::invalid_argument("A nonzero slicing range requires at least two samples"); }
}
void intervals(std::vector<MeshCutSliceSample>& samples)
{
    for (size_t i = 0; i < samples.size(); ++i) {
        samples[i].IntervalBegin = i == 0 ? samples[i].Coordinate : samples[i-1].Coordinate/2+samples[i].Coordinate/2;
        samples[i].IntervalEnd = i+1 == samples.size() ? samples[i].Coordinate : samples[i].Coordinate/2+samples[i+1].Coordinate/2;
    }
}
Graphic3d_Vec3d rotate(const Graphic3d_Vec3d& v, const Graphic3d_Vec3d& k, double angle)
{
    return v*std::cos(angle)+Graphic3d_Vec3d::Cross(k,v)*std::sin(angle)+k*k.Dot(v)*(1-std::cos(angle));
}
MeshCutFrame frame(const Graphic3d_Vec3d& position, const Graphic3d_Vec3d& normal)
{
    MeshCutFrame result; result.Origin = position; result.Z = normalized(normal);
    const auto seed = std::abs(result.Z.z()) < 0.9 ? Graphic3d_Vec3d(0,0,1) : Graphic3d_Vec3d(0,1,0);
    result.X = normalized(Graphic3d_Vec3d::Cross(seed,result.Z)); result.Y = Graphic3d_Vec3d::Cross(result.Z,result.X);
    return result;
}
}
namespace ViewCaeGeom {
std::vector<MeshCutSliceSample> MeshCutSlicing::Regular(const MeshCutDefinition& plane, double start, double spacing, size_t count)
{
    if (!std::isfinite(spacing) || spacing <= 0 || count == 0 || count > 10000) { throw std::invalid_argument("Regular spacing/count is invalid"); }
    return Range(plane,start,start+spacing*(count-1),count);
}
std::vector<MeshCutSliceSample> MeshCutSlicing::Range(const MeshCutDefinition& plane, double start, double end, size_t count)
{
    validate(plane,start,end,count);
    if (end < start) { throw std::invalid_argument("Slice range must be increasing"); }
    const Graphic3d_Vec3d source(plane.Plane.x(),plane.Plane.y(),plane.Plane.z());
    const auto normal = normalized(source); const double norm = std::hypot(source.x(),source.y(),source.z());
    std::vector<MeshCutSliceSample> samples(count);
    for (size_t i = 0; i < count; ++i) {
        auto& sample = samples[i]; const double t = count == 1 ? 0 : (double)i/(count-1);
        sample.Coordinate = start*(1-t)+end*t; sample.Definition = plane;
        sample.Definition.Id = plane.Id+"_"+std::to_string(i); sample.Definition.Name = sample.Definition.Id;
        const double distance = -plane.Plane.w()/norm+sample.Coordinate;
        if (!std::isfinite(distance)) { throw std::invalid_argument("Slice offset overflows model units"); }
        sample.Definition.Plane = Graphic3d_Vec4d(normal.x(),normal.y(),normal.z(),-distance);
        sample.Frame = frame(normal*distance,normal);
    }
    intervals(samples); return samples;
}
std::vector<MeshCutSliceSample> MeshCutSlicing::Rotational(const MeshCutDefinition& plane,
    const Graphic3d_Vec3d& axis, const Graphic3d_Vec3d& origin, double start, double end, size_t count)
{
    validate(plane,start,end,count); finite(origin); if (end < start) { throw std::invalid_argument("Rotation range must be increasing"); }
    const auto direction = normalized(axis), normal = normalized(Graphic3d_Vec3d(plane.Plane.x(),plane.Plane.y(),plane.Plane.z()));
    const double norm = std::hypot(plane.Plane.x(),plane.Plane.y(),plane.Plane.z());
    const auto point = normal*(-plane.Plane.w()/norm); std::vector<MeshCutSliceSample> samples(count);
    for (size_t i = 0; i < count; ++i) {
        auto& sample = samples[i]; const double t = count == 1 ? 0 : (double)i/(count-1);
        sample.Coordinate = start*(1-t)+end*t;
        const auto n = rotate(normal,direction,sample.Coordinate), p = origin+rotate(point-origin,direction,sample.Coordinate);
        sample.Definition = plane; sample.Definition.Id = plane.Id+"_"+std::to_string(i); sample.Definition.Name = sample.Definition.Id;
        sample.Definition.Plane = Graphic3d_Vec4d(n.x(),n.y(),n.z(),-n.Dot(p)); sample.Frame = frame(p,n);
        if (!sample.Frame.IsValid() || !std::isfinite(sample.Definition.Plane.w())) { throw std::invalid_argument("Rotation frame overflow"); }
    }
    intervals(samples); return samples;
}
std::vector<MeshCutSliceSample> MeshCutSlicing::Path(const std::string& prefix,
    const std::vector<Graphic3d_Vec3d>& points, const Graphic3d_Vec3d& up)
{
    if (prefix.empty() || points.size() < 2 || points.size() > 10000) { throw std::invalid_argument("Path requires an ID and 2..10000 points"); }
    const auto guide = normalized(up); std::vector<MeshCutSliceSample> samples(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        finite(points[i]);
        const auto normal = normalized(i+1 < points.size() ? points[i+1]-points[i] : points[i]-points[i-1]);
        auto& sample = samples[i]; sample.Frame.Origin = points[i]; sample.Frame.Z = normal;
        if (i == 0) { sample.Frame.X = normalized(Graphic3d_Vec3d::Cross(guide,normal)); }
        else {
            const auto& previous = samples[i-1].Frame;
            const auto cross = Graphic3d_Vec3d::Cross(previous.Z,normal); const double sine = cross.Modulus();
            const double cosine = std::clamp(previous.Z.Dot(normal),-1.0,1.0);
            sample.Frame.X = sine > 1e-12 ? rotate(previous.X,cross/sine,std::atan2(sine,cosine)) : previous.X;
            sample.Frame.X = normalized(sample.Frame.X-normal*normal.Dot(sample.Frame.X));
            sample.Coordinate = samples[i-1].Coordinate+(points[i]-points[i-1]).Modulus();
        }
        sample.Frame.Y = Graphic3d_Vec3d::Cross(normal,sample.Frame.X);
        if (!sample.Frame.IsValid() || !std::isfinite(sample.Coordinate)) { throw std::invalid_argument("Invalid path frame"); }
        sample.Definition.Id = prefix+"_"+std::to_string(i); sample.Definition.Name = sample.Definition.Id;
        sample.Definition.Plane = Graphic3d_Vec4d(normal.x(),normal.y(),normal.z(),-normal.Dot(points[i]));
        if (!std::isfinite(sample.Definition.Plane.w())) { throw std::invalid_argument("Path plane offset overflow"); }
    }
    intervals(samples); return samples;
}
}
