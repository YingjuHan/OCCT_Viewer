/**
 * @file
 * @brief This file defines auxiliary classes for defining a simple user interface
 * via controls like Panels, Sliders, Buttons, etc.
 */

/**
 * Defines the list of supported controls.
 */
export const ControlType = {
  SLIDER: "SLIDER",
  DUALSLIDER: "DUALSLIDER",
  CHECKBOX: "CHECKBOX",
  COLOR: "COLOR",
  BUTTON: "BUTTON",
  FILEINPUT: "FILEINPUT",
  GROUP: "GROUP",
  LABEL: "LABEL",
  TEXTINPUT: "TEXTINPUT",
  TEXTAREA: "INPUTTEXTAREA",
  DROPDOWN: "DROPDOWN",
  NUMBERINPUT: "NUMBERINPUT",
  TREEVIEWINPUT: "TREEVIEWINPUT"
};

/**
 * Represents UI control model such as button slider and etc.
 */
export class Control {
  constructor(theName, theType, theInitialValue, theOnChange, theOptions) {
    this.name = theName;
    this.type = theType;
    this.controls = [];
    this.initialValue = theInitialValue;
    this.onChange = theOnChange;
    this.options = theOptions || {};
    this._isVisible = true;
    this.element = null;
    this.parent = null;
    this.lastFocusTime = -1.0;
  }

  /** Value getter placeholder. */
  getValue() { throw new Error("Undefined property getValue"); return null; }

  /** Value setter placeholder. */
  setValue(theVal) { throw new Error("Undefined property setValue"); }

  /** Value getter. */
  get value() { return this.getValue(); }

  /** Value setter. */
  set value(theVal) { this.setValue(theVal); }

  /** Create checkbox (Boolean flag input) control. */
  createHtmlItem() { throw new Error("Unimplemented method"); }

  /** Return name property with spaces replaced by underscores. */
  getSafeName() {
    return this.name.replace(/ /g,"_");
  }

  /** Generate control id from it's name. */
  getControlId() {
    let aPrefix = this.parent ? this.parent.getControlId() : "control";
    return aPrefix + "_" + this.getSafeName();
  }

  /** Add single controls. */
  addControl(theControl) {
    if (theControl) {
      theControl.parent = this;
      this.controls = [...this.controls, theControl];
    }
  }

  /** Add list of controls. */
  addControls(theControls) {
    if (theControls) {
      for (let aCtrlIter of theControls) {
        aCtrlIter.parent = this;
      }
      this.controls = [...this.controls, ...theControls];
    }
  }

  /** Find control by name recursively. */
  findControlByName(theName) {
    for (let aControl of this.controls) {
      if (aControl.name == theName) {
        return aControl;
      }

      let aGroupControl = aControl.findControlByName(theName);
      if (aGroupControl != null) {
        return aGroupControl;
      }
    }

    return null;
  }

  /** Find an element from control's name. */
  getElementByControlName(theName) {
    let aControl = this.findControlByName(theName);
    if (aControl == null) {
      throw new Error(`Control '${theName}' not found in '${this.name}' panel`);
    }
    return aControl.element;
  }

  /** Visibility flag getter. */
  get isVisible() { return this._isVisible; }

  /** Visibility flag setter. */
  set isVisible(theVal) { this.setVisibility(theVal); }

  /** Set visibility of control. */
  setVisibility(theIsVisible) {
    this._isVisible = theIsVisible;
    if (this.element) {
      this.element.hidden = !theIsVisible;
    }
    if (this.options && this.options.inlineBlock) {
      this.element.className = theIsVisible ? "controlItemInlineBlock" : "controlItem";
    }
  }

  /** Add list of DOM elements item to parent element. */
  addControlItem (theElements) {
    let anItem = document.createElement("div");
    anItem.className = this.options.inlineBlock ? "controlItemInlineBlock" : "controlItem";
    for (let anEl of theElements) {
      anItem.appendChild(anEl);
    }
    this.element = anItem;
    if (!this._isVisible) { this.element.hidden = true; }
    this.parent.element.appendChild(anItem);
  }
}

