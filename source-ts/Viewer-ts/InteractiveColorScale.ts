import type {IManager, ColorString, IVec2, ObjectId} from './CommonTools.js';
import {ViewCorner} from './CommonTools.js';
import {InteractiveObject} from './InteractiveObject.js';

import type {SCwebviewer} from './SCwebviewer.js';

import type {UserEvents} from './ViewerEventManager.js';

/**
 * Clicking regions on colorscale (C++ `SCPrsCae::InteractiveColorScale::SelectionPart`),
 * accessible in {@link ObjectMouseClickEvent.part}.
 */
export enum ColorScaleSelectionPart {
  ColorBar = 0, RangeTipMin = 1, RangeTipMax = 2, LabelMin = 3, LabelMax = 4, OutOfRangeMin = 5, OutOfRangeMax = 6,
};

/**
 * Parameters interface for color scale object.
 */
export interface IColorScaleParams {
  /** Default value range (actual range of property values), always in ascending order. */
  defaultRange: IVec2;
  /** Display value range, always in ascending order. */
  displayRange: IVec2;
  /** Selected normalized range (within 0..1 range), always in ascending order. */
  selectedRange: IVec2;
  /** Defines if display min/max values should be swapped in display. */
  isReversed: boolean;
  /** Defines if colors are discrete or smooth. */
  isSmooth: boolean;
  /** Number of intervals (colors). */
  nbIntervals: number;
  /** Size of label text. */
  labelSize: number;
  /** Left-bottom position of color scale. */
  pos: IVec2;
  /** Width, height of color scale. */
  size: IVec2;
  /** Max height of color scale (as ratio from viewer height if <= 1.0 and in pixels otherwise). */
  maxSize: number;
  /** Defines if used a continuous colors scale (from minColor to maxColor)
   *  or array of colors for each interval. */
  isCustomColors: boolean;
  /** Defines color for out-of-range values (including alpha). */
  outOfRangeColor: ColorString;
  /** Min color used if isCustomColors = false. */
  minColor: ColorString;
  /** Max color used if isCustomColors = false. */
  maxColor: ColorString;
  /** Vector of colors (StringVector) used if isCustomColors = true. */
  customColors: any;
  /** Custom labels (StringVector) used if isCustomColors = true. */
  customLabels: any;
  /** Flag to skip empty custom labels in legend within non-smoothed presentation. */
  toSkipEmptyLabels: boolean;
  /** Title text. */
  title: string;
  /** Display units. */
  displayUnits: string;
  /** Related property id. */
  propertyId: string;
  /** Related property component. */
  propertyComponent: number;
  /** Flag to use special color for detected interval if isCustomColors = true. */
  toHighlightIntervals: boolean;
  /** Color used for detected interval if isCustomColors = true. */
  intervalHighlightColor: ColorString;
  /** Controls the side of the screen to which the colorscale is attached. */
  positionSide: ViewCorner;
  /** Controls whether the mode of colorscale is logarithmic. */
  isLogarithmic: boolean;
};

/**
 * Parameters object for color scale object.
 */
