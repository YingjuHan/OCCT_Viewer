#ifndef _MESHDATAIMPORTEXPORT_HEADERFILE
#define _MESHDATAIMPORTEXPORT_HEADERFILE

#include <RWCae/MeshPropertyData.h>

#include <NCollection_DataMap.hxx>
#include <NCollection_DoubleMap.hxx>
#include <TCollection_AsciiString.hxx>

namespace RWCae
{

    namespace MeshImportExport
    {
        //! Header string for mesh binary format.
        constexpr const char *MESH_DATA_HEADER = "MDB";
        constexpr uint16_t MESH_DATA_VERSION_MAJOR = 14;
        constexpr uint16_t MESH_DATA_VERSION_MINOR = 0;

        //! Changes:
        //! - added support for "timestamp" equal to NaN in the JSON header
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_14 = 14;

        //! - added 2nd order elements
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_13 = 13;

        //! Changes:
        //! - added polygonal/polyhedral elements
        //! - changed storing of mesh elements
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_12 = 12;

        //! Changes:
        //! - store group property info in metadata
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_11 = 11;

        //! Changes:
        //! - changed draco compression algorithm
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_10 = 10;

        //! Changes:
        //! - added splitting of compressed binary data into several parts
        //!   to process them in parallel
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_9 = 9;

        //! Changes:
        //! - mesh properties now can be written without binary data
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_8 = 8;

        //! Changes:
        //! - remove writing/reading of mesh indexed elements
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_7 = 7;

        //! Changes:
        //! - changed draco compression algorithm for mesh elements
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_6 = 6;

        //! Changes:
        //! - added draco compression of binary data
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_5 = 5;

        //! Changes:
        //! - buffers length and offset are now stored in Json as fixed-size string
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_4 = 4;

        //! Changes:
        //! - uint32_t offsets have been replaced with uint64_t
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_3 = 3;

        //! Changes:
        //! - scalar and vector mesh properties have been replaced with general multi-component properties
        constexpr uint16_t MESH_DATA_VERSION_MAJOR_2 = 2;
    }

    //! Options used in mesh binary reader/writer.
    enum class MeshDataImportExportOptions
    {
        //! Default option that means reading/writing all contents from/to MDB files.
        DEFAULTS = 0x0,
        //! Flag to skip reading/writing mesh geometry definition (nodes+elements) from/to MDB file.
        //! This flag could be used to split geometry and properties into separate MDB files.
        SKIP_GEOMETRY = 0x1,
        //! Flag to skip reading/writing nodal+elemental properties (fields) from/to MDB file.
        //! This flag could be used to split geometry and properties into separate MDB files.
        SKIP_PROPERTIES = 0x2,
        //! Flag to skip loading of binary data chunks for geometry,
        //! so that only file header (JSON) will be read/written from/to MDB file.
        SKIP_GEOMETRY_BINARY_DATA = 0x4,
        //! Flag to skip loading of binary data chunks for properties,
        //! so that only file header (JSON) will be read/written from/to MDB file.
        SKIP_PROPERTIES_BINARY_DATA = 0x8,
        //! Flag to skip loading of binary data chunks (geometry and properties),
        //! so that only file header (JSON) will be read/written from/to MDB file.
        //! This flag could be useful to determine file metadata without reading the whole data.
        SKIP_BINARY_DATA = SKIP_GEOMETRY_BINARY_DATA | SKIP_PROPERTIES_BINARY_DATA,
        //! Flag to save for properties the file path during reading.
        SAVE_PROPERTY_REF_FILE = 0x10,
        //! Flag to import/export MDB file in parallel mode.
        //! For now applied only to compressed files.
        PARALLEL_PROCESSING = 0x20
    };

    //! @internal Type of buffer used in mesh binary reader/writer.
    enum class MeshDataBufferType
    {
        NODE,
        ELEMENT_0D,
        ELEMENT_1D,
        ELEMENT_2D, //!< old type for 2d triangles/quads
        ELEMENT_TRIANGLE,
        ELEMENT_TRIANGLE_2ND,
        ELEMENT_QUAD,
        ELEMENT_QUAD_2ND,
        ELEMENT_POLYGON,
        ELEMENT_TETRAHEDRON,
        ELEMENT_TETRAHEDRON_2ND,
        ELEMENT_PYRAMID,
        ELEMENT_PYRAMID_2ND,
        ELEMENT_PRISM,
        ELEMENT_PRISM_2ND,
        ELEMENT_HEXAHEDRON,
        ELEMENT_HEXAHEDRON_2ND,
        ELEMENT_POLYHEDRON,
        ELEMENT_SURFACE, //!< old type for 2d outer surface
        ELEMENT_SURFTRIANGLE,
        ELEMENT_SURFTRIANGLE_2ND,
        ELEMENT_SURFQUAD,
        ELEMENT_SURFQUAD_2ND,
        ELEMENT_SURFPOLYGON,
        ELEMENT_INDEXED,
        PROPERTY_SCALAR,
        PROPERTY_NODAL_ELEMENTAL,
        PROPERTY_GROUP
    };

