/**
 * Created by: Kirill Gavrilov
 * Copyright (c) 2023 Huawei Technologies Co., Ltd.
 */
import * as SCwebviewer from '/wasm32/SCwebviewer.js';
import * as SCcontrols from './../SCcontrols.js';

/**
 * Create panel for building simple models.
 */
export class BuildersPanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Builders");

    this._scViewer = theSample.scViewer;
    this._scSample = theSample;

    this._dirOptions = {step: 0.01};
    this._lenOptions = {step: 0.01, units: ""};
    this._angOptions = {step: 1.0, units: ""};
    this._lenInputs = [];
    this._angInputs = [];
    this._tmpShape = new SCwebviewer.InteractiveShape("previewShape");
    this._tmpLabel = null;
    this._tmpLen1  = new SCwebviewer.InteractiveDimension("previewLen1");
    this._tmpLen2  = new SCwebviewer.InteractiveDimension("previewLen2");
    this._tmpLen3  = new SCwebviewer.InteractiveDimension("previewLen3");
    this._tmpRad1  = new SCwebviewer.InteractiveDimension("previewRad1");
    this._tmpAng1  = new SCwebviewer.InteractiveDimension("previewAng1");

    this._builderInput = new SCcontrols.DropdownInput("Builder", "Vertex", (theBuilder) => {
      this._tmpShape.remove();
      this._tmpShape = new SCwebviewer.InteractiveShape("previewShape");
      this._updateSection(theBuilder);
      this._scViewer.requestViewUpdate();
    }, {items: [
      {name: "Vertex",    value: "Vertex"},
      {name: "Segment",   value: "Segment"},
      {name: "Box",       value: "Box"},
      {name: "Sphere",    value: "Sphere"},
      {name: "Cylinder",  value: "Cylinder"},
      {name: "Cone",      value: "Cone"},
      {name: "TextLabel", value: "TextLabel"},
    ]});
    this.addControl(this._builderInput);

    this.addVertexBuilder();
    this.addSegmentBuilder();
    this.addBoxBuilder();
    this.addSphereBuilder();
    this.addCylinderBuilder();
    this.addConeBuilder();
    this.addTextLabelBuilder();

    this.addControl(new SCcontrols.ButtonInput("Create", ()=> {
      let anId = this._scViewer.interactiveContext.generateId(this._builderInput.value);
      if (this._tmpLabel != null) {
        this._tmpLabel.baseColor = this._tmpLabel.toDrawRectangle ? "" : "WHITE";
        this._tmpLabel = null;
      } else {
        let aCopy = this._tmpShape.clone(anId);
        aCopy.zlayer = SCwebviewer.ZLayerId.Default;
        aCopy.presentationMode = SCwebviewer.ShapeObjectPrsMode.Shaded;
        aCopy.baseColor = "";
        aCopy.isVisible = true;
        this._tmpShape.remove();
      }
      this._tmpLen1.isVisible = false;
      this._tmpLen2.isVisible = false;
      this._tmpLen3.isVisible = false;
      this._tmpRad1.isVisible = false;
      this._tmpAng1.isVisible = false;

      window.dispatchEvent(new Event('sceneChanged'));
      this._scViewer.requestViewUpdate();
    }));

    this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onUnitsChanged, e => {
      this._updateUnits();
    });

    this._onSelectionChanged = e => {
      const aSelObj = this._scViewer.selectionManager.oneSelectedObject;
      let   aSelPnt = this._scViewer.selectionManager.lastDetectedInfo.pickedPoint;
      if (!aSelObj) { return; }

      aSelPnt = this._scViewer.unitManager.length.systemToDisplayVec3(aSelPnt);

      const aPntCtrl = this._getPntForBuilder(this._builderInput.value);
      aPntCtrl.x.setValue(aSelPnt.x);
      aPntCtrl.y.setValue(aSelPnt.y);
      aPntCtrl.z.setValue(aSelPnt.z);
      this._updateSection(this._builderInput.value);
    };
  }

  /** Hide temporary objects. */
  onActivePanelChanged(theIsActivated) {
    super.onActivePanelChanged(theIsActivated);
    if (!theIsActivated) {
      if (this._tmpShape.isAlive) {
        this._tmpShape.remove();
        this._scViewer.requestViewUpdate();
      }
      if (this._tmpLabel != null) {
        this._tmpLabel.remove();
        this._tmpLabel = null;
        this._scViewer.requestViewUpdate();
      }
      if (this._tmpLen1.isAlive) { this._tmpLen1.remove(); }
      if (this._tmpLen2.isAlive) { this._tmpLen2.remove(); }
      if (this._tmpLen3.isAlive) { this._tmpLen3.remove(); }
      if (this._tmpRad1.isAlive) { this._tmpRad1.remove(); }
      if (this._tmpAng1.isAlive) { this._tmpAng1.remove(); }
      window.dispatchEvent(new Event('sceneChanged'));
    }
    if (theIsActivated) {
      this._updateUnits();
      this._updateSection(this._builderInput.value);

      this._scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onSelectedObjectsChanged, this._onSelectionChanged);
    } else {
      this._scViewer.eventManager.removeEventListener(SCwebviewer.UserEvents.onSelectedObjectsChanged, this._onSelectionChanged);
    }
  }

  /** Display preview shape. */
  _displayPreview() {
    if (this._tmpLen1.isAlive) { this._tmpLen1.isVisible = false; }
    if (this._tmpLen2.isAlive) { this._tmpLen2.isVisible = false; }
    if (this._tmpLen3.isAlive) { this._tmpLen3.isVisible = false; }
    if (this._tmpRad1.isAlive) { this._tmpRad1.isVisible = false; }
    if (this._tmpAng1.isAlive) { this._tmpAng1.isVisible = false; }
    if (this._tmpLabel != null) {
      this._tmpLabel.baseColor = "RED";
      this._tmpLabel.isVisible = true;
      this._scViewer.requestViewUpdate();
      return;
    }

    this._tmpShape.zlayer = SCwebviewer.ZLayerId.Topmost;
    this._tmpShape.baseColor = "RED";
    this._tmpShape.transparency = 0.5;
    this._tmpShape.isVisible = true;
    this._scViewer.requestViewUpdate();
  }

  /** Update section visibility */
  _updateSection(theBuilder) {
    this._vertGroup.isVisible   = (theBuilder === "Vertex");
    this._segGroup.isVisible    = (theBuilder === "Segment");
    this._boxGroup.isVisible    = (theBuilder === "Box");
    this._sphereGroup.isVisible = (theBuilder === "Sphere");
    this._cylGroup.isVisible    = (theBuilder === "Cylinder");
    this._coneGroup.isVisible   = (theBuilder === "Cone");
    this._labelGroup.isVisible  = (theBuilder === "TextLabel");

    if (this._tmpLabel != null) {
      this._tmpLabel.remove();
      this._tmpLabel = null;
    }

    if (this._vertGroup.isVisible)   { this._updateVertex(); }
    if (this._segGroup.isVisible)    { this._updateSegment(); }
    if (this._boxGroup.isVisible)    { this._updateBox(); }
    if (this._sphereGroup.isVisible) { this._updateSphere(); }
    if (this._cylGroup.isVisible)    { this._updateCylinder(); }
    if (this._coneGroup.isVisible)   { this._updateCone(); }
    if (this._labelGroup.isVisible)  { this._updateTextLabel(); }
  }

  /** Return point input for specified builder. */
  _getPntForBuilder(theBuilder) {
    switch (theBuilder) {
      case "Vertex":    return this._vertOrig;
      case "Segment": {
        const aFoc1 = Math.max(this._segPnt1.x.lastFocusTime, this._segPnt1.y.lastFocusTime, this._segPnt1.z.lastFocusTime);
        const aFoc2 = Math.max(this._segPnt2.x.lastFocusTime, this._segPnt2.y.lastFocusTime, this._segPnt2.z.lastFocusTime);
        return aFoc2 > aFoc1
             ? this._segPnt2
             : this._segPnt1;
      }
      case "Box":       return this._boxOrig;
      case "Sphere":    return this._sphereOrig;
      case "Cylinder":  return this._cylOrig;
      case "Cone":      return this._coneOrig;
      case "TextLabel": return this._labelPos;
    }
    return null;
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
  _getSystemVec3(theInputs) {
    return this._scViewer.unitManager.length.systemFromDisplayVec3({
      x: theInputs.x.value, y: theInputs.y.value, z: theInputs.z.value
    });
  }

  /** Update vertex preview. */
  _updateVertex() {
    const aPnt = this._getSystemVec3(this._vertOrig);
    this._tmpShape.buildPoint(this._scViewer, aPnt);
    this._displayPreview();
    window.dispatchEvent(new Event('sceneChanged'));
  }

  /**
   * Add shape vertex builder.
   */
  addVertexBuilder() {
    this._vertGroup = new SCcontrols.GroupInput("VertexGroup");
    this._vertGroup.isVisible = false;
    this._vertOrig = {
      x: new SCcontrols.NumberInput("X", 0.0, (theVal) => { this._updateVertex(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Y", 0.0, (theVal) => { this._updateVertex(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Z", 0.0, (theVal) => { this._updateVertex(); }, this._lenOptions)
    };
    this._lenInputs.push(this._vertOrig.x, this._vertOrig.y, this._vertOrig.z);
    this._vertGroup.addControls([this._vertOrig.x, this._vertOrig.y, this._vertOrig.z]);

    this.addControl(this._vertGroup);
  }

  /** Update line segment preview. */
  _updateSegment() {
    const aPnt1 = this._getSystemVec3(this._segPnt1);
    const aPnt2 = this._getSystemVec3(this._segPnt2);
    this._tmpShape.buildLineSegment(this._scViewer, aPnt1, aPnt2);
    this._displayPreview();

    this._tmpLen1.buildLength(this._scViewer, aPnt1, aPnt2);
    this._tmpLen1.specialSymbol = "L";
    this._tmpLen1.isVisible = true;
    window.dispatchEvent(new Event('sceneChanged'));
  }

  /**
   * Add line segment builder.
   */
  addSegmentBuilder() {
    this._segGroup = new SCcontrols.GroupInput("SegmentGroup");
    this._segGroup.isVisible = false;
    this._segPnt1 = {
      x: new SCcontrols.NumberInput("Pnt1 X", 0.0, (theVal) => { this._updateSegment(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Pnt1 Y", 0.0, (theVal) => { this._updateSegment(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Pnt1 Z", 0.0, (theVal) => { this._updateSegment(); }, this._lenOptions)
    };
    this._lenInputs.push(this._segPnt1.x, this._segPnt1.y, this._segPnt1.z);
    this._segGroup.addControls([this._segPnt1.x, this._segPnt1.y, this._segPnt1.z]);

    this._segPnt2 = {
      x: new SCcontrols.NumberInput("Pnt2 X", 0.0, (theVal) => { this._updateSegment(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Pnt2 Y", 0.0, (theVal) => { this._updateSegment(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Pnt2 Z", 0.0, (theVal) => { this._updateSegment(); }, this._lenOptions)
    };
    this._lenInputs.push(this._segPnt2.x, this._segPnt2.y, this._segPnt2.z);
    this._segGroup.addControls([this._segPnt2.x, this._segPnt2.y, this._segPnt2.z]);

    this.addControl(this._segGroup);
  }

  /** Update box preview. */
  _updateBox() {
    const anAx2 = {
      location:   this._getSystemVec3(this._boxOrig),
      direction:  this._getSystemVec3(this._boxDir),
      xDirection: this._getSystemVec3(this._boxXDir)
    };
    const aDirNorm   = SCwebviewer.Vec3.normalize(anAx2.direction);
    const anXDirNorm = SCwebviewer.Vec3.normalize(anAx2.xDirection);
    const anAx2Norm = {
      location:   anAx2.location,
      direction:  aDirNorm,
      xDirection: SCwebviewer.Vec3.normalize(SCwebviewer.Vec3.cross(SCwebviewer.Vec3.cross(aDirNorm, anXDirNorm), aDirNorm))
    };

    const aDims = this._getSystemVec3(this._boxDims);

    this._tmpShape.buildBox(this._scViewer, anAx2, aDims);
    this._displayPreview();

    const aPntX = SCwebviewer.Vec3.add(anAx2.location,
                                       SCwebviewer.Vec3.multiply(anAx2Norm.xDirection, aDims.x));
    const aPlnX = {
      location:   anAx2.location,
      direction:  anAx2Norm.direction,
      xDirection: SCwebviewer.Vec3.zero()
    };
    this._tmpLen1.buildLength(this._scViewer, aPntX, anAx2.location, aPlnX);
    this._tmpLen1.specialSymbol = "W";
    this._tmpLen1.zlayer = SCwebviewer.ZLayerId.Topmost;
    this._tmpLen1.isVisible = true;

    const aDirY = SCwebviewer.Vec3.cross(anAx2Norm.direction, anAx2Norm.xDirection);
    const aPntY = SCwebviewer.Vec3.add(anAx2.location,
                                       SCwebviewer.Vec3.multiply(SCwebviewer.Vec3.normalize(aDirY), aDims.y));
    const aPlnY = {
      location:   anAx2.location,
      direction:  SCwebviewer.Vec3.reverse(anAx2Norm.direction),
      xDirection: SCwebviewer.Vec3.zero()
    };
    this._tmpLen2.buildLength(this._scViewer, aPntY, anAx2.location, aPlnY);
    this._tmpLen2.specialSymbol = "T";
    this._tmpLen2.zlayer = SCwebviewer.ZLayerId.Topmost;
    this._tmpLen2.isVisible = true;

    const aPntZ = SCwebviewer.Vec3.add(anAx2.location,
                                       SCwebviewer.Vec3.multiply(anAx2Norm.direction, aDims.z));
    const aPlnZ = {
      location:   anAx2.location,
      direction:  SCwebviewer.Vec3.reverse(anAx2Norm.xDirection),
      xDirection: SCwebviewer.Vec3.zero()
    };
    this._tmpLen3.buildLength(this._scViewer, aPntZ, anAx2.location, aPlnZ);
    this._tmpLen3.specialSymbol = "H";
    this._tmpLen3.zlayer = SCwebviewer.ZLayerId.Topmost;
    this._tmpLen3.isVisible = true;
    window.dispatchEvent(new Event('sceneChanged'));
  }

  /**
   * Add box builder.
   */
  addBoxBuilder() {
    this._boxGroup = new SCcontrols.GroupInput("BoxGroup");
    this._boxGroup.isVisible = false;

    this._boxOrig = {
      x: new SCcontrols.NumberInput("Origin X", 0.0, (theVal) => { this._updateBox(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Origin Y", 0.0, (theVal) => { this._updateBox(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Origin Z", 0.0, (theVal) => { this._updateBox(); }, this._lenOptions)
    };
    this._lenInputs.push(this._boxOrig.x, this._boxOrig.y, this._boxOrig.z);
    this._boxGroup.addControls([this._boxOrig.x, this._boxOrig.y, this._boxOrig.z]);

    this._boxDir = {
      x: new SCcontrols.NumberInput("Dir DX", 0.0, (theVal) => { this._updateBox(); }, this._dirOptions),
      y: new SCcontrols.NumberInput("Dir DY", 0.0, (theVal) => { this._updateBox(); }, this._dirOptions),
      z: new SCcontrols.NumberInput("Dir DZ", 1.0, (theVal) => { this._updateBox(); }, this._dirOptions)
    };
    this._lenInputs.push(this._boxDir.x, this._boxDir.y, this._boxDir.z);
    this._boxGroup.addControls([this._boxDir.x, this._boxDir.y, this._boxDir.z]);

    this._boxXDir = {
      x: new SCcontrols.NumberInput("XDir DX", 1.0, (theVal) => { this._updateBox(); }, this._dirOptions),
      y: new SCcontrols.NumberInput("XDir DY", 0.0, (theVal) => { this._updateBox(); }, this._dirOptions),
      z: new SCcontrols.NumberInput("XDir DZ", 0.0, (theVal) => { this._updateBox(); }, this._dirOptions)
    };
    this._lenInputs.push(this._boxXDir.x, this._boxXDir.y, this._boxXDir.z);
    this._boxGroup.addControls([this._boxXDir.x, this._boxXDir.y, this._boxXDir.z]);

    this._boxDims = {
      x: new SCcontrols.NumberInput("Dims DX", 0.01, (theVal) => { this._updateBox(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Dims DY", 0.01, (theVal) => { this._updateBox(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Dims DZ", 0.01, (theVal) => { this._updateBox(); }, this._lenOptions)
    };
    this._lenInputs.push(this._boxDims.x, this._boxDims.y, this._boxDims.z);
    this._boxGroup.addControls([this._boxDims.x, this._boxDims.y, this._boxDims.z]);

    this.addControl(this._boxGroup);
  }

  /** Update box preview. */
  _updateSphere() {
    let anAx2 = SCwebviewer.Ax2.xoy();
    anAx2.location = this._getSystemVec3(this._sphereOrig);
    const aRadius = this._scViewer.unitManager.length.systemFromDisplay(this._sphereRadius.value);
    this._tmpShape.buildSphere(this._scViewer, anAx2, aRadius);
    this._displayPreview();

    this._tmpRad1.buildRadius(this._scViewer, anAx2, aRadius);
    this._tmpRad1.zlayer = SCwebviewer.ZLayerId.Topmost;
    this._tmpRad1.isVisible = true;
    window.dispatchEvent(new Event('sceneChanged'));
  }

  /**
   * Add sphere builder.
   */
  addSphereBuilder() {
    this._sphereGroup = new SCcontrols.GroupInput("SphereGroup");
    this._sphereGroup.isVisible = false;

    this._sphereOrig = {
      x: new SCcontrols.NumberInput("Center X", 0.0, (theVal) => { this._updateSphere(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Center Y", 0.0, (theVal) => { this._updateSphere(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Center Z", 0.0, (theVal) => { this._updateSphere(); }, this._lenOptions)
    };
    this._lenInputs.push(this._sphereOrig.x, this._sphereOrig.y, this._sphereOrig.z);
    this._sphereGroup.addControls([this._sphereOrig.x, this._sphereOrig.y, this._sphereOrig.z]);

    this._sphereRadius = new SCcontrols.NumberInput("Radius", 0.01, (theVal) => { this._updateSphere(); }, { min: 0, ...this._lenOptions });
    this._lenInputs.push(this._sphereRadius);
    this._sphereGroup.addControls([this._sphereRadius]);

    this.addControl(this._sphereGroup);
  }

  /** Update cylinder preview. */
  _updateCylinder() {
    let aCylParams = new SCwebviewer.CylinderParams();
    aCylParams.ax2 = {
      location:   this._getSystemVec3(this._cylOrig),
      direction:  this._getSystemVec3(this._cylDir),
      xDirection: this._getSystemVec3(this._cylXDir)
    };
    aCylParams.radius = this._scViewer.unitManager.length.systemFromDisplay(this._cylRadius.value);
    aCylParams.height = this._scViewer.unitManager.length.systemFromDisplay(this._cylHeight.value);
    aCylParams.angle  = this._scViewer.unitManager.angle.systemFromDisplay(this._cylAngle.value);

    this._tmpShape.buildCylinder(this._scViewer, aCylParams);
    this._displayPreview();

    const aDir  = SCwebviewer.Vec3.normalize(aCylParams.ax2.direction);
    let   aDirX = SCwebviewer.Vec3.normalize(aCylParams.ax2.xDirection);
    const aDirY = SCwebviewer.Vec3.cross(aDir, aDirX);
    aDirX       = SCwebviewer.Vec3.normalize(SCwebviewer.Vec3.reverse(SCwebviewer.Vec3.cross(aDir, aDirY)));
    const aDimPnt1 = SCwebviewer.Vec3.add(aCylParams.ax2.location, SCwebviewer.Vec3.multiply(aDirX, aCylParams.radius));
    const aDimPnt2 = SCwebviewer.Vec3.add(aDimPnt1,                SCwebviewer.Vec3.multiply(aDir,  aCylParams.height));
    const aDimPln = {
      location:   aDimPnt1,
      direction:  SCwebviewer.Vec3.reverse(aDirY),
      xDirection: SCwebviewer.Vec3.zero()
    };
    this._tmpLen1.buildLength(this._scViewer, aDimPnt2, aDimPnt1, aDimPln);
    this._tmpLen1.specialSymbol = "H";
    this._tmpLen1.zlayer = SCwebviewer.ZLayerId.Topmost;
    this._tmpLen1.isVisible = true;

    this._tmpRad1.buildRadius(this._scViewer, aCylParams.ax2, aCylParams.radius);
    this._tmpRad1.zlayer = SCwebviewer.ZLayerId.Topmost;
    this._tmpRad1.isVisible = true;

    const aDirXX = SCwebviewer.Vec3.rotateVector(aDirX, aDir, aCylParams.angle);
    const aDimPnt3 = SCwebviewer.Vec3.add(aCylParams.ax2.location, SCwebviewer.Vec3.multiply(aDirXX, aCylParams.radius));

    this._tmpAng1.buildAngle(this._scViewer, aDimPnt1, aCylParams.ax2.location, aDimPnt3);
    this._tmpAng1.zlayer = SCwebviewer.ZLayerId.Topmost;
    this._tmpAng1.isVisible = true;
    window.dispatchEvent(new Event('sceneChanged'));
  }

  /**
   * Add cylinder builder.
   */
  addCylinderBuilder() {
    this._cylGroup = new SCcontrols.GroupInput("CylinderGroup");
    this._cylGroup.isVisible = false;

    this._cylOrig = {
      x: new SCcontrols.NumberInput("Origin X", 0.0, (theVal) => { this._updateCylinder(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Origin Y", 0.0, (theVal) => { this._updateCylinder(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Origin Z", 0.0, (theVal) => { this._updateCylinder(); }, this._lenOptions)
    };
    this._lenInputs.push(this._cylOrig.x, this._cylOrig.y, this._cylOrig.z);
    this._cylGroup.addControls([this._cylOrig.x, this._cylOrig.y, this._cylOrig.z]);

    this._cylDir = {
      x: new SCcontrols.NumberInput("Dir DX", 0.0, (theVal) => { this._updateCylinder(); }, this._dirOptions),
      y: new SCcontrols.NumberInput("Dir DY", 0.0, (theVal) => { this._updateCylinder(); }, this._dirOptions),
      z: new SCcontrols.NumberInput("Dir DZ", 1.0, (theVal) => { this._updateCylinder(); }, this._dirOptions)
    };
    this._lenInputs.push(this._cylDir.x, this._cylDir.y, this._cylDir.z);
    this._cylGroup.addControls([this._cylDir.x, this._cylDir.y, this._cylDir.z]);

    this._cylXDir = {
      x: new SCcontrols.NumberInput("XDir DX", 1.0, (theVal) => { this._updateCylinder(); }, this._dirOptions),
      y: new SCcontrols.NumberInput("XDir DY", 0.0, (theVal) => { this._updateCylinder(); }, this._dirOptions),
      z: new SCcontrols.NumberInput("XDir DZ", 0.0, (theVal) => { this._updateCylinder(); }, this._dirOptions)
    };
    this._lenInputs.push(this._cylXDir.x, this._cylXDir.y, this._cylXDir.z);
    this._cylGroup.addControls([this._cylXDir.x, this._cylXDir.y, this._cylXDir.z]);

    this._cylRadius = new SCcontrols.NumberInput("Radius", 0.01, (theVal) => { this._updateCylinder(); }, { min: 0, ...this._lenOptions });
    this._cylHeight = new SCcontrols.NumberInput("Height", 0.01, (theVal) => { this._updateCylinder(); }, this._lenOptions);
    this._lenInputs.push(this._cylRadius, this._cylHeight);

    this._cylAngle  = new SCcontrols.NumberInput("Angle",  360.0, (theVal) => { this._updateCylinder(); }, { min: 0, max: 360, ...this._angOptions });
    this._angInputs.push(this._cylAngle);
    this._cylGroup.addControls([this._cylRadius, this._cylHeight, this._cylAngle]);

    this.addControl(this._cylGroup);
  }

  /** Update cone preview. */
  _updateCone() {
    let aConeParams = new SCwebviewer.ConeParams();
    aConeParams.ax2 = {
      location:   this._getSystemVec3(this._coneOrig),
      direction:  this._getSystemVec3(this._coneDir),
      xDirection: this._getSystemVec3(this._coneXDir)
    };
    aConeParams.radius    = this._scViewer.unitManager.length.systemFromDisplay(this._coneRadius.value);
    aConeParams.topRadius = this._scViewer.unitManager.length.systemFromDisplay(this._coneTopRadius.value);
    aConeParams.height    = this._scViewer.unitManager.length.systemFromDisplay(this._coneHeight.value);
    aConeParams.angle     = this._scViewer.unitManager.angle.systemFromDisplay(this._coneAngle.value);

    this._tmpShape.buildCone(this._scViewer, aConeParams);
    this._displayPreview();

    this._tmpRad1.buildRadius(this._scViewer, aConeParams.ax2, aConeParams.radius);
    this._tmpRad1.zlayer = SCwebviewer.ZLayerId.Topmost;
    this._tmpRad1.isVisible = true;
    window.dispatchEvent(new Event('sceneChanged'));
  }

  /**
   * Add cone builder.
   */
  addConeBuilder() {
    this._coneGroup = new SCcontrols.GroupInput("ConeGroup");
    this._coneGroup.isVisible = false;

    this._coneOrig = {
      x: new SCcontrols.NumberInput("Origin X", 0.0, (theVal) => { this._updateCone(); }, this._lenOptions),
      y: new SCcontrols.NumberInput("Origin Y", 0.0, (theVal) => { this._updateCone(); }, this._lenOptions),
      z: new SCcontrols.NumberInput("Origin Z", 0.0, (theVal) => { this._updateCone(); }, this._lenOptions)
    };
    this._lenInputs.push(this._coneOrig.x, this._coneOrig.y, this._coneOrig.z);
    this._coneGroup.addControls([this._coneOrig.x, this._coneOrig.y, this._coneOrig.z]);

    this._coneDir = {
      x: new SCcontrols.NumberInput("Dir DX", 0.0, (theVal) => { this._updateCone(); }, this._dirOptions),
      y: new SCcontrols.NumberInput("Dir DY", 0.0, (theVal) => { this._updateCone(); }, this._dirOptions),
      z: new SCcontrols.NumberInput("Dir DZ", 1.0, (theVal) => { this._updateCone(); }, this._dirOptions)
    };
    this._lenInputs.push(this._coneDir.x, this._coneDir.y, this._coneDir.z);
    this._coneGroup.addControls([this._coneDir.x, this._coneDir.y, this._coneDir.z]);

    this._coneXDir = {
      x: new SCcontrols.NumberInput("XDir DX", 1.0, (theVal) => { this._updateCone(); }, this._dirOptions),
      y: new SCcontrols.NumberInput("XDir DY", 0.0, (theVal) => { this._updateCone(); }, this._dirOptions),
      z: new SCcontrols.NumberInput("XDir DZ", 0.0, (theVal) => { this._updateCone(); }, this._dirOptions)
    };
    this._lenInputs.push(this._coneXDir.x, this._coneXDir.y, this._coneXDir.z);
    this._coneGroup.addControls([this._coneXDir.x, this._coneXDir.y, this._coneXDir.z]);

    this._coneRadius    = new SCcontrols.NumberInput("Bottom radius", 0.01, (theVal) => { this._updateCone(); }, { min: 0, ...this._lenOptions });
    this._coneTopRadius = new SCcontrols.NumberInput("Top radius", 0.0, (theVal) => { this._updateCone(); }, { min: 0, ...this._lenOptions });
    this._coneHeight    = new SCcontrols.NumberInput("Height", 0.01, (theVal) => { this._updateCone(); }, this._lenOptions);
    this._lenInputs.push(this._coneRadius, this._coneTopRadius, this._coneHeight);

    this._coneAngle  = new SCcontrols.NumberInput("Angle",  360.0, (theVal) => { this._updateCone(); }, { min: 0, max: 360, ...this._angOptions });
    this._angInputs.push(this._coneAngle);
    this._coneGroup.addControls([this._coneRadius, this._coneTopRadius, this._coneHeight, this._coneAngle]);

    this.addControl(this._coneGroup);
  }

  /** Update text label preview. */
  _updateTextLabel() {
    const aPnt = this._getSystemVec3(this._labelPos);
    if (this._tmpLabel == null) {
      let anId = this._scViewer.interactiveContext.generateId(this._builderInput.value);
      this._tmpLabel = new SCwebviewer.InteractiveTextLabel(this._scViewer, anId);
    }
    this._tmpLabel.position = aPnt;
    this._tmpLabel.text = this._labelText.value;
    this._tmpLabel.attachMarker = this._labelAttachMarker.value;
    this._tmpLabel.attachMarkerScale = this._labelAttachMarkerScale.value;
    this._tmpLabel.toDrawRectangle = this._labelDrawRect.value;
    this._tmpLabel.pointPadding = this._labelPointPadding.value;
    this._tmpLabel.rectanglePadding = this._labelRectPadding.value;
    this._tmpLabel.aspects.textHeight = this._labelHeight.value;
    this._tmpLabel.aspects.textFontAspect = this._labelAspect.value;
    this._tmpLabel.aspects.textFont = this._labelFont.value;
    this._tmpLabel.aspects.textJustification = this._labelAlign.value;

    const anIcon = this._labelIcon.value;
    if (anIcon == "") {
      this._tmpLabel.icon = anIcon;
    } else if (this._tmpLabel.icon != anIcon) {
      if (this._scViewer.isImageRegistered(anIcon)) {
        this._tmpLabel.icon = anIcon;
      } else {
        this._scViewer.registerImageFromUrl(anIcon, anIcon).then(() => {
          this._tmpLabel.icon = anIcon;
          this._displayPreview();
        });
        return;
      }
    }

    this._displayPreview();
    window.dispatchEvent(new Event('sceneChanged'));
  }

  /**
   * Add text label builder.
   */
  addTextLabelBuilder() {
    this._labelGroup = new SCcontrols.GroupInput("TextLabelGroup");
    this._labelGroup.isVisible = false;
    this._labelText = new SCcontrols.TextAreaInput("Text", "Label", (theVal) => { this._updateTextLabel(); });
    this._labelPos = { x: new SCcontrols.NumberInput("X", 0.0, (theVal) => { this._updateTextLabel(); }, this._lenOptions),
                       y: new SCcontrols.NumberInput("Y", 0.0, (theVal) => { this._updateTextLabel(); }, this._lenOptions),
                       z: new SCcontrols.NumberInput("Z", 0.0, (theVal) => { this._updateTextLabel(); }, this._lenOptions) };
    this._labelAttachMarker = new SCcontrols.DropdownInput("Attach marker", SCwebviewer.TypeOfMarker.O_PLUS,
      (theVal) => { this._updateTextLabel(); },
    { items: [ {name: "NONE", value: SCwebviewer.TypeOfMarker.EMPTY},
               {name: ".",    value: SCwebviewer.TypeOfMarker.POINT},
               {name: "+",    value: SCwebviewer.TypeOfMarker.PLUS},
               {name: "🞵",    value: SCwebviewer.TypeOfMarker.STAR},
               {name: "✕",    value: SCwebviewer.TypeOfMarker.X},
               {name: "ⵔ",    value: SCwebviewer.TypeOfMarker.O},
               {name: "⊙",    value: SCwebviewer.TypeOfMarker.O_POINT},
               {name: "⊕",    value: SCwebviewer.TypeOfMarker.O_PLUS},
               {name: "⎈",    value: SCwebviewer.TypeOfMarker.O_STAR},
               {name: "⊗",    value: SCwebviewer.TypeOfMarker.O_X} ]});
    this._labelAttachMarkerScale = new SCcontrols.NumberInput("Attach marker scale", 1, (theVal) => { this._updateTextLabel(); },
      { min: 1, max: 7, step: 1 });
    this._labelDrawRect  = new SCcontrols.CheckboxInput("Draw rectangle", true, (theVal) => { this._updateTextLabel(); });

    this._labelHeight = new SCcontrols.NumberInput  ("Font height", 12.0, (theVal) => { this._updateTextLabel(); })
    this._labelAspect = new SCcontrols.DropdownInput("Font aspect", "Regular", (theVal) => { this._updateTextLabel(); },
    { items: [ {name: "Regular",    value: SCwebviewer.FontAspect.Regular},
               {name: "Bold",       value: SCwebviewer.FontAspect.Bold},
               {name: "Italic",     value: SCwebviewer.FontAspect.Italic},
               {name: "BoldItalic", value: SCwebviewer.FontAspect.BoldItalic}, ]});
    this._labelFont = new SCcontrols.DropdownInput("Font name", "sans-serif", (theVal) => { this._updateTextLabel(); },
    { items: [ {name: "sans-serif", value: "sans-serif"},
               {name: "monospace",  value: "monospace"}, ]});
    this._labelAlign = new SCcontrols.DropdownInput("Text alignment", SCwebviewer.ViewCorner.LEFT_LOWER,
      (theVal) => { this._updateTextLabel(); },
    { items: [
      {name: "upper-left",    value: SCwebviewer.ViewCorner.LEFT_UPPER},
      {name: "upper-center",  value: SCwebviewer.ViewCorner.CENTER_UPPER},
      {name: "upper-right",   value: SCwebviewer.ViewCorner.RIGHT_UPPER},
      {name: "center-left",   value: SCwebviewer.ViewCorner.LEFT_CENTER},
      {name: "center",        value: SCwebviewer.ViewCorner.CENTER_CENTER},
      {name: "center-right",  value: SCwebviewer.ViewCorner.RIGHT_CENTER},
      {name: "lower-left",    value: SCwebviewer.ViewCorner.LEFT_LOWER},
      {name: "lower-center",  value: SCwebviewer.ViewCorner.CENTER_LOWER},
      {name: "lower-right",   value: SCwebviewer.ViewCorner.RIGHT_LOWER},
    ]});
    this._labelIcon = new SCcontrols.DropdownInput("Icon image", "", (theVal) => { this._updateTextLabel(); },
    { items: [ {name: "", value: ""},
               {name: "sc-logo.svg",  value: "/images/sc-logo.svg"},
               {name: "lamp.ico",     value: "/images/lamp.ico"}, ]});

    this._labelPointPadding = new SCcontrols.NumberInput("Point padding",    20.0, (theVal) => { this._updateTextLabel(); })
    this._labelRectPadding  = new SCcontrols.NumberInput("Rectangle padding", 5.0, (theVal) => { this._updateTextLabel(); })
    this._lenInputs.push(this._labelPos.x, this._labelPos.y, this._labelPos.z);
    this._labelGroup.addControls([this._labelPos.x, this._labelPos.y, this._labelPos.z,
      this._labelText, this._labelAttachMarker, this._labelAttachMarkerScale, this._labelDrawRect,
      this._labelHeight, this._labelAspect, this._labelFont, this._labelAlign, this._labelIcon,
      this._labelPointPadding, this._labelRectPadding]);

    this.addControl(this._labelGroup);
  }
};
