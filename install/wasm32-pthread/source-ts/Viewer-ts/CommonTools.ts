// Created by: Kirill Gavrilov
// Copyright (c) 2022 Huawei Technologies Co., Ltd.

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
};

/**
 * Represents (sub)view id.
 */
export type ViewId = number;

/**
 * (Sub)view corner defined by `SCwebviewer::ViewCorner` enumeration.
 */
export enum ViewCorner {
  FREE3D = -1,
  CENTER_CENTER = 0,
  CENTER_UPPER = 1,
  CENTER_LOWER = 2,
  LEFT_CENTER = 4,
  LEFT_UPPER = 5,
  LEFT_LOWER = 6,
  RIGHT_CENTER = 8,
  RIGHT_UPPER = 9,
  RIGHT_LOWER = 10,
};

/**
 * Represents 2D vector interface (based on `Graphic3d_Vec2d` from C++ OCCT).
 */
export interface IVec2 { x: number; y: number };

/**
 * Represents 2D vector object (based on `Graphic3d_Vec2d` from C++ OCCT).
 */
export class Vec2 implements IVec2 {
  x: number = 0.0;
  y: number = 0.0;

  /** Return zero vector. */
  public static zero(): IVec2 { return { x: 0.0, y: 0.0 }; }
};

/**
 * Represents 3D vector interface (based on `Graphic3d_Vec3d` from C++ OCCT).
 */
export interface IVec3 { x: number; y: number; z: number };

/**
 * Represents 3D vector interface as an RGB color.
 */
export interface IRgb { r: number; g: number; b: number };

/**
 * Represents 4D vector interface (based on `Graphic3d_Vec4d` from C++ OCCT).
 */
export interface IVec4 { x: number; y: number; z: number; w: number };

/**
 * Represents 3D vector interface as an RGBA color.
 */
export interface IRgba { r: number; g: number; b: number; a: number };

/**
 * Represents 4D vector object (based on `Graphic3d_Vec4d` from C++ OCCT).
 */
export class Vec4 implements IVec4 {
  x: number = 0.0;
  y: number = 0.0;
  z: number = 0.0;
  w: number = 0.0;
};

/**
 * Represents 4x4 matrix interface (based on `Graphic3d_Mat4d` from C++ OCCT).
 */
export interface IMat4 { row1: IVec4; row2: IVec4; row3: IVec4; row4: IVec4 };

/**
 * Represents 3D axes-aligned bounding box interface defined by pair of it's corner points (based on `Graphic3d_BndBox3d` from C++ OCCT).
 * Invalid (void) bounding box is defined by negative extent.
 */
export interface IBndBox3 { cornerMin: IVec3; cornerMax: IVec3 };

/**
 * Represents 3D axes-aligned bounding box object defined by pair of it's corner points (based on `Graphic3d_BndBox3d` from C++ OCCT).
 */
export class BndBox3 implements IBndBox3 {
  cornerMin: IVec3 = Vec3.invalid(); cornerMax: IVec3 = Vec3.zero();

  /** Return TRUE if box is invalid (void). */
  public static isVoid(theBox: IBndBox3): boolean {
    return theBox.cornerMin.x > theBox.cornerMax.x || theBox.cornerMin.y > theBox.cornerMax.y || theBox.cornerMin.z > theBox.cornerMax.z;
  }

  /** Return void box. */
  public static voidBox(): IBndBox3 { return { cornerMin: Vec3.invalid(), cornerMax: Vec3.zero() }; }

  /** Return box diagonal */
  public static diagonal(theBox: IBndBox3): IVec3 {
    if (BndBox3.isVoid(theBox)) { return {x: 0.0, y: 0.0, z: 0.0}; }
    return Vec3.subtract(theBox.cornerMax, theBox.cornerMin);
  }

  /** Return box extent (diagonal modulus). */
  public static extent(theBox: IBndBox3): number {
    if (BndBox3.isVoid(theBox)) { return 0.0; }
    return Vec3.modulus(BndBox3.diagonal(theBox));
  }
};

/**
 * Represents axes interface (based on `gp_Ax2` from C++ OCCT).
 */
export interface IAx2 { location: IVec3; direction: IVec3; xDirection: IVec3 };

/**
 * Represents axes object (based on `gp_Ax2` from C++ OCCT).
 */
export class Ax2 implements IAx2 {
  location: IVec3 = Vec3.zero();
  direction: IVec3 = Vec3.dz();
  xDirection: IVec3 = Vec3.dx();

  /**
   * Identifies a coordinate system where its origin is Origin,
   * and its "main Direction" and "X Direction" coordinates
   * Z = 1.0, X = Y =0.0 and X direction coordinates X = 1.0, Y = Z = 0.0
   */
  public static xoy(): IAx2 { return { location: Vec3.zero(), direction: Vec3.dz(), xDirection: Vec3.dx() }; }

  /**
   * Identifies a coordinate system where its origin is Origin,
   * and its "main Direction" and "X Direction" coordinates
   * Y = 1.0, X = Z =0.0 and X direction coordinates Z = 1.0, X = Y = 0.0
   */
  public static zox(): IAx2 { return { location: Vec3.zero(), direction: Vec3.dy(), xDirection: Vec3.dz() }; }

  /**
   * Identifies a coordinate system where its origin is Origin,
   * and its "main Direction" and "X Direction" coordinates
   * X = 1.0, Z = Y =0.0 and X direction coordinates Y = 1.0, X = Z = 0.0
   * In 2D space
   */
  public static yoz(): IAx2 { return { location: Vec3.zero(), direction: Vec3.dx(), xDirection: Vec3.dy() }; }

