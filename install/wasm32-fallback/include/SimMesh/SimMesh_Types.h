#ifndef SIMMESH_TYPES_HEADERFILE
#define SIMMESH_TYPES_HEADERFILE

#include <NCollection_Mat3.hxx>
#include <NCollection_Mat4.hxx>
#include <NCollection_Vec3.hxx>
#include <NCollection_Vec4.hxx>

#include <cstddef>
#include <limits>

//! Integer type used as identifier of vertices and triangles.
using SimMesh_Id = size_t;

//! Value used as invalid identifier.
constexpr SimMesh_Id SimMesh_InvalidId = std::numeric_limits<SimMesh_Id>::max();

//! 3x3 doubles Matrix class.
using SimMesh_Mat3d = NCollection_Mat3<double>;

//! 4x4 doubles Matrix class.
using SimMesh_Mat4d = NCollection_Mat4<double>;

//! Vector of 2 Ids.
using SimMesh_Vec2i = NCollection_Vec2<SimMesh_Id>;

//! Vector of 3 Ids.
using SimMesh_Vec3i = NCollection_Vec3<SimMesh_Id>;

//! Vector of 2 floats.
using SimMesh_Vec2f = NCollection_Vec2<float>;

//! Vector of 3 floats.
using SimMesh_Vec3f = NCollection_Vec3<float>;

//! Vector of 3 doubles.
using SimMesh_Vec3d = NCollection_Vec3<double>;

//! Vector of 4 doubles.
using SimMesh_Vec4d = NCollection_Vec4<double>;

//! Vector of 4 unsigned chars.
using SimMesh_Vec4uc = NCollection_Vec4<unsigned char>;

#endif