/**
 * Represents group of controls.
 */
export class GroupInput extends Control {
  constructor(theName, theIsCollapsible = false, theIsCollapsed = false) {
    super(theName, ControlType.GROUP, null, null, null);
    this._isCollapsible = theIsCollapsible;
    this._isCollapsed = theIsCollapsed;
    this._minWidth = -1;
  }

  /** Get minWidth style property; -1 means undefined. */
  get minWidth() { return this._minWidth; }

  /** Set minWidth style property. Should be set before displaying element. */
  set minWidth(theWidth) { this._minWidth = theWidth; }

  /**
   * Create control grouping other controls.
   */
  createHtmlItem() {
    let aGroup = document.createElement("div");
    aGroup.id = this.getControlId();
    aGroup.className = "group";
    this.parent.element.appendChild(aGroup);
    this.element = aGroup;
    if (this._minWidth != -1) {
      aGroup.style.minWidth = this._minWidth.toString() + "px";
    }

    if (this._isCollapsible) {
      this._expandButton = document.createElement("input");
      this._expandButton.id = aGroup.id + "_expand";
      this._expandButton.type = "button";
      this._expandButton.className = "collapsible";
      this._expandButton.onclick = () => {
        this.setCollapsed(!this._isCollapsed);
        this.onCollapsedChanged(this._isCollapsed);
      };
      aGroup.appendChild(this._expandButton);

      let aSubGroup = document.createElement("div");
      aSubGroup.id = aGroup.id + "_sub";
      aSubGroup.className = "group";
      this.element = aSubGroup;
      aGroup.appendChild(aSubGroup);
    }

    if (!this._isVisible) { this.element.hidden = true; }
    for (let anItem of this.controls) {
      anItem.createHtmlItem();
    }

    if (this._isCollapsible) {
      this.setCollapsed(this._isCollapsed);
    }
  }

  /** Collapsed flag getter. */
  get isCollapsed() { return this._isCollapsed; }

  /** Collapsed flag setter. */
  set isCollapsed(theToCollapse) { this.setCollapsed(theToCollapse); }

  /**
   * Update visibility of children if marked as collapsible.
   */
  setCollapsed(theIsCollapsed) {
    this._isCollapsed = theIsCollapsed;
    this._expandButton.value = (this._isCollapsed ? "+" : "-") + " " + this.name;
    if (this._isCollapsed) {
      this._expandButton.classList.remove("active");
    } else {
      this._expandButton.classList.add("active");
    }
    this.isVisible = !this._isCollapsed;
  }

  /** Callback for handling collapsed state changes.  */
  onCollapsedChanged(theIsCollapsed) {
    //
  }
};

/**
 * Represents checkbox control.
 */
export class CheckboxInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions = null) {
    super(theName, ControlType.CHECKBOX, theInitialValue, theOnChange, theOptions);
  }

  /**
   * Create checkbox (Boolean flag input) control.
   */
  createHtmlItem() {
    let anEl = document.createElement("input");
    anEl.id = this.getControlId();
    anEl.type = "checkbox";
    anEl.checked = this.initialValue;

    this.getValue = () => { return anEl.checked; };
    this.setValue = (theVal) => { anEl.checked = theVal; }
    anEl.onchange = () => { this.onChange(this.getValue()) };
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();

    this.addControlItem([aLabel, anEl]);
  }
};

/**
 * Represents slider control.
 */