  /** Return invalid system. */
  public static invalid(): IAx2 { return { location: Vec3.invalid(), direction: Vec3.zero(), xDirection: Vec3.zero() }; }
};

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
export class Vec3 implements IVec3 {
  x: number = 0.0;
  y: number = 0.0;
  z: number = 0.0;

  /** Return zero vector. */
  public static zero(): IVec3 { return { x: 0.0, y: 0.0, z: 0.0 }; }

  /** Return DX vector. */
  public static dx(): IVec3 { return { x: 1.0, y: 0.0, z: 0.0 }; }

  /** Return DY vector. */
  public static dy(): IVec3 { return { x: 0.0, y: 1.0, z: 0.0 }; }

  /** Return DZ vector. */
  public static dz(): IVec3 { return { x: 0.0, y: 0.0, z: 1.0 }; }

  /** Return invalid (`RealLast()`) value. */
  public static invalidDouble(): number { return Number.MAX_VALUE; }

  /** Return invalid vector (`RealLast()` values). */
  public static invalid(): IVec3 { return { x: Number.MAX_VALUE, y: Number.MAX_VALUE, z: Number.MAX_VALUE }; }

  /** Return TRUE if point has infinite range (same as `Precision::IsInfinite()` check in OCCT). */
  public static isInfinite(theA: IVec3 | number): boolean {
    if (typeof (theA) === 'number') {
      return Math.abs(theA) >= (0.5 * 2.e+100);
    }
    return Math.abs(theA.x) >= (0.5 * 2.e+100) ||
           Math.abs(theA.y) >= (0.5 * 2.e+100) ||
           Math.abs(theA.z) >= (0.5 * 2.e+100);
  }

  /** Return opposite vector. */
  public static reverse(theV: IVec3): IVec3 {
    return { x: -theV.x, y: -theV.y, z: -theV.z };
  }

  /** Return coordinate with specified index within [0,2] range. */
  public static coord(theV: IVec3, theI: number): number {
    switch (theI) {
      case 0: return theV.x;
      case 1: return theV.y;
      case 2: return theV.z;
      default: throw new Error('Out of range index');
    }
  }

  /** Compute per-component addition between two vectors A+B. */
  public static add(theA: IVec3, theB: IVec3 | number): IVec3 {
    if (typeof (theB) === 'number') {
      return { x: theA.x + theB, y: theA.y + theB, z: theA.z + theB };
    }
    return { x: theA.x + theB.x, y: theA.y + theB.y, z: theA.z + theB.z };
  }

  /** Compute per-component subtraction between two vectors A-B. */
  public static subtract(theA: IVec3, theB: IVec3 | number): IVec3 {
    if (typeof (theB) === 'number') {
      return { x: theA.x - theB, y: theA.y - theB, z: theA.z - theB };
    }
    return { x: theA.x - theB.x, y: theA.y - theB.y, z: theA.z - theB.z };
  }

  /** Compute per-component multiplication between two vectors A*B. */
  public static multiply(theA: IVec3, theB: IVec3 | number): IVec3 {
    if (typeof (theB) === 'number') {
      return { x: theA.x * theB, y: theA.y * theB, z: theA.z * theB };
    }
    return { x: theA.x * theB.x, y: theA.y * theB.y, z: theA.z * theB.z };
  }

  /** Compute per-component division between two vectors A/B. */
  public static divide(theA: IVec3, theB: IVec3 | number): IVec3 {
    if (typeof (theB) === 'number') {
      return { x: theA.x / theB, y: theA.y / theB, z: theA.z / theB };
    }
    return { x: theA.x / theB.x, y: theA.y / theB.y, z: theA.z / theB.z };
  }

  /** Compute dot product between two vectors. */
  public static dot(theA: IVec3, theB: IVec3): number {
    return (theA.x * theB.x) + (theA.y * theB.y) + (theA.z * theB.z);
  }

  /** Return vector modulus. */
  public static modulus(theV: IVec3): number {
    return Math.sqrt(Vec3.dot(theV, theV));
  }

  /** Compute cross product between two vectors. */
  public static cross(theA: IVec3, theB: IVec3): IVec3 {
    return {
      x: (theA.y * theB.z) - (theA.z * theB.y),
      y: (theA.z * theB.x) - (theA.x * theB.z),
      z: (theA.x * theB.y) - (theA.y * theB.x),
    };
  }

  /** Return normalized vector. */
  public static normalize(theV: IVec3): IVec3 {
    const aMod: number = Vec3.modulus(theV);
    if (aMod === 0.0) { return theV; }
    return Vec3.divide(theV, aMod);
  }

  /** Compute angle between two normalized vectors */
  public static angle(theA: IVec3, theB: IVec3): number {
    const aCos: number = Vec3.dot(theA, theB);
    if (aCos > -0.70710678118655 && aCos < 0.70710678118655) {
      return Math.acos(aCos);
    }

    const aSin: number = Vec3.modulus(Vec3.cross(theA, theB));
    if (aCos < 0.0) { return Math.PI - Math.asin(aSin); }
    return Math.asin(aSin);
  }

