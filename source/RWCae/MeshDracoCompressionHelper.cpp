#include <RWCae/MeshDracoCompressionHelper.h>

#include <RWCae/MeshPropertyData.h>

#ifdef OCCTV_HAVE_DRACO
#include <Standard_WarningsDisable.hxx> // NOLINT
#include <draco/attributes/attribute_quantization_transform.h>
#include <draco/compression/point_cloud/point_cloud_sequential_decoder.h>
#include <draco/compression/point_cloud/point_cloud_sequential_encoder.h>
#include <draco/compression/attributes/attributes_decoder.h>
#include <draco/compression/attributes/linear_sequencer.h>
#include <draco/compression/attributes/sequential_attribute_decoders_controller.h>
#include <draco/compression/attributes/sequential_attribute_encoders_controller.h>
#include <draco/compression/attributes/sequential_integer_attribute_decoder.h>
#include <draco/compression/attributes/sequential_integer_attribute_encoder.h>
#include <draco/metadata/geometry_metadata.h>
#include <Standard_WarningsRestore.hxx> // NOLINT

#include <limits>
#endif

namespace
{
#ifdef OCCTV_HAVE_DRACO
    //! Extended draco transform for quantized attributes that can contain NaN values.
    class AttributeQuantizationTransformExt : public draco::AttributeQuantizationTransform
    {
    public:
        //! Set quantization parameters.
        bool SetParametersExt(int theQuantBits, const float *theMinValues, const float *theMaxValues, int theNbComps)
        {
            if (!draco::AttributeQuantizationTransform::SetParameters(theQuantBits, theMinValues, theNbComps, 0.0f))
            {
                return false;
            }
            myQuantBits = theQuantBits;
            myMaxValues.assign(theMaxValues, theMaxValues + theNbComps);
            return true;
        }

        //! Encode quantization parameters.
        bool EncodeParameters(draco::EncoderBuffer *theBuffer) const override
        {
            if (is_initialized())
            {
                theBuffer->Encode(myMaxValues.data(), sizeof(float) * myMaxValues.size());
            }

            return draco::AttributeQuantizationTransform::EncodeParameters(theBuffer);
        }

        //! Decode quantization parameters.
        bool DecodeParameters(const draco::PointAttribute &theAttr, draco::DecoderBuffer *theBuffer) override
        {
            myMaxValues.resize(theAttr.num_components());
            if (!myToUseOriginRange)
            {
                if (!theBuffer->Decode(&myMaxValues[0], sizeof(float) * myMaxValues.size()))
                {
                    return false;
                }
            }

            if (!draco::AttributeQuantizationTransform::DecodeParameters(theAttr, theBuffer))
            {
                return false;
            }

            theBuffer->Advance(-(int64_t)sizeof(uint8_t));
            uint8_t aQuantBits = 0;
            theBuffer->Decode(&aQuantBits);
            myQuantBits = aQuantBits;

            if (myToUseOriginRange)
            {
                for (int i = 0; i < (int)myMaxValues.size(); i++)
                {
                    myMaxValues[i] = min_values()[i] + range();
                }
            }

            return true;
        }

        //! Quantize attribute values.
        bool TransformAttribute(const draco::PointAttribute &theAttr,
                                const std::vector<draco::PointIndex> &theIds,
                                draco::PointAttribute *theTargetAttr) override
        {
            if (theIds.empty())
            {
                GeneratePortableAttributeExt(theAttr, static_cast<int>(theTargetAttr->size()), theTargetAttr);
            }
            else
            {
                GeneratePortableAttributeExt(theAttr, theIds, static_cast<int>(theTargetAttr->size()), theTargetAttr);
            }
            return true;
        }

