
#ifndef SIMMESH_VERTEX_HEADERFILE
#define SIMMESH_VERTEX_HEADERFILE

#include <SimMesh_Types.h>

//! Definition of a mesh vertex.
class SimMesh_Vertex : public SimMesh_Vec3f {
public:
    using SimMesh_Vec3f::NCollection_Vec3;

    explicit SimMesh_Vertex(const SimMesh_Vec3f& theOther) : SimMesh_Vec3f(theOther) {}

    // Copy coords from another object
    void SetCoords(const SimMesh_Vec3f& theOther) { SimMesh_Vec3f::operator=(theOther); }

    //! Returns true if the vertex is valid
    bool IsValid() const { return myValid; }

    //! Sets validity
    void SetValid(bool theVal) { myValid = theVal; }

    //! Returns true if the vertex is marked
    bool IsMarked() const { return myMarked; }

    //! Sets whether the vertex is marked or not
    void SetMarked(bool theVal) { myMarked = theVal; }

private:
    bool myValid  = true;  //!< validity flag
    bool myMarked = false; //!< marked flag
};

#endif // SIMMESH_VERTEX_HEADERFILE



