#ifndef _INTERACTIVESHAPEMANAGER_HEADERFILE
#define _INTERACTIVESHAPEMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/ColorTools.h"
#include "Utils/ObjectId.h"
#include "Utils/ShapeInfo.h"
#include "Utils/SelectionShapeElems.h"
#include "Utils/VecN.h"

#include <AIS_DataMapOfIOStatus.hxx>
#include <XCAFDoc_VisMaterialPBR.hxx>

namespace Viewer {

//! Structure defining visual material (PBR) properties.
struct VisMaterial {
    std::string             name;
    ColorTools::ColorString baseColor;
    Vec3                    emissiveFactor;
    float                   metallic        = 0.0f;
    float                   roughness       = 0.0f;
    float                   refractionIndex = 0.0f;
    bool                    isDefined       = false;

    //! Copy material properties.
    void Init(const XCAFDoc_VisMaterialPBR& theMat)
    {
        baseColor       = Quantity_ColorRGBA::ColorToHex(theMat.BaseColor).ToCString();
        metallic        = theMat.Metallic;
        emissiveFactor  = Vec3(Graphic3d_Vec3d(theMat.EmissiveFactor));
        refractionIndex = theMat.RefractionIndex;
        roughness       = theMat.Roughness;
        isDefined       = theMat.IsDefined;
    }

    //! Copy material properties.
    void Init(const Graphic3d_PBRMaterial& theMat)
    {
        baseColor       = Quantity_ColorRGBA::ColorToHex(theMat.Color()).ToCString();
        metallic        = theMat.Metallic();
        emissiveFactor  = Vec3(Graphic3d_Vec3d(theMat.Emission()));
        refractionIndex = theMat.IOR();
        roughness       = theMat.Roughness();
        isDefined       = true;
    }
};

//! Class provides public interface for managing Interactive shapes.
class InteractiveShapeManager : public IManager {
public:
    //! Open shape model from specified file.
    Standard_EXPORT static bool OpenShape(const Handle(AIS_InteractiveContext)& theCtx,
                                          const ObjectId&                       theObjId,
                                          const std::string&                    theFilePath);

    //! Explode shape with specified Id.
    Standard_EXPORT static bool ExplodeShape(const Handle(AIS_InteractiveContext)& theCtx,
                                             const ObjectId&                       theObjId,
                                             const int                             theExplodeDepth,
                                             const bool                            theIsAutoNaming,
                                             const bool                            theToGenId,
                                             const bool                            theToPropagateId,
                                             std::string&                          theResObjList);

public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveShapeManager(WasmViewer* theViewer);

    //! Detect supported format from file header.
    Standard_EXPORT std::string probeFileFormat(const std::string& theName,
                                                uintptr_t          theBuffer,
                                                int                theDataLen,
                                                bool               theToFree);

    //! Open shape model from specified file.
    Standard_EXPORT bool openModelShape(const ObjectId& theObjId, const std::string& theFilePath);

    //! Dump shape into a file.
    Standard_EXPORT bool saveModelShape(const ObjectId&                         theObjId,
                                        const Viewer::SelectionShapeElems& theElems,
                                        const std::string&                      theFile) const;

    //! Clone object with specified subshapes.
    Standard_EXPORT ObjectId cloneSubshapes(const ObjectId&                         theObjIdOld,
                                            const Viewer::SelectionShapeElems& theElems,
                                            const ObjectId&                         theObjIdNew) const;

