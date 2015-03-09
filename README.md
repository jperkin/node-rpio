node-rpio
=========

This is a super-fast nodejs addon which wraps around Mike McCauley's
[bcm2835](http://www.open.com.au/mikem/bcm2835/) library to allow `mmap`
access via `/dev/mem` to the GPIO pins.

Most other GPIO modules use the slower `/sys` file system interface.  You
should find this module significantly faster than the alternatives.  The only
drawback is that root access is required to open `/dev/mem`.

This module also includes support for SPI.

## Install

Easily install the latest via npm:

```bash
$ npm install rpio
```

## API

Requiring the addon initializes the `bcm2835` driver ready to accept commands.

```js
var rpio = require('rpio');
```

### GPIO

By default the `gpio` layout is used, i.e. `GPIOxx`, which maps directly to the
underlying pin identifiers used by the `bcm2835` chip.  If you prefer, you can
refer to pins by their physical header location instead.

```js
rpio.setMode('physical);   /* Use the physical P1-P26/P40 layout */
rpio.setMode('gpio');      /* The default GPIOxx numbering system */
```

Before reading from / writing to a pin you need to configure it as read-only or
read-write.

```js
rpio.setInput(17);         /* Configure GPIO17 as read-only */
rpio.setOutput(18);        /* Configure GPIO18 as read-write */
```

Now you can read or write values with `.read()` and `.write()`.  The two
`rpio.LOW` and `rpio.HIGH` constants are provided if you prefer to avoid magic
numbers.

```js
/* Read value of GPIO17 */
console.log('GPIO17 is set to ' + rpio.read(17));

/* Set GPIO18 high (i.e. write '1' to it) */
rpio.write(18, rpio.HIGH);

/* Set GPIO18 low (i.e. write '0' to it) */
rpio.write(18, rpio.LOW);
```

### SPI

SPI switches pins GPIO7-GPIO11 to a special mode where you can bulk transfer
data at high speeds to and from SPI devices, with the controller handling the
chip enable, clock and data in/out functions.

Once SPI is enabled, the SPI pins are unavailable for GPIO use until `spiEnd()`
is called.

```js
rpio.spiBegin();           /* Switch GPIO7-GPIO11 to SPI mode */
```

Choose which of the chip select / chip enable pins to control:

Value | Pin
------|----
0     | SPI_CE0 (GPIO8)
1     | SPI_CE1 (GPIO7)
2     | Both

```js
rpio.spiChipSelect(0);
```

Commonly chip enable (CE) pins are active low, and this is the default.  If
your device's CE pin is active high, use `spiSetCSPolarity()` to change the
polarity.

```js
rpio.spiSetCSPolarity(0, rpio.HIGH);    /* Set CE0 high to activate */
```

Set the SPI clock speed with `spiSetClockDivider()`.  This is a power-of-two
divisor of the base 250MHz rate, defaulting to `0 == 65536 == 3.81kHz`.

```js
rpio.spiSetClockDivider(128);   /* Set SPI speed to 1.95MHz */
```

Set the SPI Data Mode:

Mode | CPOL | CPHA
:---:|:----:|:---:
  0  |  0   |  0
  1  |  0   |  1
  2  |  1   |  0
  3  |  1   |  1

```js
rpio.spiSetDataMode(0);         /* 0 is the default */
```

Once everything is set up we can transfer data.  Data is sent and received in
8-bit chunks via buffers which should be the same size.

```js
var tx = new Buffer([0x3, 0x0, 0xff, 0xff]);
var rx = new Buffer(tx.length);

rx = rpio.spiTransfer(tx, tx.length);
```

When you're finished call `.spiEnd()` to release the pins back to general
purpose use.

```js
rpio.spiEnd();
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

## Authors/Licenses

Mike McCauley wrote `src/bcm2835.{c,h}` which are under the GPL.

I wrote the rest, which is under the ISC license unless otherwise specified.
