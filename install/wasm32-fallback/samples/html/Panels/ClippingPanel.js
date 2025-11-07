/**
 * Created by: Kirill Gavrilov
 * Copyright (c) 2023 Huawei Technologies Co., Ltd.
 */
import * as SCwebviewer from '/wasm32/SCwebviewer.js';
import * as SCcontrols from './../SCcontrols.js';

/**
 * Create panel for managing a clipping box.
 */
export class ClippingPanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Clipping");

    this._scViewer = theSample.scViewer;
    this._scSample = theSample;
    this._clipBox = null;

    this.addConfigControl();

    this._clippingCheck  = new SCcontrols.CheckboxInput("Clipping", true,
      (theVal) => { this._clipBox.isClippingOn = theVal; this._scViewer.requestViewUpdate(); });
    this._visibleCheck  = new SCcontrols.CheckboxInput("Visible", true,
      (theVal) => { this._clipBox.isVisible = theVal; this._scViewer.requestViewUpdate(); });
    this._enabledCheck  = new SCcontrols.CheckboxInput("Dragging", true,
      (theVal) => { this._clipBox.isEnabled = theVal; this._scViewer.requestViewUpdate(); });
    this._flipCheck     = new SCcontrols.CheckboxInput("Flip",    false,
      (theVal) => { this._clipBox.toFlipClipping = theVal; this._scViewer.requestViewUpdate(); });
    this._cappingCheck  = new SCcontrols.CheckboxInput("Capping", false,
      (theVal) => { this._clipBox.isCappingOn = theVal; this._scViewer.requestViewUpdate(); });
    this._matColorCheck = new SCcontrols.CheckboxInput("Use object color", true,
      (theVal) => { this._clipBox.toUseObjectMaterial = theVal; this._scViewer.requestViewUpdate(); });
    this._shadedCheck  = new SCcontrols.CheckboxInput("Shaded", true,
      (theVal) => { this._clipBox.isShaded = theVal; this._scViewer.requestViewUpdate(); });
    this._resetBtn  = new SCcontrols.ButtonInput("Reset",  () => {
        this._clipBox.relativeClippingCorner = { x: 0.50, y: 0.50, z: 0.50 };
        const aCfg = this._clipBox.nbPlanesXYZ;
        this._clipBox.relativeClippingExtent = { x: 0.51, y: 0.51, z: 0.51 };
        this._clipBox.nbPlanesXYZ = aCfg;
        this.updateClipInfo();
        this._scViewer.requestViewUpdate();
    });

    this._clipInfo = new SCcontrols.TextAreaInput("", "\n\n\n", null, {autoHeight: true, readOnly: true});
    this._clipInfo.minWidth = 240;

    this.addControls([this._clippingCheck, this._visibleCheck, this._enabledCheck,
      this._flipCheck, this._cappingCheck, this._matColorCheck, this._shadedCheck,
      this._resetBtn, this._clipInfo
    ]);
  }

  /** Hide temporary objects. */
  onActivePanelChanged(theIsActivated) {
    super.onActivePanelChanged(theIsActivated);
    if (!theIsActivated && this._clipBox != null && this._clipBox.isAlive) {
      // @c this._clipBox.remove();
      this._clipBox.isEnabled = false;
      this._scViewer.requestViewUpdate();
    }
    if (theIsActivated && (this._clipBox == null || !this._clipBox.isAlive)) {
      this._clipBox = new SCwebviewer.InteractiveClipping(this._scViewer, this._scViewer.focusView);
      this._clipBox.nbPlanesXYZ = { x: 1, y: 0, z: 0 };
      this._clipBox.relativeClippingCorner = { x: 0.5, y: 0.5, z: 0.5 };
      this.updateClipInfo();

      this._clipBox.addEventListener(SCwebviewer.UserEvents.onObjectDragged, (theEvent) => {
        this.updateClipInfo();
      });
    }
    if (theIsActivated) {
      this._clipBox.isVisible = this._visibleCheck.value;
      this._clipBox.isEnabled = this._enabledCheck.value;
      this._clipBox.isClippingOn = this._clippingCheck.value;
      this._clipBox.isVisible = this._visibleCheck.value;
      this._clipBox.toFlipClipping = this._flipCheck.value;
      this._clipBox.isCappingOn = this._cappingCheck.value;
      this._clipBox.toUseObjectMaterial = this._matColorCheck.value;
      this._clipBox.isShaded = this._shadedCheck.value;
      this._scViewer.requestViewUpdate();
    }
  }

  /** Format value. */
  formatVal(theVal, theMult) {
    if (SCwebviewer.Vec3.isInfinite(theVal)) {
      return theVal < 0.0 ? "-inf" : "+inf";
    }
    return (theVal * theMult).toFixed(1);
  }

  /** Update clipping information. */
  updateClipInfo() {
    let anInfo = "";
    const aCorn = this._clipBox.relativeClippingCorner;
    const anExt = this._clipBox.relativeClippingExtent;
    anInfo += "Corner: " + this.formatVal(aCorn.x, 100) + "% "
                         + this.formatVal(aCorn.y, 100) + "% "
                         + this.formatVal(aCorn.z, 100) + "%\n";
    anInfo += "Extent: " + this.formatVal(anExt.x, 100) + "% "
                         + this.formatVal(anExt.y, 100) + "% "
                         + this.formatVal(anExt.z, 100) + "%\n";
    this._clipInfo.value = anInfo;
  }

  /** Add configuration control. */
  addConfigControl() {
    this._cfgInput = new SCcontrols.DropdownInput("Configuration", "x", (theCfg) => {
      let aCfg = { x: 0, y: 0, z: 0 };
      switch(theCfg) {
        case "": break;
        case "x":  aCfg.x =  1; break;
        case "y":  aCfg.y = -1; break;
        case "z":  aCfg.z =  1; break;
        case "xx": aCfg.x =  2; break;
        case "yy": aCfg.y = -2; break;
        case "zz": aCfg.z =  2; break;
        case "xy": aCfg.x = 1; aCfg.y = -1; break;
        case "xyz": aCfg.x = 1; aCfg.y = -1; aCfg.z = 1; break;
        case "box": aCfg.x = 2; aCfg.y = -2; aCfg.z = 2; break;
      }
      this._clipBox.nbPlanesXYZ = aCfg;
      // @c this._clipBox.isVisible = true;
      this.updateClipInfo();
      this._scViewer.requestViewUpdate();
    }, {items: [
      {name: "",         value: ""},
      {name: "+X",       value: "x"},
      {name: "-Y",       value: "y"},
      {name: "+Z",       value: "z"},
      {name: "+X/-X",    value: "xx"},
      {name: "+Y/-Y",    value: "yy"},
      {name: "+X/-Y",    value: "xy"},
      {name: "+X/-Y/+Z", value: "xyz"},
      {name: "Box",      value: "box"},
    ]});
    this.addControl(this._cfgInput);
  }
};
