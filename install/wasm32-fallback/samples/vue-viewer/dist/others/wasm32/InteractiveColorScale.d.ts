import type { ColorString, IVec2, ObjectId } from './CommonTools.js';
import { ViewCorner } from './CommonTools.js';
import { InteractiveObject } from './InteractiveObject.js';
import type { SCwebviewer } from './SCwebviewer.js';
/**
 * Clicking regions on colorscale (C++ `SCPrsCae::InteractiveColorScale::SelectionPart`),
 * accessible in {@link ObjectMouseClickEvent.part}.
 */
export declare enum ColorScaleSelectionPart {
    ColorBar = 0,
    RangeTipMin = 1,
    RangeTipMax = 2,
    LabelMin = 3,
    LabelMax = 4,
    OutOfRangeMin = 5,
    OutOfRangeMax = 6
}
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
}
/**
 * Parameters object for color scale object.
 */
export declare class ColorScaleParams implements IColorScaleParams {
    /** Default value range (actual range of property values). */
    defaultRange: IVec2;
    /** Display value range. */
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
    /** Defines if used a continuous colors scale. */
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
}
/**
 * minColor + maxColor pair for colorscale.
 */
interface IColorScaleMinMaxColors {
    minColor: ColorString;
    maxColor: ColorString;
}
/**
 * Interactive Color Scale Object.
 *
 * Application may subscribe to the following additional events:
 * - {@link UserEvents.onColorMapperChanged}, when color map is changed.
 */
export declare class InteractiveColorScale extends InteractiveObject {
    constructor(theViewer: SCwebviewer | null, theObjId?: ObjectId);
    /** Get scale object parameters. */
    get params(): IColorScaleParams;
    /** Get default value range (actual range of property values). */
    get defaultRange(): IVec2;
    /** Get display value range. */
    get displayRange(): IVec2;
    /** Get selected range. */
    get selectedRange(): IVec2;
    /** Get scale object isReversed parameter. */
    get isReversed(): boolean;
    /** Get scale object isSmooth parameter. */
    get isSmooth(): boolean;
    /** Get scale object nbIntervals parameter. */
    get nbIntervals(): number;
    /** Get scale object labelSize parameter. */
    get labelSize(): number;
    /** Get scale object title. */
    get title(): string;
    /** Get scale display units. */
    get displayUnits(): string;
    /** Get color for out-of-range values (including alpha). */
    get outOfRangeColor(): ColorString;
    /** Get scale object pos parameter. */
    get pos(): IVec2;
    /** Get scale object size parameter. */
    get size(): IVec2;
    /** Get scale object isCustomColors parameter. */
    get isCustomColors(): boolean;
    /** Get scale object minColor/maxColor parameters. */
    get minMaxColors(): IColorScaleMinMaxColors;
    /** Get scale object customColors parameter. */
    get customColors(): ColorString[];
    /** Get scale object customLabels parameter. */
    get customLabels(): string[];
    /** Get flag to skip empty custom labels in legend within non-smoothed presentation. */
    get toSkipEmptyLabels(): boolean;
    /** Get flag to use special color for detected interval if isCustomColors = true. */
    get toHighlightIntervals(): boolean;
    /** Get color used for detected interval if isCustomColors = true. */
    get intervalHighlightColor(): ColorString;
    /** Returns the side of the screen to which the colorscale is attached. */
    get positionSide(): ViewCorner;
    /** Returns true if colorscale is logarithmic. */
    get isLogarithmic(): boolean;
    /** Shortcut to interactive color scale context. */
    private get colorScaleMgr();
    /** Set scale object parameters. */
    set params(theParams: IColorScaleParams);
    /** Set display value range. */
    set displayRange(theRange: IVec2);
    /** Set selected range. */
    set selectedRange(theRange: IVec2);
    /** Set scale object isReversed parameter. */
    set isReversed(theToReverse: boolean);
    /** Set scale object isSmooth parameter. */
    set isSmooth(theIsSmooth: boolean);
    /** Set scale object nbIntervals parameter. */
    set nbIntervals(theNbIntervals: number);
    /** Set scale object labelSize parameter. */
    set labelSize(theLabelSize: number);
    /** Set scale object title. */
    set title(theTitle: string);
    /** Set scale display units. */
    set displayUnits(theUnits: string);
    /** Set color for out-of-range values (including alpha). */
    set outOfRangeColor(theUnits: ColorString);
    /** Set scale object pos parameter. */
    set pos(thePos: IVec2);
    /** Set scale object size parameter. */
    set size(theSize: IVec2);
    /** Set scale object minColor/maxColor parameters. */
    set minMaxColors(colors: IColorScaleMinMaxColors);
    /**
     * Set scale object customColors parameter
     * and automatically sets isCustomColors if list is empty or not.
     */
    set customColors(theColors: ColorString[]);
    /** Set scale object customLabels parameter; have effect only with isCustomColors=true. */
    set customLabels(theLabels: string[]);
    /** Set flag to skip empty custom labels in legend within non-smoothed presentation. */
    set toSkipEmptyLabels(theToSkip: boolean);
    /** Set flag to use special color for detected interval if isCustomColors = true. */
    set toHighlightIntervals(theToHighlight: boolean);
    /** Set color used for detected interval if isCustomColors = true. */
    set intervalHighlightColor(theUnits: ColorString);
    /** Sets the side of the screen to which the colorscale is attached. */
    set positionSide(thePosSide: ViewCorner);
    /** Sets whether the colorscale is logarithmic. */
    set isLogarithmic(theIsLogarithmic: boolean);
    /** Make a copy of existing object with new id */
    clone(theObjId?: ObjectId): InteractiveColorScale | null;
    /** Attach this color scale object to mesh object for specified property. */
    attach(theObjId: ObjectId | InteractiveObject, thePropertyId: string, theComponentIndex: number): void;
}
export {};
