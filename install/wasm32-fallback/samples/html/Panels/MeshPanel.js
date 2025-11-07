/**
 * Created by: Andrey Golodyaev
 * Copyright (c) 2022 Huawei Technologies Co., Ltd.
 */
import * as SCwebviewer from '/wasm32/SCwebviewer.js';
import * as SCcontrols from './../SCcontrols.js';

/**
 * Panel for displaying CAE Mesh object properties.
 */
export class MeshPanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Meshes");

    this._scViewer = theSample.scViewer;
    this._selectedMesh = null;
    this._selectedMeshPrs = null;

    this.addMeshSelector();

    this._meshGroup = new SCcontrols.GroupInput("MeshGroup");
    this._meshPrsGroup = new SCcontrols.GroupInput("MeshPrsGroup");
    this._meshScalarPrsGroup = new SCcontrols.GroupInput("MeshScalarPrsGroup");
    this._meshVectorPrsGroup = new SCcontrols.GroupInput("MeshVectorPrsGroup");
    this._meshStreamlinesPrsGroup = new SCcontrols.GroupInput("MeshStreamlinesPrsGroup");
    this.addControl(this._meshGroup);

    this._meshPresentationExpandableGroup = new SCcontrols.GroupInput("Presentation", true, false);
    this._meshAnimationExpandableGroup = new SCcontrols.GroupInput("Animation", true, true);
    this._meshDisplayOptionsExpandableGroup = new SCcontrols.GroupInput("Display options", true, true);
    this._meshVisOptionsExpandableGroup = new SCcontrols.GroupInput("Presentation aspects", true, true);
    this._meshQuantityExpandableGroup = new SCcontrols.GroupInput("Quantity", true, false);
    this._meshGeometryExpandableGroup = new SCcontrols.GroupInput("Geometry", true, true);
    this._meshSelectionExpandableGroup = new SCcontrols.GroupInput("Selection", true, true);
    this._meshVectorExpandableGroup = new SCcontrols.GroupInput("Vectors", true, false);
    this._meshStreamlinesExpandableGroup = new SCcontrols.GroupInput("Streamlines", true, false);
    this._meshColorScaleExpandableGroup = new SCcontrols.GroupInput("Color Scale", true, true);
    this._meshPropertyLoadingExpandableGroup = new SCcontrols.GroupInput("Import property", true, true);
    this._meshInfoExpandableGroup = new SCcontrols.GroupInput("Info", true, true);
    this._meshGroupExpandableGroup = new SCcontrols.GroupInput("Group", true, true);

    this.addCurrentMeshControls();

    this._meshGroup.addControl(this._meshPresentationExpandableGroup);
    this.addPresentationControls();

    this._meshGroup.addControl(this._meshAnimationExpandableGroup);
    this.addAnimationControls();

    this._meshGroup.addControl(this._meshPrsGroup);
    this._meshPrsGroup.addControl(this._meshScalarPrsGroup);
    this._meshPrsGroup.addControl(this._meshVectorPrsGroup);
    this._meshPrsGroup.addControl(this._meshStreamlinesPrsGroup);

    this._meshScalarPrsGroup.addControl(this._meshDisplayOptionsExpandableGroup);
    this.addDisplayModeControls();
    this.addDisplayedElementTypeControls();

    this._meshScalarPrsGroup.addControl(this._meshVisOptionsExpandableGroup);
    this.addStyleControls();

    this._meshVectorPrsGroup.addControl(this._meshVectorExpandableGroup);
    this.addVectorControls();

    this._meshStreamlinesPrsGroup.addControl(this._meshStreamlinesExpandableGroup);
    this.addStreamlinesControls();

    this._meshPrsGroup.addControl(this._meshQuantityExpandableGroup);
    this.addQuantityControls();

    this._meshPrsGroup.addControl(this._meshGeometryExpandableGroup);
    this.addDeformationControls();
    this.addClippingControls();
    this.addSliceControls();
    this.addIsoControls();
    this.addBlocksControls();

    this._meshGroup.addControl(this._meshColorScaleExpandableGroup);
    this.addColorScaleControls();

    this._meshGroup.addControl(this._meshSelectionExpandableGroup);
    this.addSelectionControls();
    this.addPickingControls();

    this._meshGroup.addControl(this._meshPropertyLoadingExpandableGroup);
    this.addLoadPropertiesControls();

    this._meshGroup.addControl(this._meshInfoExpandableGroup);
    this.addInfoControls();

    this._meshPrsGroup.addControl(this._meshGroupExpandableGroup);
    this.addGroupControls();

    // add event handlers
    window.addEventListener("sceneChanged", e => {
      this._selectedMesh = this._scViewer.selectionManager.oneSelectedObject;
      if (this._selectedMesh == null || !(this._selectedMesh instanceof SCwebviewer.InteractiveCaeMesh)) {
        this._selectedMesh = null;
        this._selectedMeshPrs = null;
      }
      this.fillObjects();
      this.updateControls();
    });

    this.update = this.updateControls;

    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onSelectedObjectsChanged, e => {
      this._selectedMesh = this._scViewer.selectionManager.oneSelectedObject;
      if (this._selectedMesh == null || !(this._selectedMesh instanceof SCwebviewer.InteractiveCaeMesh)) {
        this._selectedMesh = null;
        this._selectedMeshPrs = null;
      }
      this.updateControls();
    });

    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onMeshPropertiesRequested, (theEvent) => {
      if (theEvent.mesh == null) {
        return;
      }

      if (theEvent.propsToLoad.length == 0) {
        return;
      }

      let anIndex = 0;
      const aTimeSteps = theEvent.mesh.availableTimeSteps;
      for (let aTimeStep of aTimeSteps) {
        if (theEvent.timeStep === aTimeStep.timeStep || theEvent.timeStep === -1) {
          break;
        }
        anIndex++;
      }
      if (anIndex == aTimeSteps.length) {
        console.log("Unknown timestamp");
        return;
      }

      console.log("Timestamp " + aTimeSteps[anIndex].timestamp + ", waiting for properties: " + theEvent.propsToLoad.join(", "));
    });

    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onMeshGenGeomRequested, (theEvent) => {
      if (theEvent.mesh == null) {
        return;
      }

      console.log("Requested geom for prs '" + theEvent.prsId + "', part '" + theEvent.partId + "'");
      console.log("Parameters '" + theEvent.parameters + "'");
    });
  }

  /**
   * Fill in mesh list.
   */
  fillObjects() {
    this._meshListInput.options.items = this._scViewer.interactiveContext.objectsInside
      .filter((theElem) => {return (theElem instanceof SCwebviewer.InteractiveCaeMesh)})
      .map(elem => { return {name: elem.id, value: elem.id} });
    this._meshListInput.options.items.unshift({name: "None", value: ""});
    this._meshListInput.updateValues();
  }

  /**
   * Add mesh object selector control.
   */
  addMeshSelector() {
    this._meshListInput = new SCcontrols.DropdownInput("Mesh", "",
      (object) => {
        this._selectedMesh = this._scViewer.interactiveContext.findInteractiveObject(object);
        if (this._selectedMesh == null || !(this._selectedMesh instanceof SCwebviewer.InteractiveCaeMesh)) {
          this._selectedMesh = null;
          this._selectedMeshPrs = null;
        }
        // workaround for non-selected objects
        let aMesh = this._selectedMesh;
        this._scViewer.selectionManager.oneSelectedObject = this._selectedMesh;
        this._selectedMesh = aMesh;
        this.updateControls();
        this._scViewer.requestViewUpdate();
      },
      {items: [{name: "None", value: ""}]}
    );

    this.addControl(this._meshListInput);
  }

  /**
   * Add current mesh controls.
   */
  addCurrentMeshControls() {
    this._currentIdControl = new SCcontrols.TextInput("Current ID:", "", null, {readOnly: true});

    this._visibleControl = new SCcontrols.CheckboxInput("Visible", false, (val) => {
      this._selectedMesh.isVisible = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._removeControl = new SCcontrols.ButtonInput("Remove", () => {
      if (this._selectedMesh != null) {
        if (this._selectedMesh instanceof SCwebviewer.InteractiveCaeMesh) {
          let aColorScales = this._selectedMesh.colorScaleObjects;
          for (let aColorScale of aColorScales) {
            aColorScale.remove();
          }
        }
        this._selectedMeshPrs = null;
        this._selectedMesh.remove();
        this._scViewer.viewManager.invalidate2dObjects("");
        this._scViewer.requestViewUpdate();
        window.dispatchEvent(new Event('sceneChanged'));
      }
    }, { inlineBlock: true });

    this._meshGroup.addControls([this._currentIdControl, this._visibleControl, this._removeControl]);
  }

  /**
   * Add mesh presentations controls.
   */
  addPresentationControls() {
    this._meshPresentationsInput = new SCcontrols.DropdownInput("Presentations", "", (val) => {
      this._selectedMeshPrs = this._selectedMesh.getPresentation(val);
      this.updatePrsRelatedControls();
      this._scViewer.requestViewUpdate();
    }, {items: []});

    this._meshCreateScalarPrsControl = new SCcontrols.ButtonInput("Create Scalar Presentation", () => {
      let aPrs = this._selectedMesh.createPresentation(SCwebviewer.CaeMeshPresentationType.Scalar);
      this.updatePresentationControls();
      this._selectedMeshPrs = aPrs;
      this._meshPresentationsInput.setValue(this._selectedMeshPrs.id);
      this.updatePrsRelatedControls();
      this._scViewer.requestViewUpdate();
    });

    this._meshCreateVectorPrsControl = new SCcontrols.ButtonInput("Create Vector Presentation", () => {
      let aPrs = this._selectedMesh.createPresentation(SCwebviewer.CaeMeshPresentationType.Vector);
      this.updatePresentationControls();
      this._selectedMeshPrs = aPrs;
      this._meshPresentationsInput.setValue(this._selectedMeshPrs.id);
      this.updatePrsRelatedControls();
      this._scViewer.requestViewUpdate();
    });

    this._meshCreateStreamlinesPrsControl = new SCcontrols.ButtonInput("Create Streamlines Presentation", () => {
      let aPrs = this._selectedMesh.createPresentation(SCwebviewer.CaeMeshPresentationType.Streamlines);
      this.updatePresentationControls();
      this._selectedMeshPrs = aPrs;
      this._meshPresentationsInput.setValue(this._selectedMeshPrs.id);
      this.updatePrsRelatedControls();
      this._scViewer.requestViewUpdate();
    });

    this._meshRemovePresentationControl = new SCcontrols.ButtonInput("Remove Presentation", () => {
      if (this._selectedMeshPrs == null) {
        return;
      }
      this._selectedMesh.removePresentation(this._selectedMeshPrs);
      this._selectedMeshPrs = null;
      this.updateControls();
      this._scViewer.requestViewUpdate();
    });

    this._meshPrsVisibleCheckbox = new SCcontrols.CheckboxInput("Prs Visibility", false, (val) => {
      if (this._selectedMeshPrs != null) {
        this._selectedMeshPrs.isVisible = val;
        this._scViewer.requestViewUpdate();
      }
    }, { inlineBlock: true });
    
    this._meshPresentationExpandableGroup.addControls([
      this._meshPresentationsInput,
      this._meshCreateScalarPrsControl,
      this._meshCreateVectorPrsControl,
      this._meshCreateStreamlinesPrsControl,
      this._meshRemovePresentationControl,
      this._meshPrsVisibleCheckbox
    ]);
  }

  /**
   * Add presentation mode controls.
   */
  addDisplayModeControls() {
    this._meshDispModeInput = new SCcontrols.DropdownInput("Display mode", SCwebviewer.CaeMeshObjectPrsMode.MeshEdges, (mode) => {
      this._selectedMeshPrs.presentationMode = mode;
      this._scViewer.requestViewUpdate();
    }, {items: [
      {name: "Mesh edges", value: SCwebviewer.CaeMeshObjectPrsMode.MeshEdges},
      {name: "Flat shaded", value: SCwebviewer.CaeMeshObjectPrsMode.FlatShaded},
      {name: "Shaded + mesh edges", value: SCwebviewer.CaeMeshObjectPrsMode.ShadedMeshEdges},
      {name: "Shaded (shrunk mesh)", value: SCwebviewer.CaeMeshObjectPrsMode.ShadedMeshShrinked},
      {name: "Shaded + free edges", value: SCwebviewer.CaeMeshObjectPrsMode.ShadedFreeEdges},
      {name: "Shaded + sharp edges", value: SCwebviewer.CaeMeshObjectPrsMode.ShadedSharpEdges}
    ]});

    this._meshDisplayOptionsExpandableGroup.addControl(this._meshDispModeInput);
  }

  /**
   * Add active quantity controls.
   */
  addQuantityControls() {
    this._meshQuantValInput = new SCcontrols.DropdownInput("Quantity", "", (val) => {
      this._selectedMeshPrs.activeProperty = val;
      this.updateComponentControl();
      this.updateGroupControls();
      this._scViewer.requestViewUpdate();
    }, {items: [ {name: "None", value: ""} ]});

    this._meshComponentInput = new SCcontrols.DropdownInput("Component", "", (val) => {
      this._selectedMeshPrs.activeComponent = val;
      this.updateGroupControls();
      this._scViewer.requestViewUpdate();
    }, {items: []});

    this._meshQuantityExpandableGroup.addControls([this._meshQuantValInput, this._meshComponentInput]);
  }

  /**
   * Add color scale controls.
   */
  addColorScaleControls() {
    this._meshColorScaleVisCheckbox = new SCcontrols.CheckboxInput("Colorscale", true, (val) => {
      let anObjs = this._selectedMesh.colorScaleObjects;
      const isVisible = anObjs.length != 0 && anObjs[0].isVisible;
      for (let anObj of anObjs) { anObj.isVisible = !isVisible; }
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });
    this._meshColorScaleSkipEmptyCheckbox = new SCcontrols.CheckboxInput("skip empty", false, (val) => {
      let anObjs = this._selectedMesh.colorScaleObjects;
      const toHide = anObjs.length != 0 && anObjs[0].toSkipEmptyLabels;
      for (let anObj of anObjs) { anObj.toSkipEmptyLabels = !toHide; }
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });
    this._meshColorScaleMin0Checkbox = new SCcontrols.CheckboxInput("min=0.0", false, (val) => {
      let anObjs = this._selectedMesh.colorScaleObjects;
      for (let anObj of anObjs) {
        anObj.displayRange = { x: val ? Math.min(0.0, anObj.defaultRange.y - 0.01) : anObj.defaultRange.x, y: anObj.displayRange.y };
      }
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });
    this._meshColorScaleMax1Checkbox = new SCcontrols.CheckboxInput("max=1.0", false, (val) => {
      let anObjs = this._selectedMesh.colorScaleObjects;
      for (let anObj of anObjs) {
        anObj.displayRange = { x: anObj.displayRange.x, y: val ? Math.max(1.0, anObj.defaultRange.x + 0.01) : anObj.defaultRange.y };
      }
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshColorScaleExpandableGroup.addControls([
      this._meshColorScaleVisCheckbox, this._meshColorScaleSkipEmptyCheckbox,
      this._meshColorScaleMin0Checkbox, this._meshColorScaleMax1Checkbox
    ]);
  }

  /**
   * Add displated element type controls.
   */
  addDisplayedElementTypeControls() {
    this._meshDisp3dGroup = new SCcontrols.GroupInput("Display3dGroup");
    this._meshDispEdgesGroup = new SCcontrols.GroupInput("DisplayEdgesGroup");
    this._meshDispGroup = new SCcontrols.GroupInput("DisplayGroup");
    this._meshDispGroup.minWidth = 330;

    this._meshDisp3dCheckbox = new SCcontrols.CheckboxInput("Display 3d elems", false, (val) => {
      this._selectedMeshPrs.toDisplay3dElements = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });
    this._meshVolumeAsSurfaceCheckbox = new SCcontrols.CheckboxInput("volume as surface", false, (val) => {
      this._selectedMeshPrs.isVolumeAsSurface = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });
    this._meshDisp3dGroup.addControls([this._meshDisp3dCheckbox, this._meshVolumeAsSurfaceCheckbox]);

    this._meshDisp2dCheckbox = new SCcontrols.CheckboxInput("2d elems", false, (val) => {
      this._selectedMeshPrs.toDisplay2dElements = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshDisp0dCheckbox = new SCcontrols.CheckboxInput("0d elems", false, (val) => {
      this._selectedMeshPrs.toDisplay0dElements = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshDispCentroidsCheckbox = new SCcontrols.CheckboxInput("centr.", false, (val) => {
      this._selectedMeshPrs.toDisplayCentroids = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshDispNodesCheckbox = new SCcontrols.CheckboxInput("nodes", false, (val) => {
      this._selectedMeshPrs.toDisplayNodes = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshDispGroup.addControls([
      this._meshDisp2dCheckbox, this._meshDisp0dCheckbox, this._meshDispCentroidsCheckbox,
      this._meshDispNodesCheckbox]);

    this._meshDispFreeNodesCheckbox = new SCcontrols.CheckboxInput("free nodes", false, (val) => {
      this._selectedMeshPrs.toProcessFreeNodes = val;
      this.updateClippingMinMax();
      this.pushClippingState();
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshDispFreeEdgesCheckbox = new SCcontrols.CheckboxInput("free edges", false, (val) => {
      this._selectedMeshPrs.toDisplayFreeEdges = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshDispSharpEdgesCheckbox = new SCcontrols.CheckboxInput("sharp edges", false, (val) => {
      this._selectedMeshPrs.toDisplaySharpEdges = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshDispEdgesGroup.addControls([
      this._meshDispFreeNodesCheckbox,
      this._meshDispFreeEdgesCheckbox,
      this._meshDispSharpEdgesCheckbox]);

    this._meshMinMaxCheckbox = new SCcontrols.CheckboxInput("Show MinMax values", false, (val) => {
      this._selectedMeshPrs.toShowMinMax = val;
      this._scViewer.requestViewUpdate();
    });

    this._meshDisplayOptionsExpandableGroup.addControls([
      this._meshDisp3dGroup,
      this._meshDispGroup,
      this._meshDispEdgesGroup,
      this._meshMinMaxCheckbox
    ]);
  }

  /**
   * Add controls for deformed mesh property.
   */
  addDeformationControls() {
    this._meshDeformInput = new SCcontrols.DropdownInput("Deformed property", "", (val) => {
      this._selectedMeshPrs.activeDeformedProp = val;
      this._selectedMeshPrs.deformedScaleCoeff = 0.0;
      this.onMeshDeformedPropChanged(val);
      this._scViewer.requestViewUpdate();
    }, {items: [ {name: "None", value: ""} ]});

    this._meshDeformScaleCoeffSlider = new SCcontrols.SliderInput("Deformed scale", 0.0, (val) => {
      this._selectedMeshPrs.deformedScaleCoeff = val;
      this._scViewer.requestViewUpdate();
    }, {min: 0.0, max: 1.0, step: 0.01, format: (num)=>{ return (100.0 * num).toFixed() + "%"; }});

    this._meshGeometryExpandableGroup.addControls([this._meshDeformInput, this._meshDeformScaleCoeffSlider]);
  }

  /**
   * Add controls for vector mesh property.
   */
  addVectorControls() {
    this._meshVecPropInput = new SCcontrols.DropdownInput("Vector property", "", (val) => {
      this._selectedMeshPrs.activeVecProp = val;
      this._selectedMeshPrs.vecPropScaleCoeff = 0.0;
      this.onMeshVecPropChanged(val);
      this._scViewer.requestViewUpdate();
    }, {items: [ {name: "None", value: ""} ]});

    this._meshVecPropScaleCoeffSlider = new SCcontrols.SliderInput("Vectors scale", 0.0, (val) => {
      this._selectedMeshPrs.vecPropScaleCoeff = val;
      this._scViewer.requestViewUpdate();
    }, {min: 0, max: 1.0, step: 0.0001});

    this._meshVecPropDistrTypeInput = new SCcontrols.DropdownInput("Vectors Distribution", SCwebviewer.CaeMeshVectorDistrType.Volume, (val) => {
      this._selectedMeshPrs.vecPropDistrType = val;
      this._scViewer.requestViewUpdate();
    }, {items: [ {name: "Volume", value: SCwebviewer.CaeMeshVectorDistrType.Volume}, {name: "Surface", value: SCwebviewer.CaeMeshVectorDistrType.Surface}]});

    this._meshVecSampleRatioSlider = new SCcontrols.SliderInput("Vectors Sampling Ratio", 1.0, (val) => {
      this._selectedMeshPrs.vecSamplingRatio = val;
      this._scViewer.requestViewUpdate();
    }, {min: 0.0, max: 1.0, step: 0.0001});

    this._meshVectorExpandableGroup.addControls([
      this._meshVecPropInput,
      this._meshVecPropScaleCoeffSlider,
      this._meshVecPropDistrTypeInput,
      this._meshVecSampleRatioSlider
    ]);
  }

  /**
   * Update streamlines presentation with parameters from the controls.
   */
  updateStreamlines(theToUpdate = true) {
    let aStreamlines = this._selectedMeshPrs.streamlines;
    const aBox = this._selectedMesh.boundingBox;
    const aBoxMin = aBox.cornerMin;
    const aBoxMax = aBox.cornerMax;
    const aBoxSize = SCwebviewer.Vec3.subtract(aBoxMax, aBoxMin);

    aStreamlines.property = this._meshStreamlinesPropInput.value;
    aStreamlines.isRegular = this._meshStreamlinesRegularToggle.value;
    aStreamlines.nbStreamlines = this._meshNbStreamlinesSlider.value;
    aStreamlines.uvStreamlines = {x: this._meshUStreamlinesSlider.value, y: this._meshVStreamlinesSlider.value};
    aStreamlines.planeOrigin = SCwebviewer.Vec3.zero();
    aStreamlines.planeSide = SCwebviewer.Vec3.zero();
    aStreamlines.planeUp = SCwebviewer.Vec3.zero();
    aStreamlines.stepCoeff = this._meshStreamlinesStepCoeffSlider.value;
    aStreamlines.lengthCoeff = this._meshStreamlinesLengthCoeffSlider.value;
    aStreamlines.thickness = this._meshStreamlinesThicknessSlider.value;
    aStreamlines.animationEnabled = this._meshStreamlinesAnimToggle.value;
    aStreamlines.animationSpeed = this._meshStreamlinesAnimSpeedSlider.value;
    aStreamlines.animationTrail = this._meshStreamlinesAnimTrailSlider.value;
    aStreamlines.animationFrequency = this._meshStreamlinesAnimFreqSlider.value;

    let aCoordOrigin, aCoordSide, aCoordUp;
    if (this._meshStreamlinesPlaneDirInput.value == "X") {
      aCoordOrigin = "x";
      aCoordSide = "y";
      aCoordUp = "z";
    } else if (this._meshStreamlinesPlaneDirInput.value == "Y") {
      aCoordOrigin = "y";
      aCoordSide = "z";
      aCoordUp = "x";
    } else {
      aCoordOrigin = "z";
      aCoordSide = "x";
      aCoordUp = "y";
    }

    let aSideLeft = aBoxMin[aCoordSide] + this._meshStreamlinesPlaneSideBoundsSlider.value.left * aBoxSize[aCoordSide];
    let aSideRight = aBoxMin[aCoordSide] + this._meshStreamlinesPlaneSideBoundsSlider.value.right * aBoxSize[aCoordSide];
    aStreamlines.planeSide[aCoordSide] = (aSideRight - aSideLeft) * 0.5;

    let anUpLeft = aBoxMin[aCoordUp] + this._meshStreamlinesPlaneUpBoundsSlider.value.left * aBoxSize[aCoordUp];
    let anUpRight = aBoxMin[aCoordUp] + this._meshStreamlinesPlaneUpBoundsSlider.value.right * aBoxSize[aCoordUp];
    aStreamlines.planeUp[aCoordUp] = (anUpRight - anUpLeft) * 0.5;

    aStreamlines.planeOrigin[aCoordOrigin] = aBoxMin[aCoordOrigin] + this._meshStreamlinesPlaneOriginSlider.value * aBoxSize[aCoordOrigin];
    aStreamlines.planeOrigin[aCoordSide] = (aSideLeft + aSideRight) * 0.5;
    aStreamlines.planeOrigin[aCoordUp] = (anUpLeft + anUpRight) * 0.5;

    if (this._previewStreamlinesPlane != null) {
      this._previewStreamlinesPlane.remove();
    }

    if (theToUpdate) {
      this._selectedMeshPrs.streamlines = aStreamlines;
    } else {
      let aLocation = aStreamlines.planeOrigin;
      aLocation = SCwebviewer.Vec3.subtract(aLocation, aStreamlines.planeSide);
      aLocation = SCwebviewer.Vec3.subtract(aLocation, aStreamlines.planeUp);
      const anAx2 = {
        location:   aLocation,
        direction:  SCwebviewer.Vec3.normalize(SCwebviewer.Vec3.cross(aStreamlines.planeSide, aStreamlines.planeUp)),
        xDirection: SCwebviewer.Vec3.normalize(aStreamlines.planeSide)
      };
      const aDims = SCwebviewer.Vec3.add(SCwebviewer.Vec3.multiply(aStreamlines.planeSide, 2.0),
                                        SCwebviewer.Vec3.multiply(aStreamlines.planeUp, 2.0));

      if (SCwebviewer.Vec3.modulus(aStreamlines.planeSide) > 0.000001
        && SCwebviewer.Vec3.modulus(aStreamlines.planeUp) > 0.000001) {
        this._previewStreamlinesPlane = new SCwebviewer.InteractiveShape("previewStreamlinesPlane");
        this._previewStreamlinesPlane.buildBox(this._scViewer, anAx2, aDims);
        this._previewStreamlinesPlane.isVisible = true;
        this._previewStreamlinesPlane.transparency = 0.75;
      }
    }
    
    this._scViewer.requestViewUpdate();
  }
  
  /**
   * Add controls for streamlines presentation.
   */
  addStreamlinesControls() {
    this._meshStreamlinesPropInput = new SCcontrols.DropdownInput("Streamlines property", "", (val) => {
      this.updateStreamlines();
    }, {items: [ {name: "None", value: ""} ]});

    this._meshStreamlinesPlaneDirInput = new SCcontrols.DropdownInput("Plane Dir", "X", (val) => {
      this.updateStreamlines();
    }, {items: [ {name: "X", value: "X"}, {name: "Y", value: "Y"}, {name: "Z", value: "Z"}]});

    this._meshStreamlinesPlaneOriginSlider = new SCcontrols.SliderInput("Plane Origin", 0.0, (val, changed) => {
      this.updateStreamlines(changed);
    }, {min: 0, max: 1, step: 0.00001, handleOnChange: true});

    this._meshStreamlinesPlaneSideBoundsSlider = new SCcontrols.DualSliderInput("Plane Side Bounds", {left: 0, right: 1}, (val, changed) => {
      this.updateStreamlines(changed);
    }, {min: 0, max: 1, step: 0.00001, handleOnChange: true});

    this._meshStreamlinesPlaneUpBoundsSlider = new SCcontrols.DualSliderInput("Plane Up Bounds", {left: 0, right: 1}, (val, changed) => {
      this.updateStreamlines(changed);
    }, {min: 0, max: 1, step: 0.00001, handleOnChange: true});

    this._meshStreamlinesRegularToggle = new SCcontrols.CheckboxInput("Regular seed points", false, (val) => {
      this.updateStreamlines();
    });

    this._meshNbStreamlinesSlider = new SCcontrols.SliderInput("Nb Streamlines", 5, (val, changed) => {
      this.updateStreamlines(changed);
    }, {min: 1, max: 1000, step: 1, handleOnChange: true});

    this._meshUStreamlinesSlider = new SCcontrols.SliderInput("Streamline columns", 5, (val, changed) => {
      this.updateStreamlines(changed);
    }, {min: 1, max: 30, step: 1, handleOnChange: true});

    this._meshVStreamlinesSlider = new SCcontrols.SliderInput("Streamline rows", 5, (val, changed) => {
      this.updateStreamlines(changed);
    }, {min: 1, max: 30, step: 1, handleOnChange: true});

    this._meshStreamlinesStepCoeffSlider = new SCcontrols.SliderInput("Step Coeff", 1.0, (val, changed) => {
      this.updateStreamlines(changed);
    }, {min: 0.01, max: 0.5, step: 0.001, handleOnChange: true});

    this._meshStreamlinesLengthCoeffSlider = new SCcontrols.SliderInput("Length Coeff", 1.0, (val, changed) => {
      this.updateStreamlines(changed);
    }, {min: 0.1, max: 2.0, step: 0.001, handleOnChange: true});

    this._meshStreamlinesThicknessSlider = new SCcontrols.SliderInput("Thickness", 0.0, (val, changed) => {
      this.updateStreamlines(changed);
    }, {min: 0.0, max: 1.0, step: 0.001, handleOnChange: true});

    this._meshStreamlinesAnimToggle = new SCcontrols.CheckboxInput("Animation", false, (val) => {
      this.updateStreamlines();
    });

    this._meshStreamlinesAnimSpeedSlider = new SCcontrols.SliderInput("Animation speed", 0.0, (val) => {
      this.updateStreamlines();
    }, {min: 0.0, max: 1.0, step: 0.01});

    this._meshStreamlinesAnimTrailSlider = new SCcontrols.SliderInput("Animation trail", 0.0, (val) => {
      this.updateStreamlines();
    }, {min: 0.0, max: 1.0, step: 0.01});

    this._meshStreamlinesAnimFreqSlider = new SCcontrols.SliderInput("Animation frequency", 1, (val) => {
      this.updateStreamlines();
    }, {min: 1, max: 100, step: 1});

    this._meshStreamlinesExpandableGroup.addControls([
      this._meshStreamlinesPropInput,
      this._meshStreamlinesPlaneDirInput,
      this._meshStreamlinesPlaneOriginSlider,
      this._meshStreamlinesPlaneSideBoundsSlider,
      this._meshStreamlinesPlaneUpBoundsSlider,
      this._meshStreamlinesRegularToggle,
      this._meshNbStreamlinesSlider,
      this._meshUStreamlinesSlider,
      this._meshVStreamlinesSlider,
      this._meshStreamlinesStepCoeffSlider,
      this._meshStreamlinesLengthCoeffSlider,
      this._meshStreamlinesThicknessSlider,
      this._meshStreamlinesAnimToggle,
      this._meshStreamlinesAnimSpeedSlider,
      this._meshStreamlinesAnimTrailSlider,
      this._meshStreamlinesAnimFreqSlider
    ]);
  }

  /**
   * Add presentation attributes controls.
   */
  addStyleControls() {
    this._meshShadingColorInput = new SCcontrols.ColorInput("Color Shading", "#000", (val) => {
      this._selectedMeshPrs.shadingColor = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshEdgesColorInput = new SCcontrols.ColorInput("Edges", "#000", (val) => {
      this._selectedMeshPrs.edgesColor = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshPointsColorInput = new SCcontrols.ColorInput("Points", "#000", (val) => {
      this._selectedMeshPrs.pointsColor = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshEdgesWidthSlider = new SCcontrols.SliderInput("Edges Width", 1, (val) => {
      this._selectedMeshPrs.edgesWidth = val;
      this._scViewer.requestViewUpdate();
    }, {min: 1, max: 10, step: 1});

    this._meshPointsSizeSlider = new SCcontrols.SliderInput("Points Size", 1, (val) => {
      this._selectedMeshPrs.pointsSize = val;
      this._scViewer.requestViewUpdate();
    }, {min: 1, max: 10, step: 1});

    this._meshShadingTranspSlider = new SCcontrols.SliderInput("Transparency", 0.0, (val) => {
      this._selectedMeshPrs.shadingTransparency = val;
      this._scViewer.requestViewUpdate();
    }, {min: 0.0, max: 1.0, step: 0.01, format: (num)=>{ return (100.0 * num).toFixed() + "%"; }});

    this._meshVisOptionsExpandableGroup.addControls([
      this._meshShadingColorInput,
      this._meshEdgesColorInput,
      this._meshPointsColorInput,
      this._meshEdgesWidthSlider,
      this._meshPointsSizeSlider,
      this._meshShadingTranspSlider
    ]);
  }

  /**
   * Set a clip plane for mesh.
   */
  pushClippingState() {
    if (!this._meshClippingToggle.value) {
      this._selectedMeshPrs.clipPlane = null;
      this._scViewer.requestViewUpdate();
      return;
    }

    let anEq = {};
    let aDirValue = this._meshClippingDir.value;
    anEq.x = aDirValue === "+X" ? 1.0 : aDirValue === "-X" ? -1.0 : 0.0;
    anEq.y = aDirValue === "+Y" ? 1.0 : aDirValue === "-Y" ? -1.0 : 0.0;
    anEq.z = aDirValue === "+Z" ? 1.0 : aDirValue === "-Z" ? -1.0 : 0.0;
    anEq.w = this._meshClippingDist.value * (aDirValue === "-X" || aDirValue === "-Y" || aDirValue === "-Z" ? 1.0 : -1.0);

    this._selectedMeshPrs.clipPlane = anEq;

    this._selectedMeshPrs.toCutOnClipPlane = this._meshClippingCutToggle.value;

    this._scViewer.requestViewUpdate();
  }
  
  /**
   * Update min/max values for clipping control.
   */
  updateClippingMinMax() {
    const aBox = this._selectedMesh.boundingBox;
    
    let aDistValue = 0;
    switch (this._meshClippingDir.value) {
      case "+X":
      case "-X":
        this._meshClippingDist.options.min = aBox.cornerMin.x;
        this._meshClippingDist.options.max = aBox.cornerMax.x;
        aDistValue = (aBox.cornerMax.x + aBox.cornerMin.x) * 0.5;
        break;
      case "+Y":
      case "-Y":
        this._meshClippingDist.options.min = aBox.cornerMin.y;
        this._meshClippingDist.options.max = aBox.cornerMax.y;
        aDistValue = (aBox.cornerMax.y + aBox.cornerMin.y) * 0.5;
        break;
      case "+Z":
      case "-Z":
        this._meshClippingDist.options.min = aBox.cornerMin.z;
        this._meshClippingDist.options.max = aBox.cornerMax.z;
        aDistValue = (aBox.cornerMax.z + aBox.cornerMin.z) * 0.5;
        break;
    }
    this._meshClippingDist.options.step = (this._meshClippingDist.options.max - this._meshClippingDist.options.min) / 10000;
    this._meshClippingDist.options.min -= this._meshClippingDist.options.step;
    this._meshClippingDist.options.max += this._meshClippingDist.options.step;
    this._meshClippingDist.updateValues();

    this._meshClippingDist.setValue(aDistValue);
  }

  /**
   * Add clipping controls.
   */
  addClippingControls() {
    this._meshClippingGroup = new SCcontrols.GroupInput("ClippingGroup");

    this._meshClippingToggle = new SCcontrols.CheckboxInput("Clipping", false, (val) => {
      this._meshClippingGroup.setVisibility(val);
      this.pushClippingState();
    });

    this._meshClippingDir = new SCcontrols.DropdownInput("Clipping direction", "+X", (val) => {
      this.updateClippingMinMax();
      this.pushClippingState();
    }, {items: [ {name: "+X", value: "+X"},
                 {name: "-X", value: "-X"},
                 {name: "+Y", value: "+Y"},
                 {name: "-Y", value: "-Y"},
                 {name: "+Z", value: "+Z"},
                 {name: "-Z", value: "-Z"} ]});
    
    this._meshClippingDist = new SCcontrols.SliderInput("Clipping distance", 0.0, (val) => {
      this.pushClippingState();
    }, {min: -1.0, max: 1.0, step: 0.01});

    this._meshClippingCutToggle = new SCcontrols.CheckboxInput("Clipping cut", false, (val) => {
      this.pushClippingState();
    });

    this._meshGeometryExpandableGroup.addControls([
      this._meshClippingToggle,
      this._meshClippingGroup
    ]);

    this._meshClippingGroup.addControls([
      this._meshClippingDir,
      this._meshClippingDist,
      this._meshClippingCutToggle
    ]);
  }

  /**
   * Set slice planes for mesh.
   */
  pushSliceState() {
    if (!this._meshSliceToggle.value) {
      this._selectedMeshPrs.slicePlanes = [];
      this._scViewer.requestViewUpdate();
      return;
    }

    const aBox = this._selectedMesh.boundingBox;
    
    let aMinDist, aMaxDist, aDir;
    switch(this._meshSliceDir.value) {
      case "X":
        aMinDist = aBox.cornerMin.x;
        aMaxDist = aBox.cornerMax.x;
        aDir = {x: 1, y: 0, z: 0};
        break;
      case "Y":
        aMinDist = aBox.cornerMin.y;
        aMaxDist = aBox.cornerMax.y;
        aDir = {x: 0, y: 1, z: 0};
        break;
      case "Z":
        aMinDist = aBox.cornerMin.z;
        aMaxDist = aBox.cornerMax.z;
        aDir = {x: 0, y: 0, z: 1};
        break;
    }

    let aSize = aMaxDist - aMinDist;
    aMinDist += this._meshSliceMin.value * aSize;
    aMaxDist -= (1.0 - this._meshSliceMax.value) * aSize;

    let aRes = [];
    let aNbSlices = this._meshSliceNumber.value;
    for (let i = 0; i < aNbSlices; i++) {
      aRes.push({x: aDir.x, y: aDir.y, z: aDir.z, w: -(aMinDist + (i + 1) * (aMaxDist - aMinDist) / (aNbSlices + 1))});
    }

    this._selectedMeshPrs.slicePlanes = aRes;
    this._scViewer.requestViewUpdate();
  }
  
  /**
   * Add slice controls.
   */
  addSliceControls() {
    this._meshSliceGroup = new SCcontrols.GroupInput("SliceGroup");

    this._meshSliceToggle = new SCcontrols.CheckboxInput("Slice", false, (val) => {
      this._meshSliceGroup.setVisibility(val);
      this.pushSliceState();
    });

    this._meshSliceDir = new SCcontrols.DropdownInput("Slice direction", "X", (val) => {
      this.pushSliceState();
    }, {items: [ {name: "X", value: "X"},
                 {name: "Y", value: "Y"},
                 {name: "Z", value: "Z"} ]});
    
    this._meshSliceMin = new SCcontrols.SliderInput("Slice Min", 0.0, (val) => {
      this.pushSliceState();
    }, {min: 0.0, max: 1.0, step: 0.0001, format: (num)=>{ return num.toFixed(4); } });

    this._meshSliceMax = new SCcontrols.SliderInput("Slice Max", 1.0, (val) => {
      this.pushSliceState();
    }, {min: 0.0, max: 1.0, step: 0.0001, format: (num)=>{ return num.toFixed(4); } });

    this._meshSliceNumber = new SCcontrols.SliderInput("Slice Num", 1, (val) => {
      this.pushSliceState();
    }, {min: 1, max: 50, step: 1});

    this._meshGeometryExpandableGroup.addControls([
      this._meshSliceToggle,
      this._meshSliceGroup
    ]);

    this._meshSliceGroup.addControls([
      this._meshSliceDir,
      this._meshSliceMin,
      this._meshSliceMax,
      this._meshSliceNumber
    ]);
  }

  /**
   * Set iso-surface values for mesh.
   */
  pushIsoState() {
    const anIsoProp = this._selectedMeshPrs.isoProperty;

    if (anIsoProp.id === "") {
      this._selectedMeshPrs.isoValues = [];
      this._scViewer.requestViewUpdate();
      return;
    }

    let aRes = [];
    let anIsoMin = this._meshIsoMin.value;
    let anIsoMax = this._meshIsoMax.value;

    const aDispUnits = this._selectedMesh.getPropertyDisplayUnits(anIsoProp.id, anIsoProp.index);
    if (aDispUnits !== "") {
      anIsoMin = this._scViewer.unitManager.convertAnyToSI(anIsoMin, aDispUnits);
      anIsoMax = this._scViewer.unitManager.convertAnyToSI(anIsoMax, aDispUnits);
    }

    let aNbIsoSurfaces = this._meshIsoNumber.value;
    for (let i = 0; i < aNbIsoSurfaces; i++) {
      aRes.push(anIsoMin + (i + 1) * (anIsoMax - anIsoMin) / (aNbIsoSurfaces + 1));
    }

    this._selectedMeshPrs.isoValues = aRes;
    this._scViewer.requestViewUpdate();
  }
  
  /**
   * Add iso-surface controls.
   */
  addIsoControls() {
    this._meshIsoGroup = new SCcontrols.GroupInput("IsoGroup");

    this._meshIsoPropInput = new SCcontrols.DropdownInput("Iso Property", "", (val) => {
      this._meshIsoGroup.setVisibility(val !== "");
      this._selectedMeshPrs.isoProperty = {id: val, index: val !== "" ? 0 : -1};
      this.updateIsoComponentControl();
      this.updateIsoValuesControls();
      this.pushIsoState();
      this._scViewer.requestViewUpdate();
    }, {items: [ {name: "None", value: ""} ]});

    this._meshIsoComponentInput = new SCcontrols.DropdownInput("Iso Component", "", (val) => {
      this._selectedMeshPrs.isoProperty = {id: this._meshIsoPropInput.value, index: val};
      this.updateIsoValuesControls();
      this.pushIsoState();
      this._scViewer.requestViewUpdate();
    }, {items: []});

    this._meshIsoMin = new SCcontrols.SliderInput("Iso Min", 0.0, (val) => {
      this.pushIsoState();
    }, {min: 0.0, max: 1.0, step: 0.0001});

    this._meshIsoMax = new SCcontrols.SliderInput("Iso Max", 1.0, (val) => {
      this.pushIsoState();
    }, {min: 0.0, max: 1.0, step: 0.0001});

    this._meshIsoNumber = new SCcontrols.SliderInput("Iso Values Num", 1, (val) => {
      this.pushIsoState();
    }, {min: 1, max: 50, step: 1});

    this._meshGeometryExpandableGroup.addControls([
      this._meshIsoPropInput,
      this._meshIsoComponentInput,
      this._meshIsoGroup
    ]);

    this._meshIsoGroup.addControls([
      this._meshIsoMin,
      this._meshIsoMax,
      this._meshIsoNumber
    ]);
  }
  
  /**
   * Add mesh blocks controls.
   */
  addBlocksControls() {
    this._meshBlocksGroup = new SCcontrols.GroupInput("BlocksGroup");

    this._meshBlocksTreeViewInput = new SCcontrols.TreeViewInput("Blocks:", {}, (val) => {
      const aChain = val.chain;
      if (!aChain || !aChain.length) {
        return;
      }

      const isEnabled = val.value;

      let aTree = {};
      for (let i = 0, aNode = aTree; i < aChain.length; i++) {
        aNode.name = aChain[i];
        if (i < aChain.length - 1) {
          aNode.children = [{}];
          aNode = aNode.children[0];
        }
      }

      this._selectedMeshPrs.setVisibleBlocks(aTree, isEnabled);
      this._scViewer.requestViewUpdate();
    }, {tree: {}, switchable: true});

    this._meshGeometryExpandableGroup.addControls([
      this._meshBlocksGroup
    ]);

    this._meshBlocksGroup.addControls([
      this._meshBlocksTreeViewInput
    ]);
  }

  /**
   * Add load properties button.
   */
  addLoadPropertiesControls() {
    this._meshLoadPropsButton = new SCcontrols.FileInput("Load Properties", (val)=> {
      if (val.files.length != 1) { return; }
      val.files[0].arrayBuffer().then((aBuffer) => {
        this._selectedMesh.loadPropertiesAsync(this._scViewer, aBuffer)
        .then(() => {
          if (this._selectedMeshPrs != null) {
            this.updateMeshQuantities();
          }
          this.updateAnimationControls();

          this._selectedMesh.animation.continue();
          this._scViewer.requestViewUpdate();
        })
        .catch(a => console.log(a));
      });
    }, { accept: ".pos, .msh, .mdb"});

    this._meshRemovePropsButton = new SCcontrols.ButtonInput("Remove Properties", ()=> {
      this._selectedMesh.removeProperties(false);

      this.updatePrsRelatedControls();
      this.updateAnimationControls();

      this._scViewer.requestViewUpdate();
    });

    this._meshPropertyLoadingExpandableGroup.addControls([this._meshLoadPropsButton, this._meshRemovePropsButton]);
  }

  /**
   * Create mesh entities selection controls.
   */
  addSelectionControls() {
    this._meshSelModeInput = new SCcontrols.DropdownInput("Selection mode", SCwebviewer.CaeMeshObjectSelMode.Global, (mode) => {
      this._selectedMesh.selectionMode = mode;
      this._subSelectedInfo.setValue("");
      this._subDetectedInfo.setValue("");
      this._scViewer.requestViewUpdate();
    }, {items: [
      {name: "Global", value: SCwebviewer.CaeMeshObjectSelMode.Global},
      {name: "Elements", value: SCwebviewer.CaeMeshObjectSelMode.Elements},
      {name: "Nodes", value: SCwebviewer.CaeMeshObjectSelMode.Nodes},
      {name: "Elements+Nodes", value: SCwebviewer.CaeMeshObjectSelMode.Elements | SCwebviewer.CaeMeshObjectSelMode.Nodes}
    ]});

    this._meshHighlightCheckbox = new SCcontrols.CheckboxInput("Highlight entities", false, (val) => {
      this._selectedMesh.toHighlightEntities = val;
      this._scViewer.requestViewUpdate();
    });
  
    this._meshSelectionExpandableGroup.addControls([this._meshSelModeInput, this._meshHighlightCheckbox]);
  }

  /**
   * Convert SCwebviewer.ISelectionMeshEntity to string.
   */
  entityToInfoString(entity, isSelection) {
    if (entity.entityId == -1 || entity.entityType == SCwebviewer.MeshEntityType.Unknown) { return ""; }

    let anInfo = "";
    anInfo += "\n* Prs = " + entity.prsId;
    anInfo += "\n* Pos = (" + this._scViewer.unitManager.length.format(entity.pos) + ")";

    if (!entity.hasRefEntity && isSelection) {
      return anInfo;
    }

    if (isSelection) {
      anInfo += "\n* ID = " + entity.entityId;
      anInfo += "\n* Type = " + SCwebviewer.MeshEntityType[entity.entityType];
    }
    if (entity.lineIndex !== -1) {
      anInfo += "\n* Line = " + entity.lineIndex;
    }

    anInfo += "\n";

    for (let [propType, propInfo] of entity.prsPropsInfo) {
      if (!isSelection && propType !== SCwebviewer.MeshPropertyUsageType.ColorMap) {
        continue;
      }
      anInfo += "\n" + SCwebviewer.MeshPropertyUsageType[propType];
      const aPropValueInfo = entity.propsInfo.get(propInfo.propertyId);
      if (aPropValueInfo == undefined) {
        anInfo += ": null";
        continue;
      }
      if (propInfo.componentIndex != -1 && propInfo.componentIndex >= aPropValueInfo.components.length) {
        anInfo += ": null";
        continue;
      }
      
      const aComps = propInfo.componentIndex != -1 ? [propInfo.componentIndex] : [1, 2, 3];
      let aValues = [];
      for (let aComp of aComps) {
        let aValue = aPropValueInfo.components[aComp].value;
        if (SCwebviewer.Vec3.isInfinite(aValue)) {
          aValues.push("null");
          continue;
        }
        const aDispUnits = aPropValueInfo.components[aComp].displayUnits;
        if (aDispUnits !== "") {
          aValue = this._scViewer.unitManager.convertAnyFromSI(aValue, aDispUnits);
        }
        aValues.push(aValue + this._scViewer.unitManager.formatUnitName(aDispUnits));
      }

      anInfo += ": (" + aValues.join(", ") + ")";
    }
    
    return anInfo;
  }

  /**
   * Create entities selection controls.
   */
  addPickingControls() {
    this._subSelectedInfo = new SCcontrols.TextAreaInput("Entity selected:", "", null, {readOnly: true});
    this._subDetectedInfo = new SCcontrols.TextAreaInput("Entity detected:", "", null, {autoHeight: true, readOnly: true});
    this._meshSelectionExpandableGroup.addControls([this._subSelectedInfo, this._subDetectedInfo]);
  
    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onDetectedObjectChanged, () => {
      let aMeshObj = this._selectedMesh;
      let aDetObj = this._scViewer.selectionManager.lastDetectedObject;
  
      let anInfo = "";
      if (aMeshObj != null && aMeshObj == aDetObj) {
        let aDetectedEntity = aMeshObj.getDetectedEntity();
        anInfo = this.entityToInfoString(aDetectedEntity, false);
      }
      this._subDetectedInfo.setValue(anInfo);
    });
  }  
  
  /**
   * Create mesh info controls.
   */
  addInfoControls() {
    this._meshInfo = new SCcontrols.TextAreaInput("Info:", "", null, {readOnly: true});
    this._meshInfoExpandableGroup.addControl(this._meshInfo);
  }  
  
  /**
   * Create mesh groups controls.
   */
  addGroupControls() {
    this._meshGroupsInfoCheckbox = new SCcontrols.CheckboxInput("Show groups info", false, (val) => {
      this._selectedMeshPrs.toShowGroupsInfo = val;
      this._scViewer.requestViewUpdate();
    });

    this._meshGroupSelInput = new SCcontrols.DropdownInput("Group", 0, (val) => {
      this.updateSelGroupInfo();
      if (val != null) {
        let aGroupToSelect = new SCwebviewer.SelectionMeshEntity();
        aGroupToSelect.entityId = val;
        aGroupToSelect.entityType = SCwebviewer.MeshEntityType.LabelGroup;
        aGroupToSelect.prsId = this._selectedMeshPrs.id;
        aGroupToSelect.lineIndex = 0;
        this._selectedMesh.setSelectedEntity(aGroupToSelect);
        this._scViewer.requestViewUpdate();
      }
    }, {items: []});

    this._meshGroupVisibleToggle = new SCcontrols.CheckboxInput("Is visible", false, (val) => { this.updateGroup(); });
    this._meshGroupTextToggle    = new SCcontrols.CheckboxInput("Show text",  false, (val) => { this.updateGroup(); });
    this._meshGroupIconsToggle   = new SCcontrols.CheckboxInput("Show icons", false, (val) => { this.updateGroup(); });

    this._meshGroupInfo = new SCcontrols.TextAreaInput("Info:", "", (val) => {});
    this._meshUpdateGroupButton = new SCcontrols.ButtonInput("Update", ()=> { this.updateGroup(); });

    this._meshGroupLoadedIcons = [
      {name: "sc-logo.svg", path: "/images/sc-logo.svg"},
      {name: "lamp.ico", path: "/images/lamp.ico"}
    ];
    this._meshGroupLoadedIcons.append = function(theName, thePath) { this.push({name: theName, path: thePath}); };
    this._meshGroupLoadedIcons.getPath = function(theName) {
      let aRes = this.find(icon => icon.name === theName);
      return aRes !== undefined ? aRes.path : "";
    };
    this._meshGroupLoadedIcons.getNames = function() { return this.map(icon => icon.name).join("\n"); };

    this._meshGroupLoadedIconsLabel = new SCcontrols.TextAreaInput("Available icons", this._meshGroupLoadedIcons.getNames(), null, {readOnly: true});

    this._meshGroupLoadIconButton = new SCcontrols.FileInput("Load icon", (val)=> {
      if (val.files.length != 1) { return; }
      val.files[0].arrayBuffer().then((aBuffer) => {
        const blob = new Blob([aBuffer], { type: val.files[0].type });
        this._meshGroupLoadedIcons.append(val.files[0].name, URL.createObjectURL(blob), this._scViewer);
        this._meshGroupLoadedIconsLabel.value = this._meshGroupLoadedIcons.getNames();
      });
    }, { accept: ".svg, .ico, .png, .jpg"});

    this._meshGroupExpandableGroup.addControls([
      this._meshGroupsInfoCheckbox, this._meshGroupSelInput, this._meshGroupVisibleToggle,
      this._meshGroupTextToggle,    this._meshGroupIconsToggle, this._meshGroupInfo,
      this._meshUpdateGroupButton,  this._meshGroupLoadedIconsLabel, this._meshGroupLoadIconButton
    ]);
  }  
  
  /**
   * Add mesh animation-related controls.
   */
  addAnimationControls() {
    this._meshTimeStepInput = new SCcontrols.SliderInput("Time Step", 0, (val) => {
      this._selectedMesh.activeTimeStep = val;
      this._scViewer.requestViewUpdate();
    }, {values: []});

    this._meshTimeAnimationToggleButton = new SCcontrols.ButtonInput("Start/Stop", () => {
      let anAnimation = this._selectedMesh.animation;
      anAnimation.isActive ? anAnimation.stop() : anAnimation.start();
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._meshAnimationLoopInput = new SCcontrols.CheckboxInput("Loop", false, (val) => {
      let anAnimation = this._selectedMesh.animation;
      anAnimation.isLoopPlayback = !anAnimation.isLoopPlayback;
    }, { inlineBlock: true });

    this._meshAnimationDurationInput = new SCcontrols.SliderInput("Duration", 5.0, (val) => {
      this._selectedMesh.animation.duration = val;
    }, {min: 1.0, max: 10.0, step: 0.1});

    this._meshAnimationUniformStepsInput = new SCcontrols.CheckboxInput("Uniform steps", false, (val) => {
      let anAnimation = this._selectedMesh.animation;
      anAnimation.uniformSteps = !anAnimation.uniformSteps;
    });

    this._meshAnimationExpandableGroup.addControls([
      this._meshTimeStepInput,
      this._meshTimeAnimationToggleButton,
      this._meshAnimationLoopInput,
      this._meshAnimationDurationInput,
      this._meshAnimationUniformStepsInput
    ]);

    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onMeshAnimationChanged, (theEvent) => {
      if (this._selectedMesh == null || this._selectedMesh !== theEvent.mesh) {
        return;
      }

      if (!theEvent.isChanged) {
        return;
      }

      let anIndex = 0;
      for (let aTimeStep of this._meshTimeStepInput.options.values) {
        if (theEvent.activeTimeStep === aTimeStep) {
          break;
        }
        anIndex++;
      }
      this._meshTimeStepInput.value = anIndex;
    });
  }

  /**
   * Update on object changed.
   */
  updateControls() {
    let aMeshObj = this._selectedMesh;
    this._meshListInput.setValue (aMeshObj == null ? "" : aMeshObj.id);
    this._meshGroup.setVisibility(aMeshObj != null);
    if (aMeshObj == null) {
      return;
    }

    this._currentIdControl.setValue(aMeshObj.id);
    this._visibleControl.setValue(aMeshObj.isVisible);

    this.updatePresentationControls();
    this.updatePrsRelatedControls();

    this._meshSelModeInput.setValue(aMeshObj.selectionMode);
    this._meshHighlightCheckbox.setValue(aMeshObj.toHighlightEntities);

    let aSelectedEntities = aMeshObj.getSelectedEntities();
    let anInfo = "";
    if (aSelectedEntities != null) {
      if (aSelectedEntities.length > 1) {
        anInfo = "Selected " + aSelectedEntities.length + " entities";
      }
      else if (aSelectedEntities.length == 1) {
        anInfo = this.entityToInfoString(aSelectedEntities[0], true);
      }
    }
    this._subSelectedInfo.setValue(anInfo);

    this._meshInfo.setValue(this._selectedMesh.infoString);
    
    this.updateAnimationControls();
  }

  /**
   * Update presentation-related controls.
   */
  updatePrsRelatedControls() {
    this._meshPrsGroup.setVisibility(this._selectedMeshPrs != null);
    if (this._selectedMeshPrs == null) {
      return;
    }

    let aType = this._selectedMeshPrs.type;
    this._meshScalarPrsGroup.setVisibility(aType == SCwebviewer.CaeMeshPresentationType.Scalar);
    this._meshVectorPrsGroup.setVisibility(aType == SCwebviewer.CaeMeshPresentationType.Vector);
    this._meshStreamlinesPrsGroup.setVisibility(aType == SCwebviewer.CaeMeshPresentationType.Streamlines);

    this._meshDispModeInput.setValue(this._selectedMeshPrs.presentationMode);
    
    this._meshVolumeAsSurfaceCheckbox.setValue(this._selectedMeshPrs.isVolumeAsSurface);

    this.updateActiveQuantityControls();

    this._meshDisp3dCheckbox.setValue(this._selectedMeshPrs.toDisplay3dElements);
    this._meshDisp2dCheckbox.setValue(this._selectedMeshPrs.toDisplay2dElements);
    this._meshDisp0dCheckbox.setValue(this._selectedMeshPrs.toDisplay0dElements);
    this._meshDispCentroidsCheckbox.setValue(this._selectedMeshPrs.toDisplayCentroids);
    this._meshDispNodesCheckbox.setValue(this._selectedMeshPrs.toDisplayNodes);
    this._meshDispFreeNodesCheckbox.setValue(this._selectedMeshPrs.toProcessFreeNodes);
    this._meshDispFreeEdgesCheckbox.setValue(this._selectedMeshPrs.toDisplayFreeEdges);
    this._meshDispSharpEdgesCheckbox.setValue(this._selectedMeshPrs.toDisplaySharpEdges);
    this._meshMinMaxCheckbox.setValue(this._selectedMeshPrs.toShowMinMax);
    
    this.updateMeshDeformedControl();

    this.updateMeshVectorControl();
    this.updateMeshStreamlinesControl();
    
    this._meshPrsVisibleCheckbox.setValue(this._selectedMeshPrs.isVisible);
    if (this._selectedMeshPrs.shadingColor.length >= 7) {
      this._meshShadingColorInput.setValue(this._selectedMeshPrs.shadingColor.slice(0, 7));
    }
    if (this._selectedMeshPrs.edgesColor.length >= 7) {
      this._meshEdgesColorInput.setValue(this._selectedMeshPrs.edgesColor.slice(0, 7));
    }
    if (this._selectedMeshPrs.pointsColor.length >= 7) {
      this._meshPointsColorInput.setValue(this._selectedMeshPrs.pointsColor.slice(0, 7));
    }
    this._meshEdgesWidthSlider.setValue(this._selectedMeshPrs.edgesWidth);
    this._meshPointsSizeSlider.setValue(this._selectedMeshPrs.pointsSize);
    this._meshShadingTranspSlider.setValue(this._selectedMeshPrs.shadingTransparency);
    
    this.updateClippingControls();
    this.updateSliceControls();
    this.updateIsoControls();
    this.updateBlocksControls();

    this.updateGroupControls();
  }

  /**
   * Update active quanity controls.
   */
  updatePresentationControls() {
    let anActivePrs = this._meshPresentationsInput.value;
    this._meshPresentationsInput.options.items = [];
    const aPresentations = this._selectedMesh.presentations;
    if (aPresentations.length == 0) {
      this._selectedMeshPrs = null;
      this._meshPresentationsInput.updateValues();
      return;
    }

    let anIndex = 0;
    for (let aPrs of aPresentations) {
      this._meshPresentationsInput.options.items.push({name: aPrs.id, value: aPrs.id});
      if (anActivePrs === aPrs.id) {
        anIndex = this._meshPresentationsInput.options.items.length - 1;
      }
    }
    this._selectedMeshPrs = aPresentations[anIndex];
    this._meshPresentationsInput.updateValues();
    this._meshPresentationsInput.setValue(this._meshPresentationsInput.options.items[anIndex].value);
  }

  /**
   * Update active quanity controls.
   */
  updateActiveQuantityControls() {
    this.updateMeshQuantities();

    let aColorScales = this._selectedMesh.colorScaleObjects;
    if (aColorScales.length > 0) {
      this._meshColorScaleVisCheckbox.setValue(aColorScales[0].isVisible);
    }
  }

  /**
   * Update mesh quantities.
   */
  updateMeshQuantities() {
    this._meshQuantValInput.options.items = [{name: "None", value: ""}];
    let aProperties = this._selectedMesh.properties;
    for (let aProperty of aProperties) {
      let aPropertyName = aProperty.name;
      let aPropertyDesc = aProperty.description;
      let isElemental = aProperty.isElemental;
      this._meshQuantValInput.options.items.push({name: aPropertyDesc !== "" ? aPropertyDesc : aPropertyName,
                                                  value: aProperty.propId,
                                                  group: isElemental ? "Elemental" : "Nodal"});
    }
    this._meshQuantValInput.updateValues();
    this._meshQuantValInput.setValue(this._selectedMeshPrs.activeProperty);

    this.updateComponentControl();
  }

  /**
   * Update mesh quantity components.
   */
  updateComponentControl() {
    this._meshComponentInput.options.items = [];
    const aProperty = this._selectedMeshPrs.activeProperty;
    if (aProperty === "") {
      this._meshComponentInput.updateValues();
      this._meshComponentInput.setVisibility(false);
      return;
    }

    this._meshComponentInput.setVisibility(!this._selectedMesh.isPropertySingleScalar(aProperty));

    const anAvailableComponents = this._selectedMesh.property(aProperty)?.comps ?? [];
    for (let aComponent of anAvailableComponents) {
      this._meshComponentInput.options.items.push({name: aComponent.name, value: aComponent.index});
    }
    this._meshComponentInput.updateValues();
    this._meshComponentInput.setValue(this._selectedMeshPrs.activeComponent);
  }

  /**
   * Update clipping controls.
   */
  updateClippingControls() {
    let aClippingPlane = this._selectedMeshPrs.clipPlane;
    this._meshClippingToggle.setValue(aClippingPlane != null);
    this._meshClippingGroup.setVisibility(this._meshClippingToggle.value);

    if (aClippingPlane != null) {
      if (aClippingPlane.x == 1.0) this._meshClippingDir.setValue("+X");
      else if (aClippingPlane.x == -1.0) this._meshClippingDir.setValue("-X");
      else if (aClippingPlane.y == 1.0) this._meshClippingDir.setValue("+Y");
      else if (aClippingPlane.y == -1.0) this._meshClippingDir.setValue("-Y");
      else if (aClippingPlane.z == 1.0) this._meshClippingDir.setValue("+Z");
      else if (aClippingPlane.z == -1.0) this._meshClippingDir.setValue("-Z");
      else throw("Incorrect clipping plane!");
    }

    this.updateClippingMinMax();

    if (aClippingPlane != null) {
      let aDirValue = this._meshClippingDir.value;
      this._meshClippingDist.setValue(aClippingPlane.w * (aDirValue === "-X" || aDirValue === "-Y" || aDirValue === "-Z" ? 1.0 : -1.0));
    }

    this._meshClippingCutToggle.setValue(this._selectedMeshPrs.toCutOnClipPlane);
  }

  /**
   * Update slice controls.
   */
  updateSliceControls() {
    let aSlicePlanes = this._selectedMeshPrs.slicePlanes;
    this._meshSliceToggle.setValue(aSlicePlanes.length > 0);
    this._meshSliceGroup.setVisibility(this._meshSliceToggle.value);

    let aDir = "X";
    let aNum = 1;
    let aMin = 0.0;
    let aMax = 1.0;
    if (aSlicePlanes.length > 0) {
      const aBox = this._selectedMesh.boundingBox;
      let aDistMin, aDistMax;
      if (aSlicePlanes[0].x == 1.0) { aDir = "X"; aDistMin = aBox.cornerMin.x; aDistMax = aBox.cornerMax.x; }
      else if (aSlicePlanes[0].y == 1.0) { aDir = "Y"; aDistMin = aBox.cornerMin.y; aDistMax = aBox.cornerMax.y; }
      else if (aSlicePlanes[0].z == 1.0) { aDir = "Z"; aDistMin = aBox.cornerMin.z; aDistMax = aBox.cornerMax.z; }
      else throw("Incorrect slice plane!");

      aNum = aSlicePlanes.length;

      let aBoundaryDist;
      if (aSlicePlanes.length == 1) {
        aBoundaryDist = Math.min(-aSlicePlanes[0].w - aDistMin, aDistMax + aSlicePlanes[0].w);
      }
      else {
        aBoundaryDist = -aSlicePlanes[1].w + aSlicePlanes[0].w;
      }
      if (aDistMax - aDistMin > 0.0000001) {
        aMin = (-aSlicePlanes[0].w - aBoundaryDist - aDistMin) / (aDistMax - aDistMin);
        aMax = (-aSlicePlanes[aSlicePlanes.length - 1].w + aBoundaryDist - aDistMin) / (aDistMax - aDistMin);
      }
    }

    this._meshSliceDir.setValue(aDir);
    this._meshSliceNumber.setValue(aNum);
    this._meshSliceMin.setValue(aMin);
    this._meshSliceMax.setValue(aMax);
  }

  /**
   * Update iso-surface controls.
   */
  updateIsoControls() {
    const anIsoProp = this._selectedMeshPrs.isoProperty;

    this._meshIsoGroup.setVisibility(anIsoProp.id !== "" && anIsoProp.index >= 0);

    this._meshIsoPropInput.options.items = [{name: "None", value: ""}];
    let aProperties = this._selectedMesh.properties;
    for (let aProperty of aProperties) {
      let aPropertyName = aProperty.name;
      let aPropertyDesc = aProperty.description;
      let isElemental = aProperty.isElemental;
      this._meshIsoPropInput.options.items.push({name: aPropertyDesc !== "" ? aPropertyDesc : aPropertyName,
                                                 value: aProperty.propId,
                                                 group: isElemental ? "Elemental" : "Nodal"});
    }
    this._meshIsoPropInput.updateValues();
    this._meshIsoPropInput.setValue(anIsoProp.id);

    this.updateIsoComponentControl();

    this.updateIsoValuesControls();
  }

  /**
   * Update iso property components.
   */
  updateIsoComponentControl() {
    this._meshIsoComponentInput.options.items = [];
    const aProperty = this._selectedMeshPrs.isoProperty;
    if (aProperty.id === "") {
      this._meshIsoComponentInput.updateValues();
      this._meshIsoComponentInput.setVisibility(false);
      return;
    }

    this._meshIsoComponentInput.setVisibility(!this._selectedMesh.isPropertySingleScalar(aProperty.id));

    const anAvailableComponents = this._selectedMesh.property(aProperty.id)?.comps ?? [];
    for (let aComponent of anAvailableComponents) {
      this._meshIsoComponentInput.options.items.push({name: aComponent.name, value: aComponent.index});
    }
    this._meshIsoComponentInput.updateValues();
    this._meshIsoComponentInput.setValue(aProperty.index);
  }

  /**
   * Update iso-surface values controls.
   */
  updateIsoValuesControls() {
    const anIsoProp = this._selectedMeshPrs.isoProperty;

    this._meshIsoGroup.setVisibility(anIsoProp.id !== "");

    if (anIsoProp.id === "") {
      return;
    }

    const anIsoValues = this._selectedMeshPrs.isoValues;

    let aValuesRange = this._selectedMesh.getPropertyValueRange(anIsoProp.id, anIsoProp.index);
    this._meshIsoMin.options.min = aValuesRange.x;
    this._meshIsoMin.options.max = aValuesRange.y;
    this._meshIsoMin.options.step = (aValuesRange.y - aValuesRange.x) / 100000.0;
    
    const aDispUnits = this._selectedMesh.getPropertyDisplayUnits(anIsoProp.id, anIsoProp.index);
    if (aDispUnits !== "") {
      this._meshIsoMin.options.min = this._scViewer.unitManager.convertAnyFromSI(this._meshIsoMin.options.min, aDispUnits);
      this._meshIsoMin.options.max = this._scViewer.unitManager.convertAnyFromSI(this._meshIsoMin.options.max, aDispUnits);
      this._meshIsoMin.options.step = this._scViewer.unitManager.convertAnyFromSI(this._meshIsoMin.options.step, aDispUnits);
    }

    this._meshIsoMin.updateValues();
    this._meshIsoMax.options.min = this._meshIsoMin.options.min;
    this._meshIsoMax.options.max = this._meshIsoMin.options.max;
    this._meshIsoMax.options.step = this._meshIsoMin.options.step;
    this._meshIsoMax.updateValues();

    let aMinValue = this._meshIsoMin.options.min;
    let aMaxValue = this._meshIsoMax.options.max;
    let aNbValues = 1;
    
    if (anIsoValues.length != 0) {
      let aBoundaryDist;
      if (anIsoValues.length == 1) {
        aBoundaryDist = Math.min(anIsoValues[0] - aValuesRange.x, aValuesRange.y - anIsoValues[0]);
      } else {
        aBoundaryDist = anIsoValues[1] - anIsoValues[0];
      }
      aMinValue = anIsoValues[0] - aBoundaryDist;
      aMaxValue = anIsoValues[anIsoValues.length - 1] + aBoundaryDist;
      aNbValues = anIsoValues.length;

      if (aDispUnits !== "") {
        aMinValue = this._scViewer.unitManager.convertAnyFromSI(aMinValue, aDispUnits);
        aMaxValue = this._scViewer.unitManager.convertAnyFromSI(aMaxValue, aDispUnits);
      }
    }

    this._meshIsoMin.setValue(aMinValue);
    this._meshIsoMax.setValue(aMaxValue);
    this._meshIsoNumber.setValue(aNbValues);
  }

  /**
   * Update mesh deformed controls.
   */
  updateMeshDeformedControl() {
    this._meshDeformInput.options.items = [{name: "None", value: ""}];
    let availableDefProps = this._selectedMesh.availableVec3Props;
    for (let prop of availableDefProps) {
      if (!this._selectedMesh.isPropertyElemental(prop.id)) {
        this._meshDeformInput.options.items.push ({name: prop.name, value: prop.id});
      }
    }
    this._meshDeformInput.updateValues();
    let anActiveDefProp = this._selectedMeshPrs.activeDeformedProp;
    this._meshDeformInput.setValue(anActiveDefProp);
    this.onMeshDeformedPropChanged (anActiveDefProp);
  }

  /**
   * Handle mesh deformed property changes.
   */
  onMeshDeformedPropChanged(defProp) {
    if (defProp === "") {
      return;
    }

    this._meshDeformScaleCoeffSlider.setValue (this._selectedMeshPrs.deformedScaleCoeff);
  }

  /**
   * Update mesh blocks controls.
   */
  updateBlocksControls() {
    const aMeshBlocks = this._selectedMesh.blocks;

    this._meshBlocksTreeViewInput.options.tree = aMeshBlocks;
    this._meshBlocksTreeViewInput.updateValues();

    const aVisBlocks = this._selectedMeshPrs.getVisibleBlocks();

    let processBlock = (theBlock, theNamesChain) => {
      if (theBlock.name === undefined) {
        return;
      }

      let aChain = [...theNamesChain];
      aChain.push(theBlock.name);

      if (theBlock.children === undefined) {
        this._meshBlocksTreeViewInput.setEnabled(aChain, true);
        return;
      }

      for (let aChild of theBlock.children) {
        processBlock(aChild, aChain);
      }
    };

    processBlock(aVisBlocks, []);
  }

  /**
   * Update mesh vector controls.
   */
  updateMeshVectorControl() {
    this._meshVecPropInput.options.items = [{name: "None", value: ""}];
    let availableVecProps = this._selectedMesh.availableVec3Props;
    for (let prop of availableVecProps) {
      this._meshVecPropInput.options.items.push ({name: prop.name, value: prop.id});
    }
    this._meshVecPropInput.updateValues();
    let anActiveVecProp = this._selectedMeshPrs.activeVecProp;
    this._meshVecPropInput.setValue(anActiveVecProp);
    this.onMeshVecPropChanged(anActiveVecProp);

    this._meshVecPropDistrTypeInput.setValue(this._selectedMeshPrs.vecPropDistrType);
    this._meshVecSampleRatioSlider.setValue(this._selectedMeshPrs.vecSamplingRatio);
  }

  /**
   * Handle mesh vector property changes.
   */
  onMeshVecPropChanged(theProp) {
    if (theProp === "") {
      return;
    }

    this._meshVecPropScaleCoeffSlider.setValue(this._selectedMeshPrs.vecPropScaleCoeff);
  }

  /**
   * Update mesh streamlines controls.
   */
  updateMeshStreamlinesControl() {
    this._meshStreamlinesPropInput.options.items = [{name: "None", value: ""}];
    const availableVecProps = this._selectedMesh.availableVec3Props;
    for (let prop of availableVecProps) {
      if (!this._selectedMesh.isPropertyElemental(prop.id)) {
        this._meshStreamlinesPropInput.options.items.push ({name: prop.name, value: prop.id});
      }
    }
    this._meshStreamlinesPropInput.updateValues();

    const aStreamlines = this._selectedMeshPrs.streamlines;
    this._meshStreamlinesPropInput.setValue(aStreamlines.property);
    this._meshStreamlinesRegularToggle.setValue(aStreamlines.isRegular);
    this._meshNbStreamlinesSlider.setValue(aStreamlines.nbStreamlines);
    this._meshUStreamlinesSlider.setValue(aStreamlines.uvStreamlines.x);
    this._meshVStreamlinesSlider.setValue(aStreamlines.uvStreamlines.y);
    this._meshStreamlinesStepCoeffSlider.setValue(aStreamlines.stepCoeff);
    this._meshStreamlinesLengthCoeffSlider.setValue(aStreamlines.lengthCoeff);
    this._meshStreamlinesThicknessSlider.setValue(aStreamlines.thickness);
    this._meshStreamlinesAnimToggle.setValue(aStreamlines.animationEnabled);
    this._meshStreamlinesAnimSpeedSlider.setValue(aStreamlines.animationSpeed);
    this._meshStreamlinesAnimTrailSlider.setValue(aStreamlines.animationTrail);
    this._meshStreamlinesAnimFreqSlider.setValue(aStreamlines.animationFrequency);

    const aBox = this._selectedMesh.boundingBox;
    const aBoxMin = aBox.cornerMin;
    const aBoxMax = aBox.cornerMax;
    const aBoxSize = SCwebviewer.Vec3.subtract(aBoxMax, aBoxMin);

    let aCoordOrigin, aCoordSide, aCoordUp;

    let aDir = SCwebviewer.Vec3.normalize(SCwebviewer.Vec3.cross(aStreamlines.planeSide, aStreamlines.planeUp));
    if (Math.abs(aDir.x) > 0.999) {
      this._meshStreamlinesPlaneDirInput.value = "X";
      aCoordOrigin = "x";
      aCoordSide = "y";
      aCoordUp = "z";
    } else if (Math.abs(aDir.y) > 0.999) {
      this._meshStreamlinesPlaneDirInput.value = "Y";
      aCoordOrigin = "y";
      aCoordSide = "z";
      aCoordUp = "x";
    } else {
      this._meshStreamlinesPlaneDirInput.value = "Z";
      aCoordOrigin = "z";
      aCoordSide = "x";
      aCoordUp = "y";
    }

    const aTol = 0.00001;

    let anOriginRel = aBoxSize[aCoordOrigin] > aTol ?
      (aStreamlines.planeOrigin[aCoordOrigin] - aBoxMin[aCoordOrigin]) / aBoxSize[aCoordOrigin] : 0.0;
    this._meshStreamlinesPlaneOriginSlider.value = anOriginRel;

    let aSideLeft = 0.0;
    let aSideRight = 1.0;
    if (aBoxSize[aCoordSide] > aTol) {
      aSideLeft = (aStreamlines.planeOrigin[aCoordSide] - aStreamlines.planeSide[aCoordSide] - aBoxMin[aCoordSide]) / aBoxSize[aCoordSide];
      aSideRight = (aStreamlines.planeOrigin[aCoordSide] + aStreamlines.planeSide[aCoordSide] - aBoxMin[aCoordSide]) / aBoxSize[aCoordSide];
    }
    this._meshStreamlinesPlaneSideBoundsSlider.value = {left: aSideLeft, right: aSideRight};

    let anUpLeft = 0.0;
    let anUpRight = 1.0;
    if (aBoxSize[aCoordUp] > aTol) {
      anUpLeft = (aStreamlines.planeOrigin[aCoordUp] - aStreamlines.planeUp[aCoordUp] - aBoxMin[aCoordUp]) / aBoxSize[aCoordUp];
      anUpRight = (aStreamlines.planeOrigin[aCoordUp] + aStreamlines.planeUp[aCoordUp] - aBoxMin[aCoordUp]) / aBoxSize[aCoordUp];
    }
    this._meshStreamlinesPlaneUpBoundsSlider.value = {left: anUpLeft, right: anUpRight};
  }

  /**
   * Update mesh groups controls.
   */
  updateGroupControls() {
    this._meshGroupsInfoCheckbox.setValue(this._selectedMeshPrs.toShowGroupsInfo);

    const aProp = this._selectedMeshPrs.activeProperty;

    this._meshGroupSelInput.options.items = [];
    if (aProp !== "" && this._selectedMesh.isPropertyGroup(aProp)) {
      const aUsedGroups = this._selectedMesh.usedGroups(aProp);
      for (let aGroup of aUsedGroups) {
        this._meshGroupSelInput.options.items.push({name: aGroup[1], value: aGroup[0]});
      }
    }
    this._meshGroupSelInput.updateValues();

    const aSelectedEntities = this._selectedMesh.getSelectedEntities();
    if (aSelectedEntities != null
      && aSelectedEntities.length == 1
      && aSelectedEntities[0].entityType == SCwebviewer.MeshEntityType.LabelGroup
      && aSelectedEntities[0].prsId === this._selectedMeshPrs.id) {
        this._meshGroupSelInput.value = aSelectedEntities[0].entityId;
      }

    this.updateSelGroupInfo();
  }

  /**
   * Update mesh group info controls.
   */
  updateSelGroupInfo() {
    let anInfoStr = [];

    const aSelGroup = this._meshGroupSelInput.value;
    
    if (aSelGroup != null) {
      const anInfo = this._selectedMeshPrs.getGroupInfo(aSelGroup);
      
      for (let aData of anInfo.data) {
        anInfoStr.push(aData.text + ";" + aData.icon);
      }

      this._meshGroupVisibleToggle.setValue(anInfo.isVisible);
      this._meshGroupTextToggle.setValue(anInfo.toShowText);
      this._meshGroupIconsToggle.setValue(anInfo.toShowIcons);
    }
    
    this._meshGroupInfo.setValue(anInfoStr.join("\n"));
  }

  /**
   * Update mesh group annotation.
   */
   async updateGroup() {
    const aSelGroup = this._meshGroupSelInput.value;
    if (aSelGroup == null) {
      return;
    }

    let anInfo = this._selectedMeshPrs.getGroupInfo(aSelGroup);
    anInfo.isVisible = this._meshGroupVisibleToggle.value;
    anInfo.toShowText = this._meshGroupTextToggle.value;
    anInfo.toShowIcons = this._meshGroupIconsToggle.value;

    anInfo.data = [];
    let aStrings = this._meshGroupInfo.value.split("\n");
    for (let aData of aStrings) {
      const aText = aData.split(";")[0];
      let anIcon = aData.split(";")[1];
      const anIconPath = this._meshGroupLoadedIcons.getPath(anIcon);
      if (anIcon && !this._scViewer.isImageRegistered(anIcon)) {
        try {
          await this._scViewer.registerImageFromUrl(anIconPath, anIcon);
        } catch (err) {
          console.error("Cannot find icon", anIcon);
          anIcon = "";
        }
      }
      anInfo.data.push({text: aText, icon: anIcon});
    }

    this._selectedMeshPrs.setGroupInfo(aSelGroup, anInfo);
    
    this._scViewer.requestViewUpdate();
  }
  
  /**
   * Update mesh animation-related controls.
   */
  updateAnimationControls() {
    const anActiveTimeStep = this._meshTimeStepInput.value;
    this._meshTimeStepInput.options.values = [];
    const aTimeSteps = this._selectedMesh.availableTimeSteps;
    if (aTimeSteps.length == 0) {
      this._meshTimeStepInput.updateValues();
      return;
    }

    let anIndex = 0;
    for (let aTimeStep of aTimeSteps) {
      this._meshTimeStepInput.options.values.push(aTimeStep.timeStep);
      if (anActiveTimeStep === aTimeStep.timeStep) {
        anIndex = this._meshTimeStepInput.options.values.length - 1;
      }
    }
    
    this._selectedMesh.activeTimeStep = aTimeSteps[anIndex].timeStep;
    this._meshTimeStepInput.updateValues();
    this._meshTimeStepInput.setValue(anIndex);

    let anAnimation = this._selectedMesh.animation;
    
    this._meshAnimationDurationInput.value = anAnimation.duration;
    this._meshAnimationUniformStepsInput.value = anAnimation.uniformSteps;
    this._meshAnimationLoopInput.value = anAnimation.isLoopPlayback;
  }
}
