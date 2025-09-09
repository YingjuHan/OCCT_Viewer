#ifndef MESHFIELDMAPPING_HEADERFILE
#define MESHFIELDMAPPING_HEADERFILE

#include <RWBase/JsonHelper.h>
#include <RWBase/ValueConverter.h>

#include <RWCae/MeshBinIOBase.h>
#include <RWCae/MeshFieldMappingInfo.h>
#include <RWCae/MeshGroupProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <RWCae/MeshResultType.h>
#include <RWCae/MeshScalarProperty.h>

#include <Standard_WarningsDisable.hxx> // NOLINT
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <Standard_WarningsRestore.hxx> // NOLINT

#include <memory>

namespace RWCae {

//! Reader for mesh property mapping file.
class MeshFieldMappingReader {
public:
    //! Constructor.
    Standard_EXPORT MeshFieldMappingReader();

    //! Perform reading from the file.
    Standard_EXPORT bool Perform(const TCollection_AsciiString& theFileName);

    //! Perform reading from the stream.
    Standard_EXPORT bool Perform(std::istream& theStream, const TCollection_AsciiString& theFileName);

    //! Return mapping information.
    const std::vector<MeshFieldMappingInfo>& GetFieldMappingInfo() const { return myMappingInfo; }

    //! Return analysis type string.
    const std::string GetAnalysisType() const { return myAnalysisType; }

    //! Return result types.
    const std::vector<MeshResultType>& GetResultTypes() const { return myResultTypes; }

protected:
    //! Read mapping file version.
    Standard_EXPORT bool readMappingVersion(int& theVersionMajor, int& theVersionMinor);

    //! Read version 0.0 of mapping file.
    Standard_EXPORT bool readMappingResults0(const rapidjson::Document::ValueType& theValue);

    //! Read version 1.0 of mapping file.
    Standard_EXPORT bool readMappingResults10(const rapidjson::Document::ValueType& theValue);

    //! Read data specific for version 1.1 of mapping file.
    Standard_EXPORT bool readMappingResults11(const rapidjson::Document::ValueType& theValue);

    //! Read data specific for version 1.2 of mapping file.
    Standard_EXPORT bool readMappingResults12(const rapidjson::Document::ValueType& theValue);

    //! Read data specific for version 1.3 of mapping file.
    Standard_EXPORT bool readMappingResults13(const rapidjson::Document::ValueType& theValue);

    //! Read data specific for version 1.4 of mapping file.
    Standard_EXPORT bool readMappingResults14(const rapidjson::Document::ValueType& theValue);

protected:
    std::shared_ptr<rapidjson::Document> myReader;
    std::vector<MeshFieldMappingInfo>    myMappingInfo;
    std::string                          myAnalysisType;
    std::vector<MeshResultType>          myResultTypes;
};

//! Writer for mesh property mapping file.
class MeshFieldMappingWriter {
public:
    //! Constructor.
    Standard_EXPORT explicit MeshFieldMappingWriter(const std::vector<MeshFieldMappingInfo>& theMappingInfo);

    //! Perform exporting to the file.
    Standard_EXPORT bool Perform(const TCollection_AsciiString& theFileName);

    //! Perform exporting to the stream.
    Standard_EXPORT bool Perform(std::ostream& theStream, const TCollection_AsciiString& theFileName);

protected:
    std::vector<MeshFieldMappingInfo>                             myMappingInfo;
    std::shared_ptr<rapidjson::Writer<rapidjson::OStreamWrapper>> myWriter;
};

using MeshMappingFieldType = RWCae::MeshMappingFieldType;

//! Bidirectional mapper between MeshMappingFieldType enumeration items and strings.
class MeshMappingFieldTypeConvertor
    : public RWBase::ValueConverter<MeshMappingFieldType, MeshMappingFieldTypeConvertor> {
private:
    // Default constructor
    MeshMappingFieldTypeConvertor()
    {
        valueMap().Bind(MeshMappingFieldType::Set, "SET");
        valueMap().Bind(MeshMappingFieldType::Scalar, "SCALAR");
        valueMap().Bind(MeshMappingFieldType::Vector, "VECTOR");
        valueMap().Bind(MeshMappingFieldType::Vector4, "VECTOR4");
        valueMap().Bind(MeshMappingFieldType::Tensor6, "TENSOR6");
        valueMap().Bind(MeshMappingFieldType::Tensor9, "TENSOR9");
        valueMap().Bind(MeshMappingFieldType::Unknown, "");
    }

    friend class RWBase::ValueConverter<MeshMappingFieldType, MeshMappingFieldTypeConvertor>;
};

using MeshMappingCalcPoint = RWCae::MeshMappingCalcPoint;

//! Bidirectional mapper between MeshMappingCalcPoint enumeration items and strings.
class MeshMappingCalcPointConvertor
    : public RWBase::ValueConverter<MeshMappingCalcPoint, MeshMappingCalcPointConvertor> {
private:
    // Default constructor
    MeshMappingCalcPointConvertor()
    {
        valueMap().Bind(MeshMappingCalcPoint::Nodes, "Nodes");
        valueMap().Bind(MeshMappingCalcPoint::ElementCentroids, "Element centroids");
        valueMap().Bind(MeshMappingCalcPoint::ElementNodes, "Element nodes");
        valueMap().Bind(MeshMappingCalcPoint::GaussPoints, "Gauss points");
        valueMap().Bind(MeshMappingCalcPoint::Unknown, "");
    }

    friend class RWBase::ValueConverter<MeshMappingCalcPoint, MeshMappingCalcPointConvertor>;
};

} // namespace

#endif // MESHFIELDMAPPING_HEADERFILE
