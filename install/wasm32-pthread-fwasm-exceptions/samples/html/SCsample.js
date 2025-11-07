/**
 * Created by: Andrey Golodyaev
 * Copyright (c) 2022 Huawei Technologies Co., Ltd.
 */
import * as SCwebviewer from '/wasm32/SCwebviewer.js';

/**
 * Viewer sample class.
 */
export class SCViewerSample {
  constructor() {
    this.scViewer = null;
    this._hasWasm = true;

    this.shape = null;
    this.mesh = null;

    this.toExplodeShape = false;
    this.shapeExplodeDepth = -1;

    this.useMeshGenGeom = false;
    this.useMeshTestCacheGenGeom = false;

    this.updateCanvasSize = this.updateCanvasSize.bind(this);
  }

  /**
   * Create the viewer.
   */
  createViewer() {
    // export API for debugging in Browser console - shouldn't be used in production
    window.SCwebviewer = SCwebviewer;

    this.scViewer = window.scViewer = new SCwebviewer.SCwebviewer('/');

    // configure messages
    this.scViewer.traceLevel = SCwebviewer.MessageGravity.Trace;
    const aPrintTerm = this.scViewer.printMessage;
    this.scViewer.printMessage = (theText, theGravity) => {
      aPrintTerm(theText, theGravity);

      // @c const aGuiLevel = SCwebviewer.MessageGravity.Warning;
      const aGuiLevel = 100; // nothing
      if (theGravity.valueOf() < aGuiLevel) { return; }

      alert(theText);
      switch (theGravity) {
        case SCwebviewer.MessageGravity.Trace:   return;
        case SCwebviewer.MessageGravity.Info:    return;
        case SCwebviewer.MessageGravity.Warning: return;
        case SCwebviewer.MessageGravity.Alarm:
        case SCwebviewer.MessageGravity.Fail:    return;
      }
    };

    if (!this.scViewer.isWasmSupported()) {
      this._hasWasm = false;
      var anElement = document.getElementById('output');
      anElement.innerHTML += "Browser is too old - WebAssembly support is missing!<br>Please check updates or install a modern browser.<br>";
    }
  }

  /**
   * Initialize the viewer.
   */
  async initViewer() {
    if (!this._hasWasm) { return; }
    try {
      await this.scViewer.init();
    } catch(anErr) {
      throw anErr;
    }

    window.addEventListener("resize", this.updateCanvasSize);
    this.updateCanvasSize();

    this.scViewer.renderingParams.backgroundColor = "black";
    this.scViewer.viewCube.isVisible = true;
    this.scViewer.viewCube.isLocked = true; // mark to avoid cleaning with scene (add custom property)
    this.scViewer.scaleRuler.isVisible = true;
    this.scViewer.scaleRuler.isLocked = true;
    this.scViewer.camera.setOrientation(SCwebviewer.CameraOrientation.FRONT);

    this.scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onColorMapperChanged, this.updateColorScale.bind(this));

    // Example viewer offsets for color scales positioning
    // @c this._testViewCornerOffsets();