        //! Quantize attribute values.
        void GeneratePortableAttributeExt(const draco::PointAttribute &theAttr,
                                          int theNbPoints,
                                          draco::PointAttribute *theTargetAttr) const
        {
            const int aNbComps = theAttr.num_components();

            int32_t *aPortAttrData = reinterpret_cast<int32_t *>(theTargetAttr->GetAddress(draco::AttributeValueIndex(0)));

            const float aMaxQuantValue = static_cast<float>((1 << (myQuantBits)) - 2);
            int32_t aDstIdx = 0;

            std::vector<float> aRange(aNbComps);
            for (int i = 0; i < aNbComps; i++)
            {
                aRange[i] = myMaxValues[i] - min_values()[i];
                if (aRange[i] == 0.0f)
                {
                    aRange[i] = 1.0f;
                }
            }

            std::vector<float> anAttrValue(aNbComps);
            for (draco::PointIndex i(0); i < theNbPoints; ++i)
            {
                const draco::AttributeValueIndex anAttrIndex = theAttr.mapped_index(i);
                theAttr.GetValue(anAttrIndex, anAttrValue.data());
                for (int c = 0; c < aNbComps; ++c)
                {
                    int32_t aQuantValue = 0;
                    if (!RWCae::IsNaN(anAttrValue[c]))
                    {
                        const float aValue = (anAttrValue[c] - min_values()[c]);
                        aQuantValue = static_cast<int32_t>(floor(aValue / aRange[c] * aMaxQuantValue + 0.5f)) + 1;
                    }
                    aPortAttrData[aDstIdx++] = aQuantValue;
                }
            }
        }

        //! Quantize attribute values.
        void GeneratePortableAttributeExt(const draco::PointAttribute &theAttr,
                                          const std::vector<draco::PointIndex> &theIds,
                                          int,
                                          draco::PointAttribute *theTargetAttr) const
        {
            const int aNbComps = theAttr.num_components();

            int32_t *aPortAttrData = reinterpret_cast<int32_t *>(theTargetAttr->GetAddress(draco::AttributeValueIndex(0)));

            std::vector<float> aRange(aNbComps);
            for (int i = 0; i < aNbComps; i++)
            {
                aRange[i] = myMaxValues[i] - min_values()[i];
                if (aRange[i] == 0.0f)
                {
                    aRange[i] = 1.0f;
                }
            }

            const float aMaxQuantValue = static_cast<float>((1 << (myQuantBits)) - 2);
            int32_t aDstIdx = 0;

            std::vector<float> anAttrVal(aNbComps);
            for (uint32_t i = 0; i < theIds.size(); ++i)
            {
                const draco::AttributeValueIndex anAttrIdx = theAttr.mapped_index(theIds[i]);
                theAttr.GetValue(anAttrIdx, anAttrVal.data());
                for (int c = 0; c < aNbComps; ++c)
                {
                    int32_t aQuantValue = 0;
                    if (!RWCae::IsNaN(anAttrVal[c]))
                    {
                        const float aValue = (anAttrVal[c] - min_values()[c]);
                        aQuantValue = static_cast<int32_t>(floor(aValue / aRange[c] * aMaxQuantValue + 0.5f)) + 1;
                    }
                    aPortAttrData[aDstIdx++] = aQuantValue;
                }
            }
        }

        //! Dequantize attribute values.
        bool InverseTransformAttribute(const draco::PointAttribute &theAttr, draco::PointAttribute *theTargetAttr) override
        {
            const int32_t aMaxQuantValue = (1u << static_cast<uint32_t>(myQuantBits)) - 2;
            const int aNbComps = theTargetAttr->num_components();
            const int anEntrySize = static_cast<int>(sizeof(float)) * aNbComps;

            std::vector<float> anAttrVal(aNbComps);

            int aQuantValId = 0;
            int anOutBytePos = 0;

            if (aMaxQuantValue <= 0)
            {
                return false;
            }
            std::vector<float> aDelta(aNbComps);
            for (int i = 0; i < aNbComps; i++)
            {
                aDelta[i] = (myMaxValues[i] - min_values()[i]) / static_cast<float>(aMaxQuantValue);
            }

            const int32_t *aSrcAttrData = reinterpret_cast<const int32_t *>(
                theAttr.GetAddress(draco::AttributeValueIndex(0)));

            const uint32_t aNbValues = static_cast<uint32_t>(theTargetAttr->size());

            for (uint32_t i = 0; i < aNbValues; ++i)
            {
                for (int c = 0; c < aNbComps; ++c)
                {
                    float aValue = std::numeric_limits<float>::quiet_NaN();
                    if (aSrcAttrData[aQuantValId] != 0)
                    {
                        aValue = static_cast<float>(aSrcAttrData[aQuantValId] - 1) * aDelta[c];
                        aValue = std::min(aValue + min_values()[c], myMaxValues[c]);
                    }
                    aQuantValId++;
                    anAttrVal[c] = aValue;
                }
                theTargetAttr->buffer()->Write(anOutBytePos, anAttrVal.data(), anEntrySize);
                anOutBytePos += anEntrySize;
            }
            return true;
        }

