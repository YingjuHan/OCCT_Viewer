#ifndef SIMMESH_HEADERFILE
#define SIMMESH_HEADERFILE

#include <Standard_Handle.hxx>
#include <Standard_TypeDef.hxx>

class SimMesh_Model;
class Poly_Triangulation;
class TopoDS_Shape;

//! The set of static methods for creation of simplification model from a shape and
//! creation of shape or triangulation from a model.
class SimMesh {
public:
    //! Creates a mesh model from the given shape.
    Standard_EXPORT static Handle(SimMesh_Model) ModelFromShape(const TopoDS_Shape& theShape);

    //! Builds a new shape from the given mesh model.
    //! Group IDs are used to separate triangles by faces and mesh edges by topology edges,
    //! thus restoring the structure of the original shape.
    Standard_EXPORT static TopoDS_Shape ShapeFromModel(const Handle(SimMesh_Model)& theModel);

    //! Extracts triangulation from the given mesh model.
    Standard_EXPORT static Handle(Poly_Triangulation) ExtractTriangulation(const Handle(SimMesh_Model)& theModel);
};

#endif // SIMMESH_HEADERFILE