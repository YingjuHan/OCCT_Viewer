import type {IManager, ColorString, IVec2, IVec4, ObjectId, IBndBox3} from './CommonTools.js';
import type { CaeMeshObjectPrsMode, CaeMeshPresentationType, CaeMeshVectorDistrType, ICaeMeshAnnotationInfo, ICaeMeshInfo, ICaeMeshPropCompInfo, ICaeMeshPropInfo, ICaeMeshStreamlinesInfo, ICaeMesh2DAxesNamingInfo, SCwebviewer, CaeMeshHighlightType, CaeMeshObjectSelMode } from './SCwebviewer.js';

/**
 * @internal Represents Context for Interactive CAE mesh objects.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveCaeMesh} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveCaeMeshManager} property.
 */
export interface InteractiveCaeMeshManager extends IManager {
// C++ methods

  /** @internal Probe supported format from file header. */
  probeFileFormat: (theName: string, theBuffer: number, theDataLen: number, theToFree: boolean) => string;

  /** @internal Open mesh from specified file. */
  openModelMesh: (theObjId: ObjectId, theFilePath: string, theToCacheProps: boolean) => boolean;

  /** @internal Dump mesh into a file. */
  saveModelMesh: (theObjId: ObjectId, theFile: string) => boolean;

  /** @internal Load mesh properties from the file and append to the existing mesh. */
  loadMeshProperties: (theObjId: ObjectId, theFilePath: string) => boolean;

  /** @internal Remove all mesh properties. */
  removeMeshProperties: (theObjId: ObjectId, theWithMetadata: boolean) => void;

  /** @internal Create new mesh presentation of the specified type and return its id. */
  createPresentation: (theMeshId: ObjectId,
                       theType: CaeMeshPresentationType,
                       thePrsId: string) => string;

  /** @internal Remove mesh presentation by its id. */
  removePresentation: (theMeshId: ObjectId,
                       thePrsId: string) => void;

  /** @internal Return all mesh presentations. */
  getPresentations: (theMeshId: ObjectId) => SCwebviewer['StringVector'];

  /** @internal Return type of mesh presentation by its id. */
  getMeshPresentationType: (theObjId: ObjectId, thePrsId: string) => CaeMeshPresentationType;

  /** @internal Return mesh presentation mode. */
  getMeshPresentationMode: (theObjId: ObjectId,
                            thePrsId: string) => CaeMeshObjectPrsMode;

  /** @internal Set mesh presentation mode. */
  setMeshPresentationMode: (theObjId: ObjectId,
                            thePrsId: string,
                            theMode: CaeMeshObjectPrsMode) => void;

  /** @internal Return mesh properties info. */
  getMeshProperties: (theObjId: ObjectId) => any;

  /** @internal Return mesh property info. */
  getMeshProperty: (theObjId: ObjectId,
                    thePropertyId: string) => ICaeMeshPropInfo;

  /** @internal Return mesh property component info. */
  getMeshPropertyComponent: (theObjId: ObjectId,
                             thePropertyId: string,
                             theCompIndex: number) => ICaeMeshPropCompInfo;

  /** @internal Return active mesh property. */
  getActiveMeshProperty: (theObjId: ObjectId,
                          thePrsId: string) => string;

  /** @internal Set active mesh property. */
  setActiveMeshProperty: (theObjId: ObjectId,
                          thePrsId: string,
                          thePropertyId: string) => void;

  /** @internal Return index of active mesh property component. */
  getActiveMeshComponentIndex: (theObjId: ObjectId,
                                thePrsId: string) => number;

  /** @internal Set index of active mesh property component. */
  setActiveMeshComponentIndex: (theObjId: ObjectId,
                                thePrsId: string,
                                theIndex: number) => void;

  /** @internal Return mesh element types to display. */
  getMeshElemTypes: (theObjId: ObjectId,
                     thePrsId: string) => number;

  /** @internal Set mesh element types to display. */
  setMeshElemTypes: (theObjId: ObjectId,
                     thePrsId: string,
                     theElemTypes: number) => void;

  /** @internal Return mesh bounding box. */
  getMeshBox: (theObjId: ObjectId) => IBndBox3;

  /** @internal Find property id by name. */
  getMeshPropertyIdByName: (theObjId: ObjectId,
                            thePropertyName: string) => string;

  /** @internal Return available vector mesh properties. */
  getAvailableMeshVec3Props: (theObjId: ObjectId) => any;

  /** @internal Return id of current active deformed mesh property. */
  getMeshDeformedProp: (theObjId: ObjectId, thePrsId: string) => string;

  /** @internal Set current active deformed mesh property. */
  setMeshDeformedProp: (theObjId: ObjectId,
                        thePrsId: string,
                        thePropertyId: string) => void;

