/**
 * IPromise interface which allows to run callback written on js from c++.
 * For using this interface please use its implementation from {@link SCwebviewer.IPromise}.implement.
 */
export declare interface IPromise {
    /** Custom callback function - to be called on success. */
    resolve: () => void;
    /** Custom callback function - to be called on failure. */
    reject: (theErr: string) => void;
    /** Create {@link IPromise} wrapper from given object with callback function. */
    implement: (arg: any) => IPromise;
}
/**
 * Represents (sub)view id.
 */
export type ViewId = number;
/**
 * (Sub)view corner defined by `SCwebviewer::ViewCorner` enumeration.
 */
export declare enum ViewCorner {
    FREE3D = -1,
    CENTER_CENTER = 0,
    CENTER_UPPER = 1,
    CENTER_LOWER = 2,
    LEFT_CENTER = 4,
    LEFT_UPPER = 5,
    LEFT_LOWER = 6,
    RIGHT_CENTER = 8,
    RIGHT_UPPER = 9,
    RIGHT_LOWER = 10
}
/**
 * Represents 2D vector interface (based on `Graphic3d_Vec2d` from C++ OCCT).
 */
export interface IVec2 {
    x: number;
    y: number;
}
/**
 * Represents 2D vector object (based on `Graphic3d_Vec2d` from C++ OCCT).
 */
export declare class Vec2 implements IVec2 {
    x: number;
    y: number;
    /** Return zero vector. */
    static zero(): IVec2;
}
/**
 * Represents 3D vector interface (based on `Graphic3d_Vec3d` from C++ OCCT).
 */
export interface IVec3 {
    x: number;
    y: number;
    z: number;
}
/**
 * Represents 3D vector interface as an RGB color.
 */
export interface IRgb {
    r: number;
    g: number;
    b: number;
}
/**
 * Represents 4D vector interface (based on `Graphic3d_Vec4d` from C++ OCCT).
 */
export interface IVec4 {
    x: number;
    y: number;
    z: number;
    w: number;
}
/**
 * Represents 3D vector interface as an RGBA color.
 */
export interface IRgba {
    r: number;
    g: number;
    b: number;
    a: number;
}
/**
 * Represents 4D vector object (based on `Graphic3d_Vec4d` from C++ OCCT).
 */
export declare class Vec4 implements IVec4 {
    x: number;
    y: number;
    z: number;
    w: number;
}
/**
 * Represents 4x4 matrix interface (based on `Graphic3d_Mat4d` from C++ OCCT).
 */
export interface IMat4 {
    row1: IVec4;
    row2: IVec4;
    row3: IVec4;
    row4: IVec4;
}
/**
 * Represents 3D axes-aligned bounding box interface defined by pair of it's corner points (based on `Graphic3d_BndBox3d` from C++ OCCT).
 * Invalid (void) bounding box is defined by negative extent.
 */
export interface IBndBox3 {
    cornerMin: IVec3;
    cornerMax: IVec3;
}
/**
 * Represents 3D axes-aligned bounding box object defined by pair of it's corner points (based on `Graphic3d_BndBox3d` from C++ OCCT).
 */
export declare class BndBox3 implements IBndBox3 {
    cornerMin: IVec3;
    cornerMax: IVec3;
    /** Return TRUE if box is invalid (void). */
    static isVoid(theBox: IBndBox3): boolean;
    /** Return void box. */
    static voidBox(): IBndBox3;
    /** Return box diagonal */
    static diagonal(theBox: IBndBox3): IVec3;
    /** Return box extent (diagonal modulus). */
    static extent(theBox: IBndBox3): number;
}
/**
 * Represents axes interface (based on `gp_Ax2` from C++ OCCT).
 */
export interface IAx2 {
    location: IVec3;
    direction: IVec3;
    xDirection: IVec3;
}
/**
 * Represents axes object (based on `gp_Ax2` from C++ OCCT).
 */