export class SliderInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions) {
    super(theName, ControlType.SLIDER, theInitialValue, theOnChange, theOptions);
  }

  /**
   * Create slider (number input) control.
   */
  createHtmlItem() {
    let aCurrentValue = document.createElement("div");
    aCurrentValue.className = "slider-value";
    let anEl = document.createElement("input");
    anEl.id = this.getControlId();
    anEl.type = "range";
    let updateValues = () => {
      if (this.options.max  != undefined) { anEl.max  = this.options.max; }
      if (this.options.min  != undefined) { anEl.min  = this.options.min; }
      if (this.options.step != undefined) { anEl.step = this.options.step; }
      anEl.value = this.initialValue;
      aCurrentValue.innerHTML = this.initialValue;
      if (this.options.format != undefined) {
        aCurrentValue.innerHTML = this.options.format(this.initialValue);
      }

      if (this.options.values != undefined) {
        anEl.max  = this.options.values.length - 1;
        anEl.min  = 0;
        anEl.step = 1;
        anEl.value = this.options.values.findIndex( aVal => aVal == this.initialValue);
      }
    }
    updateValues();
    this.updateValues = updateValues;

    this.getValue = () => {
      let aRet = 0;
      if (this.options.values != undefined) {
        aRet = this.options.values[anEl.value];
      } else {
        aRet = Number(anEl.value);
      }
      aCurrentValue.innerHTML = aRet;
      if (this.options.format != undefined) {
        aCurrentValue.innerHTML = this.options.format(aRet);
      }
      return aRet;
    };

    this.setValue = (theVal) => {
      let aRet = 0;
      if (this.options.values != undefined) {
        aRet = this.options.values[theVal];
      } else {
        aRet = Number(theVal);
      }
      aCurrentValue.innerHTML = aRet;
      if (this.options.format != undefined) {
        aCurrentValue.innerHTML = this.options.format(aRet);
      }
      anEl.value = theVal;
    }

    if (this.options.handleOnChange) {
      anEl.onchange = () => { this.onChange(this.getValue(), true) };
    }
    anEl.oninput = () => { this.onChange(this.getValue(), false) };
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();

    this.addControlItem([aLabel, anEl, aCurrentValue]);
  }
};

/**
 * Represents range slider control.
 */
export class DualSliderInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions) {
    super(theName, ControlType.DUALSLIDER, theInitialValue, theOnChange, theOptions);
  }

  /**
   * Create control.
   */
  createHtmlItem() {
    let aCurrentValue = document.createElement("div");
    aCurrentValue.className = "slider-value";
    let anElLeft = document.createElement("input");
    anElLeft.id = this.getControlId() + "_left";
    anElLeft.type = "range";
    anElLeft.className = "dual-slider-left";
    let anElRight = document.createElement("input");
    anElRight.id = this.getControlId() + "_right";
    anElRight.type = "range";
    anElRight.className = "dual-slider-right";

    this.getValue = () => {
      return {left: Number(-anElLeft.value), right: Number(anElRight.value)};
    };

    this.setValue = (theVal) => {
      let aMiddle = (theVal.left + theVal.right) * 0.5;
      anElLeft.min = -aMiddle;
      anElRight.min = aMiddle;
      anElLeft.value = -Number(theVal.left);
      anElRight.value = Number(theVal.right);
      this.updateMinMax();
      this.updateLabel();
    }

    this.updateLabel = () => {
      let aValues = this.getValue();
      if (this.options.format != undefined) {
        aValues.left = this.options.format(aValues.left);
        aValues.right = this.options.format(aValues.right);
      }
      aCurrentValue.innerHTML = "[" + aValues.left + ";" + aValues.right + "]";
    }

    this.updateMinMax = () => {
      let aValues = this.getValue();
      let aMiddle = (aValues.left + aValues.right) * 0.5;
      anElLeft.min = -aMiddle;
      anElRight.min = aMiddle;
      let aTotalWidth = anElLeft.clientWidth + anElRight.clientWidth;
      if (aTotalWidth > 0) {
        let aLeftWidth = Math.floor((aMiddle - this.options.min) / (this.options.max - this.options.min) * aTotalWidth);
        let aRightWidth = aTotalWidth - aLeftWidth;
        if (aLeftWidth < 10) {
          aLeftWidth = 10;
          aRightWidth = aTotalWidth - aLeftWidth;
        } else if (aRightWidth < 10) {
          aRightWidth = 10;
          aLeftWidth = aTotalWidth - aRightWidth;
        }
        anElLeft.style.setProperty("width", aLeftWidth.toString() + "px", "important");
        anElRight.style.setProperty("width", aRightWidth.toString() + "px", "important");
      }
    }

    this.updateValues = () => {
      anElLeft.max = -this.options.min;
      anElRight.max  = this.options.max;
      anElLeft.step = this.options.step;
      anElRight.step = this.options.step;
      anElLeft.value = -this.initialValue.left;
      anElRight.value = this.initialValue.right;
      this.updateMinMax();
      this.updateLabel();
    }
    this.updateValues();

    if (this.options.handleOnChange) {
      anElLeft.onchange = () => { this.onChange(this.getValue(), true); };
      anElRight.onchange = () => { this.onChange(this.getValue(), true); };
    }
    anElLeft.oninput = () => { this.onChange(this.getValue(), false); this.updateMinMax(); this.updateLabel(); };
    anElRight.oninput = () => { this.onChange(this.getValue(), false); this.updateMinMax(); this.updateLabel(); };
    anElLeft.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }
    anElLeft.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();

    this.addControlItem([aLabel, anElLeft, anElRight, aCurrentValue]);
  }
};

