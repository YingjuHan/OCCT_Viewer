/**
 * Created by: Andrey Golodyaev
 * Copyright (c) 2022 Huawei Technologies Co., Ltd.
 */
import * as SCwebviewer from '/wasm32/SCwebviewer.js';
import * as SCcontrols from './../SCcontrols.js';

/**
 * Create panel for opening sample models.
 */
export class ImportPanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Import");

    this._scViewer = theSample.scViewer;
    this._scSample = theSample;

    this.addControl(new SCcontrols.ButtonInput("Clear Scene",
      ()=> {
        let objects = this._scViewer.interactiveContext.objectsInside;
        for (let obj of objects) {
          if (obj.isLocked !== true) {
            obj.remove();
          }
        }
        window.dispatchEvent(new Event('sceneChanged'));
        this._scViewer.requestViewUpdate();
      }
    ));

    this.addShapeImport();
    this.addMeshImport();
  }

  /**
   * On panel activated.
   */
  onActivePanelChanged(theIsActivated) {
    super.onActivePanelChanged(theIsActivated);
    if (theIsActivated) {
      this._shapeExplodeDepthInput.setValue(this._scSample.shapeExplodeDepth);
    }
  }

  /**
   * Load shape from the URL.
   */
  loadShapeByUrl (theName, theRoot = "") {
    let aShape = new SCwebviewer.InteractiveShape(theName);
    let anUrl = "/models/" + theRoot + theName;
    let aFilePath = "/" + theName;
    aShape.loadUrlAsync(this._scViewer, anUrl)
    .then(() => {
      aFilePath = aShape.getFilePathFromObjId();
      if (!this._scViewer.unlinkFile(aFilePath)) {
        throw new Error("Cannot unlink a file '" + aFilePath + "'");
      }

      const anExplodedObjects = this._scSample.toExplodeShape
                                  ? aShape.explodeShape(this._scViewer, this._scSample.shapeExplodeDepth)
                                  : null;
      if (anExplodedObjects && anExplodedObjects.length !== 0) {
        for (let i = 0; i < anExplodedObjects.length; ++i) {
          anExplodedObjects[i].isVisible = true;
        }
        this._scSample.onLoad(anExplodedObjects[0]);
      } else {
        aShape.isVisible = true;
        this._scSample.onLoad(aShape);
      }
    }).catch(a => console.log(a));
  }

  /**
   * Add shape import controls.
   */
  addShapeImport() {
    this.addControl(new SCcontrols.FileInput("Load Shape",
      (val)=> {
        if (val.files.length != 1) { return; }
        val.files[0].arrayBuffer().then((aBuffer) => {
          this._scSample.onShapeFileRead(val.files[0].name, aBuffer);
        });
      },
      { accept: ".glb, .brep, .bbrep, .binbrep, .sbrep, .stl"}
    ));
    this.addControl(new SCcontrols.CheckboxInput("Explode Shape", this._scSample.toExplodeShape, (val) => {
      this._scSample.toExplodeShape = val;
    }, { inlineBlock: true }));
    this._shapeExplodeDepthInput = new SCcontrols.NumberInput("Depth", this._scSample.shapeExplodeDepth, (val) => {
      this._scSample.shapeExplodeDepth = val;
    }, { inlineBlock: true, min: -1, max: 1000, step: 1 });
    this.addControl(this._shapeExplodeDepthInput);

    const aSampleShapes = [ "as1-oc-214.bbrep", "fuse.bbrep", "face.brep" ];
    let aBrepGroup = new SCcontrols.GroupInput("BrepGroup");
    for (let anIter = 0; anIter < aSampleShapes.length; ++anIter) {
      let aShape = aSampleShapes[anIter];
      aBrepGroup.addControl(new SCcontrols.ButtonInput(aShape,
        ()=> { this.loadShapeByUrl(aShape, "brep/"); },
        { inlineBlock: true })
      );
    }
    this.addControl(aBrepGroup);

    const aSampleMeshes = [ "as1-oc-214.glb", "ball.glb", "shape.stl" ];
    let aMeshGroup = new SCcontrols.GroupInput("ShapeMeshGroup");
    for (let anIter = 0; anIter < aSampleMeshes.length; ++anIter) {
      let aShape = aSampleMeshes[anIter];
      aMeshGroup.addControl(new SCcontrols.ButtonInput(aShape,
        ()=> { this.loadShapeByUrl(aShape, aShape.includes(".stl") ? "stl/" : "gltf/"); },
        { inlineBlock: true })
      );
    }
    this.addControl(aMeshGroup);
  }

  /**
   * Load mesh from URL.
   */
  loadMeshByUrl (theName, theRoot = "") {
    let aMesh = new SCwebviewer.InteractiveCaeMesh(theName);
    let anUrl = "/models/" + theRoot + theName;
    let aFilePath = "/" + theName;
    aMesh.loadUrlAsync(this._scViewer, anUrl)
    .then(() => {
      this._scSample.setMeshDefaultProps(aMesh);
      aMesh.isVisible = true;
      this._scSample.onLoad();
    }).catch(a => console.log(a));
  }

  /**
   * Add mesh import controls.
   */
  addMeshImport() {
    this.addControl(new SCcontrols.FileInput("Load Mesh",
      (val)=> {
        if (val.files.length != 1) { return; }
        val.files[0].arrayBuffer().then((aBuffer) => {
          this._scSample.onMeshFileRead(val.files[0].name, aBuffer);
        });
      },
      { accept: ".pos, .msh, .mdb"}
    ));

    const aSampleMeshes = [ "buckling22.pos", "LE10disp22.pos" ];
    for (let anIter = 0; anIter < aSampleMeshes.length; ++anIter) {
      let aMesh = aSampleMeshes[anIter];
      this.addControl(new SCcontrols.ButtonInput(aMesh,
        ()=> { this.loadMeshByUrl(aMesh, "gmsh/"); },
        { inlineBlock: true })
      );
    }
  }
}
