import * as SCwebviewer from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import * as SCcontrols from './../SCcontrols.js';

/**
 * Panel for displaying Shape object properties.
 */
export class ShapePanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Shapes");

    this._scViewer = theSample.scViewer;
    this._scSample = theSample;
    this._selectedShape = null;

    this._lenInputs = [];
    this._angInputs = [];

    this.addShapeSelector();

    this._shapePropsGroup = new SCcontrols.GroupInput("PropertiesGroup");
    this._shapePropsGroup.minWidth = 420;
    this.addControl(this._shapePropsGroup);

    this.addCurrentShapeControls();

    this._dispModesExpandableGroup = new SCcontrols.GroupInput("Display options", true, false);
    this._shapePropsGroup.addControl(this._dispModesExpandableGroup);
    this.addDisplayModeControls();

    this._prsAspectsExpandableGroup = new SCcontrols.GroupInput("Presentation aspects", true, true);
    this._shapePropsGroup.addControl(this._prsAspectsExpandableGroup);
    this.addColorControls();
    this.addPbrControls();

    this._selModesExpandableGroup = new SCcontrols.GroupInput("Selection mode", true, false);
    this._shapePropsGroup.addControl(this._selModesExpandableGroup);
    this.addSelectionControls();

    this._subSelectedInfoExpandableGroup = new SCcontrols.GroupInput("Subshapes selected", true, true);
    this._subDetectedInfoExpandableGroup = new SCcontrols.GroupInput("Subshapes detected", true, true);
    this._subSelectedInfoExpandableGroup.onCollapsedChanged = (theToCollpase) => { if (!theToCollpase) { this.updateControls(); } }
    this._subDetectedInfoExpandableGroup.onCollapsedChanged = (theToCollpase) => { if (!theToCollpase) { this.updateControls(); } }
    this._shapePropsGroup.addControls([this._subSelectedInfoExpandableGroup, this._subDetectedInfoExpandableGroup]);
    this.addPickingControls();

    this._trsfLocExpGroup   = new SCcontrols.GroupInput("Interactive transformation", true, true);
    this._trsfShapeExpGroup = new SCcontrols.GroupInput("Shape root location", true, true);
    this._shapePropsGroup.addControls([this._trsfLocExpGroup, this._trsfShapeExpGroup]);
    this.addTrsfControls();

    this._shapeInfoExpandableGroup = new SCcontrols.GroupInput("Info", true, true);
    this._shapeInfoExpandableGroup.onCollapsedChanged = (theToCollpase) => { if (!theToCollpase) { this.updateControls(); } }
    this._shapePropsGroup.addControl(this._shapeInfoExpandableGroup);
    this.addInfoControls();

    // add event handlers
    window.addEventListener("sceneChanged", e => {
      this._selectedShape = null;
      this.fillObjects();
      this.updateControls();
    });

    this.update = this.updateControls;

    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onUnitsChanged, e => {
      this._updateUnits();
    });

    this._onSelectionChanged = e => {
      this._selectedShape = this._scViewer.selectionManager.oneSelectedObject;
      if (this._selectedShape == null || !(this._selectedShape instanceof SCwebviewer.InteractiveShape)) {
        this._selectedShape = null;
      }
      this.updateControls();
    };
  }

  /**
   * Panel activation.
   */
  onActivePanelChanged(theIsActivated) {
    super.onActivePanelChanged(theIsActivated);
    if (theIsActivated) {
      this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onSelectedObjectsChanged, this._onSelectionChanged);
      this._updateUnits();
      this._onSelectionChanged(null);
    } else {
      this._scViewer.eventManager.removeEventListener(SCwebviewer.UserEvents.onSelectedObjectsChanged, this._onSelectionChanged);
    }
  }

  /**
   * Fill shapes list
   */
  fillObjects() {
    this._shapeListInput.options.items = this._scViewer.interactiveContext.objectsInside
      .filter((theElem) => {return (theElem instanceof SCwebviewer.InteractiveShape)})
      .map(elem => { return {name: elem.id, value: elem.id} });
      this._shapeListInput.options.items.unshift({name: "None", value: ""});
      this._shapeListInput.updateValues();
  }

  /**
   * Add shape object selector control.
   */
  addShapeSelector() {
    this._shapeListInput = new SCcontrols.DropdownInput("Shape", "",
      (object) => {
        // workaround for non-selected objects
        this._selectedShape = this._scViewer.interactiveContext.findInteractiveObject(object);
        if (this._selectedShape == null || !(this._selectedShape instanceof SCwebviewer.InteractiveShape)) {
          this._selectedShape = null;
        }
        this._scViewer.selectionManager.oneSelectedObject = this._selectedShape;
        this.updateControls();
        this._scViewer.requestViewUpdate();
      },
      {items: [{name: "None", value: ""}]}
    );

    this.addControl(this._shapeListInput);
  }

  /**
   * Add current shape controls.
   */
  addCurrentShapeControls() {
    this._currentIdInput = new SCcontrols.TextInput("Current ID:", "", null, {readOnly: true});

    this._visibleCheckbox = new SCcontrols.CheckboxInput("Visible", false, (val) => {
      this._selectedShape.isVisible = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._shapeRemoveButton = new SCcontrols.ButtonInput("Remove", () => {
      if (this._selectedShape != null) {
        this._selectedShape.remove();
        this._selectedShape = null;
        this._scViewer.requestViewUpdate();
        this.updateControls();
        this.fillObjects();
      }
    }, { inlineBlock: true });

    this._shapeExplodeButton = new SCcontrols.ButtonInput("Explode", () => {
      let anExplodedObjects = this._selectedShape.explodeShape(this._scViewer, this._scSample.shapeExplodeDepth);
      if (anExplodedObjects.length !== 0) {
        this._scViewer.requestViewUpdate();
        this._selectedShape = null;
        window.dispatchEvent(new Event('sceneChanged'));
      }
    }, { inlineBlock: true });

    this._shapeExplodeDepthInput = new SCcontrols.NumberInput("Depth", this._scSample.shapeExplodeDepth, (val) => {
      this._scSample.shapeExplodeDepth = val;
    }, { inlineBlock: true, min: -1, max: 1000, step: 1 });

    this._shapePropsGroup.addControls([
      this._currentIdInput, this._visibleCheckbox,
      this._shapeRemoveButton, this._shapeExplodeButton,
      this._shapeExplodeDepthInput]
    );
  }

  /**
   * Add presentation modes controls.
   */
  addDisplayModeControls() {
    this._shapeDispModeInput = new SCcontrols.DropdownInput("Display mode", SCwebviewer.ShapeObjectPrsMode.Wireframe, (mode) => {
      this._selectedShape.presentationMode = mode;
      this._scViewer.requestViewUpdate();
    }, {items: [
      {name: "Wireframe", value: SCwebviewer.ShapeObjectPrsMode.Wireframe},
      {name: "Shaded", value: SCwebviewer.ShapeObjectPrsMode.Shaded},
      {name: "Shaded (face boundaries)", value: SCwebviewer.ShapeObjectPrsMode.ShadedFaceBoundaries},
      {name: "Shaded (flat)", value: SCwebviewer.ShapeObjectPrsMode.ShadedFlat},
      {name: "Shaded (tessellation)", value: SCwebviewer.ShapeObjectPrsMode.ShadedTessellation},
      {name: "Hidden Line", value: SCwebviewer.ShapeObjectPrsMode.HiddenLine},
    ]});

    this._shapePriorityModifierInput = new SCcontrols.NumberInput("Priority modifier", 0, (val) => {
      this._selectedShape.priorityModifier = val;
      this._scViewer.requestViewUpdate();
    }, { min: -5, max: 5, step: 1});

    this._dispModesExpandableGroup.addControls([this._shapeDispModeInput, this._shapePriorityModifierInput]);
  }

  /**
   * Add base color controls.
   */
  addColorControls() {
    this._shapeLineTypeInput = new SCcontrols.DropdownInput("Line Type", SCwebviewer.TypeOfLine.SOLID, (mode) => {
      this._selectedShape.aspects.lineType = mode;
      this._scViewer.requestViewUpdate();
    }, {items: [
      {name: "SOLID", value: SCwebviewer.TypeOfLine.SOLID},
      {name: "DASH", value: SCwebviewer.TypeOfLine.DASH},
      {name: "DOT", value: SCwebviewer.TypeOfLine.DOT},
      {name: "DOTDASH", value: SCwebviewer.TypeOfLine.DOTDASH}
    ]});

    this._shapeLineWidthSlider = new SCcontrols.SliderInput("Line Width", 1.0, (val) => {
      this._selectedShape.aspects.lineWidth = val;
      this._scViewer.requestViewUpdate();
    }, { min: 1.0, max: 10.0, step: 1.0 });

    this._shapeForcedColorInput = new SCcontrols.ColorInput("Forced color", "#000000", (val)=> {
      this._shapeForcedColorCheckbox.setValue(true);
      this._selectedShape.forcedColor = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });
    this._shapeForcedColorCheckbox = new SCcontrols.CheckboxInput("Set forced", false, (val) => {
      this._selectedShape.forcedColor = val ? this._shapeForcedColorInput.value : "";
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._prsAspectsExpandableGroup.addControls([
      this._shapeForcedColorCheckbox, this._shapeForcedColorInput,
      this._shapeLineTypeInput, this._shapeLineWidthSlider
    ]);
  }

  /**
   * Add PBR material properties controls.
   */
  addPbrControls() {
    this._shapeHasMaterialCheckbox = new SCcontrols.CheckboxInput("Own Material", false, (val) => {
      let aMat = new SCwebviewer.VisMaterial();
      aMat.isDefined = false;
      if (val) {
        aMat = this._selectedShape.material;
        aMat.isDefined = true;
        let anRgba = SCwebviewer.ColorTools.convertColorStringToRgba(this._shapeBaseColorInput.value);
        anRgba.a = (1.0 - this._shapeTranspRange.value) * 255.0;
        aMat.baseColor = SCwebviewer.ColorTools.convertRgbaToColorString(anRgba);
        aMat.metallic  = this._shapeMetalSlider.value;
        aMat.roughness = this._shapeRoughSlider.value;
      }
      this._selectedShape.material = aMat;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._shapeBaseColorInput = new SCcontrols.ColorInput("Base color", "#000000", (val)=> {
      this._shapeHasMaterialCheckbox.setValue(true);
      this._selectedShape.baseColor = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._shapeTranspRange = new SCcontrols.SliderInput("Transparency", 0.0, (val) => {
      this._shapeHasMaterialCheckbox.setValue(true);
      this._selectedShape.transparency = val;
      this._scViewer.requestViewUpdate();
    }, { min: 0.0, max: 0.69, step: 0.01,
         format: (num)=>{ return (100.0 * num).toFixed() + "%"; }
    });

    this._shapeMetalSlider = new SCcontrols.SliderInput("Metallic", 0.0, (val) => {
      this._shapeHasMaterialCheckbox.setValue(true);
      this._selectedShape.materialMetallic = val;
      this._scViewer.requestViewUpdate();
    }, { min: 0.0, max: 1.0, step: 0.01,
         format: (num)=>{ return (100.0 * num).toFixed() + "%"; }
    });

    this._shapeRoughSlider = new SCcontrols.SliderInput("Roughness", 0.0, (val) => {
      this._shapeHasMaterialCheckbox.setValue(true);
      this._selectedShape.materialRoughness = val;
      this._scViewer.requestViewUpdate();
    }, { min: 0.0, max: 1.0, step: 0.01,
         format: (num)=>{ return (100.0 * num).toFixed() + "%"; }
    });

    this._prsAspectsExpandableGroup.addControls([
      this._shapeHasMaterialCheckbox, this._shapeBaseColorInput, this._shapeTranspRange,
      this._shapeMetalSlider, this._shapeRoughSlider
    ]);
  }

  /**
   * Create subshape selection checkbox.
   */
  _addSubshapeSelModeCheckbox(theMode) {
    let aCtrl = new SCcontrols.CheckboxInput(SCwebviewer.ShapeObjectSelMode[theMode], false, (val) => {
      if (val) { this._selectedShape.selectionMode |=  theMode;
      } else   { this._selectedShape.selectionMode &= ~theMode; }
      this._updateSelModeInput(this._selectedShape.selectionMode);
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });
    return aCtrl;
  }

  /**
   * Update selection mode controls.
   */
  _updateSelModeInput(theMode) {
    let aNbModes = 0;
    if ((theMode & SCwebviewer.ShapeObjectSelMode.CompSolid) != 0) { ++aNbModes; }
    if ((theMode & SCwebviewer.ShapeObjectSelMode.Solid)  != 0) { ++aNbModes; }
    if ((theMode & SCwebviewer.ShapeObjectSelMode.Shell)  != 0) { ++aNbModes; }
    if ((theMode & SCwebviewer.ShapeObjectSelMode.Face)   != 0) { ++aNbModes; }
    if ((theMode & SCwebviewer.ShapeObjectSelMode.Wire)   != 0) { ++aNbModes; }
    if ((theMode & SCwebviewer.ShapeObjectSelMode.Edge)   != 0) { ++aNbModes; }
    if ((theMode & SCwebviewer.ShapeObjectSelMode.Vertex) != 0) { ++aNbModes; }
    this._shapeSelModeInput.setValue(aNbModes == 1 ? theMode : 0);
  }

  /**
   * Update selection mode controls.
   */
  _updateSelModeCheckboxes(theMode) {
    this._shapeSelCompSolidCheckbox.setValue((theMode & SCwebviewer.ShapeObjectSelMode.CompSolid) != 0);
    this._shapeSelSolidCheckbox.setValue((theMode & SCwebviewer.ShapeObjectSelMode.Solid)  != 0);
    this._shapeSelShellCheckbox.setValue((theMode & SCwebviewer.ShapeObjectSelMode.Shell)  != 0);
    this._shapeSelFaceCheckbox .setValue((theMode & SCwebviewer.ShapeObjectSelMode.Face)   != 0);
    this._shapeSelWireCheckbox .setValue((theMode & SCwebviewer.ShapeObjectSelMode.Wire)   != 0);
    this._shapeSelEdgeCheckbox .setValue((theMode & SCwebviewer.ShapeObjectSelMode.Edge)   != 0);
    this._shapeSelVertCheckbox .setValue((theMode & SCwebviewer.ShapeObjectSelMode.Vertex) != 0);
  }

  /** Clone or dump selected subshapes. */
  _cloneSelectedSubshapes(theToDump) {
    if (this._selectedShape == null) { return; }

    let aCounter = 0;
    let aTypesSubshapes = null;
    for (let aShapeMgr of this._selectedShape.subshapes) {
      if (aShapeMgr.selected.length !== 0) {
        ++aCounter;
        aTypesSubshapes = aShapeMgr;
      }
    }

    let aClone = null;
    if (aCounter === 1) {
      if (theToDump) {
        aTypesSubshapes.dumpSubshapes(aTypesSubshapes.selected);
        return;
      }

      aClone = aTypesSubshapes.cloneSubshapes(aTypesSubshapes.selected);
    } else if (aCounter >= 2) {
      // this API is not expected to be used
      let aSelElems = new SCwebviewer.SelectionShapeElems();
      for (let aShapeMgr of this._selectedShape.subshapes) {
        aSelElems.setIndices(aShapeMgr.shapeType, aShapeMgr.selected);
      }
      if (theToDump) {
        this._selectedShape.dumpSubshapes("dump.brep", aSelElems);
        return;
      }

      aClone = this._selectedShape.cloneSubshapes(aSelElems);
    } else {
      console.error("Nothing to clone");
      return;
    }

    if (aClone === null) { return; }

    aClone.presentationMode = SCwebviewer.ShapeObjectPrsMode.Wireframe;
    aClone.baseColor = "RED";
    aClone.isVisible = true;
    this._scViewer.requestViewUpdate();
    window.dispatchEvent(new Event('sceneChanged'));
  }

  /**
   * Create (sub)shape selection controls.
   */
  addSelectionControls() {
    this._shapeSelModeInput = new SCcontrols.DropdownInput("Selection mode", SCwebviewer.ShapeObjectSelMode.Global, (mode) => {
      this._selectedShape.selectionMode = mode;
      this._updateSelModeCheckboxes(mode);
      this._scViewer.requestViewUpdate();
    }, {items: [
      {name: "Global", value: SCwebviewer.ShapeObjectSelMode.Global},
      {name: "CompSolid", value: SCwebviewer.ShapeObjectSelMode.CompSolid},
      {name: "Solid",  value: SCwebviewer.ShapeObjectSelMode.Solid},
      {name: "Shell",  value: SCwebviewer.ShapeObjectSelMode.Shell},
      {name: "Face",   value: SCwebviewer.ShapeObjectSelMode.Face},
      {name: "Wire",   value: SCwebviewer.ShapeObjectSelMode.Wire},
      {name: "Edge",   value: SCwebviewer.ShapeObjectSelMode.Edge},
      {name: "Vertex", value: SCwebviewer.ShapeObjectSelMode.Vertex}
    ], inlineBlock: true
    });

    this._shapeCloneSubshapesButton = new SCcontrols.ButtonInput("Clone selected", () => {
      this._cloneSelectedSubshapes(false);
    }, { inlineBlock: true });
    this._shapeDumpSubshapesButton = new SCcontrols.ButtonInput("Dump selected", () => {
      this._cloneSelectedSubshapes(true);
    }, { inlineBlock: true });

    this._shapeSelModesGroup = new SCcontrols.GroupInput("SelModesGroup");
    this._shapeSelCompSolidCheckbox = this._addSubshapeSelModeCheckbox(SCwebviewer.ShapeObjectSelMode.CompSolid);
    this._shapeSelSolidCheckbox = this._addSubshapeSelModeCheckbox(SCwebviewer.ShapeObjectSelMode.Solid);
    this._shapeSelShellCheckbox = this._addSubshapeSelModeCheckbox(SCwebviewer.ShapeObjectSelMode.Shell);
    this._shapeSelFaceCheckbox  = this._addSubshapeSelModeCheckbox(SCwebviewer.ShapeObjectSelMode.Face);
    this._shapeSelWireCheckbox  = this._addSubshapeSelModeCheckbox(SCwebviewer.ShapeObjectSelMode.Wire);
    this._shapeSelEdgeCheckbox  = this._addSubshapeSelModeCheckbox(SCwebviewer.ShapeObjectSelMode.Edge);
    this._shapeSelVertCheckbox  = this._addSubshapeSelModeCheckbox(SCwebviewer.ShapeObjectSelMode.Vertex);
    this._shapeSelModesGroup.addControls([
      this._shapeSelCompSolidCheckbox,
      this._shapeSelSolidCheckbox, this._shapeSelShellCheckbox, this._shapeSelFaceCheckbox,
      this._shapeSelWireCheckbox,  this._shapeSelEdgeCheckbox,  this._shapeSelVertCheckbox
    ]);

    this._selModesExpandableGroup.addControls([this._shapeSelModeInput, this._shapeCloneSubshapesButton, this._shapeDumpSubshapesButton]);
    this._selModesExpandableGroup.addControl(this._shapeSelModesGroup);

    this.addPickingFilters();
  }

  /**
   * Create (sub)shape selection controls.
   */
  addPickingControls() {
    this._subSelectedInfo = new SCcontrols.TextAreaInput("", "\n\n\n", null, {autoHeight: true, readOnly: true});
    this._subSelectedInfo.minWidth = 350;
    this._subSelectedInfoExpandableGroup.addControl(this._subSelectedInfo);

    this._subDetectedInfo = new SCcontrols.TextAreaInput("", "\n\n\n", null, {autoHeight: true, readOnly: true});
    this._subDetectedInfo.minWidth = 350;
    this._subDetectedInfoExpandableGroup.addControl(this._subDetectedInfo);

    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onDetectedObjectChanged, () => {
      let aShapeObj = this._selectedShape;
      let aDetObj = this._scViewer.selectionManager.lastDetectedObject;

      let anInfo = "";
      if (aShapeObj != null && aShapeObj == aDetObj) {
        const aShapeInfo = aShapeObj.lastDetectedSubshapeInfo;
        if (aShapeInfo.shapeType != SCwebviewer.ShapeEnum.SHAPE) {
          anInfo += SCwebviewer.ShapeInfo.format(aShapeInfo, this._scViewer.unitManager);
        }
        if (anInfo == "") {
          anInfo = "GLOBAL ";
          let aColor = aShapeObj.baseColorRgba;
          if (aColor !== "") { anInfo += "[" + aColor + "]"; }
        }
      } else {
        anInfo = "NONE";
      }
      this._subDetectedInfo.setValue(anInfo);
    });
  }

  /**
   * Create shape info controls.
   */
  addInfoControls() {
    this._shapeInfo = new SCcontrols.TextAreaInput("", "\n\n\n", null, {autoHeight: true, readOnly: true});
    this._shapeInfo.minWidth = 350;
    this._shapeInfoExpandableGroup.addControl(this._shapeInfo);
  }

  /**
   * Set surface/curve type filter.
   */
  _setSurfaceCurveTypeFilter(theShape, theSurfType, theCurveType) {
    let aFaces = [];
    let anEdges = [];
    for (let aFaceIter of theShape.faces.shapeInfo) {
      if (aFaceIter.surfaceType === theSurfType) {
        aFaces.push(aFaceIter.index);
      }
    }
    for (let anEdgeIter of theShape.edges.shapeInfo) {
      if (anEdgeIter.curveType === theCurveType) {
        anEdges.push(anEdgeIter.index);
      }
    }
    theShape.faces.enabled = aFaces;
    theShape.edges.enabled = anEdges;
  }

  /**
   * Set connected subshapes filter.
   */
  _setConnectedFilter(theEvent) {
    if (this._myPrevShapeObj) {
      this._myPrevShapeObj.compSolids.disabled = [];
      this._myPrevShapeObj.solids.disabled = [];
      this._myPrevShapeObj.shells.disabled = [];
      this._myPrevShapeObj.faces.disabled = [];
      this._myPrevShapeObj.wires.disabled = [];
      this._myPrevShapeObj.edges.disabled = [];
      this._myPrevShapeObj.vertices.disabled = [];
    }

    let aShapeObj = this._selectedShape;
    this._myPrevShapeObj = aShapeObj;
    if (!aShapeObj) { return; }

    aShapeObj.compSolids.disabled = [];
    aShapeObj.solids.disabled = [];
    aShapeObj.shells.disabled = [];
    aShapeObj.faces.disabled = [];
    aShapeObj.wires.disabled = [];
    aShapeObj.edges.disabled = [];
    aShapeObj.vertices.disabled = [];
    aShapeObj.faces.enabled = aShapeObj.faces.getConnectedSubshapes(aShapeObj.faces.selected);
    aShapeObj.edges.enabled = aShapeObj.edges.getConnectedSubshapes(aShapeObj.edges.selected);
  }

  /**
   * Create (sub)shape selection filter controls.
   */
  addPickingFilters() {
    this._setConnectedFilter = this._setConnectedFilter.bind(this);
    this._shapeSelFilterInput = new SCcontrols.DropdownInput("Filter", 0, (theFilter) => {
      let aShapeObj = this._selectedShape;
      if (!aShapeObj) { return; }
      aShapeObj.compSolids.disabled = [];
      aShapeObj.solids.disabled = [];
      aShapeObj.shells.disabled = [];
      aShapeObj.faces.disabled = [];
      aShapeObj.wires.disabled = [];
      aShapeObj.edges.disabled = [];
      aShapeObj.vertices.disabled = [];
      this._scViewer.eventManager.removeEventListener(SCwebviewer.UserEvents.onSelectedObjectsChanged, this._setConnectedFilter);
      switch(theFilter) {
        case 0: {
          break;
        }
        case 1: {
          this._setSurfaceCurveTypeFilter(aShapeObj, SCwebviewer.ShapeSurfaceType.BSplineSurface, SCwebviewer.ShapeCurveType.BSplineCurve);
          break;
        }
        case 2: {
          this._setSurfaceCurveTypeFilter(aShapeObj, SCwebviewer.ShapeSurfaceType.Plane, SCwebviewer.ShapeCurveType.Line);
          break;
        }
        case 3: {
          this._setSurfaceCurveTypeFilter(aShapeObj, SCwebviewer.ShapeSurfaceType.OtherSurface, SCwebviewer.ShapeCurveType.OtherCurve);
          break;
        }
        case 4: {
          this._setConnectedFilter(null);
          this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onSelectedObjectsChanged, this._setConnectedFilter);
          break;
        }
      }
    }, {items: [
      {name: "Any", value: 0},
      {name: "B-Spline", value: 1},
      {name: "Plane/Line", value: 2},
      {name: "Other", value: 3},
      {name: "Connected", value: 4}
    ]});

    this._selModesExpandableGroup.addControl(this._shapeSelFilterInput);
  }

  /** Update length inputs. */
  _updateUnits() {
    const aLenUnit = this._scViewer.unitManager.length.unitDisplay;
    for (const aNumInput of this._lenInputs) {
      aNumInput.unit = aLenUnit;
    }

    const anAngUnit = this._scViewer.unitManager.angle.unitDisplay;
    for (const aNumInput of this._angInputs) {
      aNumInput.unit = anAngUnit;
    }
  }

  /** Get 3D vector from a triplet of number inputs, converted into system units. */
  _getSystemVec3Len(theInputs) {
    return this._scViewer.unitManager.length.systemFromDisplayVec3({
      x: theInputs.x.value, y: theInputs.y.value, z: theInputs.z.value
    });
  }

  /** Get 3D vector from a triplet of number inputs, converted into system units. */
  _getSystemVec3Ang(theInputs) {
    return this._scViewer.unitManager.angle.systemFromDisplayVec3({
      x: theInputs.x.value, y: theInputs.y.value, z: theInputs.z.value
    });
  }

  /** Update local transformation. */
  _updateTrsf() {
    if (this._selectedShape == null) {
      return;
    }

    const aTrans = this._getSystemVec3Len(this._trsfLocTrans);
    const anAngles = this._getSystemVec3Ang(this._trsfLocRot);

    let aTrsf = new SCwebviewer.Trsf();
    aTrsf = SCwebviewer.Trsf.setEulerAngles(aTrsf, anAngles, this._angSequence);
    aTrsf.translation = aTrans;
    aTrsf.scale = this._trsfLocScale.value;

    this._selectedShape.localTransformation = aTrsf;
    this._scViewer.requestViewUpdate();
  }

  /** Update shape location. */
  _updateShapeLoc() {
    if (this._selectedShape == null) {
      return;
    }

    const aTrans = this._getSystemVec3Len(this._trsfShapeTrans);
    const anAngles = this._getSystemVec3Ang(this._trsfShapeRot);

    let aTrsf = new SCwebviewer.Trsf();
    aTrsf = SCwebviewer.Trsf.setEulerAngles(aTrsf, anAngles, this._angSequence);
    aTrsf.translation = aTrans;
    aTrsf.scale = this._trsfShapeScale.value;

    this._selectedShape.shapeLocation = aTrsf;
    this._scViewer.requestViewUpdate();
  }

  /**
   * Local transformation controls.
   */
  addTrsfControls() {
    this._lenOptions = {step: 0.01, units: ""};
    this._angOptions = {step: 1.0, units: ""};
    this._angSequence = SCwebviewer.EulerSequence.EulerAngles;

    // interaction local transformation
    this._trsfLocTrans = {
      x: new SCcontrols.NumberInput("Trans. DX", 0.0, (theVal) => { this._updateTrsf(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Trans. DY", 0.0, (theVal) => { this._updateTrsf(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Trans. DZ", 0.0, (theVal) => { this._updateTrsf(); }, this._lenOptions)
    };
    this._lenInputs.push(this._trsfLocTrans.x, this._trsfLocTrans.y, this._trsfLocTrans.z);
    this._trsfLocExpGroup.addControls([this._trsfLocTrans.x, this._trsfLocTrans.y, this._trsfLocTrans.z]);

    this._trsfLocRot = {
      x: new SCcontrols.NumberInput("Rotation 1", 0.0, (theVal) => { this._updateTrsf(); }, this._angOptions),
      y: new SCcontrols.NumberInput("Rotation 2", 0.0, (theVal) => { this._updateTrsf(); }, this._angOptions),
      z: new SCcontrols.NumberInput("Rotation 3", 0.0, (theVal) => { this._updateTrsf(); }, this._angOptions)
    };
    this._angInputs.push(this._trsfLocRot.x, this._trsfLocRot.y, this._trsfLocRot.z);
    this._trsfLocExpGroup.addControls([this._trsfLocRot.x, this._trsfLocRot.y, this._trsfLocRot.z]);

    this._trsfLocScale = new SCcontrols.NumberInput("Scale", 1.0, (theVal) => { this._updateTrsf(); }, {step: 0.01, units: ""});
    this._trsfLocExpGroup.addControl(this._trsfLocScale);

    // shape location
    this._trsfShapeTrans = {
      x: new SCcontrols.NumberInput("Trans. DX", 0.0, (theVal) => { this._updateShapeLoc(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Trans. DY", 0.0, (theVal) => { this._updateShapeLoc(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Trans. DZ", 0.0, (theVal) => { this._updateShapeLoc(); }, this._lenOptions)
    };
    this._lenInputs.push(this._trsfShapeTrans.x, this._trsfShapeTrans.y, this._trsfShapeTrans.z);
    this._trsfShapeExpGroup.addControls([this._trsfShapeTrans.x, this._trsfShapeTrans.y, this._trsfShapeTrans.z]);

    this._trsfShapeRot = {
      x: new SCcontrols.NumberInput("Rotation 1", 0.0, (theVal) => { this._updateShapeLoc(); }, this._angOptions),
      y: new SCcontrols.NumberInput("Rotation 2", 0.0, (theVal) => { this._updateShapeLoc(); }, this._angOptions),
      z: new SCcontrols.NumberInput("Rotation 3", 0.0, (theVal) => { this._updateShapeLoc(); }, this._angOptions)
    };
    this._angInputs.push(this._trsfShapeRot.x, this._trsfShapeRot.y, this._trsfShapeRot.z);
    this._trsfShapeExpGroup.addControls([this._trsfShapeRot.x, this._trsfShapeRot.y, this._trsfShapeRot.z]);

    this._trsfShapeScale = new SCcontrols.NumberInput("Scale", 1.0, (theVal) => { this._updateShapeLoc(); }, {step: 0.01, units: ""});
    this._trsfShapeExpGroup.addControl(this._trsfShapeScale);
  }

  /**
   * Format list into a string.
   */
  _formatIndexList(theList) {
    let aStr = "";
    for (let aSubIter of theList) {
      if (aStr != "") {
        aStr += " ";
      }
      aStr += aSubIter;
    }
    return aStr;
  }

  /**
   * Format list into a string.
   */
  _formatSubshapes(theName, theList) {
    return theList.length != 0
         ? theName + " [" + this._formatIndexList(theList) + "] "
         : "";
  }

  /**
   * Update on object changed
   */
  updateControls() {
    let aShapeObj = this._selectedShape;
    this._shapeListInput.setValue (aShapeObj == null ? "" : aShapeObj.id);
    this._shapePropsGroup.setVisibility(aShapeObj != null);
    if (aShapeObj == null) {
      return;
    }

    // General properties
    this._currentIdInput.setValue(aShapeObj.id);
    this._visibleCheckbox.setValue(aShapeObj.isVisible);

    this._shapeDispModeInput.setValue(aShapeObj.presentationMode);
    this._shapePriorityModifierInput.setValue(aShapeObj.priorityModifier);
    this._shapeHasMaterialCheckbox.setValue(aShapeObj.hasMaterial);
    if (aShapeObj.baseColor.length >= 7)   { this._shapeBaseColorInput.setValue(aShapeObj.baseColor.slice(0, 7)); }
    if (aShapeObj.forcedColor.length >= 7) { this._shapeForcedColorInput.setValue(aShapeObj.forcedColor.slice(0, 7)); }
    this._shapeForcedColorCheckbox.setValue(aShapeObj.forcedColor !== "");
    this._shapeTranspRange.setValue(aShapeObj.transparency);
    this._shapeMetalSlider.setValue(aShapeObj.materialMetallic);
    this._shapeRoughSlider.setValue(aShapeObj.materialRoughness);
    this._shapeLineTypeInput.setValue(aShapeObj.aspects.lineType);
    this._shapeLineWidthSlider.setValue(aShapeObj.aspects.lineWidth);

    this._updateSelModeInput(aShapeObj.selectionMode);
    this._updateSelModeCheckboxes(aShapeObj.selectionMode);

    let aSubInfo = "";
    const aSubshapes = aShapeObj.selectedSubshapesInfo;
    if (aSubshapes.length == 1) {
      const aShapeInfo = aSubshapes[0];
      aSubInfo += SCwebviewer.ShapeInfo.format(aShapeInfo, this._scViewer.unitManager);
    } else {
      aSubInfo += this._formatSubshapes("COMPSOLIDS", aShapeObj.compSolids.selected);
      aSubInfo += this._formatSubshapes("SOLIDS",   aShapeObj.solids.selected);
      aSubInfo += this._formatSubshapes("SHELLS",   aShapeObj.shells.selected);
      aSubInfo += this._formatSubshapes("FACES",    aShapeObj.faces.selected);
      aSubInfo += this._formatSubshapes("WIRES",    aShapeObj.wires.selected);
      aSubInfo += this._formatSubshapes("EDGES",    aShapeObj.edges.selected);
      aSubInfo += this._formatSubshapes("VERTICES", aShapeObj.vertices.selected);
    }
    this._subSelectedInfo.setValue(aSubInfo);

    this._shapeInfo.setValue(!this._shapeInfoExpandableGroup.isCollapsed ? aShapeObj.infoString : "");

    let aTrsf = aShapeObj.localTransformation;
    let aTrans = this._scViewer.unitManager.length.systemToDisplayVec3(aTrsf.translation);
    let anAngles = this._scViewer.unitManager.angle.systemToDisplayVec3(SCwebviewer.Trsf.getEulerAngles(aTrsf, this._angSequence));
    this._trsfLocTrans.x.setValue(aTrans.x);
    this._trsfLocTrans.y.setValue(aTrans.y);
    this._trsfLocTrans.z.setValue(aTrans.z);
    this._trsfLocScale.setValue(aTrsf.scale);
    this._trsfLocRot.x.setValue(anAngles.x);
    this._trsfLocRot.y.setValue(anAngles.y);
    this._trsfLocRot.z.setValue(anAngles.z);

    aTrsf = aShapeObj.shapeLocation;
    aTrans = this._scViewer.unitManager.length.systemToDisplayVec3(aTrsf.translation);
    anAngles = this._scViewer.unitManager.angle.systemToDisplayVec3(SCwebviewer.Trsf.getEulerAngles(aTrsf, this._angSequence));
    this._trsfShapeTrans.x.setValue(aTrans.x);
    this._trsfShapeTrans.y.setValue(aTrans.y);
    this._trsfShapeTrans.z.setValue(aTrans.z);
    this._trsfShapeScale.setValue(aTrsf.scale);
    this._trsfShapeRot.x.setValue(anAngles.x);
    this._trsfShapeRot.y.setValue(anAngles.y);
    this._trsfShapeRot.z.setValue(anAngles.z);

    this._shapeExplodeDepthInput.setValue(this._scSample.shapeExplodeDepth);
  }
}