export declare class Ax2 implements IAx2 {
    location: IVec3;
    direction: IVec3;
    xDirection: IVec3;
    /**
     * Identifies a coordinate system where its origin is Origin,
     * and its "main Direction" and "X Direction" coordinates
     * Z = 1.0, X = Y =0.0 and X direction coordinates X = 1.0, Y = Z = 0.0
     */
    static xoy(): IAx2;
    /**
     * Identifies a coordinate system where its origin is Origin,
     * and its "main Direction" and "X Direction" coordinates
     * Y = 1.0, X = Z =0.0 and X direction coordinates Z = 1.0, X = Y = 0.0
     */
    static zox(): IAx2;
    /**
     * Identifies a coordinate system where its origin is Origin,
     * and its "main Direction" and "X Direction" coordinates
     * X = 1.0, Z = Y =0.0 and X direction coordinates Y = 1.0, X = Z = 0.0
     * In 2D space
     */
    static yoz(): IAx2;
    /** Return invalid system. */
    static invalid(): IAx2;
}
/**
 * Represents Interactive Object id.
 */
export type ObjectId = string;
/**
 * Represents RGB(A) color in HEX format `'#RRGGBB'` and `'#RRGGBBAA'` passed as string value.
 * Color is transparently converted from/to `Quantity_ColorRGBA`.
 * In addition, setters accept X11 color names like `'red'`, `'cyan'`, or `'gray50'` (see `Quantity_NameOfColor`).
 */
export type ColorString = string;
/**
 * Represents 3D vector object (based on `Graphic3d_Vec3d` from C++ OCCT).
 * Provides also math operations between 3D vectors.
 */
export declare class Vec3 implements IVec3 {
    x: number;
    y: number;
    z: number;
    /** Return zero vector. */
    static zero(): IVec3;
    /** Return DX vector. */
    static dx(): IVec3;
    /** Return DY vector. */
    static dy(): IVec3;
    /** Return DZ vector. */
    static dz(): IVec3;
    /** Return invalid (`RealLast()`) value. */
    static invalidDouble(): number;
    /** Return invalid vector (`RealLast()` values). */
    static invalid(): IVec3;
    /** Return TRUE if point has infinite range (same as `Precision::IsInfinite()` check in OCCT). */
    static isInfinite(theA: IVec3 | number): boolean;
    /** Return opposite vector. */
    static reverse(theV: IVec3): IVec3;
    /** Return coordinate with specified index within [0,2] range. */
    static coord(theV: IVec3, theI: number): number;
    /** Compute per-component addition between two vectors A+B. */
    static add(theA: IVec3, theB: IVec3 | number): IVec3;
    /** Compute per-component subtraction between two vectors A-B. */
    static subtract(theA: IVec3, theB: IVec3 | number): IVec3;
    /** Compute per-component multiplication between two vectors A*B. */
    static multiply(theA: IVec3, theB: IVec3 | number): IVec3;
    /** Compute per-component division between two vectors A/B. */
    static divide(theA: IVec3, theB: IVec3 | number): IVec3;
    /** Compute dot product between two vectors. */
    static dot(theA: IVec3, theB: IVec3): number;
    /** Return vector modulus. */
    static modulus(theV: IVec3): number;
    /** Compute cross product between two vectors. */
    static cross(theA: IVec3, theB: IVec3): IVec3;
    /** Return normalized vector. */
    static normalize(theV: IVec3): IVec3;
    /** Compute angle between two normalized vectors */
    static angle(theA: IVec3, theB: IVec3): number;
    /**
     * Rotate normalized vector around another vector by angle.
     * @param theVec vector to rotate
     * @param theAxis vector to rotate around
     * @param theAngle angle in radians
     * @return rotated vector
     */
    static rotateVector(theVec: IVec3, theAxis: IVec3, theAngle: number): IVec3;
}
/**
 * Interface defining visual material (PBR) properties (based on `XCAFDoc_VisMaterialPBR` from C++ OCCT).
 */
