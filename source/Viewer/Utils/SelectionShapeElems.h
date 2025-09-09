#ifndef _SELECTIONSHAPEELEMS_HEADERFILE
#define _SELECTIONSHAPEELEMS_HEADERFILE

#include <ViewShape/ShapeSelElemType.h>

#include <Standard_ProgramError.hxx>

#include <string>

namespace Viewer {

//! Structure to define detected shape element.
struct DetectedShapeElem {
    int entityId   = -1;
    int entityType = -1; // InteractiveShapeSelMode
};

//! Structure to define detected/selected shape elements.
struct SelectionShapeElems {
    std::string groups;
    std::string compSolids;
    std::string solids;
    std::string shells;
    std::string faces;
    std::string wires;
    std::string edges;
    std::string verts;

    //! Empty constructor.
    SelectionShapeElems() {}

    //! Clear lists.
    void Clear();

    //! Return elements.
    const std::string& GetElems(ViewShape::ShapeSelElemType theType) const
    {
        switch (theType) {
            case ViewShape::ShapeSelElemType::Vertex: return this->verts;
            case ViewShape::ShapeSelElemType::Edge: return this->edges;
            case ViewShape::ShapeSelElemType::Wire: return this->wires;
            case ViewShape::ShapeSelElemType::Face: return this->faces;
            case ViewShape::ShapeSelElemType::Shell: return this->shells;
            case ViewShape::ShapeSelElemType::Solid: return this->solids;
            case ViewShape::ShapeSelElemType::CompSolid: return this->compSolids;
            case ViewShape::ShapeSelElemType::Group: return this->groups;
            default: break;
        }
        throw Standard_ProgramError("Internal error");
    }

    //! Return elements.
    std::string& ChangeElems(ViewShape::ShapeSelElemType theType)
    {
        switch (theType) {
            case ViewShape::ShapeSelElemType::Group: return this->groups;
            case ViewShape::ShapeSelElemType::CompSolid: return this->compSolids;
            case ViewShape::ShapeSelElemType::Solid: return this->solids;
            case ViewShape::ShapeSelElemType::Shell: return this->shells;
            case ViewShape::ShapeSelElemType::Face: return this->faces;
            case ViewShape::ShapeSelElemType::Wire: return this->wires;
            case ViewShape::ShapeSelElemType::Edge: return this->edges;
            case ViewShape::ShapeSelElemType::Vertex: return this->verts;
            default: break;
        }
        throw Standard_ProgramError("Internal error");
    }

    //! Return TRUE if at list one list is not empty.
    bool IsValid() const
    {
        return !groups.empty() || !compSolids.empty() || !solids.empty() || !shells.empty() || !faces.empty()
            || !wires.empty() || !edges.empty() || !verts.empty();
    }
};

} // namespace

#endif // _SELECTIONSHAPEELEMS_HEADERFILE
