node-rpio
=========

This is a node.js add-on which wraps around Mike McCauley's
[bcm2835](http://www.open.com.au/mikem/bcm2835/) library to allow `mmap`
access via `/dev/mem` to the GPIO pins.

If you want something more featureful then for now you should check out James
Barwell's [rpio-gpio.js](https://github.com/JamesBarwell/rpi-gpio.js).  The
reason for me writing this module instead of using James' is that his uses the
`/sys` interface which is too slow for my requirements.

## Install

Easily install the latest via npm:

```bash
$ npm install rpio
```

## API

```js
var rpio = require('rpio')

// Enable a pin and mark as read-only
rpio.setInput(11)

// Enable a pin and mark as read-write
rpio.setOutput(12)

// Read value of pin
console.log('pin 11 is set to ' + rpio.read(11))

// Set pin high (i.e. write '1' to it)
rpio.write(12, rpio.HIGH)

// Set pin low (i.e. write '0' to it)
rpio.write(12, rpio.LOW)
```

## Simple demo

```js
/*
 * Simple demo to flash pin 11 at 100Hz (assuming it's routed to an LED).
 */
var rpio = require('rpio')

// Set the pin for write mode
rpio.setOutput(11)

/*
 * Set the pin high every 10ms, and low 5ms after each transition to high.
 */
setInterval(function() {

  rpio.write(11, rpio.HIGH)

  setTimeout(function() {
    rpio.write(11, rpio.LOW)
  }, 5)

}, 10)
```

## Authors and licenses

Mike McCauley wrote src/bcm2835.{cc,h} which are under the GPL.

I wrote the rest, which is under the ISC license unless otherwise specified.