/**
 * Represents color control.
 */
export class ColorInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions = null) {
    super(theName, ControlType.COLOR, theInitialValue, theOnChange, theOptions);
  }

  /**
   * Create color input control.
   */
  createHtmlItem() {
    let anEl = document.createElement("input");
    anEl.id = this.getControlId();
    anEl.type = "color";
    anEl.value = this.initialValue;

    this.getValue = () => { return anEl.value; };
    this.setValue = (theVal) => { anEl.value = theVal; }
    anEl.oninput = () => { this.onChange(this.getValue()); };
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();

    this.addControlItem([aLabel, anEl]);
  }
};

/**
 * Represents button control.
 */
export class ButtonInput extends Control {
  constructor(theName, theOnChange, theOptions) {
    super(theName, ControlType.BUTTON, "", theOnChange, theOptions);
  }

  /**
   * Add button item.
   */
  createHtmlItem() {
    let anEl = document.createElement("input");
    anEl.innerHTML = this.name;
    anEl.id = this.getControlId();
    anEl.type = "button";
    anEl.value = this.name;

    this.getValue = () => { return false; };
    anEl.onclick = () => { this.onChange(); };
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    this.addControlItem([anEl]);
  }
};

/**
 * Represents file input control.
 */
export class FileInput extends Control {
  constructor(theName, theOnChange, theOptions) {
    super(theName, ControlType.FILEINPUT, "", theOnChange, theOptions);
  }

  /**
   * Create file input item.
   */
  createHtmlItem() {
    let anEl = document.createElement("input");
    anEl.id = this.getControlId();
    anEl.type = "file";
    anEl.value = this.initialValue;
    anEl.accept = this.options.accept;

    this.getValue = () => { return anEl };
    anEl.oninput = () => { this.onChange(this.getValue()) };
    anEl.onclick = () => { anEl.value = null; }
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();

    this.addControlItem([aLabel, anEl]);
  }
};

/**
 * Represents dropdown input control.
 */
