#ifndef _INTERACTIVECAEMESHMANAGER_HEADERFILE
#define _INTERACTIVECAEMESHMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/CaeMeshAnnotationInfo.h"
#include "Utils/CaeMeshInfo.h"
#include "Utils/CaeMeshPropInfo.h"
#include "Utils/CaeMeshStreamlinesInfo.h"
#include "Utils/ColorTools.h"
#include "Utils/ObjectId.h"
#include "Utils/VecN.h"

#include <ViewCae/MeshColorMapper.h>
#include <RWCae/MeshImportExport.h>

#include <AIS_DataMapOfIOStatus.hxx>
#include <AIS_InteractiveContext.hxx>

#include <map>
#include <vector>

namespace Viewer {
//! Class provides public interface for managing InteractiveCaeMesh.
class InteractiveCaeMeshManager : public IManager {
public:
    //! Load mesh properties from the file and append to the existing mesh.
    Standard_EXPORT static bool LoadMeshProperties(const Handle(AIS_InteractiveContext)& theCtx,
                                                   const ObjectId&                       theObjId,
                                                   const std::string&                    theFilePath,
                                                   const int                             theOptions = 0);

    //! Open mesh model from specified file.
    Standard_EXPORT static bool OpenMesh(const Handle(AIS_InteractiveContext)& theCtx,
                                         const ObjectId&                       theObjId,
                                         const std::string&                    theFilePath,
                                         const std::string&                    theMappingFilePath,
                                         const std::string&                    theArtifactFilePath,
                                         const int theOptions = (int)RWCae::MeshDataImportExportOptions::DEFAULTS);

public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveCaeMeshManager(WasmViewer* theViewer);

    //! Probe format from file header.
    Standard_EXPORT std::string probeFileFormat(const std::string& theName,
                                                uintptr_t          theBuffer,
                                                int                theDataLen,
                                                bool               theToFree);

    //! Open mesh model from specified file.
    Standard_EXPORT bool openModelMesh(const ObjectId& theObjId, const std::string& theFilePath, bool theToCacheProps);

    //! Dump mesh into a file.
    Standard_EXPORT bool saveModelMesh(const ObjectId& theObjId, const std::string& theFile) const;

    //! Load mesh properties from the file and append to the existing mesh.
    Standard_EXPORT bool loadMeshProperties(const ObjectId& theObjId, const std::string& theFilePath);

    //! Remove all mesh properties (or clear their values data).
    Standard_EXPORT void removeMeshProperties(const ObjectId& theObjId, bool theWithMetadata);

public:
    //! Create new mesh presentation of the specified type and return its id.
    Standard_EXPORT std::string createPresentation(const ObjectId& theMeshId, int theType, const std::string& thePrsId);

    //! Remove mesh presentation by its id.
    Standard_EXPORT void removePresentation(const ObjectId& theMeshId, const std::string& thePrsId);

    //! Return all mesh presentations.
    Standard_EXPORT std::vector<std::string> getPresentations(const ObjectId& theMeshId);