  /** @internal Return scale coefficient for mesh deformed presentation. */
  getMeshDeformedScaleCoeff: (theObjId: ObjectId,
                              thePrsId: string) => number;

  /** @internal Set scale coefficient for mesh deformed presentation. */
  setMeshDeformedScaleCoeff: (theObjId: ObjectId,
                              thePrsId: string,
                              theDefScaleCoeff: number) => void;

  /** @internal Return id of current active vector mesh property. */
  getMeshVecProp: (theObjId: ObjectId, thePrsId: string) => string;

  /** @internal Set current active vector mesh property. */
  setMeshVecProp: (theObjId: ObjectId, thePrsId: string, thePropertyId: string) => void;

  /** @internal Return scale coefficient for mesh vector presentation. */
  getMeshVecPropScaleCoeff: (theObjId: ObjectId, thePrsId: string) => number;

  /** @internal Set scale coefficient for mesh vector presentation. */
  setMeshVecPropScaleCoeff: (theObjId: ObjectId, thePrsId: string, theScaleCoeff: number) => void;

  /** @internal Return distribution type for mesh vector presentation. */
  getMeshVecPropDistrType: (theObjId: ObjectId, thePrsId: string) => CaeMeshVectorDistrType;

  /** @internal Set distribution type for mesh vector presentation. */
  setMeshVecPropDistrType: (theObjId: ObjectId, thePrsId: string, theType: CaeMeshVectorDistrType) => void;

  /** @internal Return percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0. */
  getMeshVecSamplingRatio: (theObjId: ObjectId, thePrsId: string) => number;

  /** @internal Set percentage of displayed vectors for mesh vector presentation, from 0.0 to 1.0. */
  setMeshVecSamplingRatio: (theObjId: ObjectId, thePrsId: string, theRatio: number) => void;

  /** @internal Return map of mesh groups with non-zero number of entities. */
  getMeshUsedGroups: (theObjId: ObjectId,
                      thePropertyId: string) => SCwebviewer['IntStringMap'];

  /**
   * @internal Return true if it is needed to render/select 3d mesh only by its surface
   * and to not include internal parts.
   */
  isMeshVolumeAsSurface: (theObjId: ObjectId,
                          thePrsId: string) => boolean;

  /**
   * @internal Set if it is needed to render/select 3d mesh only by its surface
   * and to not include internal parts.
   */
  setMeshVolumeAsSurface: (theObjId: ObjectId,
                           thePrsId: string,
                           theVolumeAsSurface: boolean) => void;

  /** @internal Return visibility of specified mesh presentation. */
  isMeshPrsVisible: (theObjId: ObjectId,
                     thePrsId: string) => boolean;

  /** @internal Set visibility of specified mesh presentation. */
  setMeshPrsVisible: (theObjId: ObjectId,
                      thePrsId: string,
                      theIsVisible: boolean) => void;

  /** @internal Return mesh presentation (or whole mesh if thePrsId is empty) selection mode. */
  getMeshSelectionMode: (theObjId: ObjectId, thePrsId: string) => number;

  /** @internal Set mesh presentation (or whole mesh if thePrsId is empty) selection mode. */
  setMeshSelectionMode: (theObjId: ObjectId, thePrsId: string, theMode: number) => void;

  /** @internal Return mesh presentation (or whole mesh if thePrsId is empty) shading color. */
  getMeshShadingColor: (theObjId: ObjectId, thePrsId: string, theHiType: CaeMeshHighlightType) => ColorString;

  /** @internal Set mesh presentation (or whole mesh if thePrsId is empty) shading color. */
  setMeshShadingColor: (theObjId: ObjectId, thePrsId: string, theColor: ColorString, theHiType: CaeMeshHighlightType) => void;

  /** @internal Return mesh presentation (or whole mesh if thePrsId is empty) edges color. */
  getMeshEdgesColor: (theObjId: ObjectId, thePrsId: string, theHiType: CaeMeshHighlightType) => ColorString;

  /** @internal Set mesh presentation (or whole mesh if thePrsId is empty) edges color. */
  setMeshEdgesColor: (theObjId: ObjectId, thePrsId: string, theColor: ColorString, theHiType: CaeMeshHighlightType) => void;

  /** @internal Return mesh presentation (or whole mesh if thePrsId is empty) points color. */
  getMeshPointsColor: (theObjId: ObjectId, thePrsId: string, theHiType: CaeMeshHighlightType) => ColorString;

  /** @internal Set mesh presentation (or whole mesh if thePrsId is empty) points color. */
  setMeshPointsColor: (theObjId: ObjectId, thePrsId: string, theColor: ColorString, theHiType: CaeMeshHighlightType) => void;

