/**
 * Created by: Andrey Golodyaev
 * Copyright (c) 2022 Huawei Technologies Co., Ltd.
 */
import * as SCwebviewer from '/wasm32/SCwebviewer.js';
import * as SCcontrols from './../SCcontrols.js';

/**
 * Create a sample panel for testing all control types.
 */
export class TemplatePanel extends SCcontrols.PanelInput {
  constructor(theSample) {
    super("Template");

    this._scViewer = theSample.scViewer;

    // Slider control
    // Current value can be get/set with aSliderControl.getValue()/aSliderControl.setValue()
    // If options are changed, should call aSliderControl.updateValues()
    let aSliderControl = new SCcontrols.SliderInput("Slider", /* initial value */0, (val)=> {
        // on modified
      },
      {min: 0, max: 1, step: 0.1}
    );

    // Checkbox control
    // Current value can be get/set with aCheckboxControl.getValue()/aCheckboxControl.setValue()
    let aCheckboxControl = new SCcontrols.CheckboxInput("Checkbox", /* initial value */false, (val)=> {
      // on modified
    });

    // Button control
    let aButtonControl = new SCcontrols.ButtonInput("Button", (val)=> {
      // on pressed
    });

    // Color control
    // Current value can be get/set with aColorControl.getValue()/aColorControl.setValue()
    // Values in format #rrggbb
    let aColorControl = new SCcontrols.ColorInput("Color", /* initial value */"#000000", (val)=> {
      // on modified
    });

    // Text control
    // Current value can be get/set with aCheckboxControl.getValue()/aCheckboxControl.setValue()
    let aTextControl = new SCcontrols.TextInput("Text", /* initial value */"textual value");

    // File input control
    let aFileInputControl = new SCcontrols.FileInput("FileInput", (val)=> {
      // val.files[]
    }, { accept: ".jpg, .png" });

    // Dropdown control
    // Current value can be get/set with aDropdownControl.getValue()/aDropdownControl.setValue()
    // If items in options are changed, should call aDropdownControl.updateValues()
    // Values can be grouped with adding "group" field
    let aDropdownControl = new SCcontrols.DropdownInput("Dropdown", /* initial value */"aaa", (val) => {
      // on modified
    }, {items: [
      {name: "AAA", value: "aaa"},
      {name: "BBB", value: "bbb"},
      {name: "CCC", value: "ccc", group: "Group 1"},
      {name: "DDD", value: "ddd", group: "Group 1"},
    ]});

    // Control for grouping
    let aGroupControl = new SCcontrols.GroupInput("Group");
    aGroupControl.addControls([aFileInputControl, aDropdownControl ]);

    this.addControls([
      aSliderControl,
      aCheckboxControl,
      aButtonControl,
      aColorControl,
      aTextControl,
      aGroupControl
    ]);
  }
}
