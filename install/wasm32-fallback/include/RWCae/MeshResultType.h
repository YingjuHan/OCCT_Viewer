#ifndef _MESHERESULTTYPE_HEADERFILE
#define _MESHERESULTTYPE_HEADERFILE

#include <map>
#include <string>
#include <vector>

namespace RWCae
{
    //! Type of mesh result.
    struct MeshResultType
    {
        MeshResultType() {}

        std::string TypeId;
        std::string Description;
        std::string Format;
        std::map<std::string, std::string> Options;
    };
}
#endif