    //! Explode shape with specified Id.
    //! @param[in] theObjId         object id to be exploded
    //! @param[in] theExplodeDepth  depth of explode process (should be >= -1,
    //!                             default value is -1 - explode to maximal possible depth)
    //! @param[in] theIsAutoNaming  generate id by shape type ("compound", "solid", etc)
    //!                             depending on parents in case of attributes absence
    //! @param[in] theToGenId       generate unique ids or use initial ones in case of existing attributes
    //! @param[in] theToPropagateId generate id depending on parents in case of existing attributes
    //! @param[in] theNewShapeIds   suggested ids for new objects or empty string to generate them
    //! @return ids of result interactive objects
    Standard_EXPORT std::string explodeShape(const ObjectId&    theObjId,
                                             const int          theExplodeDepth  = -1,
                                             const bool         theIsAutoNaming  = true,
                                             const bool         theToGenId       = true,
                                             const bool         theToPropagateId = true,
                                             const std::string& theNewShapeIds   = std::string());

public:
    //! Construct vertex shape.
    Standard_EXPORT void buildPoint(const ObjectId& theObjId, const Vec3& thePnt);

    //! Construct line segment shape.
    Standard_EXPORT void buildLineSegment(const ObjectId& theObjId, const Vec3& thePnt1, const Vec3& thePnt2);

    //! Construct box shape.
    Standard_EXPORT void buildBox(const ObjectId& theObjId, const Ax2& theAx2, const Vec3& theDims);

    //! Construct sphere shape.
    Standard_EXPORT void buildSphere(const ObjectId& theObjId,
                                     const Ax2&      theAx2,
                                     const double    theRadius,
                                     const double    theDevAngle);

    //! Construct cylinder shape.
    Standard_EXPORT void buildCylinder(const ObjectId&       theObjId,
                                       const CylinderParams& theParams,
                                       const double          theDevAngle);

    //! Construct cone shape.
    Standard_EXPORT void buildCone(const ObjectId& theObjId, const ConeParams& theParams, const double theDevAngle);

public:
    //! Get material preset.
    Standard_EXPORT VisMaterial GetMaterialPreset(const std::string& thePreset);

    //! Get default shape material.
    Standard_EXPORT VisMaterial GetDefaultMaterial();

    //! Set default shape material.
    Standard_EXPORT void SetDefaultMaterial(const VisMaterial& theMat);

    //! Get object's material.
    Standard_EXPORT VisMaterial GetMaterial(const ObjectId& theObjId);

    //! Set object's material.
    Standard_EXPORT void SetMaterial(const ObjectId& theObjId, const VisMaterial& theMat);

    //! Return object's overcolor.
    Standard_EXPORT ColorTools::ColorString GetForcedColor(const ObjectId& theObjId) const;

    //! Set object's overcolor.
    Standard_EXPORT void SetForcedColor(const ObjectId& theObjId, const ColorTools::ColorString& theHexColor);

    //! Return object's overtransparency.
    Standard_EXPORT float GetForcedTransparency(const ObjectId& theObjId) const;

    //! Set object's overtransparency.
    Standard_EXPORT void SetForcedTransparency(const ObjectId& theObjId, const float theTransp);

    //! Return object's presentation mode, @sa ViewBase::InteractiveShapePrsMode.
    Standard_EXPORT int GetPresentationMode(const ObjectId& theObjId) const;

    //! Set object's presentation mode, @sa ViewBase::InteractiveShapePrsMode.
    Standard_EXPORT void SetPresentationMode(const ObjectId& theObjId, int theMode);

    //! Return vertex position.
    Standard_EXPORT Viewer::Vec3 GetSubshapeVertex(const ObjectId& theObjId, int theVertId) const;

    //! Return subshape information.
    Standard_EXPORT Viewer::ShapeInfo GetSubshapeInfo(const ObjectId& theObjId,
                                                           const int       theElemType,
                                                           const int       theIndex) const;

    //! Return subshapes information.
    Standard_EXPORT std::vector<Viewer::ShapeInfo> GetSubshapesInfo(const ObjectId&    theObjId,
                                                                         const int          theElemType,
                                                                         const std::string& theIndices) const;

    //! Return subshape color or empty string if not assigned.
    Standard_EXPORT ColorTools::ColorString GetSubshapeColor(const ObjectId& theObjId,
                                                             const int       theElemType,
                                                             const int       theIndex) const;