export class DropdownInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions) {
    super(theName, ControlType.DROPDOWN, theInitialValue, theOnChange, theOptions);
    this.selectElem = null;
  }

  /**
   * Create dropdown input list control.
   */
  createHtmlItem() {
    let anEl = document.createElement("select");
    anEl.id = this.getControlId();
    anEl.name = this.name;
    anEl.value = 0;
    this.selectElem = anEl;
    this.updateValues();
    this.getValue = () => { return this.options.items.length > 0 ? this.options.items[anEl.value].value : null};
    this.setValue = (theVal) => {
      let anIdx = this.options.items.map ((theItem => theItem.value)).indexOf (theVal);
      if (anIdx == -1) {
        throw Error ("Element not found in the list " + theVal);
      }
      anEl.value = anIdx;
      return this.options.items[anEl.value].value;
    };

    anEl.oninput = () => { this.onChange(this.getValue()) };
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();

    this.addControlItem([aLabel, anEl]);
  }

  /**
   * Update input values.
   */
  updateValues() {
    let anEl = this.selectElem;
    if (!anEl) { return; }
    anEl.innerHTML = "";
    let anInitialValue = 0;
    let anIdx = 0;
    let aGroups = {};
    for (let aVal of this.options.items) {
      if (this.initialValue == aVal.value) {
        anInitialValue = anIdx;
      }
      let anItem = document.createElement("option");
      anItem.value = anIdx++;
      anItem.innerHTML = aVal.name;
      if (aVal.group != undefined) {
        if (aGroups[aVal.group] == undefined) {
          let aGroupElem = document.createElement("optgroup");
          aGroupElem.label = aVal.group;
          anEl.appendChild(aGroupElem);
          aGroups[aVal.group] = aGroupElem;
        }
        aGroups[aVal.group].appendChild(anItem);
      } else {
        anEl.appendChild(anItem);
      }
    }
    anEl.value = anInitialValue;
  }
};

/**
 * Represents text label control.
 */
export class LabelInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions) {
    super(theName, ControlType.LABEL, theInitialValue, theOnChange, theOptions);
  }

  /**
   * Create text label control.
   */
  createHtmlItem() {
    let anEl = document.createElement("div");
    anEl.id = this.getControlId();
    anEl.textContent = this.initialValue;
    anEl.className = "text-value";

    this.getValue = () => { return anEl.textContent; };
    this.setValue = (theVal) => { anEl.textContent = theVal; }
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();
    this.addControlItem([aLabel, anEl]);
  }
};

/**
 * Represents text input control.
 */
export class TextInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions) {
    super(theName, ControlType.TEXTINPUT, theInitialValue, theOnChange, theOptions);
  }

  /**
   * Create text input control.
   */
  createHtmlItem() {
    let anEl = document.createElement("input");
    anEl.id = this.getControlId();
    anEl.type = "text";
    anEl.value = this.initialValue;
    anEl.style.verticalAlign = "top";
    if (this.options && this.options.readOnly) {
      anEl.readOnly = true;
    }

    this.getValue = () => { return anEl.value; };
    this.setValue = (theVal) => {
      anEl.value = theVal;
    }

    anEl.oninput = () => { this.onChange(this.getValue()) };
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();
    this.addControlItem([aLabel, anEl]);
  }
};

/**
 * Represents text area input control.
 */
 export class TextAreaInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions) {
    super(theName, ControlType.TEXTAREA, theInitialValue, theOnChange, theOptions);
    this._minWidth = -1;
  }

  /** Get minWidth style property; -1 means undefined. */
  get minWidth() { return this._minWidth; }

  /** Set minWidth style property. Should be set before displaying element. */
  set minWidth(theWidth) { this._minWidth = theWidth; }

  /**
   * Create text area label control.
   */
  createHtmlItem() {
    let anEl = document.createElement("textarea");
    anEl.id = this.getControlId();
    anEl.value = this.initialValue;
    anEl.style.verticalAlign = "top";
    if (this.options && this.options.readOnly) {
      anEl.readOnly = true;
    }
    if (this._minWidth != -1) {
      anEl.style.minWidth = this._minWidth.toString() + "px";
    }

    this.getValue = () => { return anEl.value; };
    this.setValue = (theVal) => {
      anEl.value = theVal;
      if (this.options.autoHeight) {
        anEl.style.height = 0;
        anEl.style.height = (anEl.scrollHeight) + "px";
      }
    }

    anEl.oninput = () => { this.onChange(this.getValue()) };
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();
    this.addControlItem([aLabel, anEl]);
  }
};

