[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / IPromise

# Interface: IPromise

IPromise interface which allows to run callback written on js from c++.
For using this interface please use its implementation from [SCwebviewer.IPromise](../classes/SCwebviewer.md#ipromise).implement.

## Properties

### implement()

> **implement**: (`arg`) => `IPromise`

Create IPromise wrapper from given object with callback function.

#### Parameters

##### arg

`any`

#### Returns

`IPromise`

***

### reject()

> **reject**: (`theErr`) => `void`

Custom callback function - to be called on failure.

#### Parameters

##### theErr

`string`

#### Returns

`void`

***

### resolve()

> **resolve**: () => `void`

Custom callback function - to be called on success.

#### Returns

`void`
