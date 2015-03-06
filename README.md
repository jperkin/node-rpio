node-rpio
=========

This is a super-fast nodejs addon which wraps around Mike McCauley's
[bcm2835](http://www.open.com.au/mikem/bcm2835/) library to allow `mmap`
access via `/dev/mem` to the GPIO pins.

Most other GPIO modules use the slower `/sys` file system interface.  You
should find this module significantly faster than the alternatives.

## Install

Easily install the latest via npm:

```bash
$ npm install rpio
```

## API

```js
var rpio = require('rpio');

/*
 * By default the 'gpio' layout is used, i.e. GPIOxx, which maps directly to
 * the underlying pin identifiers used by the bcm2835 chip.  If you prefer,
 * you can refer to pins by their physical header location instead by using:
 *
 *    rpio.setMode('physical').
 */
rpio.setMode('gpio');

// Enable GPIO17 (physical pin 11) and set as read-only
rpio.setInput(17);

// Enable GPIO18 (physical pin 12) and set as read-write
rpio.setOutput(18);

// Read value of GPIO17
console.log('GPIO17 is set to ' + rpio.read(17));

// Set GPIO18 high (i.e. write '1' to it)
rpio.write(18, rpio.HIGH);

// Set GPIO18 low (i.e. write '0' to it)
rpio.write(18, rpio.LOW);
```

## Simple demo

```js
/*
 * Simple demo to flash pin 11 at 100Hz (assuming it's routed to an LED).
 */
var rpio = require('rpio');

// Use physical layout for this one.
rpio.setMode('physical');

// Set physical pin 11 / GPIO17 for write mode
rpio.setOutput(11);

/*
 * Set the pin high every 10ms, and low 5ms after each transition to high.
 */
setInterval(function() {

	rpio.write(11, rpio.HIGH);

	setTimeout(function() {
		rpio.write(11, rpio.LOW);
	}, 5);

}, 10);
```

## Authors and licenses

Mike McCauley wrote src/bcm2835.{cc,h} which are under the GPL.

I wrote the rest, which is under the ISC license unless otherwise specified.