export interface IVisMaterial {
    /** flag indicating undefined material. */
    isDefined: boolean;
    /** base RGBA color (includes transparency); empty color indicates undefined material. */
    baseColor: ColorString;
    /** emissibe RGB color. */
    emissiveFactor: IVec3;
    /** metallic factor within [0..1] range. */
    metallic: number;
    /** roughness factor within [0..1] range. */
    roughness: number;
    /** IOR. */
    refractionIndex: number;
}
/**
 * Object defining visual material (PBR) properties (based on `XCAFDoc_VisMaterialPBR` from C++ OCCT).
 */
export declare class VisMaterial implements IVisMaterial {
    /** flag indicating undefined material. */
    isDefined: boolean;
    /** base RGBA color (includes transparency). */
    baseColor: ColorString;
    /** emissibe RGB color. */
    emissiveFactor: IVec3;
    /** metallic factor within [0..1] range. */
    metallic: number;
    /** roughness factor within [0..1] range. */
    roughness: number;
    /** IOR. */
    refractionIndex: number;
}
/**
 * Auxiliary tool for color converting.
 */
export declare class ColorTools {
    /**
     * Converts ColorString to Rgb.
     * @param theColorStr string in #xxxxxxxx format
     * @return object with rgb values in [0, 255] range
     */
    static convertColorStringToRgb(theColorStr: ColorString): IRgb;
    /**
     * Converts ColorString to Rgba.
     * @param theColorStr string in #xxxxxxxx format
     * @return object with rgba values in [0, 255] range
     */
    static convertColorStringToRgba(theColorStr: ColorString): IRgba;
    /**
     * Converts Rgb to ColorString.
     * @param theColor object with rgb values in [0, 255] range
     * @return string in #xxxxxx format
     */
    static convertRgbToColorString(theColor: IRgb): ColorString;
    /**
     * Converts Rgba to ColorString.
     * @param theColor object with rgb values in [0, 255] range
     * @return string in #xxxxxxxx format
     */
    static convertRgbaToColorString(theColor: IRgba): ColorString;
}
/**
 * Standard camera orientations (based on `V3d_TypeOfOrientation` from C++ OCCT).
 * For using this enum please use its implementation from {@link SCwebviewer.CameraOrientation}
 * See also {@link Camera.setOrientation} method.
 */
export declare enum CameraOrientation {
    LEFT = 3,
    RIGHT = 0,
    BOTTOM = 5,
    TOP = 2,
    FRONT = 4,
    BACK = 1,
    AXORIGHT = 19,
    AXOLEFT = 24,
    XnegYneg = 9,
    XposYneg = 15,
    Axial = 5,// BOTTOM
    Frontal = 4,// FRONT
    Sagittal = 0,// RIGHT
    FrontalLeft = 15,// FRONT+RIGHT (left anatomical)
    FrontalRight = 9
}
/**
 * Represents camera View Point parameters interface (see `Graphic3d_Camera` from C++ OCCT).
 * See also {@link Camera.viewPoint}.
 */
export interface ICameraViewPoint {
    /** Eye location (3d point). */
    eye: IVec3;
    /** Direction of up vector (normalized 3D vector). */
    up: IVec3;
    /** Direction of look (normalized 3D vector). */
    direction: IVec3;
    /** Distance between camera and point to look. */
    distance: number;
    /** Viewport relative scale. */
    scale: number;
}
/**
 * Represents camera View Point parameters object (see `Graphic3d_Camera` from C++ OCCT).
 */
export declare class CameraViewPoint implements ICameraViewPoint {
    /** Eye location (3d point). */
    eye: IVec3;
    /** Direction of up vector (normalized 3D vector). */
    up: IVec3;
    /** Direction of look (normalized 3D vector). */
    direction: IVec3;
    /** Distance between camera and point to look. */
    distance: number;
    /** Viewport relative scale. */
    scale: number;
}
/**
 * B-Rep shape type (based on `TopAbs_ShapeEnum` from C++ OCCT).
 */
export declare enum ShapeEnum {
    COMPOUND = 0,
    COMPSOLID = 1,
    SOLID = 2,
    SHELL = 3,
    FACE = 4,
    WIRE = 5,
    EDGE = 6,
    VERTEX = 7,
    SHAPE = 8
}
/**
 * B-Rep shape orientation (based on `TopAbs_Orientation` from C++ OCCT).
 */