  /** @internal Return mesh presentation (or whole mesh if thePrsId is empty) edges width. */
  getMeshEdgesWidth: (theObjId: ObjectId, thePrsId: string, theHiType: CaeMeshHighlightType) => number;

  /** @internal Set mesh presentation (or whole mesh if thePrsId is empty) edges width. */
  setMeshEdgesWidth: (theObjId: ObjectId, thePrsId: string, theWidth: number, theHiType: CaeMeshHighlightType) => void;

  /** @internal Return mesh presentation (or whole mesh if thePrsId is empty) points size. */
  getMeshPointsSize: (theObjId: ObjectId, thePrsId: string, theHiType: CaeMeshHighlightType) => number;

  /** @internal Set mesh presentation (or whole mesh if thePrsId is empty) points size. */
  setMeshPointsSize: (theObjId: ObjectId, thePrsId: string, theSize: number, theHiType: CaeMeshHighlightType) => void;

  /** @internal Return mesh presentation (or whole mesh if thePrsId is empty) shading transparency. */
  getMeshShadingTransparency: (theObjId: ObjectId, thePrsId: string, theHiType: CaeMeshHighlightType) => number;

  /** @internal Set mesh presentation (or whole mesh if thePrsId is empty) shading transparency. */
  setMeshShadingTransparency: (theObjId: ObjectId, thePrsId: string, theValue: number, theHiType: CaeMeshHighlightType) => void;

  /** @internal Return true if clip plane for mesh has been set. */
  hasClipPlane: (theObjId: ObjectId, thePrsId: string) => boolean;

  /** @internal Return equation of clip plane used to filter mesh elements. */
  getClipPlane: (theObjId: ObjectId, thePrsId: string) => IVec4;

  /** @internal Set clip plane used to filter mesh elements. */
  setClipPlane: (theObjId: ObjectId, thePrsId: string, theEq: IVec4) => void;

  /** @internal Unset a current active mesh clip plane. */
  unsetClipPlane: (theObjId: ObjectId, thePrsId: string) => void;

  /** @internal Check whether mesh clipping cuts or filters elements. */
  toCutOnClipPlane: (theObjId: ObjectId, thePrsId: string) => boolean;

  /** @internal Define whether mesh clipping cuts or filters elements. */
  setCutOnClipPlane: (theObjId: ObjectId, thePrsId: string, theToCut: boolean) => void;

  /** @internal Return array of planes used to cut mesh. */
  getSlicePlanes: (theObjId: ObjectId, thePrsId: string) => SCwebviewer['Vec4Vector'];

  /** @internal Set array of planes used to cut mesh. */
  setSlicePlanes: (theObjId: ObjectId, thePrsId: string, theEqs: SCwebviewer['Vec4Vector']) => void;

  /** @internal Return id of current active iso-surface mesh property. */
  getIsoPropId: (theObjId: ObjectId, thePrsId: string) => string;

  /** @internal Return index of current active iso-surface mesh property component. */
  getIsoPropComp: (theObjId: ObjectId, thePrsId: string) => number;

  /** @internal Set current active iso-surface mesh property. */
  setIsoProp: (theObjId: ObjectId, thePrsId: string, thePropId: string, theCompIndex: number) => void;

  /** @internal Return array of iso values. */
  getIsoValues: (theObjId: ObjectId, thePrsId: string) => SCwebviewer['DoubleVector'];

  /** @internal Set array of iso values. */
  setIsoValues: (theObjId: ObjectId, thePrsId: string, theValues: SCwebviewer['DoubleVector']) => void;

  /** @internal Return visibility of annotations for min/max values of the current active mesh property. */
  toShowPrsMinMax: (theObjId: ObjectId, thePrsId: string) => boolean;

  /** @internal Set visibility of annotations for min/max values of the current active mesh property. */
  setShowPrsMinMax: (theObjId: ObjectId, thePrsId: string, theToShow: boolean) => void;

  /** @internal Return visibility of annotations for the current active group mesh property. */
  toShowGroupsInfo: (theObjId: ObjectId, thePrsId: string) => boolean;

  /** @internal Set visibility of annotations for the current active group mesh property. */
  setShowGroupsInfo: (theObjId: ObjectId, thePrsId: string, theToShow: boolean) => void;

  /** @internal Return annotation for specified mesh group. */
  getGroupInfo: (theObjId: ObjectId, thePrsId: string, theGroupId: number) => ICaeMeshAnnotationInfo;

  /** @internal Set annotation for specified mesh group. */
  setGroupInfo: (theObjId: ObjectId, thePrsId: string, theGroupId: number, theInfo: ICaeMeshAnnotationInfo) => void;