  /**
   * Rotate normalized vector around another vector by angle.
   * @param theVec vector to rotate
   * @param theAxis vector to rotate around
   * @param theAngle angle in radians
   * @return rotated vector
   */
  public static rotateVector(theVec: IVec3, theAxis: IVec3, theAngle: number): IVec3 {
    // gp_Quaternion::SetVectorAndAngle()
    const anAxis: IVec3 = Vec3.normalize(theAxis);
    const anAngleHalf: number = 0.5 * theAngle;
    const sin_a: number = Math.sin(anAngleHalf);
    const aQuat: IVec4 = { x: anAxis.x * sin_a, y: anAxis.y * sin_a, z: anAxis.z * sin_a, w: Math.cos(anAngleHalf) };
    // do the math
    const u: IVec3 = { x: aQuat.x, y: aQuat.y, z: aQuat.z };
    const s: number = aQuat.w;
    const p1: IVec3 = Vec3.multiply(u, Vec3.dot(u, theVec) * 2.0);
    const p2: IVec3 = Vec3.multiply(theVec, (s * s) - Vec3.dot(u, u));
    const p3: IVec3 = Vec3.multiply(Vec3.cross(u, theVec), 2.0 * s);
    const aRes: IVec3 = Vec3.add(Vec3.add(p1, p2), p3);
    return aRes;
  }
};

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
};

/**
 * Object defining visual material (PBR) properties (based on `XCAFDoc_VisMaterialPBR` from C++ OCCT).
 */
export class VisMaterial implements IVisMaterial {
  /** flag indicating undefined material. */
  isDefined: boolean = true;
  /** base RGBA color (includes transparency). */
  baseColor: ColorString = 'WHITE';
  /** emissibe RGB color. */
  emissiveFactor: IVec3 = Vec3.zero();
  /** metallic factor within [0..1] range. */
  metallic: number = 0.0;
  /** roughness factor within [0..1] range. */
  roughness: number = 1.0;
  /** IOR. */
  refractionIndex: number = 1.5;
};

/**
 * Auxiliary tool for color converting.
 */
export class ColorTools {
  /**
   * Converts ColorString to Rgb.
   * @param theColorStr string in #xxxxxxxx format
   * @return object with rgb values in [0, 255] range
   */
  public static convertColorStringToRgb(theColorStr: ColorString): IRgb {
    let anRgba = ColorTools.convertColorStringToRgba(theColorStr);
    return { r: anRgba.r, g: anRgba.g, b: anRgba.b };
  }

  /**
   * Converts ColorString to Rgba.
   * @param theColorStr string in #xxxxxxxx format
   * @return object with rgba values in [0, 255] range
   */
  public static convertColorStringToRgba(theColorStr: ColorString): IRgba {
    if ((theColorStr.length !== 9 && theColorStr.length !== 7) || theColorStr[0] !== '#') {
      throw new Error(`Error: invalid color format: ${theColorStr}, should be #xxxxxxxx`);
    }
    let red = parseInt(theColorStr[1] + theColorStr[2], 16);
    let green = parseInt(theColorStr[3] + theColorStr[4], 16);
    let blue = parseInt(theColorStr[5] + theColorStr[6], 16);
    let alpha = theColorStr.length === 9 ? parseInt(theColorStr[7] + theColorStr[8], 16) : 255;
    return { r: red, g: green, b: blue, a: alpha };
  }

  /**
   * Converts Rgb to ColorString.
   * @param theColor object with rgb values in [0, 255] range
   * @return string in #xxxxxx format
   */
  public static convertRgbToColorString(theColor: IRgb): ColorString {
    let anRgba = { r: theColor.r, g: theColor.g, b: theColor.b, a: 255 };
    let aHex = ColorTools.convertRgbaToColorString(anRgba);
    return aHex.substring(0, 7);
  }

  /**
   * Converts Rgba to ColorString.
   * @param theColor object with rgb values in [0, 255] range
   * @return string in #xxxxxxxx format
   */
  public static convertRgbaToColorString(theColor: IRgba): ColorString {
    if (theColor.r < 0 || theColor.r > 255) {
      throw new Error(`Error: red component is ${theColor.r}, should be in [0, 255] range`);
    }
    if (theColor.g < 0 || theColor.g > 255) {
      throw new Error(`Error: green component is ${theColor.g}, should be in [0, 255] range`);
    }
    if (theColor.b < 0 || theColor.b > 255) {
      throw new Error(`Error: blue component is ${theColor.b}, should be in [0, 255] range`);
    }
    if (theColor.a < 0 || theColor.a > 255) {
      throw new Error(`Error: alpha component is ${theColor.a}, should be in [0, 255] range`);
    }

    let red = Math.floor(theColor.r).toString(16);
    if (red.length === 1) { red = `0${red}`; }
    let green = Math.floor(theColor.g).toString(16);
    if (green.length === 1) { green = `0${green}`; }
    let blue = Math.floor(theColor.b).toString(16);
    if (blue.length === 1) { blue = `0${blue}`; }
    let alpha = Math.floor(theColor.a).toString(16);
    if (alpha.length === 1) { alpha = `0${alpha}`; }

    return `#${red}${green}${blue}${alpha}`;
  }
};

/**
 * Standard camera orientations (based on `V3d_TypeOfOrientation` from C++ OCCT).
 * For using this enum please use its implementation from {@link SCwebviewer.CameraOrientation}
 * See also {@link Camera.setOrientation} method.
 */
