// Created by: Kirill Gavrilov
// Copyright (c) 2022 Huawei Technologies Co., Ltd.
;
/**
 * (Sub)view corner defined by `SCwebviewer::ViewCorner` enumeration.
 */
export var ViewCorner;
(function (ViewCorner) {
    ViewCorner[ViewCorner["FREE3D"] = -1] = "FREE3D";
    ViewCorner[ViewCorner["CENTER_CENTER"] = 0] = "CENTER_CENTER";
    ViewCorner[ViewCorner["CENTER_UPPER"] = 1] = "CENTER_UPPER";
    ViewCorner[ViewCorner["CENTER_LOWER"] = 2] = "CENTER_LOWER";
    ViewCorner[ViewCorner["LEFT_CENTER"] = 4] = "LEFT_CENTER";
    ViewCorner[ViewCorner["LEFT_UPPER"] = 5] = "LEFT_UPPER";
    ViewCorner[ViewCorner["LEFT_LOWER"] = 6] = "LEFT_LOWER";
    ViewCorner[ViewCorner["RIGHT_CENTER"] = 8] = "RIGHT_CENTER";
    ViewCorner[ViewCorner["RIGHT_UPPER"] = 9] = "RIGHT_UPPER";
    ViewCorner[ViewCorner["RIGHT_LOWER"] = 10] = "RIGHT_LOWER";
})(ViewCorner || (ViewCorner = {}));
;
;
/**
 * Represents 2D vector object (based on `Graphic3d_Vec2d` from C++ OCCT).
 */
export class Vec2 {
    constructor() {
        this.x = 0.0;
        this.y = 0.0;
    }
    /** Return zero vector. */
    static zero() { return { x: 0.0, y: 0.0 }; }
}
;
;
;
;
;
/**
 * Represents 4D vector object (based on `Graphic3d_Vec4d` from C++ OCCT).
 */
export class Vec4 {
    constructor() {
        this.x = 0.0;
        this.y = 0.0;
        this.z = 0.0;
        this.w = 0.0;
    }
}
;
;
;
/**
 * Represents 3D axes-aligned bounding box object defined by pair of it's corner points (based on `Graphic3d_BndBox3d` from C++ OCCT).
 */
export class BndBox3 {
    constructor() {
        this.cornerMin = Vec3.invalid();
        this.cornerMax = Vec3.zero();
    }
    /** Return TRUE if box is invalid (void). */
    static isVoid(theBox) {
        return theBox.cornerMin.x > theBox.cornerMax.x || theBox.cornerMin.y > theBox.cornerMax.y || theBox.cornerMin.z > theBox.cornerMax.z;
    }
    /** Return void box. */
    static voidBox() { return { cornerMin: Vec3.invalid(), cornerMax: Vec3.zero() }; }
    /** Return box diagonal */
    static diagonal(theBox) {
        if (BndBox3.isVoid(theBox)) {
            return { x: 0.0, y: 0.0, z: 0.0 };
        }
        return Vec3.subtract(theBox.cornerMax, theBox.cornerMin);
    }
    /** Return box extent (diagonal modulus). */
    static extent(theBox) {
        if (BndBox3.isVoid(theBox)) {
            return 0.0;
        }
        return Vec3.modulus(BndBox3.diagonal(theBox));
    }
}
;
;
/**
 * Represents axes object (based on `gp_Ax2` from C++ OCCT).
 */
export class Ax2 {
    constructor() {
        this.location = Vec3.zero();
        this.direction = Vec3.dz();
        this.xDirection = Vec3.dx();
    }
    /**
     * Identifies a coordinate system where its origin is Origin,
     * and its "main Direction" and "X Direction" coordinates
     * Z = 1.0, X = Y =0.0 and X direction coordinates X = 1.0, Y = Z = 0.0
     */
    static xoy() { return { location: Vec3.zero(), direction: Vec3.dz(), xDirection: Vec3.dx() }; }
    /**
     * Identifies a coordinate system where its origin is Origin,
     * and its "main Direction" and "X Direction" coordinates
     * Y = 1.0, X = Z =0.0 and X direction coordinates Z = 1.0, X = Y = 0.0
     */
    static zox() { return { location: Vec3.zero(), direction: Vec3.dy(), xDirection: Vec3.dz() }; }
    /**
     * Identifies a coordinate system where its origin is Origin,
     * and its "main Direction" and "X Direction" coordinates
     * X = 1.0, Z = Y =0.0 and X direction coordinates Y = 1.0, X = Z = 0.0
     * In 2D space
     */
    static yoz() { return { location: Vec3.zero(), direction: Vec3.dx(), xDirection: Vec3.dy() }; }
    /** Return invalid system. */
    static invalid() { return { location: Vec3.invalid(), direction: Vec3.zero(), xDirection: Vec3.zero() }; }
}
;
/**
 * Represents 3D vector object (based on `Graphic3d_Vec3d` from C++ OCCT).
 * Provides also math operations between 3D vectors.
 */
