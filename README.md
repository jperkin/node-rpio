node-rpio
=========

This is a node add-on which wraps around Mike McCauley's
[bcm2835](http://www.open.com.au/mikem/bcm2835/) library to allow `mmap`'d
access to the GPIO pins.

## Status

Very basic, just enough to interface with the library and flash a pin (assuming
it is hooked up to an LED).  Much more to follow...

## Usage

```js
var rpio = require('rpio')

/*
 * Flash pin 17 at 25Hz 1000 times.
 *
 * Note that the currently the pin addressing is BCM mode, RPi will be made
 * the default soon (so this example == RPi pin 11).
 */
rpio.flash(17, 25, 1000)
```
