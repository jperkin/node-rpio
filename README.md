node-rpio
=========

This is a node.js add-on which wraps around Mike McCauley's
[bcm2835](http://www.open.com.au/mikem/bcm2835/) library to allow `mmap`
access via `/dev/mem` to the GPIO pins.

If you want something more featureful then for now you should check out James
Barwell's [rpio-gpio.js](https://github.com/JamesBarwell/rpi-gpio.js).  The
reason for me writing this module instead of using James' is that his uses the
`/sys` interface which is too slow for my requirements.

## Status

Very basic, just enough to interface with the library and flash a pin (assuming
it is hooked up to an LED) in order to verify it actually works.

Much more to follow...

## Usage

```bash
$ npm install rpio
```

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

## Authors and licenses

Mike McCauley wrote src/bcm2835.{cc,h} which are under the GPL.

I wrote the rest, which is under the ISC license unless otherwise specified.
