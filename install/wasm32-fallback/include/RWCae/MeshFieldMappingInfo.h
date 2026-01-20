#ifndef _MESHFIELDMAPPINGINFO_HEADERFILE
#define _MESHFIELDMAPPINGINFO_HEADERFILE

#include <string>
#include <vector>

namespace RWCae
{
    //! Property type stored in mapping file.
    enum class MeshMappingFieldType
    {
        Set,
        Scalar,
        Vector,
        Vector4,
        Tensor6,
        Tensor9,
        Unknown
    };

    //! Calculation point stored in mapping file.
    enum class MeshMappingCalcPoint
    {
        Nodes,
        ElementCentroids,
        ElementNodes,
        GaussPoints,
        Unknown
    };

    //! Mapping information parsed from JSON file.
    struct MeshFieldMappingInfo
    {
        MeshFieldMappingInfo() {}

        std::string FieldId;
        std::string FieldDescription;
        std::string FieldMask;
        std::vector<std::string> ComponentMasks;
        std::vector<std::string> ComponentUnits;
        std::vector<std::string> SourceUnits;
        MeshMappingFieldType ValueType = MeshMappingFieldType::Unknown;
        MeshMappingCalcPoint CalcPoint = MeshMappingCalcPoint::Unknown;
        bool NormalizeFlag = false;
    };

} // namespace
#endif