export class Vec3 {
    constructor() {
        this.x = 0.0;
        this.y = 0.0;
        this.z = 0.0;
    }
    /** Return zero vector. */
    static zero() { return { x: 0.0, y: 0.0, z: 0.0 }; }
    /** Return DX vector. */
    static dx() { return { x: 1.0, y: 0.0, z: 0.0 }; }
    /** Return DY vector. */
    static dy() { return { x: 0.0, y: 1.0, z: 0.0 }; }
    /** Return DZ vector. */
    static dz() { return { x: 0.0, y: 0.0, z: 1.0 }; }
    /** Return invalid (`RealLast()`) value. */
    static invalidDouble() { return Number.MAX_VALUE; }
    /** Return invalid vector (`RealLast()` values). */
    static invalid() { return { x: Number.MAX_VALUE, y: Number.MAX_VALUE, z: Number.MAX_VALUE }; }
    /** Return TRUE if point has infinite range (same as `Precision::IsInfinite()` check in OCCT). */
    static isInfinite(theA) {
        if (typeof (theA) === 'number') {
            return Math.abs(theA) >= (0.5 * 2.e+100);
        }
        return Math.abs(theA.x) >= (0.5 * 2.e+100) ||
            Math.abs(theA.y) >= (0.5 * 2.e+100) ||
            Math.abs(theA.z) >= (0.5 * 2.e+100);
    }
    /** Return opposite vector. */
    static reverse(theV) {
        return { x: -theV.x, y: -theV.y, z: -theV.z };
    }
    /** Return coordinate with specified index within [0,2] range. */
    static coord(theV, theI) {
        switch (theI) {
            case 0: return theV.x;
            case 1: return theV.y;
            case 2: return theV.z;
            default: throw new Error('Out of range index');
        }
    }
    /** Compute per-component addition between two vectors A+B. */
    static add(theA, theB) {
        if (typeof (theB) === 'number') {
            return { x: theA.x + theB, y: theA.y + theB, z: theA.z + theB };
        }
        return { x: theA.x + theB.x, y: theA.y + theB.y, z: theA.z + theB.z };
    }
    /** Compute per-component subtraction between two vectors A-B. */
    static subtract(theA, theB) {
        if (typeof (theB) === 'number') {
            return { x: theA.x - theB, y: theA.y - theB, z: theA.z - theB };
        }
        return { x: theA.x - theB.x, y: theA.y - theB.y, z: theA.z - theB.z };
    }
    /** Compute per-component multiplication between two vectors A*B. */
    static multiply(theA, theB) {
        if (typeof (theB) === 'number') {
            return { x: theA.x * theB, y: theA.y * theB, z: theA.z * theB };
        }
        return { x: theA.x * theB.x, y: theA.y * theB.y, z: theA.z * theB.z };
    }
    /** Compute per-component division between two vectors A/B. */
    static divide(theA, theB) {
        if (typeof (theB) === 'number') {
            return { x: theA.x / theB, y: theA.y / theB, z: theA.z / theB };
        }
        return { x: theA.x / theB.x, y: theA.y / theB.y, z: theA.z / theB.z };
    }
    /** Compute dot product between two vectors. */
    static dot(theA, theB) {
        return (theA.x * theB.x) + (theA.y * theB.y) + (theA.z * theB.z);
    }
    /** Return vector modulus. */
    static modulus(theV) {
        return Math.sqrt(Vec3.dot(theV, theV));
    }
    /** Compute cross product between two vectors. */
    static cross(theA, theB) {
        return {
            x: (theA.y * theB.z) - (theA.z * theB.y),
            y: (theA.z * theB.x) - (theA.x * theB.z),
            z: (theA.x * theB.y) - (theA.y * theB.x),
        };
    }
    /** Return normalized vector. */
    static normalize(theV) {
        const aMod = Vec3.modulus(theV);
        if (aMod === 0.0) {
            return theV;
        }
        return Vec3.divide(theV, aMod);
    }
    /** Compute angle between two normalized vectors */
    static angle(theA, theB) {
        const aCos = Vec3.dot(theA, theB);
        if (aCos > -0.70710678118655 && aCos < 0.70710678118655) {
            return Math.acos(aCos);
        }
        const aSin = Vec3.modulus(Vec3.cross(theA, theB));
        if (aCos < 0.0) {
            return Math.PI - Math.asin(aSin);
        }
        return Math.asin(aSin);
    }
    /**
     * Rotate normalized vector around another vector by angle.
     * @param theVec vector to rotate
     * @param theAxis vector to rotate around
     * @param theAngle angle in radians
     * @return rotated vector
     */
    static rotateVector(theVec, theAxis, theAngle) {
        // gp_Quaternion::SetVectorAndAngle()
        const anAxis = Vec3.normalize(theAxis);
        const anAngleHalf = 0.5 * theAngle;
        const sin_a = Math.sin(anAngleHalf);
        const aQuat = { x: anAxis.x * sin_a, y: anAxis.y * sin_a, z: anAxis.z * sin_a, w: Math.cos(anAngleHalf) };
        // do the math
        const u = { x: aQuat.x, y: aQuat.y, z: aQuat.z };
        const s = aQuat.w;
        const p1 = Vec3.multiply(u, Vec3.dot(u, theVec) * 2.0);
        const p2 = Vec3.multiply(theVec, (s * s) - Vec3.dot(u, u));
        const p3 = Vec3.multiply(Vec3.cross(u, theVec), 2.0 * s);
        const aRes = Vec3.add(Vec3.add(p1, p2), p3);
        return aRes;
    }
}
;
;
/**
 * Object defining visual material (PBR) properties (based on `XCAFDoc_VisMaterialPBR` from C++ OCCT).
 */