export enum CameraOrientation {
  LEFT = 3, RIGHT = 0, BOTTOM = 5, TOP = 2, FRONT = 4, BACK = 1, AXORIGHT = 19, AXOLEFT = 24,
  XnegYneg = 9, XposYneg = 15,
  Axial = 5, // BOTTOM
  Frontal = 4, // FRONT
  Sagittal = 0, // RIGHT
  FrontalLeft = 15, // FRONT+RIGHT (left anatomical)
  FrontalRight = 9, // FRONT+LEFT (right anatomical)
};

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
};

/**
 * Represents camera View Point parameters object (see `Graphic3d_Camera` from C++ OCCT).
 */
export class CameraViewPoint implements ICameraViewPoint {
  /** Eye location (3d point). */
  eye: IVec3 = {x: 0.0, y: 0.0, z: 0.0};
  /** Direction of up vector (normalized 3D vector). */
  up: IVec3 = {x: 0.0, y: 0.0, z: 1.0};
  /** Direction of look (normalized 3D vector). */
  direction: IVec3 = {x: 0.0, y: 1.0, z: 0.0};
  /** Distance between camera and point to look. */
  distance: number = 0.0;
  /** Viewport relative scale. */
  scale: number = 1.0;
};

/**
 * B-Rep shape type (based on `TopAbs_ShapeEnum` from C++ OCCT).
 */
export enum ShapeEnum { COMPOUND = 0, COMPSOLID = 1, SOLID = 2, SHELL = 3, FACE = 4, WIRE = 5, EDGE = 6, VERTEX = 7, SHAPE = 8 };

/**
 * B-Rep shape orientation (based on `TopAbs_Orientation` from C++ OCCT).
 */
export enum ShapeOrientation { FORWARD = 0, REVERSED = 1, INTERNAL = 2, EXTERNAL = 3 };

/**
 * Geometry surface type (based on `GeomAbs_SurfaceType` from C++ OCCT).
 */
export enum ShapeSurfaceType {
  Plane = 0, Cylinder = 1, Cone = 2, Sphere = 3, Torus = 4, BezierSurface = 5, BSplineSurface = 6,
  SurfaceOfRevolution = 7, SurfaceOfExtrusion = 8, OffsetSurface = 9, OtherSurface = 10
};

/**
 * Geometry curve type (based on `GeomAbs_CurveType` from C++ OCCT).
 */
export enum ShapeCurveType {
  Line = 0, Circle = 1, Ellipse = 2, Hyperbola = 3, Parabola = 4, BezierCurve = 5, BSplineCurve = 6, OffsetCurve = 7, OtherCurve = 8
};

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
};
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
export enum EulerSequence {
  /** Classic Euler angles, alias to Intrinsic_ZXZ */
  EulerAngles = 0,

  /** Yaw Pitch Roll (or nautical) angles, alias to Intrinsic_ZYX */
  YawPitchRoll = 1,

  /** Tait-Bryan angles (using three different axes) */
  Extrinsic_XYZ = 2, Extrinsic_XZY = 3, Extrinsic_YZX = 4, Extrinsic_YXZ = 5, Extrinsic_ZXY = 6, Extrinsic_ZYX = 7,
  Intrinsic_XYZ = 8, Intrinsic_XZY = 9, Intrinsic_YZX = 10, Intrinsic_YXZ = 11, Intrinsic_ZXY = 12, Intrinsic_ZYX = 13,

  /** Proper Euler angles (using two different axes, first and third the same) */
  Extrinsic_XYX = 14, Extrinsic_XZX = 15, Extrinsic_YZY = 16, Extrinsic_YXY = 17, Extrinsic_ZYZ = 18, Extrinsic_ZXZ = 19,
  Intrinsic_XYX = 20, Intrinsic_XZX = 21, Intrinsic_YZY = 22, Intrinsic_YXY = 23, Intrinsic_ZXZ = 24, Intrinsic_ZYZ = 25
};

/**
 * Describes gp_Trsf location (transformation) object.
 */
export class Trsf implements ITrsf {
  /** rotation quaternion. */
  rotation: IVec4 = {x: 0.0, y: 0.0, z: 0.0, w: 1.0};
  /** translation vector. */
  translation: IVec3 = {x: 0.0, y: 0.0, z: 0.0};
  /** uniform scale factor. */
  scale: number = 1.0;

  /** Return TRUE if location has scale factor. */
  public static isScaled(theLoc: ITrsf): boolean {
    return Math.abs(theLoc.scale - 1.0) > 1.e-7;
  }

  /** Return TRUE if location has translation vector. */
  public static isTranslated(theLoc: ITrsf): boolean {
    return Math.abs(theLoc.translation.x) > 1.e-7 || Math.abs(theLoc.translation.y) > 1.e-7 || Math.abs(theLoc.translation.z) > 1.e-7;
  }

  /** Return TRUE if location has rotation quaternion. */
  public static isRotated(theLoc: ITrsf): boolean {
    return Math.abs(theLoc.rotation.x) > 1.e-7 ||
           Math.abs(theLoc.rotation.y) > 1.e-7 ||
           Math.abs(theLoc.rotation.z) > 1.e-7 ||
           Math.abs(theLoc.rotation.w - 1.0) > 1.e-7;
  }

  /** Return TRUE if location is empty (identity). */
  public static isIdentity(theLoc: ITrsf): boolean {
    return !Trsf.isScaled(theLoc) && !Trsf.isTranslated(theLoc) && !Trsf.isRotated(theLoc);
  }

