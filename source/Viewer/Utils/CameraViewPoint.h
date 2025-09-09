#ifndef _CAMERAVIEWPOINT_HEADERFILE
#define _CAMERAVIEWPOINT_HEADERFILE

#include "VecN.h"

namespace Viewer {
//! Represents Graphic3d_Camera params - as an object passed by value.
struct CameraViewPoint {
    Vec3   eye;       //!< eye location
    Vec3   up;        //!< direction of up vector
    Vec3   direction; //!< direction of look
    double distance;  //!< distance between camera and point to look
    double scale;     //!< viewport relative scale

    //! Empty constructor.
    Standard_EXPORT CameraViewPoint();
};

} // namespace

#endif // _CAMERAVIEWPOINT_HEADERFILE      