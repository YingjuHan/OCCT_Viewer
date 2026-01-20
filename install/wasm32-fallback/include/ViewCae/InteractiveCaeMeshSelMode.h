#ifndef INTERACTIVECAEMESHSELMODE_HEADERFILE
#define INTERACTIVECAEMESHSELMODE_HEADERFILE

namespace ViewCae {
//! Selection modes supported by ViewCae::InteractiveCaeMesh interactive object.
enum class InteractiveCaeMeshSelMode : int {
    None     = 0x0,
    Presentation = 0x1,
    Elements = 0x2,
    Nodes    = 0x4,
    Box      = 0x8 //!< Exclusive mode, disables other modes
};

constexpr int InteractiveCaeMeshSelModeMask = (int)InteractiveCaeMeshSelMode::Presentation
                                            | (int)InteractiveCaeMeshSelMode::Elements
                                            | (int)InteractiveCaeMeshSelMode::Nodes
                                            | (int)InteractiveCaeMeshSelMode::Box;

constexpr int InteractiveCaeMeshAuxSelModeMask = (int)InteractiveCaeMeshSelMode::Box;

}

#endif // INTERACTIVECAEMESHSELMODE_HEADERFILE