import * as SCwebviewer from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import * as SCcontrols from './../SCcontrols.js';

/**
 * Create panel for export actions.
 */
export class ExportPanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Export");

    this._scViewer = theSample.scViewer;
    this._scSample = theSample;

    this.addControl(new SCcontrols.ButtonInput("Download Screenshot (PNG)", () => {
      this._scViewer.makeScreenshot("viewer.png", -1, -1);
    }));

    this.addControl(new SCcontrols.ButtonInput("Export Selected Shape", () => {
      const shape = this._getSelectedShape();
      if (!shape) {
        console.warn("Export shape: select a shape first");
        return;
      }
      shape.save();
    }));

    this.addControl(new SCcontrols.ButtonInput("Export Selected Mesh", () => {
      const mesh = this._getSelectedMesh();
      if (!mesh) {
        console.warn("Export mesh: select a mesh first");
        return;
      }
      mesh.save();
    }));
  }

  _getSelectedShape() {
    const sel = this._scViewer.selectionManager?.oneSelectedObject;
    if (sel instanceof SCwebviewer.InteractiveShape) {
      return sel;
    }
    return this._scSample.shape ?? null;
  }

  _getSelectedMesh() {
    const sel = this._scViewer.selectionManager?.oneSelectedObject;
    if (sel instanceof SCwebviewer.InteractiveCaeMesh) {
      return sel;
    }
    return this._scSample.mesh ?? null;
  }
}