        //! Set to use old origin/range parameters instead of min/max.
        void SetUseOriginRangeParams(bool theToUse) { myToUseOriginRange = theToUse; }

    protected:
        std::vector<float> myMaxValues;
        int32_t myQuantBits;
        bool myToUseOriginRange = false;
    };

    //! Extended draco sequential quantization attribute encoder.
    class SequentialQuantizationAttributeEncoderExt : public draco::SequentialIntegerAttributeEncoder
    {
    public:
        SequentialQuantizationAttributeEncoderExt() : draco::SequentialIntegerAttributeEncoder() {}

        //! Return encoder's unique id.
        uint8_t GetUniqueId() const override { return draco::SEQUENTIAL_ATTRIBUTE_ENCODER_QUANTIZATION; }

        //! Init attribute encoder.
        bool Init(draco::PointCloudEncoder *theEncoder, int theAttrId) override
        {
            if (!draco::SequentialIntegerAttributeEncoder::Init(theEncoder, theAttrId))
            {
                return false;
            }

            const int aQuantBits = theEncoder->options()->GetAttributeInt(theAttrId, "quantization_bits", -1);
            if (aQuantBits < 1)
            {
                return false;
            }

            const draco::AttributeMetadata *aMetadata = theEncoder->point_cloud()->GetAttributeMetadataByAttributeId(
                theAttrId);

            std::vector<double> aMin;
            std::vector<double> aMax;

            if (aMetadata == nullptr || !aMetadata->GetEntryDoubleArray("min", &aMin) || !aMetadata->GetEntryDoubleArray("max", &aMax))
            {
                return false;
            }

            std::vector<float> aFloatMin;
            for (const double &aValue : aMin)
            {
                aFloatMin.push_back((float)aValue);
            }
            std::vector<float> aFloatMax;
            for (const double &aValue : aMax)
            {
                aFloatMax.push_back((float)aValue);
            }
            return myAttrQuantTransform.SetParametersExt(aQuantBits,
                                                         aFloatMin.data(),
                                                         aFloatMax.data(),
                                                         static_cast<int>(aFloatMin.size()));
        }

        //! Check if encoder is lossy.
        bool IsLossyEncoder() const override { return true; }

        //! Encode quantization parameters.
        bool EncodeDataNeededByPortableTransform(draco::EncoderBuffer *theBuffer) override
        {
            return myAttrQuantTransform.EncodeParameters(theBuffer);
        }

    protected:
        //! Put quantized values in portable attribute for sequential encoding.
        bool PrepareValues(const std::vector<draco::PointIndex> &theIds, int) override
        {
            std::unique_ptr<draco::PointAttribute> aPortAttr =
                myAttrQuantTransform.InitTransformedAttribute(*attribute(), static_cast<int>(theIds.size()));
            if (!myAttrQuantTransform.TransformAttribute(*(attribute()), theIds, aPortAttr.get()))
            {
                return false;
            }
            SetPortableAttribute(std::move(aPortAttr));
            return true;
        }

    protected:
        AttributeQuantizationTransformExt myAttrQuantTransform;
    };

    //! Extended draco sequential attribute encoders controller.
    class SequentialAttributeEncodersControllerExt : public draco::SequentialAttributeEncodersController
    {
    public:
        SequentialAttributeEncodersControllerExt(std::unique_ptr<draco::PointsSequencer> theSequencer, int thePntAttrId)
            : draco::SequentialAttributeEncodersController(std::move(theSequencer), thePntAttrId)
        {
        }

    protected:
        //! Create sequential attribute encoder.
        std::unique_ptr<draco::SequentialAttributeEncoder> CreateSequentialEncoder(int theAttrIdx) override
        {
            const int32_t anAttrId = GetAttributeId(theAttrIdx);
            const draco::PointAttribute *att = encoder()->point_cloud()->attribute(anAttrId);

            switch (att->data_type())
            {
            case draco::DT_UINT8:
            case draco::DT_INT8:
            case draco::DT_UINT16:
            case draco::DT_INT16:
            case draco::DT_UINT32:
            case draco::DT_INT32:
                return std::make_unique<draco::SequentialIntegerAttributeEncoder>();
            case draco::DT_FLOAT32:
                if (encoder()->options()->GetAttributeInt(anAttrId, "quantization_bits", -1) > 0)
                {
                    return std::make_unique<SequentialQuantizationAttributeEncoderExt>();
                }
                break;
            default:
                break;
            }
            return std::make_unique<draco::SequentialAttributeEncoder>();
        }
    };