/**
 * Represents number input control.
 */
export class NumberInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions) {
    super(theName, ControlType.NUMBERINPUT, theInitialValue, theOnChange, theOptions);
  }

  /**
   * Create number input control.
   */
  createHtmlItem() {
    let anEl = document.createElement("input");
    anEl.id = this.getControlId();
    anEl.type = "number";
    anEl.value = this.initialValue;
    if (this.options) {
      if (this.options.readOnly) { anEl.readOnly = true; }
      if (this.options.placeholder != undefined) { anEl.placeholder = this.options.placeholder; }
      if (this.options.max  != undefined) { anEl.max  = this.options.max; }
      if (this.options.min  != undefined) { anEl.min  = this.options.min; }
      if (this.options.step != undefined) { anEl.step = this.options.step; }
    }

    this.getValue = () => { return Number(anEl.value); };
    this.setValue = (theVal) => {
      anEl.value = theVal;
    }

    anEl.oninput = () => { this.onChange(this.getValue()) };
    anEl.onfocus = (theEv) => { this.lastFocusTime = theEv.timeStamp; }

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();

    if (this.options && this.options.units !== undefined) {
      this._output = document.createElement("output");
      this._output.innerHTML = " " + this.options.units;
      this.addControlItem([aLabel, anEl, this._output]);
    } else {
      this._output = null;
      this.addControlItem([aLabel, anEl]);
    }
  }

  /** Return units suffix. */
  get unit() { return this.options ? this.options.units : ""; }

  /** Set units suffix. */
  set unit(theUnit) {
    if (!this.options) { return; }
    this.options.units = theUnit;
    if (this._output != null) { this._output.innerHTML = " " + theUnit; }
  }
};

/**
 * Represents tree view control.
 */
export class TreeViewInput extends Control {
  constructor(theName, theInitialValue, theOnChange, theOptions = null) {
    super(theName, ControlType.TreeViewInput, theInitialValue, theOnChange, theOptions);
  }

  /**
   * Create tree node elements.
   */
  createNodeItem(theVal, theParent) {
    if (theVal.name === undefined) {
      return;
    }

    let anElem = document.createElement("li");
    anElem.id = theParent.id + "_" + theVal.name;
    theParent.appendChild(anElem);

    let hasChildren = theVal.children !== undefined && theVal.children.length > 0;
    
    if (hasChildren) {
      this.createExpCheckbox(theVal, anElem);
    }

    if (this.options.switchable) {
      this.createToggleCheckbox(theVal, anElem);
    }

    let aNameElem = document.createElement("span");
    aNameElem.innerHTML = theVal.name;
    anElem.appendChild(aNameElem);

    if (hasChildren) {
      this.createChildrenElement(theVal, anElem);
    }
  }

  /**
   * Create expandable checkbox for tree node.
   */
  createExpCheckbox(theVal, theParent) {
    theVal.expElem = document.createElement("input");
    theVal.expElem.id = theParent.id + "_exp";
    theVal.expElem.type = "checkbox";
    theVal.expElem.className = "tree-exp";
    theParent.appendChild(theVal.expElem);
    theVal.expElem.onchange = () => { theVal.expElem.setCollapsed(!theVal.expElem.checked) };

    theVal.expElem.setCollapsed = (theIsCollapsed) => {
      if (theIsCollapsed) {
        theVal.childrenElem.classList.remove("treeul-active");
      } else {
        theVal.childrenElem.classList.add("treeul-active");
      }

      theVal.expElem.checked = !theIsCollapsed;
    }
  }

