[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / CaeMeshAnimation

# Class: CaeMeshAnimation

Auxiliary class for accessing InteractiveCaeMesh animation.

## Accessors

### duration

#### Get Signature

> **get** **duration**(): `number`

Return duration of mesh properties animation, in seconds, 5.0 by default.

##### Returns

`number`

#### Set Signature

> **set** **duration**(`theDur`): `void`

Set duration of mesh properties animation, in seconds.

##### Parameters

###### theDur

`number`

##### Returns

`void`

***

### isActive

#### Get Signature

> **get** **isActive**(): `boolean`

Check if mesh properties animation is running.

##### Returns

`boolean`

***

### isLoopPlayback

#### Get Signature

> **get** **isLoopPlayback**(): `boolean`

Check if animation should be restarted on the last update, true by default.

##### Returns

`boolean`

#### Set Signature

> **set** **isLoopPlayback**(`theIsLoop`): `void`

Define if animation should be restarted on the last update.

##### Parameters

###### theIsLoop

`boolean`

##### Returns

`void`

***

### isValid

#### Get Signature

> **get** **isValid**(): `boolean`

Check if mesh properties animation contains valid input data.

##### Returns

`boolean`

***

### uniformSteps

#### Get Signature

> **get** **uniformSteps**(): `boolean`

Check if animation steps duration should take into account timestamps values, false by default.

##### Returns

`boolean`

#### Set Signature

> **set** **uniformSteps**(`theIsUniform`): `void`

Define if animation steps duration should take into account timestamps values.

##### Parameters

###### theIsUniform

`boolean`

##### Returns

`void`

## Methods

### continue()

> **continue**(): `void`

Continue animation if it was paused after trying to move on the next time step
without not loaded related properties.
Should be called after all required properties have been loaded.

#### Returns

`void`

***

### start()

> **start**(`theToRestart`): `void`

Start animation of mesh properties.

#### Parameters

##### theToRestart

`boolean` = `false`

#### Returns

`void`

***

### stop()

> **stop**(): `void`

Stop animation of mesh properties.

#### Returns

`void`
