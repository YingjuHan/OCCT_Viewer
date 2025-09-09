#ifndef _CAEMESHPROPINFO_HEADERFILE
#define _CAEMESHPROPINFO_HEADERFILE

#include "VecN.h"

#include <RWCae/MeshData.h>

namespace Viewer {
enum class CaeMeshPropType { SCALAR_SET, VECTOR, TENSOR };
enum class CaeMeshPropElemType { SCALAR, NODEVEC, GROUP };

//! Information about CAE mesh property component.
struct CaeMeshPropCompInfo {
    int         index = -1;
    std::string name;
    std::string description;
    std::string displayUnits;
    Vec2        valueRange;

    //! Empty constructor.
    CaeMeshPropCompInfo() {}

    //! Constructor.
    Standard_EXPORT CaeMeshPropCompInfo(const Handle(RWCae::MeshNVecProperty)& theProp, int theCompIndex);
};

//! Information about CAE mesh property.
struct CaeMeshPropInfo {
    std::string      propId;
    std::vector<int> timeSteps;
    std::vector<int> loadedTimeSteps;
    std::string      name;
    std::string      description;
    int              type        = 0;
    int              elemType    = 0;
    bool             isElemental = false;

    std::vector<CaeMeshPropCompInfo> comps;

    //! Empty constructor.
    CaeMeshPropInfo() {}

    //! Constructor.
    Standard_EXPORT CaeMeshPropInfo(const Handle(RWCae::MeshData)& theMeshData,
                                    const TCollection_AsciiString&   thePropId);
};

} // namespace

#endif // _CAEMESHPROPINFO_HEADERFILE   
