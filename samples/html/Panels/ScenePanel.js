import * as SCwebviewer from '/wasm32/SCwebviewer.js';
import * as SCcontrols from './../SCcontrols.js';

/**
 * Create panel for managing interactive objectsa.
 */
export class ScenePanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Scene");

    this._scViewer = theSample.scViewer;
    this._scSample = theSample;
    this._selectedObject = null;

    this._lenInputs = [];
    this._angInputs = [];

    this.addSplitViewControls();
    this.addObjectSelector();
    this.addCurrentObjectControls();
    this.addSceneGraphControls();
    this.addSelectionControls();
    this._measureA = null;
    this._measureB = null;
    this._measureDim = null;
    this.addMeasureControls();

    this._viewExpGroup = new SCcontrols.GroupInput("Camera", true, false);
    this.addControl(this._viewExpGroup);
    this.addFitViewControls();
    this.addViewControls();

    this._trsfExpGroup = new SCcontrols.GroupInput("Local transformation", true, true);
    this.addControl(this._trsfExpGroup);
    this.addTrsfControls();

    this._pickExpGroup = new SCcontrols.GroupInput("Picking info", true, false);
    this.addControl(this._pickExpGroup);
    this.addPickingControls();

    // register event handlers
    window.addEventListener("sceneChanged", e => {
      this._selectedObject = null;
      this.fillObjects();
      this.updateControls();
    });

    this._onSelectionChanged = e => {
      this._selectedObject = this._scViewer.selectionManager.oneSelectedObject;
      this.updateControls();
    };

    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onUnitsChanged, e => {
      this._updateUnits();
    });

    this.update = this.updateControls;
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
   * Fill object list.
   */
  fillObjects() {
    this._objectInput.options.items = this._scViewer.interactiveContext.objectsInside
      .filter((theElem) => {return true})
      .map(elem => { return {name: elem.id, value: elem.id} });
    this._objectInput.options.items.unshift({name: "None", value: ""});
    this._objectInput.updateValues();
  }

  /**
   * Add split view controls.
   */
  addSplitViewControls() {
    this._splitViewInput = new SCcontrols.DropdownInput("Split view", "", (theMode) => {
      this._objectViewInput.setValue(-1);
      for (let anObjIIter of this._scViewer.interactiveContext.objectsInside) {
        anObjIIter.visibleInView = null;
      }
      this._scViewer.viewManager.removeAllSubviews();
      this._scViewer.viewManager.setSubviewBorderColor("#808080");
      switch(theMode) {
        case 1: {
          break;
        }
        case 2: {
          let aSubviewMargins = { x: 2, y: 2 };
          let aSubview1Size   = { x: 0.5, y: 1.0 };
          this._scSample.subview1 = new SCwebviewer.Subview();
          this._scSample.subview1.create(this._scViewer, aSubview1Size);
          this._scSample.subview1.subviewOffset = { x: 0.0, y: 0.0 };
          this._scSample.subview1.subviewMargins = aSubviewMargins;
          this._scSample.subview1.camera.setOrientation(SCwebviewer.CameraOrientation.AXOLEFT);
          this._scSample.subview1.camera.fitAllAuto(true);

          let aSubview2Size = { x: 0.5, y: 1.0 };
          this._scSample.subview2 = new SCwebviewer.Subview();
          this._scSample.subview2.create(this._scViewer, aSubview2Size);
          this._scSample.subview2.subviewOffset = { x: 0.5, y: 0.0 };
          this._scSample.subview2.subviewMargins = aSubviewMargins;
          this._scSample.subview2.camera.setOrientation(SCwebviewer.CameraOrientation.FRONT);
          this._scSample.subview2.camera.fitAllAuto(true);
          break;
        }
      }
      this._scViewer.requestViewUpdate();
    }, {items: [
      {name: "1 View",  value: 1},
      {name: "2 Views", value: 2}
    ]});
    this.addControl(this._splitViewInput);
  }

  /**
   * Add object selector controls.
   */
  addObjectSelector() {
    this._objectInput = new SCcontrols.DropdownInput("Object", "", (object) => {
      // workaround for non-selected objects
      let anObj = this._scViewer.interactiveContext.findInteractiveObject(object);
      this._selectedObject = anObj;
      this._scViewer.selectionManager.oneSelectedObject = anObj;
      this._selectedObject = anObj; // force picking even unselectable objects
      this.updateControls();
      this._scViewer.requestViewUpdate();
    },
    {items: [{name: "None", value: ""}]});

    this.addControl(this._objectInput);
  }

  /**
   * Add current object controls.
   */
  addCurrentObjectControls() {
    this._objectViewInput = new SCcontrols.DropdownInput("Visible in view", -1, (theView) => {
      if (this._selectedObject != null) {
        this._selectedObject.visibleInView = theView >= 0 && theView < this._scViewer.viewManager.subviews.length
                                           ? this._scViewer.viewManager.subviews[theView]
                                           : null;
        this._scViewer.requestViewUpdate();
      }
    }, {items: [
      {name: "All views", value: -1},
      {name: "1 View",    value:  0},
      {name: "2 View",    value:  1}
    ]});
    this.addControl(this._objectViewInput);

    let aVisGroup = new SCcontrols.GroupInput("VisibilityGroup");
    this._showAllButton = new SCcontrols.ButtonInput("Show All", () => {
      const anIds = this._scViewer.interactiveContext.objectsInside.map((o) => o.id);
      this._scViewer.interactiveContext.setVisibleManyObjects(anIds, true);
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._visibleCheckbox = new SCcontrols.CheckboxInput("Visible", false, (val) => {
      this._selectedObject.isVisible = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });
    this._infiniteCheckbox = new SCcontrols.CheckboxInput("Infinite", false, (val) => {
      this._selectedObject.isInfiniteState = val;
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._showOnlyButton = new SCcontrols.ButtonInput("Show Only", () => {
      if (this._selectedObject != null) {
        this._scViewer.interactiveContext.setVisibleOnlyObjects([this._selectedObject.id]);
        this._scViewer.requestViewUpdate();
      }
    }, { inlineBlock: true });

    aVisGroup.addControls([this._showAllButton, this._showOnlyButton, this._visibleCheckbox]);
    this.addControl(aVisGroup);

    let aBatchVisGroup = new SCcontrols.GroupInput("BatchVisibility");
    const applyToSelection = (label, handler) => {
      if (this._scViewer.interactiveContext == null || this._scViewer.selectionManager == null) {
        console.warn(`${label}: viewer is not ready`);
        return;
      }
      const ids = this._scViewer.selectionManager.selectedObjects
        .map((obj) => obj.id)
        .filter((id) => id !== "");
      if (ids.length === 0) {
        console.warn(`${label}: select at least one object`);
        return;
      }
      handler(ids);
      this._scViewer.requestViewUpdate();
      window.dispatchEvent(new Event('sceneChanged'));
    };

    const aShowSelectedButton = new SCcontrols.ButtonInput("Show Selected", () => {
      applyToSelection("Show Selected", (ids) => {
        this._scViewer.interactiveContext.setVisibleManyObjects(ids, true);
      });
    }, { inlineBlock: true });

    const aHideSelectedButton = new SCcontrols.ButtonInput("Hide Selected", () => {
      applyToSelection("Hide Selected", (ids) => {
        this._scViewer.interactiveContext.setVisibleManyObjects(ids, false);
      });
    }, { inlineBlock: true });

    const aIsolateSelectedButton = new SCcontrols.ButtonInput("Isolate Selected", () => {
      applyToSelection("Isolate Selected", (ids) => {
        this._scViewer.interactiveContext.setVisibleOnlyObjects(ids);
      });
    }, { inlineBlock: true });

    aBatchVisGroup.addControls([aShowSelectedButton, aHideSelectedButton, aIsolateSelectedButton]);
    this.addControl(aBatchVisGroup);

    let aRemGroup = new SCcontrols.GroupInput("RemoveGroup");
    this._clearAllButton = new SCcontrols.ButtonInput("Clear Scene", ()=> {
      let objects = [];
      for (let obj of this._scViewer.interactiveContext.objectsInside) {
        if (obj.isLocked !== true) {
          objects.push(obj);
        }
      }
      for (let obj of objects) {
        obj.remove();
      }
      window.dispatchEvent(new Event('sceneChanged'));
      this._scViewer.requestViewUpdate();
    }, { inlineBlock: true });

    this._removeButton = new SCcontrols.ButtonInput("Remove", () => {
      if (this._selectedObject != null) {
        if (this._selectedObject instanceof SCwebviewer.InteractiveCaeMesh) {
          let aColorScales = this._selectedObject.colorScaleObjects;
          for (let aColorScale of aColorScales) {
            aColorScale.remove();
          }
        }

        this._selectedObject.remove();
        this._scViewer.viewManager.invalidate2dObjects("");
        this._scViewer.requestViewUpdate();
        window.dispatchEvent(new Event('sceneChanged'));
      }
    }, { inlineBlock: true });

    aRemGroup.addControls([this._clearAllButton, this._removeButton, this._infiniteCheckbox]);
    this.addControl(aRemGroup);

    this._cloneButton = new SCcontrols.ButtonInput("Clone", () => {
      if (this._selectedObject != null) {
        let aCopy = this._selectedObject.clone();
        if (aCopy != null) {
          aCopy.isVisible = true;
          this._scViewer.requestViewUpdate();
          window.dispatchEvent(new Event('sceneChanged'));
        }
      }
    }, { inlineBlock: true });
    this._screenshotButton = new SCcontrols.ButtonInput("Make screenshot", ()=> {
      this._scViewer.makeScreenshot("viewer.png", -1, -1);
    }, { inlineBlock: true });
    this.addControls([this._cloneButton, this._screenshotButton]);
  }

  /**
   * Add SceneGraph debug controls.
   */
  addSceneGraphControls() {
    this.addControl(new SCcontrols.ButtonInput("Dump SceneGraph", () => {
      const aGraph = this._scViewer.interactiveContext.getSceneGraph();
      console.log("[SceneGraph]", aGraph);
    }));
  }

  /**
   * Add selection controls and box-selection hint.
   */
  addSelectionControls() {
    this._selectionGroup = new SCcontrols.GroupInput("Selection", true, false);
    this._selectionGroup.addControl(
      new SCcontrols.LabelInput("Box selection", "Alt + LMB drag (Shift + Alt to add)", null)
    );

    this._selectionInfo = new SCcontrols.TextAreaInput("Selected objects", "", null,
      { autoHeight: true, readOnly: true });
    this._selectionInfo.minWidth = 240;
    this._selectionGroup.addControl(this._selectionInfo);
    this.addControl(this._selectionGroup);
  }

  /**
   * Update selected objects list.
   */
  _updateSelectionInfo() {
    if (!this._selectionInfo || !this._scViewer?.selectionManager) {
      return;
    }
    const ids = (this._scViewer.selectionManager.selectedObjects || [])
      .map((obj) => obj.id)
      .filter((id) => id !== "");
    const header = `Count: ${ids.length}`;
    const body = ids.length ? `${header}\n${ids.join("\n")}` : header;
    this._selectionInfo.setValue(body);
  }

  /**
   * Capture measure target from currently detected shape element.
   */
  _captureMeasureTarget() {
    const aDetObj = this._scViewer.selectionManager.lastDetectedObject;
    if (aDetObj == null || !(aDetObj instanceof SCwebviewer.InteractiveShape)) {
      return null;
    }
    const anElem = this._scViewer.selectionManager.getDetectedShapeElem(aDetObj.id);
    return {
      objId: aDetObj.id,
      entityType: anElem.entityType,
      entityId: anElem.entityId,
    };
  }

  /**
   * Add minimal distance measurement controls (Set A / Set B / Measure).
   */
  addMeasureControls() {
    let aGroup = new SCcontrols.GroupInput("MeasureDistance", true, false);

    this._measAInfo = new SCcontrols.TextInput("A", "", null, {readOnly: true});
    this._measBInfo = new SCcontrols.TextInput("B", "", null, {readOnly: true});

    const updateText = () => {
      const fmt = (t) => t == null ? "" : `${t.objId} [mode=${t.entityType}, id=${t.entityId}]`;
      this._measAInfo.setValue(fmt(this._measureA));
      this._measBInfo.setValue(fmt(this._measureB));
    };

    aGroup.addControls([this._measAInfo, this._measBInfo]);

    aGroup.addControl(new SCcontrols.ButtonInput("Set A (hover)", () => {
      this._measureA = this._captureMeasureTarget();
      updateText();
    }, { inlineBlock: true }));

    aGroup.addControl(new SCcontrols.ButtonInput("Set B (hover)", () => {
      this._measureB = this._captureMeasureTarget();
      updateText();
    }, { inlineBlock: true }));

    aGroup.addControl(new SCcontrols.ButtonInput("Measure shortest distance", () => {
      if (this._measureA == null || this._measureB == null) {
        console.warn("Measure: Please set A and B first (hover a shape and click Set A/Set B).");
        return;
      }
      const aMgr = this._scViewer.interactiveShapeManager;
      if (aMgr == null) {
        console.warn("Measure: interactiveShapeManager is null");
        return;
      }

      const aResStr = aMgr.measureDistanceJson(
        this._measureA.objId, this._measureA.entityType, this._measureA.entityId,
        this._measureB.objId, this._measureB.entityType, this._measureB.entityId
      );

      let aRes = null;
      try { aRes = JSON.parse(aResStr); } catch { aRes = null; }
      if (aRes == null || aRes.pnt1 == null || aRes.pnt2 == null) {
        console.warn("Measure: failed to parse result", aResStr);
        return;
      }

      if (this._measureDim != null && this._measureDim.isAlive) {
        this._measureDim.remove();
      }
      this._measureDim = new SCwebviewer.InteractiveDimension();
      this._measureDim.buildLength(this._scViewer, aRes.pnt1, aRes.pnt2);
      this._measureDim.isVisible = true;
      this._scViewer.requestViewUpdate();
    }));

    aGroup.addControl(new SCcontrols.ButtonInput("Auto Radius (hover circle edge)", () => {
      const aTgt = this._captureMeasureTarget();
      if (aTgt == null) {
        console.warn("Auto Radius: hover a shape edge first");
        return;
      }
      if (aTgt.entityType !== SCwebviewer.ShapeObjectSelMode.Edge
       && aTgt.entityType !== SCwebviewer.ShapeObjectSelMode.Face) {
        console.warn("Auto Radius: only Edge/Face selection is supported for now");
        return;
      }
      const aMgr = this._scViewer.interactiveShapeManager;
      if (aMgr == null) { return; }
      const aStr = aMgr.getCircleParamsJson(aTgt.objId, aTgt.entityType, aTgt.entityId);
      let aRes = null;
      try { aRes = JSON.parse(aStr); } catch { aRes = null; }
      if (aRes == null || aRes.ax2 == null) {
        console.warn("Auto Radius: failed to parse circle params", aStr);
        return;
      }
      let aDim = new SCwebviewer.InteractiveDimension();
      aDim.buildRadius(this._scViewer, aRes.ax2, aRes.radius, aRes.anchor);
      aDim.isVisible = true;
      this._scViewer.requestViewUpdate();
    }));

    aGroup.addControl(new SCcontrols.ButtonInput("Auto Diameter (hover circle edge)", () => {
      const aTgt = this._captureMeasureTarget();
      if (aTgt == null) {
        console.warn("Auto Diameter: hover a shape edge first");
        return;
      }
      if (aTgt.entityType !== SCwebviewer.ShapeObjectSelMode.Edge
       && aTgt.entityType !== SCwebviewer.ShapeObjectSelMode.Face) {
        console.warn("Auto Diameter: only Edge/Face selection is supported for now");
        return;
      }
      const aMgr = this._scViewer.interactiveShapeManager;
      if (aMgr == null) { return; }
      const aStr = aMgr.getCircleParamsJson(aTgt.objId, aTgt.entityType, aTgt.entityId);
      let aRes = null;
      try { aRes = JSON.parse(aStr); } catch { aRes = null; }
      if (aRes == null || aRes.ax2 == null) {
        console.warn("Auto Diameter: failed to parse circle params", aStr);
        return;
      }
      let aDim = new SCwebviewer.InteractiveDimension();
      aDim.buildRadius(this._scViewer, aRes.ax2, aRes.radius, aRes.anchor);
      aDim.specialSymbol = "⌀";
      aDim.isSpecialSymbolAfter = false;
      const aDiam = 2.0 * aRes.radius;
      aDim.textValue = this._scViewer.unitManager ? this._scViewer.unitManager.length.format(aDiam) : aDiam.toString();
      aDim.isVisible = true;
      this._scViewer.requestViewUpdate();
    }));

    aGroup.addControl(new SCcontrols.ButtonInput("Clear measure", () => {
      this._measureA = null;
      this._measureB = null;
      updateText();
      if (this._measureDim != null && this._measureDim.isAlive) {
        this._measureDim.remove();
        this._scViewer.requestViewUpdate();
      }
    }, { inlineBlock: true }));

    this.addControl(aGroup);
  }

  /**
   * Add fitall / axo view controls.
   */
  addFitViewControls() {
    let aViewGroup0 = new SCcontrols.GroupInput("ViewsGroup0");
    aViewGroup0.addControls([
      new SCcontrols.ButtonInput("Fit All / Selected", ()=> {
        this._scViewer.camera.fitAllAuto(true, true);
        this._scViewer.requestViewUpdate();
      }, { inlineBlock: true }),
      new SCcontrols.ButtonInput("Axo", ()=> {
        this._scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.AXORIGHT, true);
        this._scViewer.requestViewUpdate();
      }, { inlineBlock: true })
    ]);
    this._viewExpGroup.addControl(aViewGroup0);
  }

  /**
   * Add view controls.
   */
  addViewControls() {
    let aViewGroup1 = new SCcontrols.GroupInput("ViewsGroup1");
    aViewGroup1.addControls([
      new SCcontrols.ButtonInput("Top", ()=> {
        this._scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.TOP, true);
        this._scViewer.requestViewUpdate();
      }, { inlineBlock: true })
    ]);

    let aViewGroup2 = new SCcontrols.GroupInput("ViewsGroup2");
    aViewGroup2.addControls([
      new SCcontrols.ButtonInput("Left", ()=> {
        this._scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.LEFT, true);
        this._scViewer.requestViewUpdate();
      }, { inlineBlock: true }),
      new SCcontrols.ButtonInput("Front", ()=> {
        this._scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.FRONT, true);
        this._scViewer.requestViewUpdate();
      }, { inlineBlock: true }),
      new SCcontrols.ButtonInput("Right", ()=> {
        this._scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.RIGHT, true);
        this._scViewer.requestViewUpdate();
      }, { inlineBlock: true })
    ]);

    let aViewGroup3 = new SCcontrols.GroupInput("ViewsGroup3");
    aViewGroup3.addControls([
      new SCcontrols.ButtonInput("Bottom", ()=> {
        this._scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.BOTTOM, true);
        this._scViewer.requestViewUpdate();
      }, { inlineBlock: true })
    ]);

    let aViewGroup4 = new SCcontrols.GroupInput("ViewsGroup4");
    aViewGroup4.addControls([
      new SCcontrols.ButtonInput("Back", ()=> {
        this._scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.BACK, true);
        this._scViewer.requestViewUpdate();
      }, { inlineBlock: true })
    ]);

    this._viewExpGroup.addControls([aViewGroup1, aViewGroup2, aViewGroup3, aViewGroup4]);
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
    if (this._selectedObject == null) {
      return;
    }

    const aTrans = this._getSystemVec3Len(this._trsfLocTrans);
    const anAngles = this._getSystemVec3Ang(this._trsfLocRot);

    let aTrsf = new SCwebviewer.Trsf();
    aTrsf = SCwebviewer.Trsf.setEulerAngles(aTrsf, anAngles, this._angSequence);
    aTrsf.translation = aTrans;
    aTrsf.scale = this._trsfLocScale.value;

    this._selectedObject.localTransformation = aTrsf;
    this._scViewer.requestViewUpdate();
  }

  /**
   * Local transformation controls.
   */
  addTrsfControls() {
    this._lenOptions = {step: 0.01, units: ""};
    this._angOptions = {step: 1.0, units: ""};
    this._angSequence = SCwebviewer.EulerSequence.EulerAngles;

    this._trsfLocTrans = {
      x: new SCcontrols.NumberInput("Trans. DX", 0.0, (theVal) => { this._updateTrsf(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Trans. DY", 0.0, (theVal) => { this._updateTrsf(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Trans. DZ", 0.0, (theVal) => { this._updateTrsf(); }, this._lenOptions)
    };
    this._lenInputs.push(this._trsfLocTrans.x, this._trsfLocTrans.y, this._trsfLocTrans.z);
    this._trsfExpGroup.addControls([this._trsfLocTrans.x, this._trsfLocTrans.y, this._trsfLocTrans.z]);

    this._trsfLocRot = {
      x: new SCcontrols.NumberInput("Rotation 1", 0.0, (theVal) => { this._updateTrsf(); }, this._angOptions),
      y: new SCcontrols.NumberInput("Rotation 2", 0.0, (theVal) => { this._updateTrsf(); }, this._angOptions),
      z: new SCcontrols.NumberInput("Rotation 3", 0.0, (theVal) => { this._updateTrsf(); }, this._angOptions)
    };
    this._angInputs.push(this._trsfLocRot.x, this._trsfLocRot.y, this._trsfLocRot.z);
    this._trsfExpGroup.addControls([this._trsfLocRot.x, this._trsfLocRot.y, this._trsfLocRot.z]);

    this._trsfLocScale = new SCcontrols.NumberInput("Scale", 1.0, (theVal) => { this._updateTrsf(); }, {step: 0.01, units: ""});
    this._trsfExpGroup.addControl(this._trsfLocScale);

    this._trsfInfo = new SCcontrols.TextInput("Definition:", "", null, {readOnly: true});
    this._trsfExpGroup.addControl(this._trsfInfo);
  }

  /**
   * Create (sub)shape selection controls.
   */
  addPickingControls() {
    this._detectedInfo = new SCcontrols.TextInput("Detected:", "", null, {readOnly: true});
    this._pickExpGroup.addControl(this._detectedInfo);

    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onDetectedObjectChanged, () => {
      let aDetObj = this._scViewer.selectionManager.lastDetectedObject;
      let anInfo = aDetObj ? aDetObj.id : "NONE";
      this._detectedInfo.setValue(anInfo);
    });
  }

  /**
   * Update on object changed.
   */
  updateControls() {
    this._objectInput.setValue (this._selectedObject == null ? "" : this._selectedObject.id);
    this._visibleCheckbox.setVisibility(this._selectedObject != null);
    this._infiniteCheckbox.setVisibility(this._selectedObject != null);
    this._showOnlyButton.setVisibility(this._selectedObject != null);
    this._removeButton.setVisibility(this._selectedObject != null);
    this._updateSelectionInfo();
    if (!this._trsfExpGroup.isCollapsed) {
      this._trsfExpGroup.setVisibility(this._selectedObject != null);
    }
    if (this._selectedObject == null) {
      return;
    }

    this._visibleCheckbox.setValue(this._selectedObject.isVisible);
    this._infiniteCheckbox.setValue(this._selectedObject.isInfiniteState);
    this._objectViewInput.setValue(this._selectedObject.visibleInView !== null
      ? (this._selectedObject.visibleInView.id - 1)
      : -1);

    let aTrsf = this._selectedObject.localTransformation;
    let aTrans = this._scViewer.unitManager.length.systemToDisplayVec3(aTrsf.translation);
    let anAngles = this._scViewer.unitManager.angle.systemToDisplayVec3(SCwebviewer.Trsf.getEulerAngles(aTrsf, this._angSequence));
    this._trsfLocTrans.x.setValue(aTrans.x);
    this._trsfLocTrans.y.setValue(aTrans.y);
    this._trsfLocTrans.z.setValue(aTrans.z);
    this._trsfLocScale.setValue(aTrsf.scale);
    this._trsfLocRot.x.setValue(anAngles.x);
    this._trsfLocRot.y.setValue(anAngles.y);
    this._trsfLocRot.z.setValue(anAngles.z);
    this._trsfInfo.setValue(SCwebviewer.Trsf.format(aTrsf, this._scViewer.unitManager));
  }
}