    //! Set or unset subshape color.
    //! '*' passed as indices will apply changes to all subshapes of specified type.
    Standard_EXPORT void SetSubshapeColor(const ObjectId&                theObjId,
                                          const int                      theElemType,
                                          const std::string&             theIndices,
                                          const ColorTools::ColorString& theHexColor);

    //! Return subshape hidden flag.
    Standard_EXPORT bool IsSubshapeHidden(const ObjectId& theObjId, const int theElemType, const int theIndex) const;

    //! Set or unset subshape hidden flag.
    //! '*' passed as indices will apply changes to all subshapes of specified type.
    Standard_EXPORT void SetSubshapeHidden(const ObjectId&    theObjId,
                                           const int          theElemType,
                                           const std::string& theIndices,
                                           const bool         theIsHidden,
                                           const bool         theToInvertOthers);

    //! Collect subshapes connected to specified subshapes of the same type.
    //! This function is for testing purposes only.
    Standard_EXPORT std::string GetConnectedSubshapes(const Viewer::ObjectId& theObjId,
                                                      const int                    theElemType,
                                                      const std::string&           theIndices) const;

    //! Return subshape dump.
    Standard_EXPORT std::string GetSubshapeDump(const ObjectId& theObjId,
                                                const int       theElemType,
                                                const int       theIndex) const;

    //! Measure minimal distance between two shapes / subshapes (identified by selection mode + entity index).
    //! @param[in] theObjId1 first shape object id
    //! @param[in] theSelMode1 ViewShape::InteractiveShapeSelMode value (0 for whole shape)
    //! @param[in] theEntityId1 subshape index (ignored for whole shape)
    //! @param[in] theObjId2 second shape object id
    //! @param[in] theSelMode2 ViewShape::InteractiveShapeSelMode value (0 for whole shape)
    //! @param[in] theEntityId2 subshape index (ignored for whole shape)
    //! @return JSON string: {"distance":number,"pnt1":{"x":..,"y":..,"z":..},"pnt2":{"x":..,"y":..,"z":..}}
    Standard_EXPORT std::string MeasureDistanceJson(const ObjectId& theObjId1,
                                                    const int       theSelMode1,
                                                    const int       theEntityId1,
                                                    const ObjectId& theObjId2,
                                                    const int       theSelMode2,
                                                    const int       theEntityId2) const;

    //! Compute circle parameters for specified subshape (currently supports circular Edge).
    //! @return JSON string: {"ax2":{"location":{...},"direction":{...},"xDirection":{...}},"radius":number,"anchor":{...}}
    Standard_EXPORT std::string GetCircleParamsJson(const ObjectId& theObjId,
                                                    const int       theSelMode,
                                                    const int       theEntityId) const;

    //! Compute mass properties for specified shape object.
    //! @return JSON string: {"area":number,"volume":number|null,"centroid":{"x":..,"y":..,"z":..},
    //!                       "centroidSource":"volume|surface","volumeValid":bool}
    Standard_EXPORT std::string GetMassPropsJson(const ObjectId& theObjId) const;

    //! Check interference between two shape objects (AABB coarse test + exact distance).
    //! @return JSON string: {"objId1":"..","objId2":"..","bboxHit":bool,"exactDone":bool,
    //!                       "distance":number|null,"interferes":bool}
    Standard_EXPORT std::string CheckInterferenceJson(const ObjectId& theObjId1,
                                                      const ObjectId& theObjId2,
                                                      const double    theTolerance) const;

    //! Remove all selection group.
    Standard_EXPORT void ClearSelectionGroups(const ObjectId& theObjId);

    //! Add selection group.
    Standard_EXPORT int AddSelectionGroup(const ObjectId&    theObjId,
                                          const int          theElemType,
                                          const std::string& theIndices);

private:
    //! Display shape data.
    Standard_EXPORT void displayShape(const ObjectId& theObjId, const TopoDS_Shape& theShape);
};

} // namespace

#endif // _INTERACTIVESHAPEMANAGER_HEADERFILE
