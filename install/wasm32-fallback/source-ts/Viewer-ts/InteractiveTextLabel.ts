// Created by: Kirill Gavrilov
// Copyright (c) 2023 Huawei Technologies Co., Ltd.

import type {IManager, ObjectId, IVec3} from './CommonTools.js';
import {Vec3} from './CommonTools.js';
import {InteractiveObject, TypeOfMarker} from './InteractiveObject.js';

import type {SCwebviewer} from './SCwebviewer.js';

/**
 * @internal Parameters interface for text label object.
 */
export interface ITextLabelParams {
  /** Text to be displayed. */
  text: string;
  /** Icon to be displayed before text. */
  icon: string;
  /** 3D point to display label at. */
  position: IVec3;
  /** 3D point to attach label. */
  attachPoint: IVec3;
  /** Flag to draw 3D attach point as a marker. */
  attachMarker: TypeOfMarker;
  /** Attach marker scale. */
  attachMarkerScale: number;
  /** 2d offset from 3D point to text label. */
  pointPadding: number;
  /** 2d padding for rectangle around text label. */
  rectanglePadding: number;
  /** Flag to draw rectangle around text. */
  toDrawRectangle: boolean;
};

/**
 * Represents Text Label interactive object.
 */
export class InteractiveTextLabel extends InteractiveObject {
  /** Empty constructor. */
  constructor(theViewer: SCwebviewer | null, theObjId: ObjectId = '') {
    super(theObjId);
    if (theViewer != null) {
      this._myViewer = theViewer;
      if (this._myObjId === '') {
        this._myObjId = ':::InteractiveTextLabel';
      }
      this.textLabelMgr?.createTextLabel(this._myObjId);
      this.objMgr?.addInteractiveObject(this);
    }
  }

  /** Text to be displayed. */
  public get text(): string { return this.params.text; }

  /** Icon to be displayed. */
  public get icon(): string { return this.params.icon; }

  /** 3D point to display label at. */
  public get position(): IVec3 { return this.params.position; }

  /** Return TRUE if 3D attachment point is defined. */
  public get hasAttachPoint(): boolean { return !Vec3.isInfinite(this.params.attachPoint); }

  /** 3D point to attach label. */
  public get attachPoint(): IVec3 { return this.params.attachPoint; }

  /** 2d offset from 3D point to text label. */
  public get pointPadding(): number { return this.params.pointPadding; }

  /** 2d padding for rectangle around text label. */
  public get rectanglePadding(): number { return this.params.rectanglePadding; }

  /** Flag to draw rectangle around text. */
  public get toDrawRectangle(): boolean { return this.params.toDrawRectangle; }

  /** Flag to draw 3D attach point as a marker. */
  public get attachMarker(): TypeOfMarker { return this.params.attachMarker; }

  /** 3D attach point marker scale. */
  public get attachMarkerScale(): number { return this.params.attachMarkerScale; }

  /** @internal Return base parameters. */
  public get params(): ITextLabelParams {
    if (!this.textLabelMgr) {
      return { text: '', icon: '',
               position: {x: 0, y: 0, z: 0}, attachPoint: Vec3.invalid(),
               pointPadding: 0, rectanglePadding: 0, toDrawRectangle: false,
               attachMarker: TypeOfMarker.EMPTY, attachMarkerScale: 1};
    }
    return this.textLabelMgr.getLabelParams(this._myObjId);
  }

  /** @internal Shortcut to text label context. */
  protected get textLabelMgr(): InteractiveTextLabelManager | null { return this._myViewer?.interactiveTextLabelManager ?? null; }

  /** Text to be displayed. */
  public set text(theText: string) {
    let aParams: ITextLabelParams = this.params;
    aParams.text = theText;
    this.params = aParams;
  }

  /** Icon to be displayed. */
  public set icon(theIcon: string) {
    let aParams: ITextLabelParams = this.params;
    aParams.icon = theIcon;
    this.params = aParams;
  }

  /** 3D point to display label at. */
  public set position(thePnt: IVec3) {
    let aParams: ITextLabelParams = this.params;
    aParams.position = thePnt;
    this.params = aParams;
  }

  /** Set if 3D attachment point is defined or not. */
  public set hasAttachPoint(theToDefine: boolean) {
    let aParams: ITextLabelParams = this.params;
    if (Vec3.isInfinite(this.params.attachPoint) === theToDefine) {
      return;
    }
    aParams.attachPoint = theToDefine ? aParams.position : Vec3.invalid();
    this.params = aParams;
  }

  /** 3D point to attach label. */
  public set attachPoint(thePnt: IVec3) {
    let aParams: ITextLabelParams = this.params;
    aParams.attachPoint = thePnt;
    this.params = aParams;
  }

  /** 2d offset from 3D point to text label. */
  public set pointPadding(thePad: number) {
    let aParams: ITextLabelParams = this.params;
    aParams.pointPadding = thePad;
    this.params = aParams;
  }

  /** 2d padding for rectangle around text label. */
  public set rectanglePadding(thePad: number) {
    let aParams: ITextLabelParams = this.params;
    aParams.rectanglePadding = thePad;
    this.params = aParams;
  }

  /** Flag to draw rectangle around text. */
  public set toDrawRectangle(theToDraw: boolean) {
    let aParams: ITextLabelParams = this.params;
    aParams.toDrawRectangle = theToDraw;
    this.params = aParams;
  }

  /** Flag to draw 3D attach point as a marker. */
  public set attachMarker(theMarker: TypeOfMarker) {
    let aParams: ITextLabelParams = this.params;
    aParams.attachMarker = theMarker;
    this.params = aParams;
  }

  /** 3D attach point marker scale. */
  public set attachMarkerScale(theScale: number) {
    let aParams: ITextLabelParams = this.params;
    aParams.attachMarkerScale = theScale;
    this.params = aParams;
  }

  /** @internal Set base parameters. */
  public set params(theParams: ITextLabelParams) { this.textLabelMgr?.setLabelParams(this._myObjId, theParams); }

  /** Make a copy of existing object with new id */
  public clone(theObjId: ObjectId = ''): InteractiveTextLabel | null {
    let anObjId: ObjectId = this.objMgr ? this.objMgr.clone(this._myObjId, theObjId) : '';
    if (!this._myViewer || !this.objMgr || anObjId === '') { return null; }
    let aCopy = new InteractiveTextLabel(null);
    aCopy.wrap(this._myViewer, anObjId);
    this.objMgr?.addInteractiveObject(aCopy);
    return aCopy;
  }
};

/**
 * @internal Represents Context for Interactive text label.
 * Note that there is no reason to use class directly at application level
 * as {@link InteractiveTextLabel} re-exports main properties and hides some implementation details.
 * See also {@link SCwebviewer.interactiveTextLabelManager} property.
 */
export interface InteractiveTextLabelManager extends IManager {
// C++ methods

  /** @internal Create interactive scale ruler object. */
  createTextLabel: (theObjId: ObjectId) => void;

  /** @internal Return text label base parameters. */
  getLabelParams: (theObjId: ObjectId) => ITextLabelParams;

  /** @internal Set text label base parameters. */
  setLabelParams: (theObjId: ObjectId, theParams: ITextLabelParams) => void;
};