    //! @internal Type of Json element used in mesh binary reader/writer.
    enum class MeshDataJsonElement
    {
        ASSET,
        MESH,
        BUFFERS,
        BUFFER_LENGTH,
        BUFFER_OFFSET,
        GENERATOR,
        SUBVERSION,
        MESH_NAME,
        GEOMETRY,
        PROPERTIES,
        NODES,
        ELEMENTS,
        ELEMENT_TYPE,
        ELEMENT_INDICES,
        PROPERTY_COMP_TYPE,
        PROPERTY_ID,
        PROPERTY_NAME,
        PROPERTY_DESCRIPTION,
        PROPERTY_IS_ELEMENTAL,
        PROPERTY_TYPE,
        PROPERTY_NB_COMPONENTS,
        PROPERTY_TIMESTAMP,
        PROPERTY_TIMESTEP,
        PROPERTY_COMPONENTS,
        PROPERTY_MIN,
        PROPERTY_MAX,
        PROPERTY_IS_ENABLED,
        PROPERTY_VALUES,
        PROPERTY_GROUPS,
        PROPERTY_GROUP_ID,
        PROPERTY_GROUP_NAME,
        PROPERTY_GROUP_NB_ENTITIES,
        PROPERTY_UNITS,
        PROPERTY_TIME_DESCRIPTION,
        PROPERTY_TIME_UNITS,
        PROPERTY_ELEM_TYPE,
        BOUNDS_MAX,
        BOUNDS_MIN,
        BLOCKS,
        BLOCK_NAME,
        BLOCK_CHILDREN,
        BLOCK_RANGE,
        COMPRESSION,
        COMPRESSION_CODEC,
        COMPRESSION_DRACO
    };

    //! @internal Mapper between MeshDataJsonElement and string.
    class MeshDataJsonElementConvertor
    {
    public:
        static const char *GetString(MeshDataJsonElement theElement)
        {
            static NCollection_DataMap<int, TCollection_AsciiString> aMap = fillMap();
            return aMap.Find((int)theElement).ToCString();
        }

