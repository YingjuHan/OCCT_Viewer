#ifndef INTERACTIVESHAPEPRSMODE_H
#define INTERACTIVESHAPEPRSMODE_H

namespace ViewShape
{

    //! Presentation modes supported by ViewShape::InteractiveShape object.
    enum class InteractiveShapePrsMode
    {
        Wireframe,            //!< hollow wireframe geometry (face edges)
        Shaded,               //!< shaded interior with smooth normals
        ShadedFaceBoundaries, //!< shaded interior with smooth normals and face edges
        ShadedFlat,           //!< shaded interior with flat normals
        ShadedTessellation,   //!< shaded interior with triangulation edges
        HiddenLine,           //!< hidden line removal (HLR)
    };

    constexpr int InteractiveShapePrsMode_LOWER = 0;
    constexpr int InteractiveShapePrsMode_UPPER = (int)InteractiveShapePrsMode::HiddenLine;

} // namespace ViewShape
#endif // INTERACTIVESHAPEPRSMODE_H