export declare enum ShapeOrientation {
    FORWARD = 0,
    REVERSED = 1,
    INTERNAL = 2,
    EXTERNAL = 3
}
/**
 * Geometry surface type (based on `GeomAbs_SurfaceType` from C++ OCCT).
 */
export declare enum ShapeSurfaceType {
    Plane = 0,
    Cylinder = 1,
    Cone = 2,
    Sphere = 3,
    Torus = 4,
    BezierSurface = 5,
    BSplineSurface = 6,
    SurfaceOfRevolution = 7,
    SurfaceOfExtrusion = 8,
    OffsetSurface = 9,
    OtherSurface = 10
}
/**
 * Geometry curve type (based on `GeomAbs_CurveType` from C++ OCCT).
 */
export declare enum ShapeCurveType {
    Line = 0,
    Circle = 1,
    Ellipse = 2,
    Hyperbola = 3,
    Parabola = 4,
    BezierCurve = 5,
    BSplineCurve = 6,
    OffsetCurve = 7,
    OtherCurve = 8
}
/**
 * Describes gp_Trsf location (transformation) interface.
 */
export interface ITrsf {
    /** rotation quaternion. */
    rotation: IVec4;
    /** translation vector. */
    translation: IVec3;
    /** uniform scale factor. */
    scale: number;
}
/** @deprecated deprecated alias to ITrsf */
export type IShapeLocation = ITrsf;
/**
 * Enumerates all 24 possible variants of generalized
 * Euler angles, defining general 3d rotation by three
 * rotations around main axes of coordinate system,
 * in different possible orders.
 *
 * Alias to `gp_EulerSequence` from C++ OCCT.
 */
export declare enum EulerSequence {
    /** Classic Euler angles, alias to Intrinsic_ZXZ */
    EulerAngles = 0,
    /** Yaw Pitch Roll (or nautical) angles, alias to Intrinsic_ZYX */
    YawPitchRoll = 1,
    /** Tait-Bryan angles (using three different axes) */
    Extrinsic_XYZ = 2,
    Extrinsic_XZY = 3,
    Extrinsic_YZX = 4,
    Extrinsic_YXZ = 5,
    Extrinsic_ZXY = 6,
    Extrinsic_ZYX = 7,
    Intrinsic_XYZ = 8,
    Intrinsic_XZY = 9,
    Intrinsic_YZX = 10,
    Intrinsic_YXZ = 11,
    Intrinsic_ZXY = 12,
    Intrinsic_ZYX = 13,
    /** Proper Euler angles (using two different axes, first and third the same) */
    Extrinsic_XYX = 14,
    Extrinsic_XZX = 15,
    Extrinsic_YZY = 16,
    Extrinsic_YXY = 17,
    Extrinsic_ZYZ = 18,
    Extrinsic_ZXZ = 19,
    Intrinsic_XYX = 20,
    Intrinsic_XZX = 21,
    Intrinsic_YZY = 22,
    Intrinsic_YXY = 23,
    Intrinsic_ZXZ = 24,
    Intrinsic_ZYZ = 25
}
/**
 * Describes gp_Trsf location (transformation) object.
 */
