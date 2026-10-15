
#ifndef ViewCAE_MESHPRESENTATIONTOOLS_HEADERFILE
#define ViewCAE_MESHPRESENTATIONTOOLS_HEADERFILE

#include <ViewCae/MeshPresentation.h>

namespace ViewCae {
//! Auxiliary methods for mesh presentation.
class MeshPresentationTools {
public:
    //! Dump presentation part to Json string.
    Standard_EXPORT static bool DumpToJson(const Handle(MeshPresentation)& thePrs,
                                           MeshPartType                    thePart,
                                           std::string&                    theJsonStr);

    //! Restore presentation part from Json string.
    Standard_EXPORT static bool RestoreFromJson(const std::string&                       theJsonStr,
                                                const Handle(RWCae::MeshData)&         theMeshData,
                                                const Handle(ViewCaeGeom::MeshBvhData)& theBvhData,
                                                ListOfMeshPresentations&                 thePrsList,
                                                ViewCaeGeom::MapOfMeshGeometries&       theGeoms,
                                                Handle(MeshPresentationBase)&            thePrs,
                                                int&                                     thePart,
                                                bool&                                    theIsChanged);

    //! Collect mesh properties that are used in any presentation.
    Standard_EXPORT static void GatherUsedProps(const ListOfMeshPresentations&                      thePrsList,
                                                const ViewCaeGeom::MapOfMeshGeometries&            theGeoms,
                                                NCollection_Map<Handle(RWCae::MeshNVecProperty)>& theProps);
};

} // namespace

#endif // ViewCAE_MESHPRESENTATIONTOOLS_HEADERFILE