    private:
        static NCollection_DataMap<int, TCollection_AsciiString> fillMap()
        {
            NCollection_DataMap<int, TCollection_AsciiString> aMap;
            aMap.Bind((int)MeshDataJsonElement::ASSET, "asset");
            aMap.Bind((int)MeshDataJsonElement::MESH, "mesh");
            aMap.Bind((int)MeshDataJsonElement::BUFFERS, "buffers");
            aMap.Bind((int)MeshDataJsonElement::BUFFER_LENGTH, "length");
            aMap.Bind((int)MeshDataJsonElement::BUFFER_OFFSET, "offset");
            aMap.Bind((int)MeshDataJsonElement::GENERATOR, "generator");
            aMap.Bind((int)MeshDataJsonElement::SUBVERSION, "subversion");
            aMap.Bind((int)MeshDataJsonElement::MESH_NAME, "name");
            aMap.Bind((int)MeshDataJsonElement::GEOMETRY, "geometry");
            aMap.Bind((int)MeshDataJsonElement::PROPERTIES, "properties");
            aMap.Bind((int)MeshDataJsonElement::NODES, "nodes");
            aMap.Bind((int)MeshDataJsonElement::ELEMENTS, "elements");
            aMap.Bind((int)MeshDataJsonElement::ELEMENT_TYPE, "type");
            aMap.Bind((int)MeshDataJsonElement::ELEMENT_INDICES, "indices");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_COMP_TYPE, "comptype");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_ID, "id");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_NAME, "name");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_DESCRIPTION, "description");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_IS_ELEMENTAL, "iselemental");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_TYPE, "type");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_NB_COMPONENTS, "nbcomponents");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_TIMESTAMP, "timestamp");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_TIMESTEP, "timestep");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_COMPONENTS, "components");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_MIN, "min");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_MAX, "max");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_IS_ENABLED, "isenabled");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_VALUES, "values");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_GROUPS, "groups");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_GROUP_ID, "id");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_GROUP_NAME, "name");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_GROUP_NB_ENTITIES, "nbentities");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_UNITS, "units");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_TIME_UNITS, "timeunits");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_TIME_DESCRIPTION, "timedescription");
            aMap.Bind((int)MeshDataJsonElement::PROPERTY_ELEM_TYPE, "elemtype");
            aMap.Bind((int)MeshDataJsonElement::BOUNDS_MAX, "boundsMax");
            aMap.Bind((int)MeshDataJsonElement::BOUNDS_MIN, "boundsMin");
            aMap.Bind((int)MeshDataJsonElement::BLOCKS, "blocks");
            aMap.Bind((int)MeshDataJsonElement::BLOCK_NAME, "name");
            aMap.Bind((int)MeshDataJsonElement::BLOCK_CHILDREN, "children");
            aMap.Bind((int)MeshDataJsonElement::BLOCK_RANGE, "range");
            aMap.Bind((int)MeshDataJsonElement::COMPRESSION, "compression");
            aMap.Bind((int)MeshDataJsonElement::COMPRESSION_CODEC, "codec");
            aMap.Bind((int)MeshDataJsonElement::COMPRESSION_DRACO, "draco");
            return aMap;
        }
    };

    //! @internal Mapper between MeshDataBufferType and string.
    class MeshDataBufferTypeConvertor
    {
    public:
        static const char *GetString(MeshDataBufferType theType)
        {
            static NCollection_DoubleMap<int, TCollection_AsciiString> aMap = fillMap();
            return aMap.Find1((int)theType).ToCString();
        }

        static MeshDataBufferType GetType(const char *theString)
        {
            static NCollection_DoubleMap<int, TCollection_AsciiString> aMap = fillMap();
            return (MeshDataBufferType)aMap.Find2(theString);
        }

    private:
        static NCollection_DoubleMap<int, TCollection_AsciiString> fillMap()
        {
            NCollection_DoubleMap<int, TCollection_AsciiString> aMap;
            aMap.Bind((int)MeshDataBufferType::NODE, "node");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_0D, "0d");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_1D, "1d");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_2D, "2d");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_TRIANGLE, "triangle");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_TRIANGLE_2ND, "triangle2nd");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_QUAD, "quad");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_QUAD_2ND, "quad2nd");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_POLYGON, "polygon");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_TETRAHEDRON, "tetrahedron");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_TETRAHEDRON_2ND, "tetrahedron2nd");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_PYRAMID, "pyramid");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_PYRAMID_2ND, "pyramid2nd");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_PRISM, "prism");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_PRISM_2ND, "prism2nd");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_HEXAHEDRON, "hexahedron");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_HEXAHEDRON_2ND, "hexahedron2nd");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_POLYHEDRON, "polyhedron");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_SURFACE, "surface");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_SURFTRIANGLE, "surftriangle");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_SURFTRIANGLE_2ND, "surftriangle2nd");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_SURFQUAD, "surfquad");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_SURFQUAD_2ND, "surfquad2nd");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_SURFPOLYGON, "surfpolygon");
            aMap.Bind((int)MeshDataBufferType::ELEMENT_INDEXED, "indexed");
            aMap.Bind((int)MeshDataBufferType::PROPERTY_SCALAR, "scalar");
            aMap.Bind((int)MeshDataBufferType::PROPERTY_NODAL_ELEMENTAL, "nodalelemental");
            aMap.Bind((int)MeshDataBufferType::PROPERTY_GROUP, "group");
            return aMap;
        }
    };

    //! @internal Mapper between MeshPropertyElemType and string.
    class MeshPropertyElemTypeConvertor
    {
    public:
        static const char *GetString(MeshPropertyElemType theType)
        {
            static NCollection_DoubleMap<int, TCollection_AsciiString> aMap = fillMap();
            return aMap.Find1((int)theType).ToCString();
        }

        static MeshPropertyElemType GetType(const char *theString)
        {
            static NCollection_DoubleMap<int, TCollection_AsciiString> aMap = fillMap();
            return (MeshPropertyElemType)aMap.Find2(theString);
        }

    private:
        static NCollection_DoubleMap<int, TCollection_AsciiString> fillMap()
        {
            NCollection_DoubleMap<int, TCollection_AsciiString> aMap;
            aMap.Bind((int)MeshPropertyElemType::Double, "double");
            aMap.Bind((int)MeshPropertyElemType::Float, "float");
            return aMap;
        }
    };

}
#endif