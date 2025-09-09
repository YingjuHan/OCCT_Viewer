#ifndef SIMMESH_TARGETTYPE_HEADERFILE
#define SIMMESH_TARGETTYPE_HEADERFILE

//! Enumeration that points the target vector type in the vertex pair info
enum class SimMesh_TargetType {
    Undefined, //!< Target is undefined yet
    Optimized, //!< Target was optimized with quadric
    UseFirst,  //!< Use position of first vertex
    UseSecond  //!< Use position of second vertex
};

#endif // SIMMESH_TARGETTYPE_HEADERFILE