  /**
   * Create toggle checkbox for tree node.
   */
  createToggleCheckbox(theVal, theParent) {
    theVal.toggleElem = document.createElement("input");
    theVal.toggleElem.id = theParent.id + "_toggle";
    theVal.toggleElem.type = "checkbox";
    theVal.toggleElem.checked = false;
    theParent.appendChild(theVal.toggleElem);
    theVal.toggleElem.getEnabled = () => { return !theVal.toggleElem.indeterminate ? theVal.toggleElem.checked : null; }
    theVal.toggleElem.onchange = () => {
      this.updateCheckboxes(theVal, theVal.toggleElem.checked);
      this.onChange({chain: this.getNamesChain(theVal), value: theVal.toggleElem.checked});
    };
  }

  /**
   * Create children element for tree node.
   */
  createChildrenElement(theVal, theParent) {
    theVal.childrenElem = document.createElement("ul");
    theVal.childrenElem.className = "treeul";
    theVal.childrenElem.id = theParent.id + "_ul";
    theParent.appendChild(theVal.childrenElem);
    for (let aChild of theVal.children) {
      aChild.parent = theVal;
      this.createNodeItem(aChild, theVal.childrenElem);
    }

    theVal.expElem.setCollapsed(theVal.collapsed !== undefined ? theVal.collapsed : true);
  }
  
  /**
   * Create tree view control.
   */
  createHtmlItem() {
    let anEl = document.createElement("ul");
    anEl.id = this.getControlId();
    anEl.className = "treeul";
    anEl.classList.add("treeul-active");
    this.treeElem = anEl;
    this.updateValues();

    let aLabel = document.createElement("label");
    aLabel.innerHTML = this.name;
    aLabel.htmlFor = this.getControlId();

    this.addControlItem([aLabel, anEl]);
  }

  /**
   * Update elements when tree content changed.
   */
  updateValues() {
    this.treeElem.innerHTML = "";
    this.createNodeItem(this.options.tree, this.treeElem);
  }

  /**
   * Set enabling status of tree node.
   */
  setEnabled(theNamesChain, theEnabled) {
    if (!this.options.switchable) {
      return;
    }

    let aVal = [this.options.tree];
    let aResVal = null;
    for (let aName of theNamesChain) {
      if (aVal === undefined) {
        return;
      }
      aResVal = null;
      for (let aChild of aVal) {
        if (aChild.name == aName) {
          aResVal = aChild;
          aVal = aChild.children;
          break;
        }
      }
    }

    if (aResVal != null) {
      this.updateCheckboxes(aResVal, theEnabled);
    }
  }

  /**
   * Update status of children and parents checkboxes.
   */
  updateCheckboxes(theVal, theEnabled) {
    let aQueue = [theVal];
    while (aQueue.length > 0) {
      let aNode = aQueue.shift();
      aNode.toggleElem.checked = theEnabled;
      aNode.toggleElem.indeterminate = false;
      if (aNode.children !== undefined) {
        for (let aChild of aNode.children) {
          aQueue.push(aChild);
        }
      }
    }

    for (let aParent = theVal.parent; aParent !== undefined; aParent = aParent.parent) {
      let hasEnabled = false;
      let hasDisabled = false;
      if (aParent.children === undefined) {
        continue;
      }
      for (let aChild of aParent.children) {
        let aChildEnabled = aChild.toggleElem.getEnabled();
        if (aChildEnabled === null) {
          hasEnabled = true;
          hasDisabled = true;
        } else if (aChildEnabled) {
          hasEnabled = true;
        } else {
          hasDisabled = true;
        }

        if (hasEnabled && hasDisabled) {
          break;
        }
      }

      if (hasEnabled && hasDisabled) {
        aParent.toggleElem.checked = false;
        aParent.toggleElem.indeterminate = true;
      } else {
        aParent.toggleElem.indeterminate = false;
        if (hasEnabled) {
          aParent.toggleElem.checked = true;
        } else {
          aParent.toggleElem.checked = false;
        }
      }
    }
  }