export declare class Trsf implements ITrsf {
    /** rotation quaternion. */
    rotation: IVec4;
    /** translation vector. */
    translation: IVec3;
    /** uniform scale factor. */
    scale: number;
    /** Return TRUE if location has scale factor. */
    static isScaled(theLoc: ITrsf): boolean;
    /** Return TRUE if location has translation vector. */
    static isTranslated(theLoc: ITrsf): boolean;
    /** Return TRUE if location has rotation quaternion. */
    static isRotated(theLoc: ITrsf): boolean;
    /** Return TRUE if location is empty (identity). */
    static isIdentity(theLoc: ITrsf): boolean;
    /** Format location info. */
    static format(theLoc: ITrsf, theUMgr?: UnitManager | null): string;
    /** Multiply two transformations in specified order. */
    static multiply(theLoc1: ITrsf, theLoc2: ITrsf): ITrsf;
    /** Compute inverted transformation. */
    static invert(theLoc: ITrsf): ITrsf;
    /** Return 4x4 matrix. */
    static getMat4(theLoc: ITrsf): IMat4;
    /** Return rotation part as 3 Euler angles. */
    static getEulerAngles(theLoc: ITrsf, theSeq?: EulerSequence): IVec3;
    /** Setup rotation part from 3 Euler angles. */
    static setEulerAngles(theLoc: ITrsf, theAngles: IVec3, theSeq?: EulerSequence): ITrsf;
    /** Setup rotation transformation around specified axis (origin will define translation part) and angle in radians. */
    static rotateAroundAxis(theDir: IVec3, theAngle: number, theOrigin?: IVec3): ITrsf;
    /** Return transformed 3D point. */
    static transformPnt(theLoc: ITrsf, thePnt: IVec3): IVec3;
    /** Return transformed 3D direction. */
    static transformDir(theLoc: ITrsf, theDir: IVec3): IVec3;
}
/** @deprecated deprecated alias to ITrsf */
export type ShapeLocation = Trsf;
/**
 * Describes information about TopoDS_Shape.
 */
export interface IShapeInfo {
    /** Presentation object id. */
    objectId: ObjectId;
    /** Shape type. */
    shapeType: ShapeEnum;
    /** Selection index within subshapes of the same type. */
    index: number;
    /** Number of direct subshapes. */
    nbChildren: number;
    /** Shape orientation. */
    orientation: ShapeOrientation;
    /** Free flag. */
    free: boolean;
    /** Locked flag. */
    locked: boolean;
    /** Modified flag. */
    modified: boolean;
    /** Checked flag. */
    checked: boolean;
    /** Orientable flag. */
    orientable: boolean;
    /** Closed flag. */
    closed: boolean;
    /** Infinite flag. */
    infinite: boolean;
    /** Convex flag. */
    convex: boolean;
    /** Geometry surface type (for FACE shape type). */
    surfaceType: ShapeSurfaceType;
    /** Geometry curve type (for EDGE shape type). */
    curveType: ShapeCurveType;
    /** Number of triangles (for FACE shape type). */
    nbTriangles: number;
    /** Number of triangle nodes (for FACE shape type). */
    nbTriNodes: number;
    /** Number of tessellation segments (for EDGE shapes). */
    nbPolySegments: number;
    /** Vertex position (for VERTEX shape type). */
    point: IVec3;
    /** Subshape color. */
    color: ColorString;
    /** Subshape hidden flag. */
    hidden: boolean;
    /** Shape location. */
    location: ITrsf;
    /** Return indices of defined composite solid subshapes within parent presentation object. */
    readonly compSolids?: number[];
    /** Return indices of defined solid subshapes within parent presentation object. */
    readonly solids?: number[];
    /** Return indices of defined shell subshapes within parent presentation object. */
    readonly shells?: number[];
    /** Return indices of defined face subshapes within parent presentation object. */
    readonly faces?: number[];
    /** Return indices of defined wire subshapes within parent presentation object. */
    readonly wires?: number[];
    /** Return indices of defined edge subshapes within parent presentation object. */
    readonly edges?: number[];
    /** Return indices of defined vertex subshapes within parent presentation object. */
    readonly vertices?: number[];
}
/**
 * Describes information about TopoDS_Shape.
 */
