#ifndef INTERACTIVESHAPESELMODE_H
#define INTERACTIVESHAPESELMODE_H
namespace ViewShape
{
    //! Selection modes supported by ViewShape::InteractiveShape object.
    //! It is defined as a bitmask to support several active selection mode simultaneously in the viewer.
    enum class InteractiveShapeSelMode
    {
        Global = 0x0000,
        Vertex = 0x0001,
        Edge = 0x0002,
        Wire = 0x0004,
        Face = 0x0008,
        Shell = 0x0010,
        Solid = 0x0020,
        CompSolid = 0x0040,
        Group = 0x0080,       // compound
        NamedObject = 0x0100, // not supported yet
    };

    constexpr unsigned int InteractiveShapeSelModeMaskComposite = (unsigned int)InteractiveShapeSelMode::Wire | (unsigned int)InteractiveShapeSelMode::Shell | (unsigned int)InteractiveShapeSelMode::Solid | (unsigned int)InteractiveShapeSelMode::CompSolid | (unsigned int)InteractiveShapeSelMode::Group;

    constexpr unsigned int InteractiveShapeSelModeMask = (unsigned int)InteractiveShapeSelMode::Global | (unsigned int)InteractiveShapeSelMode::Vertex | (unsigned int)InteractiveShapeSelMode::Edge | (unsigned int)InteractiveShapeSelMode::Wire | (unsigned int)InteractiveShapeSelMode::Face | (unsigned int)InteractiveShapeSelMode::Shell | (unsigned int)InteractiveShapeSelMode::Solid | (unsigned int)InteractiveShapeSelMode::CompSolid | (unsigned int)InteractiveShapeSelMode::Group | (unsigned int)InteractiveShapeSelMode::NamedObject;

} // namespace ViewShape
#endif // INTERACTIVESHAPESELMODE_H