#ifndef MESHDRACOCOMPRESSIONHELPER_HEADERFILE
#define MESHDRACOCOMPRESSIONHELPER_HEADERFILE

#include <Standard_Macro.hxx>

#ifdef OCCTV_HAVE_DRACO
#include <Standard_WarningsDisable.hxx> // NOLINT
#include <draco/core/encoder_buffer.h>
#include <draco/point_cloud/point_cloud.h>
#include <Standard_WarningsRestore.hxx> // NOLINT
#endif

#include <ostream>
#include <vector>
#include <cstdint>

namespace RWCae
{
    //! Helper class to encode arrays of typed values using draco.
    class MeshAttributeEncoder
    {
    public:
        //! Constructor.
        MeshAttributeEncoder() : myAttrOptions({}) {}

        //! Init encoder.
        Standard_EXPORT void Init(int theNbValues);

        //! Add a new double attribute.
        Standard_EXPORT void AddAttributeFloat64(int theNbComponents);

        //! Add a new float attribute.
        Standard_EXPORT void AddAttributeFloat32(int theNbComponents);

        //! Add a new unsigned char attribute.
        Standard_EXPORT void AddAttributeUInt8(int theNbComponents);

        //! Add a new unsigned short attribute.
        Standard_EXPORT void AddAttributeUInt16(int theNbComponents);

        //! Add a new int attribute.
        Standard_EXPORT void AddAttributeInt32(int theNbComponents);

        //! Add a new unsigned int attribute.
        Standard_EXPORT void AddAttributeUInt32(int theNbComponents);

        //! Add a new unsigned long attribute.
        Standard_EXPORT void AddAttributeUInt64(int theNbComponents);

        //! Set attribute value.
        Standard_EXPORT void SetValue(int theAttrIdx, int theIndex, const void *theValue);

        //! Set attribute quantization options.
        Standard_EXPORT void SetQuantOptions(int theAttrIdx,
                                             int theQuantBit,
                                             const std::vector<double> &theMin,
                                             const std::vector<double> &theMax);

        //! Encode attributes.
        Standard_EXPORT bool Encode();

        //! Return size of encoded data.
        Standard_EXPORT uint64_t EncodedSize() const;

        //! Write encoded attributes to the stream.
        Standard_EXPORT void Write(std::ostream &theStream, uint64_t &theLength);

    protected:
        //! Add a new attribute.
        Standard_EXPORT void addAttribute(int theNbComponents, int theType);

        //! Quantization info.
        struct AttributeOptions
        {
            int QuantBit = -1;
            std::vector<double> Min;
            std::vector<double> Max;
        };

    protected:
#ifdef OCCTV_HAVE_DRACO
        draco::PointCloud myPC;
        draco::EncoderBuffer myBuffer;
#endif

        std::vector<AttributeOptions> myAttrOptions;
    };

    //! Helper class to decode arrays of typed values using draco.
    class MeshAttributeDecoder
    {
    public:
        //! Constructor.
        MeshAttributeDecoder() {}

        //! Decode encoded values.
        Standard_EXPORT bool Decode(const char *theData, size_t theSize, size_t &theOffset);

        //! Return attribute value.
        Standard_EXPORT void GetValue(int theAttrIdx, int theIndex, void *theValue) const;

        //! Return number of decoded values.
        Standard_EXPORT size_t NbValues() const;

        //! Set to use old origin/range parameters instead of min/max.
        void SetUseOriginRangeParams(bool theToUse) { myToUseOriginRange = theToUse; }

    protected:
#ifdef OCCTV_HAVE_DRACO
        draco::PointCloud myPC;
#endif
        bool myToUseOriginRange = false;
    };

}

#endif // RWCBASE_MESHDRACOCOMPRESSIONHELPER_HEADERFILE