export declare class ShapeInfo implements IShapeInfo {
    /** Presentation object id. */
    objectId: ObjectId;
    /** Shape type. */
    shapeType: ShapeEnum;
    /** Selection index within subshapes of the same type. */
    index: number;
    /** Number of direct subshapes. */
    nbChildren: number;
    /** Shape orientation. */
    orientation: ShapeOrientation;
    /** Free flag. */
    free: boolean;
    /** Locked flag. */
    locked: boolean;
    /** Modified flag. */
    modified: boolean;
    /** Checked flag. */
    checked: boolean;
    /** Orientable flag. */
    orientable: boolean;
    /** Closed flag. */
    closed: boolean;
    /** Infinite flag. */
    infinite: boolean;
    /** Convex flag. */
    convex: boolean;
    /** Geometry surface type (for FACE shape type). */
    surfaceType: ShapeSurfaceType;
    /** Geometry curve type (for EDGE shape type). */
    curveType: ShapeCurveType;
    /** Number of triangles (for FACE shape type). */
    nbTriangles: number;
    /** Number of triangle nodes (for FACE shape type). */
    nbTriNodes: number;
    /** Number of tessellation segments (for EDGE shapes). */
    nbPolySegments: number;
    /** Vertex position (for VERTEX shape type). */
    point: IVec3;
    /** Subshape color. */
    color: ColorString;
    /** Subshape hidden flag. */
    hidden: boolean;
    /** Shape location. */
    location: ITrsf;
    /** Return indices of defined composite solid subshapes within parent presentation object. */
    get compSolids(): number[];
    /** Return indices of defined solid subshapes within parent presentation object. */
    get solids(): number[];
    /** Return indices of defined shell subshapes within parent presentation object. */
    get shells(): number[];
    /** Return indices of defined face subshapes within parent presentation object. */
    get faces(): number[];
    /** Return indices of defined wire subshapes within parent presentation object. */
    get wires(): number[];
    /** Return indices of defined edge subshapes within parent presentation object. */
    get edges(): number[];
    /** Return indices of defined vertex subshapes within parent presentation object. */
    get vertices(): number[];
    /** Return indices of defined subshapes within parent presentation object. */
    protected static getSubshapes(theInfo: IShapeInfo, theType: ShapeEnum): number[];
    /** Format shape info. */
    static format(theInfo: IShapeInfo, theUMgr?: UnitManager | null): string;
    /** Format shape flags. */
    static formatFlags(theInfo: IShapeInfo): string;
}
/**
 * Cylinder shape construction parameters interface.
 */
export interface ICylinderParams {
    /** Coordinate system, center of bottom circle. */
    ax2: IAx2;
    /** Cylinder radius. */
    radius: number;
    /** Cylinder height. */
    height: number;
    /** Cylinder section angle. */
    angle: number;
}
/**
 * Cylinder shape construction parameters object.
 */
export declare class CylinderParams implements ICylinderParams {
    /** Coordinate system, center of bottom circle. */
    ax2: IAx2;
    /** Cylinder radius. */
    radius: number;
    /** Cylinder height. */
    height: number;
    /** Cylinder section angle. */
    angle: number;
}
/**
 * Cone shape construction parameters interface.
 */
export interface IConeParams {
    /** Coordinate system, center of bottom circle. */
    ax2: IAx2;
    /** Cone bottom radius. */
    radius: number;
    /** Cone top radius. */
    topRadius: number;
    /** Cone height. */
    height: number;
    /** Cone section angle. */
    angle: number;
}
/**
 * Cone shape construction parameters object.
 */
export declare class ConeParams implements IConeParams {
    /** Coordinate system, center of bottom circle. */
    ax2: IAx2;
    /** Cone bottom radius. */
    radius: number;
    /** Cone top radius. */
    topRadius: number;
    /** Cone height. */
    height: number;
    /** Cone section angle. */
    angle: number;
}
/**
 * Quantity type defined by `SCPrsBase::QuantityType` enumeration.
 */
export declare enum QuantityType {
    /** unknown / dimensionless unit */
    UNKNOWN = -1,
    /** Time, base SI quantity defined in seconds, s */
    Time = 0,
    /** Length, base SI quantity defined in meters, m */
    Length = 1,
    /** Mass, base SI quantity defined in kilograms, kg */
    Mass = 2,
    /** Electric Current, base SI quantity defined in amperes, A */
    ElectricCurrent = 3,
    /** Thermodynamic temperature, base SI quantity defined in kelvins, K */
    ThermodynamicTemperature = 4,
    /** Amount of Substance, base SI quantity defined in moles, mol */
    AmountOfSubstance = 5,
    /** Luminous Intensity, base SI quantity defined in candelas, cd */
    LuminousIntensity = 6,
    /** Plane Angle, derived quantity defined in radians, rad */
    PlaneAngle = 7,
    /** Area, derived quantity defined in squared meters, m^2 */
    Area = 8,
    /** Volume, derived quantity defined in cubic meters, m^3 */
    Volume = 9,
    /** Speed, derived quantity defined in m/s */
    Speed = 10,
    /** Force, derived quantity defined in newtons, N */
    Force = 11
}
/**
 * Define common length units.
 */