    // Example - get detected object with subparts
    // @c this._testDetectWithSubparts();
  }

  /** Example viewer offsets for color scales positioning. */
  _testViewCornerOffsets() {
    this.scViewer.viewManager.setViewOffset(SCwebviewer.ViewCorner.RIGHT_UPPER, {x: 300, y: 100});
    this.scViewer.viewManager.setViewOffset(SCwebviewer.ViewCorner.CENTER_UPPER, {x: 0, y: 50});
  }

  /** Example - get detected object with subparts. */
  _testDetectWithSubparts() {
    this.scViewer.eventManager.addEventListener(SCwebviewer.UserEvents.onDetectedObjectChanged, () => {
      const aDetObj = this.scViewer.selectionManager.lastDetectedInfo;
      if (aDetObj.object != null) {
        if (aDetObj.object instanceof SCwebviewer.InteractiveColorScale) {
          console.log("Detected color scale, part "
                    + SCwebviewer.ColorScaleSelectionPart[aDetObj.part.id]
                    + ", subpart " + aDetObj.part.subId);
        } else if (aDetObj.object instanceof SCwebviewer.InteractiveCaeMesh) {
          console.log("Detected cae mesh, part "
                    + SCwebviewer.MeshEntityType[aDetObj.part.id]
                    + ", subpart " + aDetObj.part.subId);
        } else if (aDetObj.object instanceof SCwebviewer.InteractiveShape) {
          console.log("Detected shape, part "
                    + SCwebviewer.ShapeObjectSelMode[aDetObj.part.id]
                    + ", subpart " + aDetObj.part.subId);
        } else {
          console.log("Detected object, part " + aDetObj.part.id + ", subpart " + aDetObj.part.subId);
        }
      } else {
        console.log("Detected cleared");
      }
    });
  }

  /**
   * Resize canvas to fit into window.
   */
  updateCanvasSize() {
    // size of canvas in logical (density-independent) units
    ViewerCanvas.style.width = `100%`;
    ViewerCanvas.style.height = `100%`;

    // drawing buffer size (aka backing store)
    const aDevicePixelRatio = window.devicePixelRatio || 1;
    ViewerCanvas.width  = ViewerCanvas.offsetWidth * aDevicePixelRatio;
    ViewerCanvas.height = ViewerCanvas.offsetHeight * aDevicePixelRatio;
    this.scViewer.resizeViewport();
  }

  /**
   * File uploading
   */
  onFileInputChanged (theInput) {
    for (let i = 0; i < theInput.files.length; ++i) {
      const aFile = theInput.files[i];
      this.onFileDropped(aFile);
    }
  }

  /**
   * Handle (file) drag over event.
   */
  onDragOverEvent (theEvent) {
    // prevent file from being opened by browser itself
    theEvent.preventDefault();
  }

  /**
   * Handle (file) drop event.
   */
  onDropEvent (theEvent) {
    // prevent file from being opened by browser itself
    theEvent.preventDefault();
    if (theEvent.dataTransfer.items) {
      // use DataTransferItemList interface to access the file(s)
      for (let i = 0; i < theEvent.dataTransfer.items.length; ++i) {
        // if dropped items aren't files, reject them
        if (theEvent.dataTransfer.items[i].kind === 'file') {
          const aFile = theEvent.dataTransfer.items[i].getAsFile();
          // @c console.log('D... file[' + i + '].name = ' + aFile.name);
          this.onFileDropped(aFile);
        }
      }
    } else {
      // use DataTransfer interface to access the file(s)
      for (let i = 0; i < theEvent.dataTransfer.files.length; ++i) {
        const aFile = theEvent.dataTransfer.files[i];
        // @c console.log('F... file[' + i + '].name = ' + aFile.name);
        this.onFileDropped(aFile);
      }
    }
  }

  /**
   * File uploading
   */
  onFileDropped (theFile) {
    let aReader = new FileReader();
    let aViewer = this.scViewer;
    let aSample = this;
    aReader.onload = function() {
      let aMeshFormat  = SCwebviewer.InteractiveCaeMesh.probeFileFormat(aViewer, aReader.result, theFile.name);
      let aShapeFormat = SCwebviewer.InteractiveShape.probeFileFormat(aViewer, aReader.result, theFile.name);
      if (aMeshFormat !== "") {
        const aMeshPrs = aSample.mesh;
        if (aMeshFormat === "MDBPROP" && aMeshPrs != null) {
          aMeshPrs.loadPropertiesAsync(aViewer, aReader.result)
          .then(() => {
            aSample.setMeshDefaultProps(aMeshPrs);
            aSample.onLoad(aMeshPrs);
          })
          .catch(a => console.log(a));
        } else {
          aSample.onMeshFileRead(theFile.name, aReader.result);
        }
      } else if (aShapeFormat !== "") {
        aSample.onShapeFileRead(theFile.name, aReader.result);
      } else {
        console.error("Unable to open unsupported file format '" + theFile.name + "'");
      }
    };
    aReader.readAsArrayBuffer(theFile);
  }

  /**
   * Open shape file.
   */
  onShapeFileRead (theFileName, theBuffer) {
    let aFilePath = "/models/" + theFileName;
    let aShapePrs = new SCwebviewer.InteractiveShape();
    aShapePrs.openBufferAsync(this.scViewer, theBuffer, aFilePath).then(() => {
      // @c if (this.shape != null && this.shape.id !== aShapePrs.id) { this.shape.remove(); }
      this.shape = aShapePrs;

      aShapePrs.loadDataAsync(this.scViewer).then(() => {
        aFilePath = aShapePrs.getFilePathFromObjId();
        if (!this.scViewer.unlinkFile(aFilePath)) {
          throw new Error("Cannot unlink a file '" + aFilePath + "'");
        }

        const anExplodedObjects = this.toExplodeShape ? aShapePrs.explodeShape(this.scViewer, this.shapeExplodeDepth) : null;
        if (anExplodedObjects && anExplodedObjects.length !== 0) {
          for (let i = 0; i < anExplodedObjects.length; ++i) {
            anExplodedObjects[i].isVisible = true;
          }
          this.onLoad(anExplodedObjects[0]);
        } else {
          aShapePrs.isVisible = true;
          this.onLoad(aShapePrs);
        }
      }).catch(anErr => {
        console.error("Loading shape failed: " + anErr);
      });
    }).catch(anErr => {
      console.error("Opening shape failed: " + anErr);
    });
  };

  /**
   * Open mesh file.
   */
  onMeshFileRead (theFileName, theBuffer) {
    let aFilePath = "/models/" + theFileName;
    let aMeshPrs  = new SCwebviewer.InteractiveCaeMesh();
    aMeshPrs.toCacheProperties = true;
    aMeshPrs.loadBufferAsync(this.scViewer, theBuffer, aFilePath).then(() => {
      // @c if (this.mesh != null && this.mesh.id !== aMeshPrs.id) { this.mesh.remove(); }
      this.mesh = aMeshPrs;

      if (this.useMeshGenGeom) {
        aMeshPrs.setUseGenGeometry();
      }
      if (this.useMeshTestCacheGenGeom) {
        aMeshPrs.setUseTestCacheGenGeometry();
      }

      aMeshPrs.isVisible = true;
      this.setMeshDefaultProps(aMeshPrs);
      this.onLoad(aMeshPrs);
    }).catch(anErr => {
      console.error ("Loading mesh failed: " + anErr);
    });
  };

  /**
   * Setup mesh presentation defaults.
   */
  setMeshDefaultProps (theMesh) {
    let aPrs = theMesh.createPresentation(SCwebviewer.CaeMeshPresentationType.Scalar);

    let aProperties = theMesh.properties;
    if (aProperties.length > 0) {
      aPrs.activeProperty = aProperties[0].propId;

      const aTimeSteps = theMesh.availableTimeSteps;
      if (aTimeSteps.length > 0) {
        theMesh.activeTimeStep = aTimeSteps[0].timeStep;
      }
    }

    aPrs.presentationMode = SCwebviewer.CaeMeshObjectPrsMode.FlatShaded;
    theMesh.selectionMode = SCwebviewer.CaeMeshObjectSelMode.Elements | SCwebviewer.CaeMeshObjectSelMode.Nodes;
  }

  /** Define default colormap. */
  _resetColorscaleColors(theParams, theIsGroup) {
    if (theIsGroup) {
      theParams.isSmooth = false;
      return;
    }

    theParams.isSmooth = true;
    theParams.isCustomColors = true;
    theParams.nbIntervals = 3;
    theParams.customColors.push_back(SCwebviewer.ColorTools.convertRgbToColorString({r: 0, g: 0, b: 255}));
    theParams.customColors.push_back(SCwebviewer.ColorTools.convertRgbToColorString({r: 0, g: 255, b: 0}));
    theParams.customColors.push_back(SCwebviewer.ColorTools.convertRgbToColorString({r: 255, g: 0, b: 0}));
  }

  /**
   * Update color scale object.
   */
  updateColorScale(theEvent) {
    if (theEvent.mesh === null) { return; }

    if (theEvent.changeStatus == SCwebviewer.ColorScaleChangeStatus.Create) {
      let aColorScale = new SCwebviewer.InteractiveColorScale(this.scViewer);
      aColorScale.attach(theEvent.mesh, theEvent.propertyId, theEvent.componentIndex);
      aColorScale.viewCorner = SCwebviewer.ViewCorner.RIGHT_CENTER;
      aColorScale.isVisible = true;

      let aParams = aColorScale.params;
      this._resetColorscaleColors(aParams, theEvent.mesh.isPropertyGroup(theEvent.propertyId));
      aParams.size = { x: 20, y: 0.96 }; // bar breadth (in pixels) and height (as ratio from canvas height)
      aParams.maxSize = 0.96;
      aParams.labelSize = 12;
      aColorScale.params = aParams;

      aColorScale.addEventListener (SCwebviewer.UserEvents.onObjectMouseClick, (theEvent) =>
      {
        // @c console.log ("CLICKED ColorScale: part" + theEvent.part + " btn" + theEvent.button + " " + theEvent.point.x + "x" + theEvent.point.y);
        if (theEvent.button == 0x2
        && (theEvent.part.id == SCwebviewer.ColorScaleSelectionPart.LabelMin
          || theEvent.part.id == SCwebviewer.ColorScaleSelectionPart.LabelMax)) {
          theEvent.preventDefault();

          const anInitVal = theEvent.part.id == SCwebviewer.ColorScaleSelectionPart.LabelMin
                          ? aColorScale.defaultRange.x : aColorScale.defaultRange.y;
          const aText = prompt("Please enter new range", anInitVal);
          if (aText) {
            if (theEvent.part.id == SCwebviewer.ColorScaleSelectionPart.LabelMin) {
              aColorScale.displayRange = { x: Number(aText), y: aColorScale.displayRange.y };
            } else {
              aColorScale.displayRange = { x: aColorScale.displayRange.x, y: Number(aText) };
            }
            this.scViewer.requestViewUpdate();
          }
        }
      });

      window.dispatchEvent(new Event('resize'));

      this.scViewer.viewManager.invalidate2dObjects(aColorScale.id);
      window.dispatchEvent(new Event('sceneChanged'));
    } else {
      // Color scale property was updated
      if (theEvent.propertyId !== "" && theEvent.colorScale != null) {
        let aParams = theEvent.colorScale.params;
        this._resetColorscaleColors(aParams, theEvent.mesh.isPropertyGroup(theEvent.propertyId));
        theEvent.colorScale.params = aParams;
      }
    }
  }

  /**
   * Update viewer after model loading.
   */
  onLoad(thePrs) {
    this.scViewer.camera.fitAllAuto(true);
    this.scViewer.requestViewUpdate();
    window.dispatchEvent(new Event('sceneChanged'));
  }
}