export class ColorScaleParams implements IColorScaleParams {
  /** Default value range (actual range of property values). */
  defaultRange: IVec2 = {x: 0.0, y: 1.0};
  /** Display value range. */
  displayRange: IVec2 = {x: 0.0, y: 1.0};
  /** Selected normalized range (within 0..1 range), always in ascending order. */
  selectedRange: IVec2 = {x: 0.0, y: 1.0};
  /** Defines if display min/max values should be swapped in display. */
  isReversed: boolean = false;
  /** Defines if colors are discrete or smooth. */
  isSmooth: boolean = false;
  /** Number of intervals (colors). */
  nbIntervals: number = 10;
  /** Size of label text. */
  labelSize: number = 12;
  /** Left-bottom position of color scale. */
  pos: IVec2 = {x: 0, y: 0};
  /** Width, height of color scale. */
  size: IVec2 = {x: 0, y: 0};
  /** Max height of color scale (as ratio from viewer height if <= 1.0 and in pixels otherwise). */
  maxSize: number = 1.0;
  /** Defines if used a continuous colors scale. */
  isCustomColors: boolean = false;
  /** Defines color for out-of-range values (including alpha). */
  outOfRangeColor: ColorString = '';
  /** Min color used if isCustomColors = false. */
  minColor: ColorString = '';
  /** Max color used if isCustomColors = false. */
  maxColor: ColorString = '';
  /** Vector of colors (StringVector) used if isCustomColors = true. */
  customColors: any = null;
  /** Custom labels (StringVector) used if isCustomColors = true. */
  customLabels: any = null;
  /** Flag to skip empty custom labels in legend within non-smoothed presentation. */
  toSkipEmptyLabels: boolean = false;
  /** Title text. */
  title: string = '';
  /** Display units. */
  displayUnits: string = '';
  /** Related property id. */
  propertyId: string = '';
  /** Related property component. */
  propertyComponent: number = -1;
  /** Flag to use special color for detected interval if isCustomColors = true. */
  toHighlightIntervals: boolean = true;
  /** Color used for detected interval if isCustomColors = true. */
  intervalHighlightColor: ColorString = '';
  /** Controls the side of the screen to which the colorscale is attached. */
  positionSide: ViewCorner = ViewCorner.RIGHT_CENTER;
  /** Controls whether the mode of colorscale is logarithmic. */
  isLogarithmic: boolean = false;
};

/**
 * minColor + maxColor pair for colorscale.
 */
interface IColorScaleMinMaxColors {
  minColor: ColorString;
  maxColor: ColorString;
};

/**
 * Interactive Color Scale Object.
 *
 * Application may subscribe to the following additional events:
 * - {@link UserEvents.onColorMapperChanged}, when color map is changed.
 */
export class InteractiveColorScale extends InteractiveObject {
  constructor(theViewer: SCwebviewer | null, theObjId: ObjectId = '') {
    super(theObjId);
    if (theViewer != null) {
      this._myViewer = theViewer;
      if (this._myObjId === '') {
        this._myObjId = ':::ColorScale';
        if (this.objMgr != null) { this._myObjId = this.objMgr.generateId(this._myObjId); }
      }
      this.colorScaleMgr?.createColorScale(this._myObjId);
      this.objMgr?.addInteractiveObject(this);
    }
  }

  /** Get scale object parameters. */
  public get params(): IColorScaleParams
  {
    if (this.colorScaleMgr) {
      return this.colorScaleMgr.getColorScaleParams(this._myObjId);
    }
    return new ColorScaleParams();
  }

  /** Get default value range (actual range of property values). */
  public get defaultRange(): IVec2 {
    const aParams = this.params;
    return aParams != null ? aParams.defaultRange : {x: 0.0, y: 1.0};
  }

  /** Get display value range. */
  public get displayRange(): IVec2 {
    const aParams = this.params;
    return aParams != null ? aParams.displayRange : {x: 0.0, y: 1.0};
  }

  /** Get selected range. */
  public get selectedRange(): IVec2 {
    const aParams = this.params;
    return aParams != null ? aParams.selectedRange : {x: 0.0, y: 1.0};
  }

  /** Get scale object isReversed parameter. */
  public get isReversed(): boolean {
    const aParams = this.params;
    return aParams?.isReversed ?? false;
  }

  /** Get scale object isSmooth parameter. */
  public get isSmooth(): boolean {
    const aParams = this.params;
    return aParams?.isSmooth ?? false;
  }

  /** Get scale object nbIntervals parameter. */
  public get nbIntervals(): number {
    const aParams = this.params;
    return aParams?.nbIntervals ?? 0;
  }

  /** Get scale object labelSize parameter. */
  public get labelSize(): number {
    const aParams = this.params;
    return aParams?.labelSize ?? 0;
  }

  /** Get scale object title. */
  public get title(): string {
    const aParams = this.params;
    return aParams?.title ?? '';
  }