export declare enum LengthUnit {
    Micron = "\u00B5m",
    Millimeter = "mm",
    Meter = "m",
    Centimeter = "cm",
    Kilometer = "km",
    Microinch = "\u00B5in",
    Inch = "in",
    Foot = "ft",
    Mile = "stat.mile"
}
/** Base manager interface. */
export interface IManager {
}
/**
 * Represents tool for managing units.
 * See also {@link SCwebviewer.unitManager} property.
 */
export interface UnitManager extends IManager {
    /** Return the array of quantites */
    readonly quantities: Quantity[];
    /** Return length quantity */
    readonly length: Quantity;
    /** Return area quantity */
    readonly area: Quantity;
    /** Return volume quantity */
    readonly volume: Quantity;
    /** Return plane angle quantity */
    readonly angle: Quantity;
    /** Format unit name (e.g. replace '**2' with squared sign and similar). */
    formatUnitName: (theUnit: string) => string;
    /** Convert value from one unit to another unit. */
    convert: (theVal: number, theFromUnit: string, theToUnit: string) => number;
    /** Convert value from SI unit to another unit. */
    convertAnyFromSI: (theVal: number, theToUnit: string) => number;
    /** Convert value to SI unit from another unit. */
    convertAnyToSI: (theVal: number, theFromUnit: string) => number;
}
/**
 * Defines quantity units convertion tool.
 */
export declare class Quantity {
    private m_unitMgr;
    private m_type;
    /** Main constructor. */
    constructor(theMgr: UnitManager, theType: QuantityType);
    /** Return Quantity type */
    get type(): QuantityType;
    /** Return Quantity name */
    get name(): string;
    /** Return quantity SI units */
    get unitSI(): string;
    /** Return quantity system units */
    get unitSystem(): string;
    /** Return quantity display units */
    get unitDisplay(): string;
    /** Set quantity system units */
    set unitSystem(theUnit: string);
    /** Set quantity display units */
    set unitDisplay(theUnit: string);
    /** Convert value from one unit to another unit. */
    convert(theVal: number, theFromUnit: string, theToUnit: string): number;
    /** Convert value from SI unit to another unit. */
    convertAnyFromSI(theVal: number, theToUnit: string): number;
    /** Convert value to SI unit from another unit. */
    convertAnyToSI(theVal: number, theFromUnit: string): number;
    /** Convert value from system units into display units. */
    systemToDisplay(theSysVal: number): number;
    /** Convert value from system units into display units. */
    systemToDisplayVec3(theSysVal: IVec3): IVec3;
    /** Convert value from display units into system units. */
    systemFromDisplay(theDispVal: number): number;
    /** Convert value from display units into system units. */
    systemFromDisplayVec3(theDispVal: IVec3): IVec3;
    /** Convert value from system units into SI units. */
    systemToSI(theSysVal: number): number;
    /** Convert value from system units into SI units. */
    systemToSIVec3(theSysVal: IVec3): IVec3;
    /** Convert value from SI units into system units. */
    systemFromSI(theSIVal: number): number;
    /** Convert value from SI units into system units. */
    systemFromSIVec3(theSIVal: IVec3): IVec3;
    /** Auxiliary tool converting real number from System to Display units and appending units. */
    formatReal(theSys: number): string;
    /** Auxiliary tool converting real number from System to Display units and appending units. */
    formatVec3(theSys: IVec3): string;
    /** Auxiliary tool converting real number from System to Display units and appending units. */
    format(theSys: IVec3 | number): string;
}
