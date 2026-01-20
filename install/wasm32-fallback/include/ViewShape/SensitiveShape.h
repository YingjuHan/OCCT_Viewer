#ifndef SENSITIVESHAPE_H
#define SENSITIVESHAPE_H

#include <StdSelect_BRepSelectionTool.hxx>
#include <TopoDS_Shape.hxx>

namespace ViewShape
{

    //! Sensitive for picking TopoDS_Shape.
    class SensitiveShape
    {
    public:
        //! Sensitive picking priority, see also StdSelect_BRepSelectionTool::GetStandardPriority().
        enum class SensPriority
        {
            Global = 4, //!< global object
            Face = 5,   //!< face
            Edge = 7,   //!< edge
            Vertex = 8, //!< vertex
        };

    public:
        //! Return shape.
        virtual TopoDS_Shape GetShape() const = 0;

        //! Return shape index.
        virtual int GetShapeIndex() const = 0;

        //! Return priority.
        virtual int GetPriority() const
        {
            const TopoDS_Shape aShape = GetShape();
            return !aShape.IsNull() ? StdSelect_BRepSelectionTool::GetStandardPriority(aShape, TopAbs_SHAPE)
                                    : (int)SensPriority::Global;
        }
    };

} // namespace ViewShape
#endif // SENSITIVESHAPE_H