  /** @internal Return mesh info. */
  getMeshInfo: (theObjId: ObjectId) => ICaeMeshInfo;

  /** @internal Return mesh blocks as a Json string. */
  getMeshBlocks: (theObjId: ObjectId) => string;

  /** @internal Return visible mesh blocks as a Json string. */
  getVisibleMeshBlocks: (theObjId: ObjectId, thePrsId: string) => string;

  /** @internal Set mesh blocks visibility. */
  setVisibleMeshBlocks: (theObjId: ObjectId, thePrsId: string, theBlocks: string, theVisible: boolean) => void;

  /** @internal Return mesh active time index. */
  getMeshActiveTimeStep: (theObjId: ObjectId) => number;

  /** @internal Set mesh active time index. */
  setMeshActiveTimeStep: (theObjId: ObjectId, theTimeStep: number) => string;

  /** @internal Return available mesh time indices. */
  getAvailableMeshTimeSteps: (theObjId: ObjectId) => SCwebviewer['Vec2Vector'];

  /** @internal Set available mesh time indices. */
  setAvailableMeshTimeSteps: (theObjId: ObjectId, theTimeSteps: Array<IVec2>) => void;

  /** @internal Check if mesh properties animation contains valid input data. */
  isMeshAnimationValid: (theObjId: ObjectId) => boolean;

  /** @internal Start animation of mesh properties. */
  startMeshAnimation: (theObjId: ObjectId, theToRestart: boolean) => void;

  /** @internal Stop animation of mesh properties. */
  stopMeshAnimation: (theObjId: ObjectId) => void;

  /** @internal Continue animation if it is paused. */
  continueMeshAnimation: (theObjId: ObjectId) => void;

  /** @internal Check if mesh properties animation is running. */
  isMeshAnimationActive: (theObjId: ObjectId) => boolean;

  /** @internal Return duration of mesh properties animation. */
  getMeshAnimationDuration: (theObjId: ObjectId) => number;

  /** @internal Set duration of mesh properties animation. */
  setMeshAnimationDuration: (theObjId: ObjectId, theDur: number) => void;

  /** @internal Check if mesh properties animation should be restarted on the last update. */
  getMeshAnimationLoopPlayback: (theObjId: ObjectId) => boolean;

  /** @internal Define if mesh properties animation should be restarted on the last update. */
  setMeshAnimationLoopPlayback: (theObjId: ObjectId, theIsLoopPlayback: boolean) => void;

  /** @internal Check if mesh properties animation steps duration should take into account timestamps values. */
  getMeshAnimationUniformSteps: (theObjId: ObjectId) => boolean;

  /** @internal Define if mesh properties animation steps duration should take into account timestamps values. */
  setMeshAnimationUniformSteps: (theObjId: ObjectId, theIsUniform: boolean) => void;

  /** @internal Return streamlines presentation parameters. */
  getMeshStreamlinesInfo: (theObjId: ObjectId, thePrsId: string) => ICaeMeshStreamlinesInfo;

  /** @internal Set streamlines presentation parameters. */
  setMeshStreamlinesInfo: (theObjId: ObjectId, thePrsId: string, theInfo: ICaeMeshStreamlinesInfo) => void;

  /** @internal Set to use custom geometry for mesh presentations. */
  setUseMeshGenGeometry: (theObjId: ObjectId) => void;

  /** @internal Set to use test custom geometry for mesh presentations. */
  setUseMeshTestCacheGenGeometry: (theObjId: ObjectId) => void;

  /** @internal Set custom mesh geometry for the specified presentation. */
  setMeshGenGeometry: (theObjId: ObjectId, thePrsId: string, thePartId: number, theFilePath: string) => boolean;

  /** @internal Check if mesh entities should be detected/selected. */
  toHighlightEntities: (theObjId: ObjectId) => boolean;

  /** @internal Define if mesh entities should be detected/selected. */
  setHighlightEntities: (theObjId: ObjectId, theToHighlight: boolean) => void;

  /** @internal Check whether a 2D grid is to be drawn when mesh is part of a graph. */
  toShow2DGrid: (theObjId: ObjectId) => boolean;

  /** @internal Set whether a 2D grid is to be drawn. */
  setShow2DGrid: (theObjId: ObjectId, theToHighlight: boolean) => void;

  /** @internal Get 2D grid axes names and tick mark labels. */
  getAxesNaming: (theObjId: ObjectId) => ICaeMesh2DAxesNamingInfo;
  
  /** @internal Set 2D grid axes names and tick mark labels. */
  setAxesNaming: (theObjId: ObjectId, theNameInfo: ICaeMesh2DAxesNamingInfo) => void;
};