  /** Format location info. */
  public static format(theLoc: ITrsf, theUMgr: UnitManager | null = null): string {
    let aStr: string = '';
    if (Trsf.isTranslated(theLoc)) {
      aStr += '[translation: ';
      if (theUMgr) {
        aStr += theUMgr.length.format(theLoc.translation);
      } else {
        aStr += `${theLoc.translation.x}, ${theLoc.translation.y}, ${theLoc.translation.z}`;
      }
      aStr += ']';
    }
    if (Trsf.isRotated(theLoc)) {
      aStr += `[rotation: ${theLoc.rotation.x}, ${theLoc.rotation.y}, ${theLoc.rotation.z}, ${theLoc.rotation.w}]`;
    }
    if (Trsf.isScaled(theLoc)) {
      aStr += `[scale: ${theLoc.scale}]`;
    }
    return aStr;
  }

  /** Multiply two transformations in specified order. */
  public static multiply(theLoc1: ITrsf, theLoc2: ITrsf): ITrsf { throw Error('Uninitialized static method Trsf.multiply()'); }

  /** Compute inverted transformation. */
  public static invert(theLoc: ITrsf): ITrsf { throw Error('Uninitialized static method Trsf.invert()'); }

  /** Return 4x4 matrix. */
  public static getMat4(theLoc: ITrsf): IMat4 { throw Error('Uninitialized static method Trsf.getMat4()'); }

  /** Return rotation part as 3 Euler angles. */
  public static getEulerAngles(theLoc: ITrsf, theSeq: EulerSequence = EulerSequence.EulerAngles): IVec3
  { throw Error('Uninitialized static method Trsf.getEulerAngles()'); }

  /** Setup rotation part from 3 Euler angles. */
  public static setEulerAngles(theLoc: ITrsf, theAngles: IVec3, theSeq: EulerSequence = EulerSequence.EulerAngles): ITrsf
  { throw Error('Uninitialized static method Trsf.setEulerAngles()'); }

  /** Setup rotation transformation around specified axis (origin will define translation part) and angle in radians. */
  public static rotateAroundAxis(theDir: IVec3, theAngle: number, theOrigin: IVec3 = {x: 0, y: 0, z: 0}): ITrsf
  { throw Error('Uninitialized static method Trsf.rotateAroundAxis()'); }

  /** Return transformed 3D point. */
  public static transformPnt(theLoc: ITrsf, thePnt: IVec3): IVec3 { throw Error('Uninitialized static method Trsf.transformPnt()'); }

  /** Return transformed 3D direction. */
  public static transformDir(theLoc: ITrsf, theDir: IVec3): IVec3 { throw Error('Uninitialized static method Trsf.transformDir()'); }
};
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
};

/**
 * Describes information about TopoDS_Shape.
 */
export class ShapeInfo implements IShapeInfo {
  /** Presentation object id. */
  objectId: ObjectId = '';

  /** Shape type. */
  shapeType: ShapeEnum = ShapeEnum.SHAPE;
  /** Selection index within subshapes of the same type. */
  index: number = -1;
  /** Number of direct subshapes. */
  nbChildren: number = 0;
  /** Shape orientation. */
  orientation: ShapeOrientation = ShapeOrientation.EXTERNAL;

  /** Free flag. */
  free: boolean = false;
  /** Locked flag. */
  locked: boolean = false;
  /** Modified flag. */
  modified: boolean = false;
  /** Checked flag. */
  checked: boolean = false;
  /** Orientable flag. */
  orientable: boolean = false;
  /** Closed flag. */
  closed: boolean = false;
  /** Infinite flag. */
  infinite: boolean = false;
  /** Convex flag. */
  convex: boolean = false;

  /** Geometry surface type (for FACE shape type). */
  surfaceType: ShapeSurfaceType = ShapeSurfaceType.OtherSurface;
  /** Geometry curve type (for EDGE shape type). */
  curveType: ShapeCurveType = ShapeCurveType.OtherCurve;

  /** Number of triangles (for FACE shape type). */
  nbTriangles: number = 0;
  /** Number of triangle nodes (for FACE shape type). */
  nbTriNodes: number = 0;
  /** Number of tessellation segments (for EDGE shapes). */
  nbPolySegments: number = 0;

  /** Vertex position (for VERTEX shape type). */
  point: IVec3 = Vec3.invalid();

  /** Subshape color. */
  color: ColorString = '';

  /** Subshape hidden flag. */
  hidden: boolean = false;

  /** Shape location. */
  location: ITrsf = {
    translation: {x: 0.0, y: 0.0, z: 0.0},
    rotation: {x: 0.0, y: 0.0, z: 0.0, w: 1.0},
    scale: 1.0,
  };

  /** Return indices of defined composite solid subshapes within parent presentation object. */
  public get compSolids(): number[] { return ShapeInfo.getSubshapes(this, ShapeEnum.COMPSOLID); }

  /** Return indices of defined solid subshapes within parent presentation object. */
  public get solids(): number[] { return ShapeInfo.getSubshapes(this, ShapeEnum.SOLID); }

  /** Return indices of defined shell subshapes within parent presentation object. */
  public get shells(): number[] { return ShapeInfo.getSubshapes(this, ShapeEnum.SHELL); }

  /** Return indices of defined face subshapes within parent presentation object. */
  public get faces(): number[] { return ShapeInfo.getSubshapes(this, ShapeEnum.FACE); }

  /** Return indices of defined wire subshapes within parent presentation object. */
  public get wires(): number[] { return ShapeInfo.getSubshapes(this, ShapeEnum.WIRE); }

