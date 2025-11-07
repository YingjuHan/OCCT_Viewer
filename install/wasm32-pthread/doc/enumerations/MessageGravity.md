[**OCCTViewer**](../README.md)

***

[OCCTViewer](../README.md) / MessageGravity

# Enumeration: MessageGravity

This enumeration defines gravity level of messages.
It reflects values of corresponding C++ `Message_Gravity` enumeration defined by OCCT,
and used to redirect message coming from C++ algorithms via [SCwebviewer.printMessage](../classes/SCwebviewer.md#printmessage) callback.
See also [SCwebviewer.traceLevel](../classes/SCwebviewer.md#tracelevel) defining which messages should be delivered.

## Enumeration Members

### Alarm

> **Alarm**: `3`

Non-critical error.

***

### Fail

> **Fail**: `4`

Fatal error.

***

### Info

> **Info**: `1`

Informative message.

***

### Trace

> **Trace**: `0`

Low-level details on algorithm execution (usually for debug purposes).

***

### Warning

> **Warning**: `2`

Warning message.