    //! Extended draco point cloud sequential encoder.
    class PointCloudSequentialEncoderExt : public draco::PointCloudSequentialEncoder
    {
    protected:
        //! Generate attributes encoder.
        bool GenerateAttributesEncoder(int32_t theAttrId) override
        {
            if (theAttrId == 0)
            {
                AddAttributesEncoder(std::make_unique<SequentialAttributeEncodersControllerExt>(
                    std::make_unique<draco::LinearSequencer>(point_cloud()->num_points()),
                    theAttrId));
            }
            else
            {
                attributes_encoder(0)->AddAttributeId(theAttrId);
            }
            return true;
        }
    };

    //! Extended draco sequential quantization attribute decoder.
    class SequentialQuantizationAttributeDecoderExt : public draco::SequentialIntegerAttributeDecoder
    {
    public:
        //! Constructor.
        SequentialQuantizationAttributeDecoderExt(bool theToUseOriginRangeParams)
        {
            myAttrQuantTransform.SetUseOriginRangeParams(theToUseOriginRangeParams);
        }

    protected:
        //! Decode quantization info.
        bool DecodeDataNeededByPortableTransform(const std::vector<draco::PointIndex> &, draco::DecoderBuffer *) override
        {
            auto att = GetPortableAttribute();
            if (att == nullptr)
            {
                att = attribute();
            }
            if (!myAttrQuantTransform.DecodeParameters(*att, decoder()->buffer()))
            {
                return false;
            }

            return myAttrQuantTransform.TransferToAttribute(portable_attribute());
        }

        //! Dequantize values.
        bool StoreValues(uint32_t) override
        {
            return myAttrQuantTransform.InverseTransformAttribute(*GetPortableAttribute(), attribute());
        }

    private:
        AttributeQuantizationTransformExt myAttrQuantTransform;
    };

    //! Extended draco sequential attribute decoders controller.
    class SequentialAttributeDecodersControllerExt : public draco::SequentialAttributeDecodersController
    {
    public:
        //! Constructor.
        explicit SequentialAttributeDecodersControllerExt(std::unique_ptr<draco::PointsSequencer> theSequencer,
                                                          bool theToUseOriginRangeParams)
            : draco::SequentialAttributeDecodersController(std::move(theSequencer)),
              myToUseOriginRange(theToUseOriginRangeParams)
        {
        }

    protected:
        //! Create sequential decoder.
        std::unique_ptr<draco::SequentialAttributeDecoder> CreateSequentialDecoder(uint8_t theDecoderType) override
        {
            switch (theDecoderType)
            {
            case draco::SEQUENTIAL_ATTRIBUTE_ENCODER_GENERIC:
                return std::make_unique<draco::SequentialAttributeDecoder>();
            case draco::SEQUENTIAL_ATTRIBUTE_ENCODER_INTEGER:
                return std::make_unique<draco::SequentialIntegerAttributeDecoder>();
            case draco::SEQUENTIAL_ATTRIBUTE_ENCODER_QUANTIZATION:
                return std::make_unique<SequentialQuantizationAttributeDecoderExt>(myToUseOriginRange);
            default:
                break;
            }
            return nullptr;
        }

    protected:
        bool myToUseOriginRange = false;
    };

    //! Extended draco point cloud sequential decoder.
    class PointCloudSequentialDecoderExt : public draco::PointCloudSequentialDecoder
    {
    public:
        //! Set to use old origin/range parameters instead of min/max.
        void SetUseOriginRangeParams(bool theToUse) { myToUseOriginRange = theToUse; }

    protected:
        //! Create attributes decoder.
        bool CreateAttributesDecoder(int32_t theAttrDecoderId) override
        {
            return SetAttributesDecoder(theAttrDecoderId,
                                        std::make_unique<SequentialAttributeDecodersControllerExt>(
                                            std::make_unique<draco::LinearSequencer>(point_cloud()->num_points()),
                                            myToUseOriginRange));
        }