  /**
   * Return array of parents names.
   */
  getNamesChain(theVal) {
    let aRes = [theVal.name];
    for (let aParent = theVal.parent; aParent !== undefined; aParent = aParent.parent) {
      aRes.unshift(aParent.name);
    }
    return aRes;
  }
};

/**
 * Represents panel which contains list of controls
 */
export class PanelInput extends Control {
  constructor(theName) {
    super(theName, "PANEL", null, null, null);
  }

  /**
   * Generate panel control id from it's name.
   */
  getControlId() {
    let aPrefix = this.parent ? this.parent.getControlId() : "panel";
    return aPrefix + "_" + this.getSafeName();
  }

  /**
   * Generate panel button control id from it's name.
   */
  getButtonControlId() {
    let aPrefix = this.parent ? this.parent.getButtonControlId() : "panelButton";
    return aPrefix + "_" + this.getSafeName();
  }

  /** Callback for handling active panel changes.  */
  onActivePanelChanged(theIsActivated) {
    //
  }

  /**
   * Redraw HTML element connected with this panel
   */
  redraw() {
    let aPanel = document.getElementById(this.getControlId());
    aPanel.innerHTML = "";
    this.element = aPanel;
    if (!this._isVisible) { this.element.hidden = true; }
    for (let aControl of this.controls) {
      aControl.createHtmlItem();
    }

    if (this.update) {
      this.update();
    }
  }
};

/**
 * Class which responses for accessing and switching of panels.
 */
export class ControlPanel {
  constructor(theName) {
    this.name = theName;
    this.panels = [];
    this.currentPanel = undefined;
  }

  // make panel is active by its name
  switchPanel(theName) {
    if (this.currentPanel && this.currentPanel.name == theName) {
      document.getElementById(this.currentPanel.getControlId()).hidden = true;
      document.getElementById(this.currentPanel.getButtonControlId()).className = "header-button";
      this.currentPanel.onActivePanelChanged(false);
      this.currentPanel = null;
      return;
    }

    if (this.currentPanel) {
      document.getElementById(this.currentPanel.getControlId()).hidden = true;
      document.getElementById(this.currentPanel.getButtonControlId()).className = "header-button";
      this.currentPanel.onActivePanelChanged(false);
    }

    for (let aPanel of this.panels) {
      if (theName == aPanel.name) {
        this.currentPanel = aPanel;
        document.getElementById(this.currentPanel.getControlId()).hidden = false;
        document.getElementById(this.currentPanel.getButtonControlId()).className = "header-button selected";
        this.currentPanel.onActivePanelChanged(true);
        return;
      }
    }
    throw new Error("Panel '" + theName + "' is not found");
  }

  // add Panel
  addPanel(thePanel) {
    for (let aPanel of this.panels) {
      if (thePanel.name == aPanel.name) {
        throw new Error("Panel '" + thePanel.name  + "' is already added");
      }
    }
    this.panels.push(thePanel);
  }

  // redraw all included panels (add panels to HTML document)
  redraw() {
    // add header buttons
    let aHeader = document.getElementById(this.name + "Header");
    for (let aPanel of this.panels) {
      let anEl = document.createElement("div");
      anEl.innerHTML = aPanel.name;
      anEl.className = "header-button";
      anEl.id = aPanel.getButtonControlId();
      anEl.onclick = (theEv) => { this.switchPanel(aPanel.name), theEv.preventDefault(); };
      aHeader.appendChild(anEl);
    }

    // add panels
    let aPanels = document.getElementById(this.name + "Body");
    let aPanelsData = "";
    for (let aPanel of this.panels) {
      aPanelsData += "<div id='" + aPanel.getControlId() + "' hidden='true' class='panel'></div>";
    }
    aPanels.innerHTML = aPanelsData;

    // redraw panels
    for (let aPanel of this.panels) {
      aPanel.redraw();
    }
  }
};
