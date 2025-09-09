#ifndef _SELECTIONMESHENTITY_HEADERFILE
#define _SELECTIONMESHENTITY_HEADERFILE

#include <ViewCae/InteractiveCaeMeshOwner.h>
#include <ViewCae/MeshAnnotations.h>
#include <ViewCae/MeshPresentation.h>
#include <ViewCaeGeom/MeshGeometry.h>
#include "VecN.h"

#include <map>
#include <vector>

namespace Viewer {
//! Enum for mesh property usage type.
enum class MeshPropertyUsageType { COLOR_MAP, DEFORMATION, ISO_SURFACE, VECTORS };

//! Information about value of mesh property component.
struct MeshComponentValueInfo {
    double      value = RealLast();
    std::string displayUnits;
    std::string description;
};

//! Information about values of mesh property.
struct MeshPropertyValuesInfo {
    std::string                         description;
    std::vector<MeshComponentValueInfo> components;
};

//! Struct describes mesh property component.
struct MeshPropertyInfo {
    std::string propertyId;
    int         componentIndex = -1;
};

//! Information about selected mesh sub entity.
class SelectionMeshEntity {
public:
    int         entityId   = -1;
    int         entityType = int(ViewCae::MeshEntityType::UNKNOWN);
    int         nbNodes    = 0;
    Vec3        pos        = Vec3::InvalidPoint();
    bool        isElemSurf = false;
    std::string prsId;
    std::string geomId;
    std::string refMeshDataPath;

    // entity properties fields
    std::map<std::string, MeshPropertyValuesInfo> propsInfo;
    std::map<int, MeshPropertyInfo>               prsPropsInfo;

    // annotations fields
    int lineIndex = -1;

public:
    //! Empty constructor.
    SelectionMeshEntity() {}

    //! Constructor.
    Standard_EXPORT SelectionMeshEntity(const ViewCae::MeshSubEntity&                   theSubEntity,
                                        const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner);

    //! Constructor.
    Standard_EXPORT SelectionMeshEntity(const TCollection_AsciiString&  thePrsId,
                                        const ViewCae::MeshAnnotation& theAnnotation,
                                        int                             theLineIndex);

    //! Check if this entity is defined.
    bool IsDefined() const { return entityId != -1; }

    void UpdateSelectedEntity(const Handle(ViewCae::InteractiveCaeMeshOwner)&   theOwner,
                                           const Handle(AIS_InteractiveContext)& theCtx) const;

protected:
    Handle(ViewCaeGeom::MeshGeometry) getPrsGeom(const Handle(ViewCae::MeshPresentation)& thePrs) const;

    //! Fill with information from node sub entity.
    Standard_EXPORT void fillNodeInfo(const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner);

    //! Fill with information from element sub entity.
    Standard_EXPORT void fillElementInfo(const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner);

    //! Fill with information from element sub entity.
    Standard_EXPORT void fillElementInfo(const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner,
                                         const Handle(RWCae::MeshData)&                 theMeshData,
                                         const RWCae::MeshData::IndexedElement&         theElem);

    //! Fill with information from element sub entity.
    template<class Elem_T>
    Standard_EXPORT void fillElementTInfo(const RWCae::MeshAdaptor<Elem_T>&              theMesh,
                                          const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner,
                                          int                                              theElemIndex);

    //! Fill with information about properties used for current presentation.
    Standard_EXPORT void fillPrsInfo(const Handle(ViewCae::InteractiveCaeMeshOwner)& theOwner);
};

} // namespace

#endif // _SELECTIONMESHENTITY_HEADERFILE