  /** Return indices of defined edge subshapes within parent presentation object. */
  public get edges(): number[] { return ShapeInfo.getSubshapes(this, ShapeEnum.EDGE); }

  /** Return indices of defined vertex subshapes within parent presentation object. */
  public get vertices(): number[] { return ShapeInfo.getSubshapes(this, ShapeEnum.VERTEX); }

  /** Return indices of defined subshapes within parent presentation object. */
  protected static getSubshapes(theInfo: IShapeInfo, theType: ShapeEnum): number[] { return []; }

  /** Format shape info. */
  public static format(theInfo: IShapeInfo, theUMgr: UnitManager | null = null): string {
    if (theInfo.shapeType === ShapeEnum.SHAPE) { return 'NULL'; }

    let anInfo: string = '';
    anInfo += ShapeEnum[theInfo.shapeType];
    if (theInfo.index !== -1) { anInfo += ` #${theInfo.index}`; }
    if (theInfo.orientation !== ShapeOrientation.FORWARD) { anInfo += ` ${ShapeOrientation[theInfo.orientation]}`; }
    if (theInfo.shapeType === ShapeEnum.FACE) { anInfo += ` ${ShapeSurfaceType[theInfo.surfaceType]}`; }
    if (theInfo.shapeType === ShapeEnum.EDGE) { anInfo += ` ${ShapeCurveType[theInfo.curveType]}`; }
    if (!Vec3.isInfinite(theInfo.point)) {
      if (theUMgr) {
        anInfo += ` [${theUMgr.length.format(theInfo.point)}]`;
      } else {
        anInfo += ` [${theInfo.point.x} ${theInfo.point.y} ${theInfo.point.z}]`;
      }
    }
    if (theInfo.nbChildren !== 0) { anInfo += ` [${theInfo.nbChildren} children]`; }
    if (theInfo.nbTriangles !== 0) {
      anInfo += ` [${theInfo.nbTriangles} triangles, ${theInfo.nbTriNodes} nodes]`;
    }
    if (theInfo.nbPolySegments !== 0) {
      anInfo += ` [${theInfo.nbPolySegments} polysegments]`;
    }
    if (theInfo.color !== '') { anInfo += ` [color: ${theInfo.color}]`; }

    let aFlags = ShapeInfo.formatFlags(theInfo);
    if (aFlags !== '') { anInfo += ` [${aFlags.substring(1)}]`; }

    if (!Trsf.isIdentity(theInfo.location)) {
      anInfo += '\n';
      anInfo += Trsf.format(theInfo.location, theUMgr);
    }

    return anInfo;
  }

  /** Format shape flags. */
  public static formatFlags(theInfo: IShapeInfo): string {
    let aFlags = '';
    if (theInfo.free === true) { aFlags += '|free'; }
    if (theInfo.locked === true) { aFlags += '|locked'; }
    if (theInfo.modified === true) { aFlags += '|modified'; }
    if (theInfo.checked === true) { aFlags += '|checked'; }
    if (theInfo.orientable === true) { aFlags += '|orientable'; }
    if (theInfo.closed === true) { aFlags += '|closed'; }
    if (theInfo.infinite === true) { aFlags += '|infinite'; }
    if (theInfo.convex === true) { aFlags += '|convex'; }
    if (theInfo.hidden === true) { aFlags += '|hidden'; }
    return aFlags;
  }
};

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
};

/**
 * Cylinder shape construction parameters object.
 */
export class CylinderParams implements ICylinderParams {
  /** Coordinate system, center of bottom circle. */
  ax2: IAx2 = Ax2.xoy();
  /** Cylinder radius. */
  radius: number = 0.0;
  /** Cylinder height. */
  height: number = 0.0;
  /** Cylinder section angle. */
  angle: number = 2.0 * Math.PI;
};

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
};

/**
 * Cone shape construction parameters object.
 */
export class ConeParams implements IConeParams {
  /** Coordinate system, center of bottom circle. */
  ax2: IAx2 = Ax2.xoy();
  /** Cone bottom radius. */
  radius: number = 0.0;
  /** Cone top radius. */
  topRadius: number = 0.0;
  /** Cone height. */
  height: number = 0.0;
  /** Cone section angle. */
  angle: number = 2.0 * Math.PI;
};

/**
 * Quantity type defined by `SCPrsBase::QuantityType` enumeration.
 */
export enum QuantityType {
  /** unknown / dimensionless unit */
  UNKNOWN = -1,

// 7 base units in SI

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

// derived quantities

  /** Plane Angle, derived quantity defined in radians, rad */
  PlaneAngle = 7,
  /** Area, derived quantity defined in squared meters, m^2 */
  Area = 8,
  /** Volume, derived quantity defined in cubic meters, m^3 */
  Volume = 9,
  /** Speed, derived quantity defined in m/s */
  Speed = 10,
  /** Force, derived quantity defined in newtons, N */
  Force = 11,
};

/**
 * Define common length units.
 */
export enum LengthUnit {
  Micron = '\u00B5m',
  Millimeter = 'mm',
  Meter = 'm',
  Centimeter = 'cm',
  Kilometer = 'km',

  Microinch = '\u00B5in',
  Inch = 'in',
  Foot = 'ft',
  Mile = 'stat.mile',
};

/**
 * @internal Unit conversion section defined by `SCPrsBase::UnitSection` enumeration.
 */