    //! Return type of mesh presentation by its id.
    Standard_EXPORT int getMeshPresentationType(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Return mesh presentation mode, @sa ViewBase::InteractiveCaeMeshPrsMode.
    Standard_EXPORT int getMeshPresentationMode(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set mesh presentation mode, @sa ViewBase::InteractiveCaeMeshPrsMode.
    Standard_EXPORT void setMeshPresentationMode(const ObjectId& theObjId, const std::string& thePrsId, int theMode);

    //! Return mesh properties info.
    Standard_EXPORT std::vector<CaeMeshPropInfo> getMeshProperties(const ObjectId& theObjId) const;

    //! Return mesh property info.
    Standard_EXPORT CaeMeshPropInfo getMeshProperty(const ObjectId& theObjId, const std::string& thePropertyId) const;

    //! Return mesh property component info.
    Standard_EXPORT CaeMeshPropCompInfo getMeshPropertyComponent(const ObjectId&    theObjId,
                                                                 const std::string& thePropertyId,
                                                                 int                theComponentIndex) const;

    //! Returns active mesh property id.
    Standard_EXPORT std::string getActiveMeshProperty(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set active mesh property by its id.
    Standard_EXPORT void setActiveMeshProperty(const ObjectId&    theObjId,
                                               const std::string& thePrsId,
                                               const std::string& thePropertyId);

    //! Return index of active mesh property component.
    Standard_EXPORT int getActiveMeshComponentIndex(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set index of active mesh property component.
    Standard_EXPORT void setActiveMeshComponentIndex(const ObjectId&    theObjId,
                                                     const std::string& thePrsId,
                                                     int                theIndex);

    //! Return mesh element types to display.
    Standard_EXPORT int getMeshElemTypes(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set mesh element types to display.
    Standard_EXPORT void setMeshElemTypes(const ObjectId& theObjId, const std::string& thePrsId, int theElemTypes);

    //! Return mesh bounding box.
    Standard_EXPORT BndBox3 getMeshBox(const ObjectId& theObjId) const;

    //! Find property id by name.
    Standard_EXPORT std::string getMeshPropertyIdByName(const ObjectId&    theObjId,
                                                        const std::string& thePropertyName) const;

    //! Return available vector mesh properties.
    Standard_EXPORT std::vector<std::string> getAvailableMeshVec3Props(const ObjectId& theObjId) const;

    //! Return id of current active deformed mesh property.
    Standard_EXPORT std::string getMeshDeformedProp(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set current active deformed mesh property.
    Standard_EXPORT void setMeshDeformedProp(const ObjectId&    theObjId,
                                             const std::string& thePrsId,
                                             const std::string& thePropertyId);

    //! Return scale coefficient for mesh deformed presentation.
    Standard_EXPORT double getMeshDeformedScaleCoeff(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set scale coefficient for mesh deformed presentation.
    Standard_EXPORT void setMeshDeformedScaleCoeff(const ObjectId&    theObjId,
                                                   const std::string& thePrsId,
                                                   double             theDefScaleCoeff);

    //! Return id of current active vector mesh property.
    Standard_EXPORT std::string getMeshVecProp(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set current active vector mesh property.
    Standard_EXPORT void setMeshVecProp(const ObjectId&    theObjId,
                                        const std::string& thePrsId,
                                        const std::string& thePropertyId);

    //! Return scale coefficient for mesh vector presentation.
    Standard_EXPORT double getMeshVecPropScaleCoeff(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set scale coefficient for mesh vector presentation.
    Standard_EXPORT void setMeshVecPropScaleCoeff(const ObjectId&    theObjId,
                                                  const std::string& thePrsId,
                                                  double             theScaleCoeff);

    //! Return distribution type for mesh vector presentation.
    Standard_EXPORT int getMeshVecPropDistrType(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set distribution type for mesh vector presentation.
    Standard_EXPORT void setMeshVecPropDistrType(const ObjectId& theObjId, const std::string& thePrsId, int theType);

    //! Return percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0.
    Standard_EXPORT double getMeshVecSamplingRatio(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0.
    Standard_EXPORT void setMeshVecSamplingRatio(const ObjectId&    theObjId,
                                                 const std::string& thePrsId,
                                                 double             theRatio);

    //! Return map of mesh groups with non-zero number of entities.
    Standard_EXPORT std::map<int, std::string> getMeshUsedGroups(const ObjectId&    theObjId,
                                                                 const std::string& thePropertyId) const;

    //! Return true if it is needed to render/select 3d mesh only by its surface
    //! and to not include internal parts.
    Standard_EXPORT bool isMeshVolumeAsSurface(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set if it is needed to render/select 3d mesh only by its surface
    //! and to not include internal parts.
    Standard_EXPORT void setMeshVolumeAsSurface(const ObjectId&    theObjId,
                                                const std::string& thePrsId,
                                                bool               theIsVolumeAsSurface);

    //! Return visibility of specified mesh presentation.
    Standard_EXPORT bool isMeshPrsVisible(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set visibility of specified mesh presentation.
    Standard_EXPORT void setMeshPrsVisible(const ObjectId& theObjId, const std::string& thePrsId, bool theIsVisible);

    //! Return mesh shading color.
    Standard_EXPORT ColorTools::ColorString getMeshShadingColor(const ObjectId&    theObjId,
                                                                const std::string& thePrsId, int theHiType) const;

    //! Set mesh shading color.
    Standard_EXPORT void setMeshShadingColor(const ObjectId&                theObjId,
                                             const std::string&             thePrsId,
                                             const ColorTools::ColorString& theColor, int theHiType);

    //! Return mesh edges color.
    Standard_EXPORT ColorTools::ColorString getMeshEdgesColor(const ObjectId&    theObjId,
                                                              const std::string& thePrsId, int theHiType) const;

    //! Set mesh edges color.
    Standard_EXPORT void setMeshEdgesColor(const ObjectId&                theObjId,
                                           const std::string&             thePrsId,
                                           const ColorTools::ColorString& theColor, int theHiType);

    //! Return mesh points color.
    Standard_EXPORT ColorTools::ColorString getMeshPointsColor(const ObjectId&    theObjId,
                                                               const std::string& thePrsId, int theHiType) const;

    //! Set mesh points color.
    Standard_EXPORT void setMeshPointsColor(const ObjectId&                theObjId,
                                            const std::string&             thePrsId,
                                            const ColorTools::ColorString& theColor, int theHiType);

    //! Return mesh edges width.
    Standard_EXPORT float getMeshEdgesWidth(const ObjectId& theObjId, const std::string& thePrsId, int theHiType) const;

    //! Set mesh edges width.
    Standard_EXPORT void setMeshEdgesWidth(const ObjectId& theObjId, const std::string& thePrsId, float theWidth, int theHiType);

    //! Return mesh points size.
    Standard_EXPORT float getMeshPointsSize(const ObjectId& theObjId, const std::string& thePrsId, int theHiType) const;

    //! Set mesh points size.
    Standard_EXPORT void setMeshPointsSize(const ObjectId& theObjId, const std::string& thePrsId, float theSize, int theHiType);

    //! Return mesh shading transparency.
    Standard_EXPORT float getMeshShadingTransparency(const ObjectId& theObjId, const std::string& thePrsId, int theHiType) const;

    //! Set mesh shading transparency.
    Standard_EXPORT void setMeshShadingTransparency(const ObjectId&    theObjId,
                                                    const std::string& thePrsId,
                                                    float              theValue,
                                                    int                theHiType);

    //! Return true if clip plane for mesh has been set.
    Standard_EXPORT bool hasClipPlane(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Return equation of clip plane used to filter mesh elements.
    Standard_EXPORT Vec4 getClipPlane(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set clip plane used to filter mesh elements.
    Standard_EXPORT void setClipPlane(const ObjectId& theObjId, const std::string& thePrsId, const Vec4& theEq);

    //! Unset a current active mesh clip plane.
    Standard_EXPORT void unsetClipPlane(const ObjectId& theObjId, const std::string& thePrsId);

    //! Return array of slice planes used to cut mesh elements.
    Standard_EXPORT std::vector<Vec4> getSlicePlanes(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set array of slice planes used to cut mesh elements.
    Standard_EXPORT void setSlicePlanes(const ObjectId&          theObjId,
                                        const std::string&       thePrsId,
                                        const std::vector<Vec4>& theEqs);

    //! Return id of current active iso-surface mesh property.
    Standard_EXPORT std::string getIsoPropId(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Return index of current active iso-surface mesh property component.
    Standard_EXPORT int getIsoPropComp(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set current active iso-surface mesh property.
    Standard_EXPORT void setIsoProp(const ObjectId&    theObjId,
                                    const std::string& thePrsId,
                                    const std::string& thePropertyId,
                                    int                theCompIndex);

    //! Return array of mesh iso values.
    Standard_EXPORT std::vector<double> getIsoValues(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set array of mesh iso values.
    Standard_EXPORT void setIsoValues(const ObjectId&            theObjId,
                                      const std::string&         thePrsId,
                                      const std::vector<double>& theValues);

    //! Return visibility of annotations for min/max values of the current active mesh property.
    Standard_EXPORT bool toShowPrsMinMax(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set visibility of annotations for min/max values of the current active mesh property.
    Standard_EXPORT void setShowPrsMinMax(const ObjectId& theObjId, const std::string& thePrsId, bool theToShow);

    //! Return visibility of annotations for the current active group mesh property.
    Standard_EXPORT bool toShowGroupsInfo(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set visibility of annotations for the current active group mesh property.
    Standard_EXPORT void setShowGroupsInfo(const ObjectId& theObjId, const std::string& thePrsId, bool theToShow);

    //! Return annotation for specified mesh group.
    Standard_EXPORT CaeMeshAnnotationInfo getGroupInfo(const ObjectId&    theObjId,
                                                       const std::string& thePrsId,
                                                       int                theGroupId) const;

    //! Set annotation for specified mesh group.
    Standard_EXPORT void setGroupInfo(const ObjectId&              theObjId,
                                      const std::string&           thePrsId,
                                      int                          theGroupId,
                                      const CaeMeshAnnotationInfo& theInfo);

    //! Return mesh info.
    Standard_EXPORT CaeMeshInfo getMeshInfo(const ObjectId& theObjId);

    //! Return mesh blocks as a Json string.
    Standard_EXPORT std::string getMeshBlocks(const ObjectId& theObjId) const;

    //! Return visible mesh blocks as a Json string.
    Standard_EXPORT std::string getVisibleMeshBlocks(const ObjectId& theObjId, const std::string& thePrsId) const;

    //! Set mesh blocks visibility.
    Standard_EXPORT void setVisibleMeshBlocks(const ObjectId&    theObjId,
                                              const std::string& thePrsId,
                                              const std::string& theBlocks,
                                              bool               theVisible);

    //! Return mesh active time index.
    Standard_EXPORT int getMeshActiveTimeStep(const ObjectId& theObjId) const;

    //! Set mesh active time index.
    Standard_EXPORT void setMeshActiveTimeStep(const ObjectId& theObjId, int theTimeStep);

    //! Return available mesh time indices.
    Standard_EXPORT std::vector<Vec2> getAvailableMeshTimeSteps(const ObjectId& theObjId) const;

    //! Set available mesh time indices.
    Standard_EXPORT void setAvailableMeshTimeSteps(const ObjectId& theObjId, const std::vector<Vec2>& theTimeSteps);

    //! Start animation of mesh properties.
    Standard_EXPORT void startMeshAnimation(const ObjectId& theObjId, bool theToRestart);

    //! Stop animation of mesh properties.
    Standard_EXPORT void stopMeshAnimation(const ObjectId& theObjId);

    //! Continue animation of mesh properties.
    Standard_EXPORT void continueMeshAnimation(const ObjectId& theObjId);

    //! Check if mesh properties animation is running.
    Standard_EXPORT bool isMeshAnimationActive(const ObjectId& theObjId) const;

    //! Return duration of mesh properties animation, in seconds.
    Standard_EXPORT double getMeshAnimationDuration(const ObjectId& theObjId) const;

    //! Set duration of mesh properties animation, in seconds.
    Standard_EXPORT void setMeshAnimationDuration(const ObjectId& theObjId, double theDur);

    //! Check if mesh properties animation should be restarted on the last update.
    Standard_EXPORT bool getMeshAnimationLoopPlayback(const ObjectId& theObjId) const;

    //! Define if mesh properties animation should be restarted on the last update.
    Standard_EXPORT void setMeshAnimationLoopPlayback(const ObjectId& theObjId, bool theIsLoopPlayback);

    //! Check if mesh properties animation steps duration should take into account timestamps values.
    Standard_EXPORT bool getMeshAnimationUniformSteps(const ObjectId& theObjId) const;

    //! Define if mesh properties animation steps duration should take into account timestamps values.
    Standard_EXPORT void setMeshAnimationUniformSteps(const ObjectId& theObjId, bool theIsUniform);

    //! Return mesh streamlines presentation parameters.
    Standard_EXPORT CaeMeshStreamlinesInfo getMeshStreamlinesInfo(const ObjectId&    theObjId,
                                                                  const std::string& thePrsId) const;

    //! Set mesh streamlines presentation parameters.
    Standard_EXPORT void setMeshStreamlinesInfo(const ObjectId&               theObjId,
                                                const std::string&            thePrsId,
                                                const CaeMeshStreamlinesInfo& theInfo);

    //! Set to use custom geometry for mesh presentations.
    Standard_EXPORT void setUseMeshGenGeometry(const ObjectId& theObjId);

    //! Set to use test custom geometry for mesh presentations.
    Standard_EXPORT void setUseMeshTestCacheGenGeometry(const ObjectId& theObjId);

    //! Set custom mesh geometry for the specified presentation.
    Standard_EXPORT bool setMeshGenGeometry(const ObjectId&    theObjId,
                                            const std::string& thePrsId,
                                            int                thePartId,
                                            const std::string& theFilePath);

protected:
    //! Called when color mapper is created or its attached property is changed.
    Standard_EXPORT static void OnColorMapperChanged(const TCollection_AsciiString&           theObjId,
                                                     const RWCae::MeshPropertyComponent&    theProp,
                                                     const Handle(ViewCae::MeshColorMapper)& theMapper);
};

} // namespace

#endif // _INTERACTIVECAEMESHMANAGER_HEADERFILE