    protected:
        bool myToUseOriginRange = false;
    };
#endif

}

namespace RWCae
{
    // ================================================================
    // Function : Init
    // ================================================================
    void MeshAttributeEncoder::Init(int theNbValues)
    {
#ifdef OCCTV_HAVE_DRACO
        myPC.set_num_points(theNbValues);
#else
        (void)theNbValues;
#endif
    }

    // ================================================================
    // Function : AddAttributeFloat64
    // ================================================================
    void MeshAttributeEncoder::AddAttributeFloat64(int theNbComponents)
    {
#ifdef OCCTV_HAVE_DRACO
        addAttribute(theNbComponents, draco::DT_FLOAT64);
#else
        (void)theNbComponents;
#endif
    }

    // ================================================================
    // Function : AddAttributeFloat32
    // ================================================================
    void MeshAttributeEncoder::AddAttributeFloat32(int theNbComponents)
    {
#ifdef OCCTV_HAVE_DRACO
        addAttribute(theNbComponents, draco::DT_FLOAT32);
#else
        (void)theNbComponents;
#endif
    }

    // ================================================================
    // Function : AddAttributeUInt8
    // ================================================================
    void MeshAttributeEncoder::AddAttributeUInt8(int theNbComponents)
    {
#ifdef OCCTV_HAVE_DRACO
        addAttribute(theNbComponents, draco::DT_UINT8);
#else
        (void)theNbComponents;
#endif
    }

    // ================================================================
    // Function : AddAttributeUInt16
    // ================================================================
    void MeshAttributeEncoder::AddAttributeUInt16(int theNbComponents)
    {
#ifdef OCCTV_HAVE_DRACO
        addAttribute(theNbComponents, draco::DT_UINT16);
#else
        (void)theNbComponents;
#endif
    }

    // ================================================================
    // Function : AddAttributeInt32
    // ================================================================
    void MeshAttributeEncoder::AddAttributeInt32(int theNbComponents)
    {
#ifdef OCCTV_HAVE_DRACO
        addAttribute(theNbComponents, draco::DT_INT32);
#else
        (void)theNbComponents;
#endif
    }

    // ================================================================
    // Function : AddAttributeUInt32
    // ================================================================
    void MeshAttributeEncoder::AddAttributeUInt32(int theNbComponents)
    {
#ifdef OCCTV_HAVE_DRACO
        addAttribute(theNbComponents, draco::DT_UINT32);
#else
        (void)theNbComponents;
#endif
    }

    // ================================================================
    // Function : AddAttributeUInt64
    // ================================================================
    void MeshAttributeEncoder::AddAttributeUInt64(int theNbComponents)
    {
#ifdef OCCTV_HAVE_DRACO
        addAttribute(theNbComponents, draco::DT_UINT64);
#else
        (void)theNbComponents;
#endif
    }

    // ================================================================
    // Function : addAttribute
    // ================================================================
    void MeshAttributeEncoder::addAttribute(int theNbComponents, int theType)
    {
#ifdef OCCTV_HAVE_DRACO
        draco::GeometryAttribute aGeomAttr;
        aGeomAttr.Init(draco::GeometryAttribute::GENERIC,
                       nullptr,
                       theNbComponents,
                       (draco::DataType)theType,
                       false,
                       draco::DataTypeLength((draco::DataType)theType) * theNbComponents,
                       0);
        myPC.AddAttribute(aGeomAttr, true, myPC.num_points());

        myAttrOptions.push_back(AttributeOptions());
#else
        (void)theNbComponents;
        (void)theType;
#endif
    }

    // ================================================================
    // Function : SetValue
    // ================================================================
    void MeshAttributeEncoder::SetValue(int theAttrIdx, int theIndex, const void *theValue)
    {
#ifdef OCCTV_HAVE_DRACO
        myPC.attribute(theAttrIdx)->SetAttributeValue(draco::AttributeValueIndex((uint32_t)theIndex), theValue);
#else
        (void)theAttrIdx;
        (void)theIndex;
        (void)theValue;
#endif
    }