enum UnitSection { SI = 0, System = 1, Display = 2 };

/** Base manager interface. */
export interface IManager {
  //
};

/**
 * Represents tool for managing units.
 * See also {@link SCwebviewer.unitManager} property.
 */
export interface UnitManager extends IManager {

// JS extensions

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

// C++ implementation

  /** Format unit name (e.g. replace '**2' with squared sign and similar). */
  formatUnitName: (theUnit: string) => string;

  /** Convert value from one unit to another unit. */
  convert: (theVal: number, theFromUnit: string, theToUnit: string) => number;

  /** Convert value from SI unit to another unit. */
  convertAnyFromSI: (theVal: number, theToUnit: string) => number;

  /** Convert value to SI unit from another unit. */
  convertAnyToSI: (theVal: number, theFromUnit: string) => number;

  /** @internal Find quantity for specified unit. */
  findQuantity: (theUnit: string) => number;

  /** @internal Return quantity name. */
  getQuantityName: (theQuantity: number) => string;

  /** @internal Return quantity units for specific section. */
  getQuantityUnit: (theQuantity: number, theSec: number) => string;

  /** @internal Set quantity units for specific section */
  setQuantityUnit: (theQuantity: number, theSec: number, theUnit: string) => void;

  /** @internal Convert value from system units into display units. */
  systemToDisplay: (theQuantity: number, theSysVal: number) => number;

  /** @internal Convert value from display units into system units. */
  systemFromDisplay: (theQuantity: number, theDispVal: number) => number;

  /** @internal Convert value from system units into SI units. */
  systemToSI: (theQuantity: number, theSysVal: number) => number;

  /** @internal Convert value from SI units into system units. */
  systemFromSI: (theQuantity: number, theSIVal: number) => number;

  /** @internal Multiply two transformations in specified order. */
  trsfMultiply: (theLoc1: ITrsf, theLoc2: ITrsf) => ITrsf;

  /** @internal Compute inverted transformation. */
  trsfInvert: (theLoc: ITrsf) => ITrsf;

  /** @internal Return 4x4 matrix from transformation. */
  trsfGetMat4: (theLoc: ITrsf) => IMat4;

  /** @internal Return rotation part as 3 Euler angles. */
  trsfGetEulerAngles: (theLoc: ITrsf, theSeq: EulerSequence) => IVec3;

  /** @internal Set rotation part from 3 Euler angles. */
  trsfSetEulerAngles: (theLoc: ITrsf, theAngles: IVec3, theSeq: EulerSequence) => ITrsf;

  /** @internal Setup rotation transformation around specified axis and angle in radians. */
  trsfRotateAroundAxis: (theDir: IVec3, theAngle: number, theOrigin: IVec3) => ITrsf;

  /** @internal Return transformed 3D point. */
  trsfTransformPnt: (theLoc: ITrsf, thePnt: IVec3) => IVec3;

  /** @internal Return transformed 3D direction. */
  trsfTransformDir: (theLoc: ITrsf, theDir: IVec3) => IVec3;
};

/**
 * Auxiliary tool to initialize JS extensions to UnitManager C++ class.
 * @internal
 */
export class UnitManagerExtensions {
  public static init(theMgr: UnitManager): void {
    Object.defineProperty(theMgr, 'length', {
      get: function (): Quantity { return new Quantity(theMgr, QuantityType.Length); },
    });
    Object.defineProperty(theMgr, 'area', {
      get: function (): Quantity { return new Quantity(theMgr, QuantityType.Area); },
    });
    Object.defineProperty(theMgr, 'volume', {
      get: function (): Quantity { return new Quantity(theMgr, QuantityType.Volume); },
    });
    Object.defineProperty(theMgr, 'angle', {
      get: function (): Quantity { return new Quantity(theMgr, QuantityType.PlaneAngle); },
    });
    Object.defineProperty(theMgr, 'quantities', {
      get: function (): Quantity[] {
        let anArray: Array<Quantity> = new Array(QuantityType.Force + 1);
        for (let aTypeIter: number = 0; aTypeIter < anArray.length; ++aTypeIter) {
          anArray[aTypeIter] = new Quantity(theMgr, aTypeIter);
        }
        return anArray;
      },
    });

    // expand Trsf prototype with useful static methods
    Object.defineProperty(Trsf, 'multiply', {
      value: function(theLoc1: ITrsf, theLoc2: ITrsf): ITrsf { return theMgr.trsfMultiply(theLoc1, theLoc2); },
    });
    Object.defineProperty(Trsf, 'invert', {
      value: function(theLoc: ITrsf): ITrsf { return theMgr.trsfInvert(theLoc); },
    });
    Object.defineProperty(Trsf, 'getMat4', {
      value: function(theLoc: ITrsf): IMat4 { return theMgr.trsfGetMat4(theLoc); },
    });
    Object.defineProperty(Trsf, 'getEulerAngles', {
      value: function(theLoc: ITrsf, theSeq: EulerSequence = EulerSequence.EulerAngles): IVec3
      { return theMgr.trsfGetEulerAngles(theLoc, theSeq); },
    });
    Object.defineProperty(Trsf, 'setEulerAngles', {
      value: function(theLoc: ITrsf, theAngles: IVec3, theSeq: EulerSequence = EulerSequence.EulerAngles): ITrsf
      { return theMgr.trsfSetEulerAngles(theLoc, theAngles, theSeq); },
    });
    Object.defineProperty(Trsf, 'rotateAroundAxis', {
      value: function(theDir: IVec3, theAngle: number, theOrigin: IVec3 = {x: 0, y: 0, z: 0}): ITrsf
      { return theMgr.trsfRotateAroundAxis(theDir, theAngle, theOrigin); },
    });
    Object.defineProperty(Trsf, 'transformPnt', {
      value: function(theLoc: ITrsf, thePnt: IVec3): IVec3 { return theMgr.trsfTransformPnt(theLoc, thePnt); },
    });
    Object.defineProperty(Trsf, 'transformDir', {
      value: function(theLoc: ITrsf, theDir: IVec3): IVec3 { return theMgr.trsfTransformDir(theLoc, theDir); },
    });
  }
};

