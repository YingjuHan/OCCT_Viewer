import * as SCcontrols from './../SCcontrols.js';

/**
 * Panel with viewer settings.
 */
export class SettingsPanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Settings");

    this._scViewer = theSample.scViewer;
    this._scSample = theSample;
    this._isFirstInit = true;
  }

  /** Hide temporary objects. */
  onActivePanelChanged(theIsActivated) {
    if (this._isFirstInit && theIsActivated && this._scViewer.unitManager) {
      this._isFirstInit = false;
      this.addUnitsSettings();
      this.addBackgroundSettings();
      this.addAntialiasingSettings();
      this.addProjectionSettings();
      this.addPbrSettings();
      this.addShadowsSettings();
      this.addTransparencySettings();
      this.addMultipleSelectionMode();
      this.addTestSettings();
      super.onActivePanelChanged(theIsActivated);
      this.redraw();
      return;
    }
    super.onActivePanelChanged(theIsActivated);
  }

  /** Create unit item list. */
  _makeUnitItems() {
    let anItems = new Array();
    const aLenUnits = Object.keys(SCwebviewer.LengthUnit);
    for (let aUnitIter of aLenUnits) {
      const aUnit = SCwebviewer.LengthUnit[aUnitIter];
      anItems.push({name: aUnitIter + " (" + aUnit + ")", value: aUnit });
    }
    // @c return {name: theName + " (" + theUnit + ")", value: theUnit },
    return anItems;
  }

  /**
   * Add units settings.
   */
  addUnitsSettings() {
    this.addControls([
      new SCcontrols.DropdownInput("Display units", this._scViewer.unitManager.length.unitDisplay,
        (theUnit) => {
          this._scViewer.unitManager.length.unitDisplay = theUnit;
          this._scViewer.requestViewUpdate();
        }, {items: this._makeUnitItems()}),
      new SCcontrols.DropdownInput("System units", this._scViewer.unitManager.length.unitSystem,
        (theUnit) => {
          this._scViewer.unitManager.length.unitSystem = theUnit;
          this._scViewer.requestViewUpdate();
        }, {items: this._makeUnitItems()}),
    ]);
  }

  /**
   * Add background settings.
   */
  addBackgroundSettings() {
    this.addControls([
      new SCcontrols.ColorInput("Background Color", this._scViewer.renderingParams.backgroundColor,
                  (val)=> {
                    this._scViewer.renderingParams.backgroundColor = val;
                    this._scViewer.requestViewUpdate();
                  }, { inlineBlock: true }),
      new SCcontrols.CheckboxInput("Show Stats", this._scViewer.renderingParams.toShowStats,
                  (val)=> {
                    this._scViewer.renderingParams.toShowStats = val;
                    this._scViewer.requestViewUpdate();
                  }, { inlineBlock: true })

    ]);
  }

  /**
   * Add antialiasing settings.
   */
  addAntialiasingSettings() {
    this.addControls([
      new SCcontrols.SliderInput("Rendering resolution ratio", this._scViewer.renderingParams.renderResolutionScale,
        (val)=> {
          this._scViewer.renderingParams.renderResolutionScale = val;
          this._scViewer.requestViewUpdate();
        },
        {min: 0.5, max: 2.0, step: 0.1}),
      new SCcontrols.SliderInput("MSAA", this._scViewer.renderingParams.nbMsaaSamples,
        (val)=> {
          this._scViewer.renderingParams.nbMsaaSamples = val;
          this._scViewer.requestViewUpdate();
        },
        {min: 0, max: 4, step: 1})
    ]);
  }

  /**
   * Add camera projection settings.
   */
  addProjectionSettings() {
    this.addControls([
      new SCcontrols.CheckboxInput("Turntable rotation", this._scViewer.camera.toLockOrbitZUp,
        (val) => {
          this._scViewer.camera.toLockOrbitZUp = val;
          this._scViewer.requestViewUpdate();
        }),
      new SCcontrols.DropdownInput("Stereoscopic output", this._scViewer.camera.stereoMode,
        (mode) => {
          this._scViewer.camera.stereoMode = mode;
          this._scViewer.requestViewUpdate();
        }, {items: [
          {name: "Monoscopic",            value: SCwebviewer.StereoMode.MONO},
          {name: "Red-Cyan Anaglyph",     value: SCwebviewer.StereoMode.REDCYAN},
          {name: "Row-interlaced",        value: SCwebviewer.StereoMode.ROWINTERLACED},
          {name: "Row-interlaced (inv.)", value: SCwebviewer.StereoMode.ROWINTERLACEDINV},
        ]}),
      new SCcontrols.CheckboxInput("Perspective projection", this._scViewer.camera.isPerspectiveProjection,
        (val) => {
          this._scViewer.camera.isPerspectiveProjection = val;
          this._scViewer.requestViewUpdate();
        }),
      new SCcontrols.SliderInput("Perspective FOVy", this._scViewer.camera.perspectiveProjectionFOVy,
        (val) => {
          this._scViewer.camera.perspectiveProjectionFOVy = val;
          this._scViewer.requestViewUpdate();
        }, {min: 10, max: 120, step: 1})
    ]);
  }

  /**
   * Add PBR settings.
   */
  addPbrSettings() {
    this.addControls([
      new SCcontrols.CheckboxInput("PBR", this._scViewer.renderingParams.isPbrEnabled,
        (val)=> {
          this._scViewer.renderingParams.isPbrEnabled = val;
          this._scViewer.requestViewUpdate();
        }),
      new SCcontrols.CheckboxInput("Image-Based Lighting", this._scViewer.renderingParams.isImageBasedLighting,
        (val)=> {
          this._scViewer.renderingParams.isImageBasedLighting = val;
          this._scViewer.requestViewUpdate();
        }, { inlineBlock: true }),
      new SCcontrols.FileInput("Cubemap",
        (val)=> {
          if (val.files.length != 1) { return; }
          let tempFile = "/" + val.files[0].name;
          val.files[0].arrayBuffer()
          .then((aBuffer) => {
            this._scViewer.uploadArrayBuffer(aBuffer, tempFile, true)
            .then(() => {
              this._scViewer.renderingParams.setBackgroundCubemap(tempFile, false);
              this._scViewer.unlinkFile(tempFile);
              this._scViewer.requestViewUpdate();
            })
            .catch(a => console.log(a));
          });
        },
        { accept: ".jpg, .png", inlineBlock: true })
    ]);
  }

  /**
   * Add shadows settings.
   */
  addShadowsSettings() {
    this._groupShadows = new SCcontrols.GroupInput("Shadows");
    this._groupShadows.addControls([
      new SCcontrols.CheckboxInput("Shadows", this._scViewer.renderingParams.isShadowEnabled,
        (val)=> {
          this._scViewer.renderingParams.isShadowEnabled = val;
          this._scViewer.requestViewUpdate();
        }),
      new SCcontrols.SliderInput("Shadow map resolution", this._scViewer.renderingParams.shadowMapResolution,
        (val)=> {
          this._scViewer.renderingParams.shadowMapResolution = val;
          this._scViewer.requestViewUpdate();
        },
        {min: 16, max: 2048, step: 1}),
      new SCcontrols.SliderInput("Shadow map bias", this._scViewer.renderingParams.shadowMapBias,
        (val)=> {
          this._scViewer.renderingParams.shadowMapBias = val;
          this._scViewer.requestViewUpdate();
        },
        { min: 0, max: 1, step: 0.0001, format: (num)=>{ return num.toFixed(4); } }),
    ]);

    this.addControl(this._groupShadows);
  }

  /**
   * Add transparency settings.
   */
  addTransparencySettings() {
    this._groupTransp = new SCcontrols.GroupInput("Transparency");
    this._groupTransp.addControls([
      new SCcontrols.DropdownInput("Transparency method", SCwebviewer.RenderTransparentMethod.Unordered,
        (mode) => {
          this._scViewer.renderingParams.transparencyMethod = mode;
          this._scViewer.requestViewUpdate();
        }, {items: [
          {name: "Unordered", value: SCwebviewer.RenderTransparentMethod.Unordered},
          {name: "Weighted OIT", value: SCwebviewer.RenderTransparentMethod.BlendOit},
          {name: "Depth Peeling OIT", value: SCwebviewer.RenderTransparentMethod.DepthPeelingOit},
        ]}),
      new SCcontrols.SliderInput("Depth peeling layers", this._scViewer.renderingParams.nbOitDepthPeelingLayers,
        (val)=> {
          this._scViewer.renderingParams.nbOitDepthPeelingLayers = val;
          this._scViewer.requestViewUpdate();
        },
        {min: 1, max: 16, step: 1}),
    ]);

    this.addControl(this._groupTransp);
  }

  /**
   * Add multiple selection mode settings.
   */
  addMultipleSelectionMode() {
    this._groupSelection = new SCcontrols.GroupInput("allowMultipleSelection");
    this._groupSelection.addControls([
      new SCcontrols.CheckboxInput("Allow multi-selection", this._scViewer.selectionManager.toAllowMultiselection,
      (val)=> {
        this._scViewer.selectionManager.toAllowMultiselection = val;
        this._scViewer.requestViewUpdate();
      })
    ]);
    this.addControl(this._groupSelection);
  }

  /**
   * Add test settings.
   */
  addTestSettings() {
    this._groupTest = new SCcontrols.GroupInput("Test");
    this._groupTest.addControls([
      new SCcontrols.CheckboxInput("Use mesh gen geometry", false,
        (val) => {
          this._scSample.useMeshGenGeom = val;
        }),
      new SCcontrols.CheckboxInput("Use mesh test cache gen geometry", false,
        (val) => {
          this._scSample.useMeshTestCacheGenGeom = val;
        }),
    ]);

    this.addControl(this._groupTest);
  }
}
