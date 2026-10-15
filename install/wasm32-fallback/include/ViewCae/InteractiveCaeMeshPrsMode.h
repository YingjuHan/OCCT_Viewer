#ifndef INTERACTIVECAEMESHPRSMODE_HEADERFILE
#define INTERACTIVECAEMESHPRSMODE_HEADERFILE

namespace ViewCae {
//! Presentation modes supported by ViewCae::InteractiveCaeMesh interactive object.
enum class InteractiveCaeMeshPrsMode : int {
    MeshEdges,          //!< mesh edges with no shading
    FlatShaded,         //!< shaded with no edges
    ShadedMeshEdges,    //!< shaded + mesh edges
    ShadedMeshShrinked, //!< shaded with shrunk mesh elements
    ShadedFreeEdges,    //!< shaded + free mesh edges
    ShadedSharpEdges,   //!< shaded + sharp mesh edges
};

constexpr int InteractiveCaeMeshNbPrsModes = (int)InteractiveCaeMeshPrsMode::ShadedSharpEdges + 1;

//! Types of displayed CAE elements.
enum class InteractiveCaeMeshElementType : int {
    Element0d = 0x1,
    Element1d = 0x2,
    Face      = 0x4,
    Volume    = 0x8,
    //
    Node     = 0x10,
    FreeNode = 0x20,
    //
    Centroid   = 0x40,
    FreeEdges  = 0x80,
    SharpEdges = 0x100,
    //
    AllElemental = Element0d | Element1d | Face | Volume
};

}

#endif // INTERACTIVECAEMESHPRSMODE_HEADERFILE
