import * as SCwebviewer from '/wasm32/SCwebviewer.js';
import * as SCcontrols from './../SCcontrols.js';

/**
 * Create panel for running simple benchmarks.
 */
export class BenchmarkPanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Benchmark");

    this._scViewer = theSample.scViewer;
    this._scSample = theSample;

    this._models = [
      { label: "BREP: as1-oc-214.bbrep", type: "shape", root: "brep/", name: "as1-oc-214.bbrep" },
      { label: "BREP: fuse.bbrep", type: "shape", root: "brep/", name: "fuse.bbrep" },
      { label: "Mesh: buckling22.pos", type: "mesh", root: "gmsh/", name: "buckling22.pos" }
    ];

    this._modelInput = new SCcontrols.DropdownInput("Model", this._models[0].name, () => {}, {
      items: this._models.map((m) => ({ name: m.label, value: m.name }))
    });
    this.addControl(this._modelInput);

    this._runBtn = new SCcontrols.ButtonInput("Run Benchmark", () => {
      this.runBenchmark().catch((err) => console.warn(err));
    });
    this.addControl(this._runBtn);

    this._resultInfo = new SCcontrols.TextAreaInput("", "", null, { autoHeight: true, readOnly: true });
    this._resultInfo.minWidth = 260;
    this.addControl(this._resultInfo);
  }

  _now() {
    return (typeof performance !== "undefined" && performance.now) ? performance.now() : Date.now();
  }

  _memInfo() {
    if (typeof performance !== "undefined" && performance.memory) {
      return {
        usedJSHeapSize: performance.memory.usedJSHeapSize,
        totalJSHeapSize: performance.memory.totalJSHeapSize
      };
    }
    return null;
  }

  async _waitFrame() {
    return new Promise((resolve) => {
      requestAnimationFrame(() => requestAnimationFrame(resolve));
    });
  }

  _clearScene() {
    const objects = this._scViewer?.interactiveContext?.objectsInside ?? [];
    for (let obj of objects) {
      if (obj.isLocked !== true) {
        obj.remove();
      }
    }
    window.dispatchEvent(new Event('sceneChanged'));
    this._scViewer.requestViewUpdate();
  }

  async _loadShape(model) {
    const shape = new SCwebviewer.InteractiveShape(model.name);
    const url = "/models/" + model.root + model.name;
    await shape.loadUrlAsync(this._scViewer, url);
    const filePath = shape.getFilePathFromObjId();
    this._scViewer.unlinkFile(filePath);
    shape.isVisible = true;
    this._scSample.onLoad(shape);
  }

  async _loadMesh(model) {
    const mesh = new SCwebviewer.InteractiveCaeMesh(model.name);
    const url = "/models/" + model.root + model.name;
    await mesh.loadUrlAsync(this._scViewer, url);
    this._scSample.setMeshDefaultProps(mesh);
    mesh.isVisible = true;
    this._scSample.onLoad(mesh);
  }

  async runBenchmark() {
    const selectedName = this._modelInput.value || this._models[0].name;
    const model = this._models.find((m) => m.name === selectedName) || this._models[0];
    this._resultInfo.setValue("Running...");

    this._clearScene();
    const memBefore = this._memInfo();
    const start = this._now();

    if (model.type === "shape") {
      await this._loadShape(model);
    } else {
      await this._loadMesh(model);
    }

    const loadEnd = this._now();
    this._scViewer.requestViewUpdate();
    await this._waitFrame();
    const firstFrame = this._now();
    const memAfter = this._memInfo();

    const result = {
      model: model.name,
      type: model.type,
      loadMs: Number((loadEnd - start).toFixed(2)),
      firstFrameMs: Number((firstFrame - start).toFixed(2)),
      memBefore,
      memAfter
    };
    this._resultInfo.setValue(JSON.stringify(result, null, 2));
  }
}
