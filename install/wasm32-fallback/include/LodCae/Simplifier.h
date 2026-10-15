#ifndef SIMPLIFIER_HEADERFILE
#define SIMPLIFIER_HEADERFILE

#include <RWCae/MeshData.h>

#include <SimMesh_Model.h>

//! @namespace LodCae
//! `LodCae` is a C++ library implementing LODs generation for CAE mesh data.
namespace LodCae {

//! The struct defining simplification parameters.
struct SimplParameters {
    //! Target ratio. It is the expected ratio of the number of mesh elements in
    //! simplified mesh to the number of elements in initial mesh.
    //! Zero value disables this stop condition.
    double TargetRatio = 0.5;

    //! Max allowed error. The algorithm stops if further
    //! simplification will lead to exceeding this error.
    //! Zero value disables this stop condition.
    double AllowedError = 0.;

    //! Aggregation mode. If true distinct mesh regions are permitted to join together.
    bool Aggregation = false;

    //! In aggregation mode it allows inversion of triangles.
    //! This relaxes simplification conditions and helps the algorithm
    //! converge quicker. It is ignored in non-aggregation mode.
    bool AllowInversion = true;
};

//! Element type chosen for simplification.
enum class SourceElemType {
    PreferSurface, //!< First non-empty container will be used, preferring surface elements.
    Elem2d,        //!< Independent 2D mesh element.
    ElemSurface    //!< Outer surface mesh element.
};

//! This class gets on input a mesh data and generates new mesh data with reduced
//! number of mesh elements.
class Simplifier {
public:
    //! Constructor.
    Simplifier() = default;

    //! Adds mesh data for simplification.
    Standard_EXPORT void AddMesh(const Handle(RWCae::MeshData)& theMesh);

    //! Sets the source of mesh elements for simplification.
    void SetSourceElemType(const SourceElemType theElemType) { m_elemType = theElemType; }

    //! Sets ID of the new mesh data.
    void SetId(const TCollection_AsciiString& theId) { m_id = theId; }

    //! Performs simplification with the given set of parameters.
    //! @return true in case of success, false otherwise.
    Standard_EXPORT bool Perform(const SimplParameters& theParam);

    //! Returns maximal error of simplification.
    double GetMaxError() const { return m_maxError; }

    //! Returns the number of vertex pair contractions led to triangles inversions.
    int NbInversions() const { return m_nbInversions; }

    //! Returns the result mesh data.
    const Handle(RWCae::MeshData)& GetResult() const { return m_dstMesh; }

public:
    struct ModelData {
        Handle(SimMesh_Model) Model;
        double                MaxError     = 0.;
        int                   NbInversions = 0;
    };

    struct DomainData {
        RWCae::MeshDataBlock* Block = nullptr;
        TCollection_AsciiString Path;
    };

protected:
    //! Determines which elements to get from the mesh, 2D elements or
    //! surface elements of 3D elements.
    bool isToUseSurface() const;

    //! Prepare simplification models, one for each domain.
    Standard_EXPORT bool prepareModels(const bool theIsSurface);

    //! Creates SimMesh_Model from source mesh data.
    Standard_EXPORT Handle(SimMesh_Model) createModel(RWCae::MeshDataBlock* theDomain, const bool theIsSurface);

    //! Simplifies the given model using the given parameters.
    Standard_EXPORT void simplifyModels(const SimplParameters& theParam);

    //! Generates target mesh data from the given SimMesh_Model.
    Standard_EXPORT bool generateMeshData(const bool theIsSurface);

private:
    Handle(RWCae::MeshData) m_srcMesh;
    Handle(RWCae::MeshData) m_dstMesh;

    TCollection_AsciiString m_id;
    SourceElemType          m_elemType = SourceElemType::PreferSurface;

    std::vector<DomainData> m_leafDomains;
    std::vector<ModelData>  m_models;

    double m_maxError     = 0.;
    int    m_nbInversions = 0;
};

} // namespace

#endif
