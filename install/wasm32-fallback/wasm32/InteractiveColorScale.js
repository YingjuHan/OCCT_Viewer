import { ViewCorner } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
/**
 * Clicking regions on colorscale (C++ `SCPrsCae::InteractiveColorScale::SelectionPart`),
 * accessible in {@link ObjectMouseClickEvent.part}.
 */
export var ColorScaleSelectionPart;
(function (ColorScaleSelectionPart) {
    ColorScaleSelectionPart[ColorScaleSelectionPart["ColorBar"] = 0] = "ColorBar";
    ColorScaleSelectionPart[ColorScaleSelectionPart["RangeTipMin"] = 1] = "RangeTipMin";
    ColorScaleSelectionPart[ColorScaleSelectionPart["RangeTipMax"] = 2] = "RangeTipMax";
    ColorScaleSelectionPart[ColorScaleSelectionPart["LabelMin"] = 3] = "LabelMin";
    ColorScaleSelectionPart[ColorScaleSelectionPart["LabelMax"] = 4] = "LabelMax";
    ColorScaleSelectionPart[ColorScaleSelectionPart["OutOfRangeMin"] = 5] = "OutOfRangeMin";
    ColorScaleSelectionPart[ColorScaleSelectionPart["OutOfRangeMax"] = 6] = "OutOfRangeMax";
})(ColorScaleSelectionPart || (ColorScaleSelectionPart = {}));
;
;
/**
 * Parameters object for color scale object.
 */
export class ColorScaleParams {
    constructor() {
        /** Default value range (actual range of property values). */
        this.defaultRange = { x: 0.0, y: 1.0 };
        /** Display value range. */
        this.displayRange = { x: 0.0, y: 1.0 };
        /** Selected normalized range (within 0..1 range), always in ascending order. */
        this.selectedRange = { x: 0.0, y: 1.0 };
        /** Defines if display min/max values should be swapped in display. */
        this.isReversed = false;
        /** Defines if colors are discrete or smooth. */
        this.isSmooth = false;
        /** Number of intervals (colors). */
        this.nbIntervals = 10;
        /** Size of label text. */
        this.labelSize = 12;
        /** Left-bottom position of color scale. */
        this.pos = { x: 0, y: 0 };
        /** Width, height of color scale. */
        this.size = { x: 0, y: 0 };
        /** Max height of color scale (as ratio from viewer height if <= 1.0 and in pixels otherwise). */
        this.maxSize = 1.0;
        /** Defines if used a continuous colors scale. */
        this.isCustomColors = false;
        /** Defines color for out-of-range values (including alpha). */
        this.outOfRangeColor = '';
        /** Min color used if isCustomColors = false. */
        this.minColor = '';
        /** Max color used if isCustomColors = false. */
        this.maxColor = '';
        /** Vector of colors (StringVector) used if isCustomColors = true. */
        this.customColors = null;
        /** Custom labels (StringVector) used if isCustomColors = true. */
        this.customLabels = null;
        /** Flag to skip empty custom labels in legend within non-smoothed presentation. */
        this.toSkipEmptyLabels = false;
        /** Title text. */
        this.title = '';
        /** Display units. */
        this.displayUnits = '';
        /** Related property id. */
        this.propertyId = '';
        /** Related property component. */
        this.propertyComponent = -1;
        /** Flag to use special color for detected interval if isCustomColors = true. */
        this.toHighlightIntervals = true;
        /** Color used for detected interval if isCustomColors = true. */
        this.intervalHighlightColor = '';
        /** Controls the side of the screen to which the colorscale is attached. */
        this.positionSide = ViewCorner.RIGHT_CENTER;
        /** Controls whether the mode of colorscale is logarithmic. */
        this.isLogarithmic = false;
    }
}
;
;
/**
 * Interactive Color Scale Object.
 *
 * Application may subscribe to the following additional events:
 * - {@link UserEvents.onColorMapperChanged}, when color map is changed.
 */