  /** Get scale display units. */
  public get displayUnits(): string {
    const aParams = this.params;
    return aParams?.displayUnits ?? '';
  }

  /** Get color for out-of-range values (including alpha). */
  public get outOfRangeColor(): ColorString {
    const aParams = this.params;
    return aParams?.outOfRangeColor ?? '';
  }

  /** Get scale object pos parameter. */
  public get pos(): IVec2 {
    const aParams = this.params;
    return aParams?.pos ?? { x: 0.0, y: 0.0 };
  }

  /** Get scale object size parameter. */
  public get size(): IVec2 {
    const aParams = this.params;
    return aParams?.size ?? { x: 0.0, y: 0.0 };
  }

  /** Get scale object isCustomColors parameter. */
  public get isCustomColors(): boolean {
    const aParams = this.params;
    return aParams?.isCustomColors ?? false;
  }

  /** Get scale object minColor/maxColor parameters. */
  public get minMaxColors(): IColorScaleMinMaxColors {
    const aParams = this.params;
    return aParams != null ? { minColor: aParams.minColor, maxColor: aParams.maxColor } : { minColor: '', maxColor: '' };
  }

  /** Get scale object customColors parameter. */
  public get customColors(): ColorString[] {
    const aParams = this.params;
    let aColors = new Array<ColorString>();
    if (aParams != null) {
      for (let i = 0; i < aParams.customColors.size(); i++) {
        aColors.push(aParams.customColors.get(i));
      }
    }
    return aColors;
  }

  /** Get scale object customLabels parameter. */
  public get customLabels(): string[] {
    const aParams = this.params;
    let aLabels = new Array<string>();
    if (aParams != null) {
      for (let i = 0; i < aParams.customLabels.size(); i++) {
        aLabels.push(aParams.customLabels.get(i));
      }
    }
    return aLabels;
  }

  /** Get flag to skip empty custom labels in legend within non-smoothed presentation. */
  public get toSkipEmptyLabels(): boolean {
    const aParams = this.params;
    return aParams?.toSkipEmptyLabels ?? false;
  }

  /** Get flag to use special color for detected interval if isCustomColors = true. */
  public get toHighlightIntervals(): boolean {
    const aParams = this.params;
    return aParams?.toHighlightIntervals ?? false;
  }

  /** Get color used for detected interval if isCustomColors = true. */
  public get intervalHighlightColor(): ColorString {
    const aParams = this.params;
    return aParams?.intervalHighlightColor ?? '';
  }

  /** Returns the side of the screen to which the colorscale is attached. */
  public get positionSide(): ViewCorner {
    const aParams = this.params;
    return aParams != null ? aParams.positionSide : ViewCorner.RIGHT_CENTER;
  }

  /** Returns true if colorscale is logarithmic. */
  public get isLogarithmic() : boolean {
    const aParams = this.params;
    return aParams != null ? aParams.isLogarithmic : false;
  }

  /** Shortcut to interactive color scale context. */
  private get colorScaleMgr(): InteractiveColorScaleManager | null { return this._myViewer?.interactiveColorScaleManager ?? null; }

  /** Set scale object parameters. */
  public set params(theParams: IColorScaleParams) { this.colorScaleMgr?.setColorScaleParams(this._myObjId, theParams); }