    // ================================================================
    // Function : SetQuantOptions
    // ================================================================
    void MeshAttributeEncoder::SetQuantOptions(int theAttrIdx,
                                               int theQuantBit,
                                               const std::vector<double> &theMin,
                                               const std::vector<double> &theMax)
    {
        myAttrOptions[theAttrIdx].QuantBit = theQuantBit;
        myAttrOptions[theAttrIdx].Min = theMin;
        myAttrOptions[theAttrIdx].Max = theMax;
    }

    // ================================================================
    // Function : Encode
    // ================================================================
    bool MeshAttributeEncoder::Encode()
    {
#ifdef OCCTV_HAVE_DRACO
        PointCloudSequentialEncoderExt anEncoder;
        anEncoder.SetPointCloud(myPC);

        draco::EncoderOptions anOptions = draco::EncoderOptions::CreateEmptyOptions();

        for (int i = 0; i < (int)myAttrOptions.size(); i++)
        {
            if (myAttrOptions[i].QuantBit == -1)
            {
                continue;
            }
            anOptions.SetAttributeInt(i, "quantization_bits", myAttrOptions[i].QuantBit);

            std::unique_ptr<draco::AttributeMetadata> metadata = std::make_unique<draco::AttributeMetadata>();
            metadata->AddEntryDoubleArray("min", myAttrOptions[i].Min);
            metadata->AddEntryDoubleArray("max", myAttrOptions[i].Max);
            myPC.AddAttributeMetadata(i, std::move(metadata));
        }

        return anEncoder.Encode(anOptions, &myBuffer).ok();
#else
        return false;
#endif
    }

    // ================================================================
    // Function : EncodedSize
    // ================================================================
    uint64_t MeshAttributeEncoder::EncodedSize() const
    {
#ifdef OCCTV_HAVE_DRACO
        return (myPC.num_points() > 0 ? myBuffer.size() : 0) + sizeof(uint64_t);
#else
        return 0;
#endif
    }

    // ================================================================
    // Function : Write
    // ================================================================
    void MeshAttributeEncoder::Write(std::ostream &theStream, uint64_t &theLength)
    {
#ifdef OCCTV_HAVE_DRACO
        const uint64_t anEncodedSize = myPC.num_points() > 0 ? myBuffer.size() : 0;
        theStream.write((const char *)&anEncodedSize, sizeof(anEncodedSize));
        theLength += sizeof(anEncodedSize);

        if (anEncodedSize > 0)
        {
            theStream.write(myBuffer.data(), myBuffer.size());
            theLength += anEncodedSize;
        }
#else
        (void)theStream;
        (void)theLength;
#endif
    }

    // ================================================================
    // Function : Decode
    // ================================================================
    bool MeshAttributeDecoder::Decode(const char *theData, size_t theSize, size_t &theOffset)
    {
#ifdef OCCTV_HAVE_DRACO
        if (theSize < sizeof(uint64_t))
        {
            return false;
        }
        const uint64_t anEncodedSize = *reinterpret_cast<const uint64_t *>(&theData[theOffset]);
        theOffset += sizeof(uint64_t);
        if (anEncodedSize > theSize - theOffset)
        {
            return false;
        }

        if (anEncodedSize == 0)
        {
            return true;
        }

        draco::DecoderBuffer aBuffer;
        aBuffer.Init(&theData[theOffset], anEncodedSize);
        theOffset += anEncodedSize;

        PointCloudSequentialDecoderExt aDecoder;
        draco::DecoderOptions anOptions;
        aDecoder.SetUseOriginRangeParams(myToUseOriginRange);
        if (!aDecoder.Decode(anOptions, &aBuffer, &myPC).ok())
        {
            return false;
        }

        return true;
#else
        (void)theData;
        (void)theSize;
        (void)theOffset;
        return false;
#endif
    }

    // ================================================================
    // Function : GetValue
    // ================================================================
    void MeshAttributeDecoder::GetValue(int theAttrIdx, int theIndex, void *theValue) const
    {
#ifdef OCCTV_HAVE_DRACO
        myPC.attribute(theAttrIdx)->GetValue(draco::AttributeValueIndex((uint32_t)theIndex), theValue);
#else
        (void)theAttrIdx;
        (void)theIndex;
        (void)theValue;
#endif
    }

    // ================================================================
    // Function : NbValues
    // ================================================================
    size_t MeshAttributeDecoder::NbValues() const
    {
#ifdef OCCTV_HAVE_DRACO
        return myPC.num_points();
#else
        return 0;
#endif
    }

}