export class InteractiveColorScale extends InteractiveObject {
    constructor(theViewer, theObjId = '') {
        var _a, _b;
        super(theObjId);
        if (theViewer != null) {
            this._myViewer = theViewer;
            if (this._myObjId === '') {
                this._myObjId = ':::ColorScale';
                if (this.objMgr != null) {
                    this._myObjId = this.objMgr.generateId(this._myObjId);
                }
            }
            (_a = this.colorScaleMgr) === null || _a === void 0 ? void 0 : _a.createColorScale(this._myObjId);
            (_b = this.objMgr) === null || _b === void 0 ? void 0 : _b.addInteractiveObject(this);
        }
    }
    /** Get scale object parameters. */
    get params() {
        if (this.colorScaleMgr) {
            return this.colorScaleMgr.getColorScaleParams(this._myObjId);
        }
        return new ColorScaleParams();
    }
    /** Get default value range (actual range of property values). */
    get defaultRange() {
        const aParams = this.params;
        return aParams != null ? aParams.defaultRange : { x: 0.0, y: 1.0 };
    }
    /** Get display value range. */
    get displayRange() {
        const aParams = this.params;
        return aParams != null ? aParams.displayRange : { x: 0.0, y: 1.0 };
    }
    /** Get selected range. */
    get selectedRange() {
        const aParams = this.params;
        return aParams != null ? aParams.selectedRange : { x: 0.0, y: 1.0 };
    }
    /** Get scale object isReversed parameter. */
    get isReversed() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.isReversed) !== null && _a !== void 0 ? _a : false;
    }
    /** Get scale object isSmooth parameter. */
    get isSmooth() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.isSmooth) !== null && _a !== void 0 ? _a : false;
    }
    /** Get scale object nbIntervals parameter. */
    get nbIntervals() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.nbIntervals) !== null && _a !== void 0 ? _a : 0;
    }
    /** Get scale object labelSize parameter. */
    get labelSize() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.labelSize) !== null && _a !== void 0 ? _a : 0;
    }
    /** Get scale object title. */
    get title() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.title) !== null && _a !== void 0 ? _a : '';
    }
    /** Get scale display units. */
    get displayUnits() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.displayUnits) !== null && _a !== void 0 ? _a : '';
    }
    /** Get color for out-of-range values (including alpha). */
    get outOfRangeColor() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.outOfRangeColor) !== null && _a !== void 0 ? _a : '';
    }
    /** Get scale object pos parameter. */
    get pos() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.pos) !== null && _a !== void 0 ? _a : { x: 0.0, y: 0.0 };
    }
    /** Get scale object size parameter. */
    get size() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.size) !== null && _a !== void 0 ? _a : { x: 0.0, y: 0.0 };
    }
    /** Get scale object isCustomColors parameter. */
    get isCustomColors() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.isCustomColors) !== null && _a !== void 0 ? _a : false;
    }
    /** Get scale object minColor/maxColor parameters. */
    get minMaxColors() {
        const aParams = this.params;
        return aParams != null ? { minColor: aParams.minColor, maxColor: aParams.maxColor } : { minColor: '', maxColor: '' };
    }
    /** Get scale object customColors parameter. */
    get customColors() {
        const aParams = this.params;
        let aColors = new Array();
        if (aParams != null) {
            for (let i = 0; i < aParams.customColors.size(); i++) {
                aColors.push(aParams.customColors.get(i));
            }
        }
        return aColors;
    }
    /** Get scale object customLabels parameter. */
    get customLabels() {
        const aParams = this.params;
        let aLabels = new Array();
        if (aParams != null) {
            for (let i = 0; i < aParams.customLabels.size(); i++) {
                aLabels.push(aParams.customLabels.get(i));
            }
        }
        return aLabels;
    }
    /** Get flag to skip empty custom labels in legend within non-smoothed presentation. */
    get toSkipEmptyLabels() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.toSkipEmptyLabels) !== null && _a !== void 0 ? _a : false;
    }
    /** Get flag to use special color for detected interval if isCustomColors = true. */
    get toHighlightIntervals() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.toHighlightIntervals) !== null && _a !== void 0 ? _a : false;
    }
    /** Get color used for detected interval if isCustomColors = true. */
    get intervalHighlightColor() {
        var _a;
        const aParams = this.params;
        return (_a = aParams === null || aParams === void 0 ? void 0 : aParams.intervalHighlightColor) !== null && _a !== void 0 ? _a : '';
    }
    /** Returns the side of the screen to which the colorscale is attached. */
    get positionSide() {
        const aParams = this.params;
        return aParams != null ? aParams.positionSide : ViewCorner.RIGHT_CENTER;
    }
    /** Returns true if colorscale is logarithmic. */
    get isLogarithmic() {
        const aParams = this.params;
        return aParams != null ? aParams.isLogarithmic : false;
    }
    /** Shortcut to interactive color scale context. */
    get colorScaleMgr() { var _a, _b; return (_b = (_a = this._myViewer) === null || _a === void 0 ? void 0 : _a.interactiveColorScaleManager) !== null && _b !== void 0 ? _b : null; }
    /** Set scale object parameters. */
    set params(theParams) { var _a; (_a = this.colorScaleMgr) === null || _a === void 0 ? void 0 : _a.setColorScaleParams(this._myObjId, theParams); }
    /** Set display value range. */
    set displayRange(theRange) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.displayRange = theRange;
            this.params = aParams;
        }
    }
    /** Set selected range. */
    set selectedRange(theRange) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.selectedRange = theRange;
            this.params = aParams;
        }
    }
    /** Set scale object isReversed parameter. */
    set isReversed(theToReverse) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.isReversed = theToReverse;
            this.params = aParams;
        }
    }
    /** Set scale object isSmooth parameter. */
    set isSmooth(theIsSmooth) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.isSmooth = theIsSmooth;
            this.params = aParams;
        }
    }
    /** Set scale object nbIntervals parameter. */
    set nbIntervals(theNbIntervals) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.nbIntervals = theNbIntervals;
            this.params = aParams;
        }
    }
    /** Set scale object labelSize parameter. */
    set labelSize(theLabelSize) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.labelSize = theLabelSize;
            this.params = aParams;
        }
    }
    /** Set scale object title. */
    set title(theTitle) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.title = theTitle;
            this.params = aParams;
        }
    }
    /** Set scale display units. */
    set displayUnits(theUnits) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.displayUnits = theUnits;
            this.params = aParams;
        }
    }
    /** Set color for out-of-range values (including alpha). */
    set outOfRangeColor(theUnits) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.outOfRangeColor = theUnits;
            this.params = aParams;
        }
    }
    /** Set scale object pos parameter. */
    set pos(thePos) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.pos = thePos;
            this.params = aParams;
        }
    }
    /** Set scale object size parameter. */
    set size(theSize) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.size = theSize;
            this.params = aParams;
        }
    }
    /** Set scale object minColor/maxColor parameters. */
    set minMaxColors(colors) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.isCustomColors = false;
            aParams.minColor = colors.minColor;
            aParams.maxColor = colors.maxColor;
            this.params = aParams;
        }
    }
    /**
     * Set scale object customColors parameter
     * and automatically sets isCustomColors if list is empty or not.
     */
    set customColors(theColors) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.isCustomColors = theColors.length > 0;
            aParams.customColors.resize(theColors.length, '');
            let i = 0;
            for (let aColor of theColors) {
                aParams.customColors.set(i++, aColor);
            }
            this.params = aParams;
        }
    }
    /** Set scale object customLabels parameter; have effect only with isCustomColors=true. */
    set customLabels(theLabels) {
        let aParams = this.params;
        if (aParams != null) {
            // @c aParams.isCustomColors = true;
            aParams.customLabels.resize(theLabels.length, '');
            let i = 0;
            for (let aLabIter of theLabels) {
                aParams.customLabels.set(i++, aLabIter);
            }
            this.params = aParams;
        }
    }
    /** Set flag to skip empty custom labels in legend within non-smoothed presentation. */
    set toSkipEmptyLabels(theToSkip) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.toSkipEmptyLabels = theToSkip;
            this.params = aParams;
        }
    }
    /** Set flag to use special color for detected interval if isCustomColors = true. */
    set toHighlightIntervals(theToHighlight) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.toHighlightIntervals = theToHighlight;
            this.params = aParams;
        }
    }
    /** Set color used for detected interval if isCustomColors = true. */
    set intervalHighlightColor(theUnits) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.intervalHighlightColor = theUnits;
            this.params = aParams;
        }
    }
    /** Sets the side of the screen to which the colorscale is attached. */
    set positionSide(thePosSide) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.positionSide = thePosSide;
            this.params = aParams;
        }
    }
    /** Sets whether the colorscale is logarithmic. */
    set isLogarithmic(theIsLogarithmic) {
        let aParams = this.params;
        if (aParams != null) {
            aParams.isLogarithmic = theIsLogarithmic;
            this.params = aParams;
        }
    }
    /** Make a copy of existing object with new id */
    clone(theObjId = '') {
        let anObjId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
        if (!this._myViewer || !this.objMgr || anObjId === '') {
            return null;
        }
        let aCopy = new InteractiveColorScale(null);
        aCopy.wrap(this._myViewer, anObjId);
        this.objMgr.addInteractiveObject(aCopy);
        return aCopy;
    }
    /** Attach this color scale object to mesh object for specified property. */
    attach(theObjId, thePropertyId, theComponentIndex) {
        var _a;
        const anObjId = (theObjId instanceof InteractiveObject) ? theObjId.id : theObjId;
        (_a = this.colorScaleMgr) === null || _a === void 0 ? void 0 : _a.attachColorScale(this._myObjId, anObjId, thePropertyId, theComponentIndex);
    }
}
;
;
//# sourceMappingURL=InteractiveColorScale.js.map