  /** Set display value range. */
  public set displayRange(theRange: IVec2) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.displayRange = theRange;
      this.params = aParams;
    }
  }

  /** Set selected range. */
  public set selectedRange(theRange: IVec2) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.selectedRange = theRange;
      this.params = aParams;
    }
  }

  /** Set scale object isReversed parameter. */
  public set isReversed(theToReverse: boolean) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.isReversed = theToReverse;
      this.params = aParams;
    }
  }

  /** Set scale object isSmooth parameter. */
  public set isSmooth(theIsSmooth: boolean) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.isSmooth = theIsSmooth;
      this.params = aParams;
    }
  }

  /** Set scale object nbIntervals parameter. */
  public set nbIntervals(theNbIntervals: number) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.nbIntervals = theNbIntervals;
      this.params = aParams;
    }
  }

  /** Set scale object labelSize parameter. */
  public set labelSize(theLabelSize: number) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.labelSize = theLabelSize;
      this.params = aParams;
    }
  }

  /** Set scale object title. */
  public set title(theTitle: string) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.title = theTitle;
      this.params = aParams;
    }
  }

  /** Set scale display units. */
  public set displayUnits(theUnits: string) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.displayUnits = theUnits;
      this.params = aParams;
    }
  }

  /** Set color for out-of-range values (including alpha). */
  public set outOfRangeColor(theUnits: ColorString) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.outOfRangeColor = theUnits;
      this.params = aParams;
    }
  }

  /** Set scale object pos parameter. */
  public set pos(thePos: IVec2) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.pos = thePos;
      this.params = aParams;
    }
  }

  /** Set scale object size parameter. */
  public set size(theSize: IVec2) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.size = theSize;
      this.params = aParams;
    }
  }

  /** Set scale object minColor/maxColor parameters. */
  public set minMaxColors(colors: IColorScaleMinMaxColors) {
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
  public set customColors(theColors: ColorString[]) {
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
  public set customLabels(theLabels: string[]) {
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
  public set toSkipEmptyLabels(theToSkip: boolean) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.toSkipEmptyLabels = theToSkip;
      this.params = aParams;
    }
  }

  /** Set flag to use special color for detected interval if isCustomColors = true. */
  public set toHighlightIntervals(theToHighlight: boolean) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.toHighlightIntervals = theToHighlight;
      this.params = aParams;
    }
  }

  /** Set color used for detected interval if isCustomColors = true. */
  public set intervalHighlightColor(theUnits: ColorString) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.intervalHighlightColor = theUnits;
      this.params = aParams;
    }
  }

  /** Sets the side of the screen to which the colorscale is attached. */
  public set positionSide(thePosSide: ViewCorner) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.positionSide = thePosSide;
      this.params = aParams;
    }
  }

  /** Sets whether the colorscale is logarithmic. */
  public set isLogarithmic(theIsLogarithmic: boolean) {
    let aParams = this.params;
    if (aParams != null) {
      aParams.isLogarithmic = theIsLogarithmic;
      this.params = aParams;
    }
  }

  /** Make a copy of existing object with new id */
  public clone(theObjId: ObjectId = ''): InteractiveColorScale | null {
    let anObjId: ObjectId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
    if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
    let aCopy = new InteractiveColorScale(null);
    aCopy.wrap(this._myViewer, anObjId);
    this.objMgr.addInteractiveObject(aCopy);
    return aCopy;
  }

  /** Attach this color scale object to mesh object for specified property. */
  public attach(theObjId: ObjectId | InteractiveObject, thePropertyId: string, theComponentIndex: number): void
  {
    const anObjId: ObjectId = (theObjId instanceof InteractiveObject) ? theObjId.id : theObjId;
    this.colorScaleMgr?.attachColorScale(this._myObjId, anObjId, thePropertyId, theComponentIndex);
  }
};

/**
 * @internal Represents Context for Interactive colorscale objects.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveColorScale} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveColorScaleManager} property.
 */
export interface InteractiveColorScaleManager extends IManager {
// C++ methods

  /** @internal Create interactive color scale presentation. */
  createColorScale: (theObjId: ObjectId) => void;

  /** @internal Attach color scale to mesh object. */
  attachColorScale: (theColorScaleId: ObjectId,
                     theMeshId: ObjectId,
                     thePropertyId: string,
                     theComponentIndex: number) => void;

  /** @internal Return color scale parameters. */
  getColorScaleParams: (theObjId: ObjectId) => IColorScaleParams;

  /** @internal Set color scale parameters and update its presentation. */
  setColorScaleParams: (theObjId: ObjectId, theParams: IColorScaleParams) => void;

  /** @internal Get ids of color scale objects attached to mesh. */
  getMeshAttachedColorScales: (theObjId: ObjectId) => SCwebviewer['StringVector'];
};
