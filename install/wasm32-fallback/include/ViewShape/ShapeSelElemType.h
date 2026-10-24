#ifndef SHAPESELELEMSTYPE_H
#define SHAPESELELEMSTYPE_H

#include <ViewShape/InteractiveShapeSelMode.h>

#include <TopAbs.hxx>

namespace ViewShape
{

    //! Supported shape type of elements to be detected/selected.
    enum class ShapeSelElemType : int
    {
        Vertex = 0,
        Edge,
        Wire,
        Face,
        Shell,
        Solid,
        CompSolid,
        Group
    };
    enum
    {
        ShapeSelElemType_NB = (int)ShapeSelElemType::Group + 1
    };

    //! Return TopAbs shape type from selection subshape type.
    inline TopAbs_ShapeEnum ShapeSelElemTypeToTopAbs(const ShapeSelElemType theType)
    {
        switch (theType)
        {
        case ShapeSelElemType::Vertex:
            return TopAbs_VERTEX;
        case ShapeSelElemType::Edge:
            return TopAbs_EDGE;
        case ShapeSelElemType::Wire:
            return TopAbs_WIRE;
        case ShapeSelElemType::Face:
            return TopAbs_FACE;
        case ShapeSelElemType::Shell:
            return TopAbs_SHELL;
        case ShapeSelElemType::Solid:
            return TopAbs_SOLID;
        case ShapeSelElemType::CompSolid:
            return TopAbs_COMPSOLID;
        case ShapeSelElemType::Group:
            return TopAbs_COMPOUND;
        }
        return TopAbs_SHAPE;
    }

    //! Return selection subshape type from TopAbs shape type.
    inline ShapeSelElemType ShapeSelElemTypeFromTopAbs(const TopAbs_ShapeEnum theType)
    {
        switch (theType)
        {
        case TopAbs_VERTEX:
            return ShapeSelElemType::Vertex;
        case TopAbs_EDGE:
            return ShapeSelElemType::Edge;
        case TopAbs_WIRE:
            return ShapeSelElemType::Wire;
        case TopAbs_FACE:
            return ShapeSelElemType::Face;
        case TopAbs_SHELL:
            return ShapeSelElemType::Shell;
        case TopAbs_SOLID:
            return ShapeSelElemType::Solid;
        case TopAbs_COMPSOLID:
            return ShapeSelElemType::CompSolid;
        case TopAbs_COMPOUND:
            return ShapeSelElemType::Group;
        default:
            return ShapeSelElemType::Solid;
        }
    }

    //! Return string identifier of selection shape type.
    inline const char *ShapeSelElemTypeToString(const ShapeSelElemType theType)
    {
        switch (theType)
        {
        case ShapeSelElemType::Vertex:
            return "VERTEX";
        case ShapeSelElemType::Edge:
            return "EDGE";
        case ShapeSelElemType::Wire:
            return "WIRE";
        case ShapeSelElemType::Face:
            return "FACE";
        case ShapeSelElemType::Shell:
            return "SHELL";
        case ShapeSelElemType::Solid:
            return "SOLID";
        case ShapeSelElemType::CompSolid:
            return "COMPSOLID";
        case ShapeSelElemType::Group:
            return "GROUP";
        }
        return "";
    }

    //! Convert selection mode into shape element type.
    inline bool ShapeElemTypeFromSelMode(ShapeSelElemType &theElemType, int theSelMode)
    {
        switch (theSelMode)
        {
        case (int)InteractiveShapeSelMode::Group:
            theElemType = ShapeSelElemType::Group;
            return true;
        case (int)InteractiveShapeSelMode::CompSolid:
            theElemType = ShapeSelElemType::CompSolid;
            return true;
        case (int)InteractiveShapeSelMode::Solid:
            theElemType = ShapeSelElemType::Solid;
            return true;
        case (int)InteractiveShapeSelMode::Shell:
            theElemType = ShapeSelElemType::Shell;
            return true;
        case (int)InteractiveShapeSelMode::Face:
            theElemType = ShapeSelElemType::Face;
            return true;
        case (int)InteractiveShapeSelMode::Wire:
            theElemType = ShapeSelElemType::Wire;
            return true;
        case (int)InteractiveShapeSelMode::Edge:
            theElemType = ShapeSelElemType::Edge;
            return true;
        case (int)InteractiveShapeSelMode::Vertex:
            theElemType = ShapeSelElemType::Vertex;
            return true;
        default:
            return false;
        }
    }

    //! Convert shape element type into selection mode.
    inline InteractiveShapeSelMode ShapeElemTypeToSelMode(ShapeSelElemType theElemType)
    {
        switch (theElemType)
        {
        case ShapeSelElemType::Group:
            return InteractiveShapeSelMode::Group;
        case ShapeSelElemType::CompSolid:
            return InteractiveShapeSelMode::CompSolid;
        case ShapeSelElemType::Solid:
            return InteractiveShapeSelMode::Solid;
        case ShapeSelElemType::Shell:
            return InteractiveShapeSelMode::Shell;
        case ShapeSelElemType::Face:
            return InteractiveShapeSelMode::Face;
        case ShapeSelElemType::Wire:
            return InteractiveShapeSelMode::Wire;
        case ShapeSelElemType::Edge:
            return InteractiveShapeSelMode::Edge;
        case ShapeSelElemType::Vertex:
            return InteractiveShapeSelMode::Vertex;
        }
        return InteractiveShapeSelMode::Global;
    }

} // namespace
#endif // SHAPESELELEMSTYPE_H