export class VisMaterial {
    constructor() {
        /** flag indicating undefined material. */
        this.isDefined = true;
        /** base RGBA color (includes transparency). */
        this.baseColor = 'WHITE';
        /** emissibe RGB color. */
        this.emissiveFactor = Vec3.zero();
        /** metallic factor within [0..1] range. */
        this.metallic = 0.0;
        /** roughness factor within [0..1] range. */
        this.roughness = 1.0;
        /** IOR. */
        this.refractionIndex = 1.5;
    }
}
;
/**
 * Auxiliary tool for color converting.
 */
export class ColorTools {
    /**
     * Converts ColorString to Rgb.
     * @param theColorStr string in #xxxxxxxx format
     * @return object with rgb values in [0, 255] range
     */
    static convertColorStringToRgb(theColorStr) {
        let anRgba = ColorTools.convertColorStringToRgba(theColorStr);
        return { r: anRgba.r, g: anRgba.g, b: anRgba.b };
    }
    /**
     * Converts ColorString to Rgba.
     * @param theColorStr string in #xxxxxxxx format
     * @return object with rgba values in [0, 255] range
     */
    static convertColorStringToRgba(theColorStr) {
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
    static convertRgbToColorString(theColor) {
        let anRgba = { r: theColor.r, g: theColor.g, b: theColor.b, a: 255 };
        let aHex = ColorTools.convertRgbaToColorString(anRgba);
        return aHex.substring(0, 7);
    }
    /**
     * Converts Rgba to ColorString.
     * @param theColor object with rgb values in [0, 255] range
     * @return string in #xxxxxxxx format
     */
    static convertRgbaToColorString(theColor) {
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
        if (red.length === 1) {
            red = `0${red}`;
        }
        let green = Math.floor(theColor.g).toString(16);
        if (green.length === 1) {
            green = `0${green}`;
        }
        let blue = Math.floor(theColor.b).toString(16);
        if (blue.length === 1) {
            blue = `0${blue}`;
        }
        let alpha = Math.floor(theColor.a).toString(16);
        if (alpha.length === 1) {
            alpha = `0${alpha}`;
        }
        return `#${red}${green}${blue}${alpha}`;
    }
}
;
/**
 * Standard camera orientations (based on `V3d_TypeOfOrientation` from C++ OCCT).
 * For using this enum please use its implementation from {@link SCwebviewer.CameraOrientation}
 * See also {@link Camera.setOrientation} method.
 */
export var CameraOrientation;
(function (CameraOrientation) {
    CameraOrientation[CameraOrientation["LEFT"] = 3] = "LEFT";
    CameraOrientation[CameraOrientation["RIGHT"] = 0] = "RIGHT";
    CameraOrientation[CameraOrientation["BOTTOM"] = 5] = "BOTTOM";
    CameraOrientation[CameraOrientation["TOP"] = 2] = "TOP";
    CameraOrientation[CameraOrientation["FRONT"] = 4] = "FRONT";
    CameraOrientation[CameraOrientation["BACK"] = 1] = "BACK";
    CameraOrientation[CameraOrientation["AXORIGHT"] = 19] = "AXORIGHT";
    CameraOrientation[CameraOrientation["AXOLEFT"] = 24] = "AXOLEFT";
    CameraOrientation[CameraOrientation["XnegYneg"] = 9] = "XnegYneg";
    CameraOrientation[CameraOrientation["XposYneg"] = 15] = "XposYneg";
    CameraOrientation[CameraOrientation["Axial"] = 5] = "Axial";
    CameraOrientation[CameraOrientation["Frontal"] = 4] = "Frontal";
    CameraOrientation[CameraOrientation["Sagittal"] = 0] = "Sagittal";
    CameraOrientation[CameraOrientation["FrontalLeft"] = 15] = "FrontalLeft";
    CameraOrientation[CameraOrientation["FrontalRight"] = 9] = "FrontalRight";
})(CameraOrientation || (CameraOrientation = {}));
;
;
/**
 * Represents camera View Point parameters object (see `Graphic3d_Camera` from C++ OCCT).
 */
export class CameraViewPoint {
    constructor() {
        /** Eye location (3d point). */
        this.eye = { x: 0.0, y: 0.0, z: 0.0 };
        /** Direction of up vector (normalized 3D vector). */
        this.up = { x: 0.0, y: 0.0, z: 1.0 };
        /** Direction of look (normalized 3D vector). */
        this.direction = { x: 0.0, y: 1.0, z: 0.0 };
        /** Distance between camera and point to look. */
        this.distance = 0.0;
        /** Viewport relative scale. */
        this.scale = 1.0;
    }
}
;
/**
 * B-Rep shape type (based on `TopAbs_ShapeEnum` from C++ OCCT).
 */
export var ShapeEnum;
(function (ShapeEnum) {
    ShapeEnum[ShapeEnum["COMPOUND"] = 0] = "COMPOUND";
    ShapeEnum[ShapeEnum["COMPSOLID"] = 1] = "COMPSOLID";
    ShapeEnum[ShapeEnum["SOLID"] = 2] = "SOLID";
    ShapeEnum[ShapeEnum["SHELL"] = 3] = "SHELL";
    ShapeEnum[ShapeEnum["FACE"] = 4] = "FACE";
    ShapeEnum[ShapeEnum["WIRE"] = 5] = "WIRE";
    ShapeEnum[ShapeEnum["EDGE"] = 6] = "EDGE";
    ShapeEnum[ShapeEnum["VERTEX"] = 7] = "VERTEX";
    ShapeEnum[ShapeEnum["SHAPE"] = 8] = "SHAPE";
})(ShapeEnum || (ShapeEnum = {}));
;
/**
 * B-Rep shape orientation (based on `TopAbs_Orientation` from C++ OCCT).
 */
export var ShapeOrientation;
(function (ShapeOrientation) {
    ShapeOrientation[ShapeOrientation["FORWARD"] = 0] = "FORWARD";
    ShapeOrientation[ShapeOrientation["REVERSED"] = 1] = "REVERSED";
    ShapeOrientation[ShapeOrientation["INTERNAL"] = 2] = "INTERNAL";
    ShapeOrientation[ShapeOrientation["EXTERNAL"] = 3] = "EXTERNAL";
})(ShapeOrientation || (ShapeOrientation = {}));
;
/**
 * Geometry surface type (based on `GeomAbs_SurfaceType` from C++ OCCT).
 */
export var ShapeSurfaceType;
(function (ShapeSurfaceType) {
    ShapeSurfaceType[ShapeSurfaceType["Plane"] = 0] = "Plane";
    ShapeSurfaceType[ShapeSurfaceType["Cylinder"] = 1] = "Cylinder";
    ShapeSurfaceType[ShapeSurfaceType["Cone"] = 2] = "Cone";
    ShapeSurfaceType[ShapeSurfaceType["Sphere"] = 3] = "Sphere";
    ShapeSurfaceType[ShapeSurfaceType["Torus"] = 4] = "Torus";
    ShapeSurfaceType[ShapeSurfaceType["BezierSurface"] = 5] = "BezierSurface";
    ShapeSurfaceType[ShapeSurfaceType["BSplineSurface"] = 6] = "BSplineSurface";
    ShapeSurfaceType[ShapeSurfaceType["SurfaceOfRevolution"] = 7] = "SurfaceOfRevolution";
    ShapeSurfaceType[ShapeSurfaceType["SurfaceOfExtrusion"] = 8] = "SurfaceOfExtrusion";
    ShapeSurfaceType[ShapeSurfaceType["OffsetSurface"] = 9] = "OffsetSurface";
    ShapeSurfaceType[ShapeSurfaceType["OtherSurface"] = 10] = "OtherSurface";
})(ShapeSurfaceType || (ShapeSurfaceType = {}));
;
/**
 * Geometry curve type (based on `GeomAbs_CurveType` from C++ OCCT).
 */
export var ShapeCurveType;
(function (ShapeCurveType) {
    ShapeCurveType[ShapeCurveType["Line"] = 0] = "Line";
    ShapeCurveType[ShapeCurveType["Circle"] = 1] = "Circle";
    ShapeCurveType[ShapeCurveType["Ellipse"] = 2] = "Ellipse";
    ShapeCurveType[ShapeCurveType["Hyperbola"] = 3] = "Hyperbola";
    ShapeCurveType[ShapeCurveType["Parabola"] = 4] = "Parabola";
    ShapeCurveType[ShapeCurveType["BezierCurve"] = 5] = "BezierCurve";
    ShapeCurveType[ShapeCurveType["BSplineCurve"] = 6] = "BSplineCurve";
    ShapeCurveType[ShapeCurveType["OffsetCurve"] = 7] = "OffsetCurve";
    ShapeCurveType[ShapeCurveType["OtherCurve"] = 8] = "OtherCurve";
})(ShapeCurveType || (ShapeCurveType = {}));
;
;
/**
 * Enumerates all 24 possible variants of generalized
 * Euler angles, defining general 3d rotation by three
 * rotations around main axes of coordinate system,
 * in different possible orders.
 *
 * Alias to `gp_EulerSequence` from C++ OCCT.
 */
export var EulerSequence;
(function (EulerSequence) {
    /** Classic Euler angles, alias to Intrinsic_ZXZ */
    EulerSequence[EulerSequence["EulerAngles"] = 0] = "EulerAngles";
    /** Yaw Pitch Roll (or nautical) angles, alias to Intrinsic_ZYX */
    EulerSequence[EulerSequence["YawPitchRoll"] = 1] = "YawPitchRoll";
    /** Tait-Bryan angles (using three different axes) */
    EulerSequence[EulerSequence["Extrinsic_XYZ"] = 2] = "Extrinsic_XYZ";
    EulerSequence[EulerSequence["Extrinsic_XZY"] = 3] = "Extrinsic_XZY";
    EulerSequence[EulerSequence["Extrinsic_YZX"] = 4] = "Extrinsic_YZX";
    EulerSequence[EulerSequence["Extrinsic_YXZ"] = 5] = "Extrinsic_YXZ";
    EulerSequence[EulerSequence["Extrinsic_ZXY"] = 6] = "Extrinsic_ZXY";
    EulerSequence[EulerSequence["Extrinsic_ZYX"] = 7] = "Extrinsic_ZYX";
    EulerSequence[EulerSequence["Intrinsic_XYZ"] = 8] = "Intrinsic_XYZ";
    EulerSequence[EulerSequence["Intrinsic_XZY"] = 9] = "Intrinsic_XZY";
    EulerSequence[EulerSequence["Intrinsic_YZX"] = 10] = "Intrinsic_YZX";
    EulerSequence[EulerSequence["Intrinsic_YXZ"] = 11] = "Intrinsic_YXZ";
    EulerSequence[EulerSequence["Intrinsic_ZXY"] = 12] = "Intrinsic_ZXY";
    EulerSequence[EulerSequence["Intrinsic_ZYX"] = 13] = "Intrinsic_ZYX";
    /** Proper Euler angles (using two different axes, first and third the same) */
    EulerSequence[EulerSequence["Extrinsic_XYX"] = 14] = "Extrinsic_XYX";
    EulerSequence[EulerSequence["Extrinsic_XZX"] = 15] = "Extrinsic_XZX";
    EulerSequence[EulerSequence["Extrinsic_YZY"] = 16] = "Extrinsic_YZY";
    EulerSequence[EulerSequence["Extrinsic_YXY"] = 17] = "Extrinsic_YXY";
    EulerSequence[EulerSequence["Extrinsic_ZYZ"] = 18] = "Extrinsic_ZYZ";
    EulerSequence[EulerSequence["Extrinsic_ZXZ"] = 19] = "Extrinsic_ZXZ";
    EulerSequence[EulerSequence["Intrinsic_XYX"] = 20] = "Intrinsic_XYX";
    EulerSequence[EulerSequence["Intrinsic_XZX"] = 21] = "Intrinsic_XZX";
    EulerSequence[EulerSequence["Intrinsic_YZY"] = 22] = "Intrinsic_YZY";
    EulerSequence[EulerSequence["Intrinsic_YXY"] = 23] = "Intrinsic_YXY";
    EulerSequence[EulerSequence["Intrinsic_ZXZ"] = 24] = "Intrinsic_ZXZ";
    EulerSequence[EulerSequence["Intrinsic_ZYZ"] = 25] = "Intrinsic_ZYZ";
})(EulerSequence || (EulerSequence = {}));
;
/**
 * Describes gp_Trsf location (transformation) object.
 */
export class Trsf {
    constructor() {
        /** rotation quaternion. */
        this.rotation = { x: 0.0, y: 0.0, z: 0.0, w: 1.0 };
        /** translation vector. */
        this.translation = { x: 0.0, y: 0.0, z: 0.0 };
        /** uniform scale factor. */
        this.scale = 1.0;
    }
    /** Return TRUE if location has scale factor. */
    static isScaled(theLoc) {
        return Math.abs(theLoc.scale - 1.0) > 1.e-7;
    }
    /** Return TRUE if location has translation vector. */
    static isTranslated(theLoc) {
        return Math.abs(theLoc.translation.x) > 1.e-7 || Math.abs(theLoc.translation.y) > 1.e-7 || Math.abs(theLoc.translation.z) > 1.e-7;
    }
    /** Return TRUE if location has rotation quaternion. */
    static isRotated(theLoc) {
        return Math.abs(theLoc.rotation.x) > 1.e-7 ||
            Math.abs(theLoc.rotation.y) > 1.e-7 ||
            Math.abs(theLoc.rotation.z) > 1.e-7 ||
            Math.abs(theLoc.rotation.w - 1.0) > 1.e-7;
    }
    /** Return TRUE if location is empty (identity). */
    static isIdentity(theLoc) {
        return !Trsf.isScaled(theLoc) && !Trsf.isTranslated(theLoc) && !Trsf.isRotated(theLoc);
    }
    /** Format location info. */
    static format(theLoc, theUMgr = null) {
        let aStr = '';
        if (Trsf.isTranslated(theLoc)) {
            aStr += '[translation: ';
            if (theUMgr) {
                aStr += theUMgr.length.format(theLoc.translation);
            }
            else {
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
    static multiply(theLoc1, theLoc2) { throw Error('Uninitialized static method Trsf.multiply()'); }
    /** Compute inverted transformation. */
    static invert(theLoc) { throw Error('Uninitialized static method Trsf.invert()'); }
    /** Return 4x4 matrix. */
    static getMat4(theLoc) { throw Error('Uninitialized static method Trsf.getMat4()'); }
    /** Return rotation part as 3 Euler angles. */
    static getEulerAngles(theLoc, theSeq = EulerSequence.EulerAngles) { throw Error('Uninitialized static method Trsf.getEulerAngles()'); }
    /** Setup rotation part from 3 Euler angles. */
    static setEulerAngles(theLoc, theAngles, theSeq = EulerSequence.EulerAngles) { throw Error('Uninitialized static method Trsf.setEulerAngles()'); }
    /** Setup rotation transformation around specified axis (origin will define translation part) and angle in radians. */
    static rotateAroundAxis(theDir, theAngle, theOrigin = { x: 0, y: 0, z: 0 }) { throw Error('Uninitialized static method Trsf.rotateAroundAxis()'); }
    /** Return transformed 3D point. */
    static transformPnt(theLoc, thePnt) { throw Error('Uninitialized static method Trsf.transformPnt()'); }
    /** Return transformed 3D direction. */
    static transformDir(theLoc, theDir) { throw Error('Uninitialized static method Trsf.transformDir()'); }
}
;
;
/**
 * Describes information about TopoDS_Shape.
 */
export class ShapeInfo {
    constructor() {
        /** Presentation object id. */
        this.objectId = '';
        /** Shape type. */
        this.shapeType = ShapeEnum.SHAPE;
        /** Selection index within subshapes of the same type. */
        this.index = -1;
        /** Number of direct subshapes. */
        this.nbChildren = 0;
        /** Shape orientation. */
        this.orientation = ShapeOrientation.EXTERNAL;
        /** Free flag. */
        this.free = false;
        /** Locked flag. */
        this.locked = false;
        /** Modified flag. */
        this.modified = false;
        /** Checked flag. */
        this.checked = false;
        /** Orientable flag. */
        this.orientable = false;
        /** Closed flag. */
        this.closed = false;
        /** Infinite flag. */
        this.infinite = false;
        /** Convex flag. */
        this.convex = false;
        /** Geometry surface type (for FACE shape type). */
        this.surfaceType = ShapeSurfaceType.OtherSurface;
        /** Geometry curve type (for EDGE shape type). */
        this.curveType = ShapeCurveType.OtherCurve;
        /** Number of triangles (for FACE shape type). */
        this.nbTriangles = 0;
        /** Number of triangle nodes (for FACE shape type). */
        this.nbTriNodes = 0;
        /** Number of tessellation segments (for EDGE shapes). */
        this.nbPolySegments = 0;
        /** Vertex position (for VERTEX shape type). */
        this.point = Vec3.invalid();
        /** Subshape color. */
        this.color = '';
        /** Subshape hidden flag. */
        this.hidden = false;
        /** Shape location. */
        this.location = {
            translation: { x: 0.0, y: 0.0, z: 0.0 },
            rotation: { x: 0.0, y: 0.0, z: 0.0, w: 1.0 },
            scale: 1.0,
        };
    }
    /** Return indices of defined composite solid subshapes within parent presentation object. */
    get compSolids() { return ShapeInfo.getSubshapes(this, ShapeEnum.COMPSOLID); }
    /** Return indices of defined solid subshapes within parent presentation object. */
    get solids() { return ShapeInfo.getSubshapes(this, ShapeEnum.SOLID); }
    /** Return indices of defined shell subshapes within parent presentation object. */
    get shells() { return ShapeInfo.getSubshapes(this, ShapeEnum.SHELL); }
    /** Return indices of defined face subshapes within parent presentation object. */
    get faces() { return ShapeInfo.getSubshapes(this, ShapeEnum.FACE); }
    /** Return indices of defined wire subshapes within parent presentation object. */
    get wires() { return ShapeInfo.getSubshapes(this, ShapeEnum.WIRE); }
    /** Return indices of defined edge subshapes within parent presentation object. */
    get edges() { return ShapeInfo.getSubshapes(this, ShapeEnum.EDGE); }
    /** Return indices of defined vertex subshapes within parent presentation object. */
    get vertices() { return ShapeInfo.getSubshapes(this, ShapeEnum.VERTEX); }
    /** Return indices of defined subshapes within parent presentation object. */
    static getSubshapes(theInfo, theType) { return []; }
    /** Format shape info. */
    static format(theInfo, theUMgr = null) {
        if (theInfo.shapeType === ShapeEnum.SHAPE) {
            return 'NULL';
        }
        let anInfo = '';
        anInfo += ShapeEnum[theInfo.shapeType];
        if (theInfo.index !== -1) {
            anInfo += ` #${theInfo.index}`;
        }
        if (theInfo.orientation !== ShapeOrientation.FORWARD) {
            anInfo += ` ${ShapeOrientation[theInfo.orientation]}`;
        }
        if (theInfo.shapeType === ShapeEnum.FACE) {
            anInfo += ` ${ShapeSurfaceType[theInfo.surfaceType]}`;
        }
        if (theInfo.shapeType === ShapeEnum.EDGE) {
            anInfo += ` ${ShapeCurveType[theInfo.curveType]}`;
        }
        if (!Vec3.isInfinite(theInfo.point)) {
            if (theUMgr) {
                anInfo += ` [${theUMgr.length.format(theInfo.point)}]`;
            }
            else {
                anInfo += ` [${theInfo.point.x} ${theInfo.point.y} ${theInfo.point.z}]`;
            }
        }
        if (theInfo.nbChildren !== 0) {
            anInfo += ` [${theInfo.nbChildren} children]`;
        }
        if (theInfo.nbTriangles !== 0) {
            anInfo += ` [${theInfo.nbTriangles} triangles, ${theInfo.nbTriNodes} nodes]`;
        }
        if (theInfo.nbPolySegments !== 0) {
            anInfo += ` [${theInfo.nbPolySegments} polysegments]`;
        }
        if (theInfo.color !== '') {
            anInfo += ` [color: ${theInfo.color}]`;
        }
        let aFlags = ShapeInfo.formatFlags(theInfo);
        if (aFlags !== '') {
            anInfo += ` [${aFlags.substring(1)}]`;
        }
        if (!Trsf.isIdentity(theInfo.location)) {
            anInfo += '\n';
            anInfo += Trsf.format(theInfo.location, theUMgr);
        }
        return anInfo;
    }
    /** Format shape flags. */
    static formatFlags(theInfo) {
        let aFlags = '';
        if (theInfo.free === true) {
            aFlags += '|free';
        }
        if (theInfo.locked === true) {
            aFlags += '|locked';
        }
        if (theInfo.modified === true) {
            aFlags += '|modified';
        }
        if (theInfo.checked === true) {
            aFlags += '|checked';
        }
        if (theInfo.orientable === true) {
            aFlags += '|orientable';
        }
        if (theInfo.closed === true) {
            aFlags += '|closed';
        }
        if (theInfo.infinite === true) {
            aFlags += '|infinite';
        }
        if (theInfo.convex === true) {
            aFlags += '|convex';
        }
        if (theInfo.hidden === true) {
            aFlags += '|hidden';
        }
        return aFlags;
    }
}
;
;
/**
 * Cylinder shape construction parameters object.
 */
export class CylinderParams {
    constructor() {
        /** Coordinate system, center of bottom circle. */
        this.ax2 = Ax2.xoy();
        /** Cylinder radius. */
        this.radius = 0.0;
        /** Cylinder height. */
        this.height = 0.0;
        /** Cylinder section angle. */
        this.angle = 2.0 * Math.PI;
    }
}
;
;
/**
 * Cone shape construction parameters object.
 */
export class ConeParams {
    constructor() {
        /** Coordinate system, center of bottom circle. */
        this.ax2 = Ax2.xoy();
        /** Cone bottom radius. */
        this.radius = 0.0;
        /** Cone top radius. */
        this.topRadius = 0.0;
        /** Cone height. */
        this.height = 0.0;
        /** Cone section angle. */
        this.angle = 2.0 * Math.PI;
    }
}
;
/**
 * Quantity type defined by `SCPrsBase::QuantityType` enumeration.
 */
export var QuantityType;
(function (QuantityType) {
    /** unknown / dimensionless unit */
    QuantityType[QuantityType["UNKNOWN"] = -1] = "UNKNOWN";
    // 7 base units in SI
    /** Time, base SI quantity defined in seconds, s */
    QuantityType[QuantityType["Time"] = 0] = "Time";
    /** Length, base SI quantity defined in meters, m */
    QuantityType[QuantityType["Length"] = 1] = "Length";
    /** Mass, base SI quantity defined in kilograms, kg */
    QuantityType[QuantityType["Mass"] = 2] = "Mass";
    /** Electric Current, base SI quantity defined in amperes, A */
    QuantityType[QuantityType["ElectricCurrent"] = 3] = "ElectricCurrent";
    /** Thermodynamic temperature, base SI quantity defined in kelvins, K */
    QuantityType[QuantityType["ThermodynamicTemperature"] = 4] = "ThermodynamicTemperature";
    /** Amount of Substance, base SI quantity defined in moles, mol */
    QuantityType[QuantityType["AmountOfSubstance"] = 5] = "AmountOfSubstance";
    /** Luminous Intensity, base SI quantity defined in candelas, cd */
    QuantityType[QuantityType["LuminousIntensity"] = 6] = "LuminousIntensity";
    // derived quantities
    /** Plane Angle, derived quantity defined in radians, rad */
    QuantityType[QuantityType["PlaneAngle"] = 7] = "PlaneAngle";
    /** Area, derived quantity defined in squared meters, m^2 */
    QuantityType[QuantityType["Area"] = 8] = "Area";
    /** Volume, derived quantity defined in cubic meters, m^3 */
    QuantityType[QuantityType["Volume"] = 9] = "Volume";
    /** Speed, derived quantity defined in m/s */
    QuantityType[QuantityType["Speed"] = 10] = "Speed";
    /** Force, derived quantity defined in newtons, N */
    QuantityType[QuantityType["Force"] = 11] = "Force";
})(QuantityType || (QuantityType = {}));
;
/**
 * Define common length units.
 */
export var LengthUnit;
(function (LengthUnit) {
    LengthUnit["Micron"] = "\u00B5m";
    LengthUnit["Millimeter"] = "mm";
    LengthUnit["Meter"] = "m";
    LengthUnit["Centimeter"] = "cm";
    LengthUnit["Kilometer"] = "km";
    LengthUnit["Microinch"] = "\u00B5in";
    LengthUnit["Inch"] = "in";
    LengthUnit["Foot"] = "ft";
    LengthUnit["Mile"] = "stat.mile";
})(LengthUnit || (LengthUnit = {}));
;
/**
 * @internal Unit conversion section defined by `SCPrsBase::UnitSection` enumeration.
 */
var UnitSection;
(function (UnitSection) {
    UnitSection[UnitSection["SI"] = 0] = "SI";
    UnitSection[UnitSection["System"] = 1] = "System";
    UnitSection[UnitSection["Display"] = 2] = "Display";
})(UnitSection || (UnitSection = {}));
;
;
;
/**
 * Auxiliary tool to initialize JS extensions to UnitManager C++ class.
 * @internal
 */
export class UnitManagerExtensions {
    static init(theMgr) {
        Object.defineProperty(theMgr, 'length', {
            get: function () { return new Quantity(theMgr, QuantityType.Length); },
        });
        Object.defineProperty(theMgr, 'area', {
            get: function () { return new Quantity(theMgr, QuantityType.Area); },
        });
        Object.defineProperty(theMgr, 'volume', {
            get: function () { return new Quantity(theMgr, QuantityType.Volume); },
        });
        Object.defineProperty(theMgr, 'angle', {
            get: function () { return new Quantity(theMgr, QuantityType.PlaneAngle); },
        });
        Object.defineProperty(theMgr, 'quantities', {
            get: function () {
                let anArray = new Array(QuantityType.Force + 1);
                for (let aTypeIter = 0; aTypeIter < anArray.length; ++aTypeIter) {
                    anArray[aTypeIter] = new Quantity(theMgr, aTypeIter);
                }
                return anArray;
            },
        });
        // expand Trsf prototype with useful static methods
        Object.defineProperty(Trsf, 'multiply', {
            value: function (theLoc1, theLoc2) { return theMgr.trsfMultiply(theLoc1, theLoc2); },
        });
        Object.defineProperty(Trsf, 'invert', {
            value: function (theLoc) { return theMgr.trsfInvert(theLoc); },
        });
        Object.defineProperty(Trsf, 'getMat4', {
            value: function (theLoc) { return theMgr.trsfGetMat4(theLoc); },
        });
        Object.defineProperty(Trsf, 'getEulerAngles', {
            value: function (theLoc, theSeq = EulerSequence.EulerAngles) { return theMgr.trsfGetEulerAngles(theLoc, theSeq); },
        });
        Object.defineProperty(Trsf, 'setEulerAngles', {
            value: function (theLoc, theAngles, theSeq = EulerSequence.EulerAngles) { return theMgr.trsfSetEulerAngles(theLoc, theAngles, theSeq); },
        });
        Object.defineProperty(Trsf, 'rotateAroundAxis', {
            value: function (theDir, theAngle, theOrigin = { x: 0, y: 0, z: 0 }) { return theMgr.trsfRotateAroundAxis(theDir, theAngle, theOrigin); },
        });
        Object.defineProperty(Trsf, 'transformPnt', {
            value: function (theLoc, thePnt) { return theMgr.trsfTransformPnt(theLoc, thePnt); },
        });
        Object.defineProperty(Trsf, 'transformDir', {
            value: function (theLoc, theDir) { return theMgr.trsfTransformDir(theLoc, theDir); },
        });
    }
}
;
/**
 * Defines quantity units convertion tool.
 */
export class Quantity {
    /** Main constructor. */
    constructor(theMgr, theType) {
        this.m_unitMgr = theMgr;
        this.m_type = theType;
    }
    /** Return Quantity type */
    get type() { return this.m_type; }
    /** Return Quantity name */
    get name() { return this.m_unitMgr.getQuantityName(this.m_type); }
    /** Return quantity SI units */
    get unitSI() { return this.m_unitMgr.getQuantityUnit(this.m_type, UnitSection.SI); }
    /** Return quantity system units */
    get unitSystem() { return this.m_unitMgr.getQuantityUnit(this.m_type, UnitSection.System); }
    /** Return quantity display units */
    get unitDisplay() { return this.m_unitMgr.getQuantityUnit(this.m_type, UnitSection.Display); }
    /** Set quantity system units */
    set unitSystem(theUnit) { this.m_unitMgr.setQuantityUnit(this.m_type, UnitSection.System, theUnit); }
    /** Set quantity display units */
    set unitDisplay(theUnit) { this.m_unitMgr.setQuantityUnit(this.m_type, UnitSection.Display, theUnit); }
    /** Convert value from one unit to another unit. */
    convert(theVal, theFromUnit, theToUnit) {
        return this.m_unitMgr.convert(theVal, theFromUnit, theToUnit);
    }
    /** Convert value from SI unit to another unit. */
    convertAnyFromSI(theVal, theToUnit) {
        return this.m_unitMgr.convertAnyFromSI(theVal, theToUnit);
    }
    /** Convert value to SI unit from another unit. */
    convertAnyToSI(theVal, theFromUnit) {
        return this.m_unitMgr.convertAnyToSI(theVal, theFromUnit);
    }
    /** Convert value from system units into display units. */
    systemToDisplay(theSysVal) { return this.m_unitMgr.systemToDisplay(this.m_type, theSysVal); }
    /** Convert value from system units into display units. */
    systemToDisplayVec3(theSysVal) {
        return { x: this.systemToDisplay(theSysVal.x),
            y: this.systemToDisplay(theSysVal.y),
            z: this.systemToDisplay(theSysVal.z) };
    }
    /** Convert value from display units into system units. */
    systemFromDisplay(theDispVal) { return this.m_unitMgr.systemFromDisplay(this.m_type, theDispVal); }
    /** Convert value from display units into system units. */
    systemFromDisplayVec3(theDispVal) {
        return { x: this.systemFromDisplay(theDispVal.x),
            y: this.systemFromDisplay(theDispVal.y),
            z: this.systemFromDisplay(theDispVal.z) };
    }
    /** Convert value from system units into SI units. */
    systemToSI(theSysVal) { return this.m_unitMgr.systemToSI(this.m_type, theSysVal); }
    /** Convert value from system units into SI units. */
    systemToSIVec3(theSysVal) {
        return { x: this.systemToSI(theSysVal.x),
            y: this.systemToSI(theSysVal.y),
            z: this.systemToSI(theSysVal.z) };
    }
    /** Convert value from SI units into system units. */
    systemFromSI(theSIVal) { return this.m_unitMgr.systemFromSI(this.m_type, theSIVal); }
    /** Convert value from SI units into system units. */
    systemFromSIVec3(theSIVal) {
        return { x: this.systemFromSI(theSIVal.x),
            y: this.systemFromSI(theSIVal.y),
            z: this.systemFromSI(theSIVal.z) };
    }
    /** Auxiliary tool converting real number from System to Display units and appending units. */
    formatReal(theSys) {
        if (Vec3.isInfinite(theSys)) {
            return theSys >= 0.0 ? '+inf' : '-inf';
        }
        const aVal = this.systemToDisplay(theSys);
        return `${aVal} ${this.unitDisplay}`;
    }
    /** Auxiliary tool converting real number from System to Display units and appending units. */
    formatVec3(theSys) {
        return `${this.formatReal(theSys.x)}, ${this.formatReal(theSys.y)}, ${this.formatReal(theSys.z)}`;
    }
    /** Auxiliary tool converting real number from System to Display units and appending units. */
    format(theSys) {
        if (typeof (theSys) === 'number') {
            return this.formatReal(theSys);
        }
        return this.formatVec3(theSys);
    }
}
;
//# sourceMappingURL=CommonTools.js.map