/**
 * Defines quantity units convertion tool.
 */
export class Quantity {
  private m_unitMgr: UnitManager;
  private m_type: QuantityType;

  /** Main constructor. */
  constructor(theMgr: UnitManager, theType: QuantityType) {
    this.m_unitMgr = theMgr;
    this.m_type = theType;
  }

  /** Return Quantity type */
  public get type(): QuantityType { return this.m_type; }

  /** Return Quantity name */
  public get name(): string { return this.m_unitMgr.getQuantityName(this.m_type); }

  /** Return quantity SI units */
  public get unitSI(): string { return this.m_unitMgr.getQuantityUnit(this.m_type, UnitSection.SI); }

  /** Return quantity system units */
  public get unitSystem(): string { return this.m_unitMgr.getQuantityUnit(this.m_type, UnitSection.System); }

  /** Return quantity display units */
  public get unitDisplay(): string { return this.m_unitMgr.getQuantityUnit(this.m_type, UnitSection.Display); }

  /** Set quantity system units */
  public set unitSystem(theUnit: string) { this.m_unitMgr.setQuantityUnit(this.m_type, UnitSection.System, theUnit); }

  /** Set quantity display units */
  public set unitDisplay(theUnit: string) { this.m_unitMgr.setQuantityUnit(this.m_type, UnitSection.Display, theUnit); }

  /** Convert value from one unit to another unit. */
  public convert(theVal: number, theFromUnit: string, theToUnit: string): number {
    return this.m_unitMgr.convert(theVal, theFromUnit, theToUnit);
  }

  /** Convert value from SI unit to another unit. */
  public convertAnyFromSI(theVal: number, theToUnit: string): number {
    return this.m_unitMgr.convertAnyFromSI(theVal, theToUnit);
  }

  /** Convert value to SI unit from another unit. */
  public convertAnyToSI(theVal: number, theFromUnit: string): number {
    return this.m_unitMgr.convertAnyToSI(theVal, theFromUnit);
  }

  /** Convert value from system units into display units. */
  public systemToDisplay(theSysVal: number): number { return this.m_unitMgr.systemToDisplay(this.m_type, theSysVal); }

  /** Convert value from system units into display units. */
  public systemToDisplayVec3(theSysVal: IVec3): IVec3 {
    return { x: this.systemToDisplay(theSysVal.x),
             y: this.systemToDisplay(theSysVal.y),
             z: this.systemToDisplay(theSysVal.z) };
  }

  /** Convert value from display units into system units. */
  public systemFromDisplay(theDispVal: number): number { return this.m_unitMgr.systemFromDisplay(this.m_type, theDispVal); }

  /** Convert value from display units into system units. */
  public systemFromDisplayVec3(theDispVal: IVec3): IVec3 {
    return { x: this.systemFromDisplay(theDispVal.x),
             y: this.systemFromDisplay(theDispVal.y),
             z: this.systemFromDisplay(theDispVal.z) };
  }

  /** Convert value from system units into SI units. */
  public systemToSI(theSysVal: number): number { return this.m_unitMgr.systemToSI(this.m_type, theSysVal); }

  /** Convert value from system units into SI units. */
  public systemToSIVec3(theSysVal: IVec3): IVec3 {
    return { x: this.systemToSI(theSysVal.x),
             y: this.systemToSI(theSysVal.y),
             z: this.systemToSI(theSysVal.z) };
  }

  /** Convert value from SI units into system units. */
  public systemFromSI(theSIVal: number): number { return this.m_unitMgr.systemFromSI(this.m_type, theSIVal); }

  /** Convert value from SI units into system units. */
  public systemFromSIVec3(theSIVal: IVec3): IVec3 {
    return { x: this.systemFromSI(theSIVal.x),
             y: this.systemFromSI(theSIVal.y),
             z: this.systemFromSI(theSIVal.z) };
  }

  /** Auxiliary tool converting real number from System to Display units and appending units. */
  public formatReal(theSys: number): string {
    if (Vec3.isInfinite(theSys)) { return theSys >= 0.0 ? '+inf' : '-inf'; }
    const aVal = this.systemToDisplay(theSys);
    return `${aVal} ${this.unitDisplay}`;
  }

  /** Auxiliary tool converting real number from System to Display units and appending units. */
  public formatVec3(theSys: IVec3): string {
    return `${this.formatReal(theSys.x)}, ${this.formatReal(theSys.y)}, ${this.formatReal(theSys.z)}`;
  }

  /** Auxiliary tool converting real number from System to Display units and appending units. */
  public format(theSys: IVec3 | number): string {
    if (typeof (theSys) === 'number') {
      return this.formatReal(theSys);
    }
    return this.formatVec3(theSys);
  }
};
