node-rpio
=========

This is a super-fast nodejs addon which wraps around Mike McCauley's
[bcm2835](http://www.open.com.au/mikem/bcm2835/) library to allow `mmap`
access via `/dev/mem` to the GPIO pins.

Most other GPIO modules use the slower `/sys` file system interface.  You
should find this module significantly faster than the alternatives.  The only
drawback is that root access is required to open `/dev/mem`.

This module also includes support for i²c, PWM, and SPI.

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
rpio.setMode('physical');  /* Use the physical P1-P26/P40 layout */
rpio.setMode('gpio');      /* The default GPIOxx numbering system */
```

Before reading from / writing to a pin you need to configure it as read-only or
read-write.

```js
rpio.setInput(17);         /* Configure GPIO17/Pin11 as read-only */
rpio.setOutput(18);        /* Configure GPIO18/Pin12 as read-write */

/* Alternatively use the general purpose setFunction() */
rpio.setFunction(17, rpio.INPUT);
rpio.setFunction(18, rpio.OUTPUT);
```

Now you can read or write values with `.read()` and `.write()`.  The two
`rpio.LOW` and `rpio.HIGH` constants are provided if you prefer to avoid magic
numbers.

```js
/* Read value of GPIO17/Pin11 */
console.log('GPIO17/Pin11 is set to ' + rpio.read(17));

/* Set GPIO18/Pin12 high (i.e. write '1' to it) */
rpio.write(18, rpio.HIGH);

/* Set GPIO18/Pin12 low (i.e. write '0' to it) */
rpio.write(18, rpio.LOW);
```

#### GPIO Demo

The code below continuously flashes an LED connected to pin 11 at 100Hz.

```js
var rpio = require('rpio');

/* Use physical layout for this example */
rpio.setMode('physical');

/* GPIO17/Pin11 as we're in physical mode */
rpio.setOutput(11);

/* Set the pin high every 10ms, and low 5ms after each transition to high */
setInterval(function() {
        rpio.write(11, rpio.HIGH);
        setTimeout(function() {
                rpio.write(11, rpio.LOW);
        }, 5);
}, 10);
```

### i²c

i²c is primarily of use for driving LCD displays, and makes use of pins 3 and 5
(GPIO0/GPIO1 on Rev 1, GPIO2/GPIO3 on Rev 2 and newer).  The bcm2835 library
automatically detects which Raspberry Pi revision you are running, so you do
not need to worry about which i²c bus to configure.

To get started call `.i2cBegin()` which assigns pins 3 and 5 to i²c use.  Until
`.i2cEnd()` is called they won't be available for GPIO use.

```js
rpio.i2cBegin();
```

Configure the slave address.  This is between `0 - 0x7f`, and it can be helpful
to run the `i2cdetect` program to figure out where your devices are if you are
unsure.

```js
rpio.i2cSetSlaveAddress(0x20);
```

Set the baud rate.  You can do this two different ways, depending on your
preference.  Either use `.i2cSetBaudRate()` to directly set the speed in hertz,
or `.i2cSetClockDivider()` to set it based on a divisor of the base 250MHz
rate.

```js
rpio.i2cSetBaudRate(100000);    /* 100kHz */
rpio.i2cSetClockDivider(2500);  /* 250MHz / 2500 = 100kHz */
```

Read `len` bytes from the i²c slave, returning a Buffer of bytes.

```js
var buf = new Buffer(32);
buf = rpio.i2cRead(32);
```

Write `len` bytes of a buffer to the i²c slave.

```js
var buf = new Buffer([0x0b, 0x0e, 0x0e, 0x0f]);
rpio.i2cWrite(buf, buf.length);
```

Finally, turn off the i²c interface and return the pins to GPIO.

```js
rpio.i2cEnd();
```

#### i²c Demo

The code below writes two strings to a 16x2 LCD.

```js
var rpio = require('rpio');

/*
 * Magic numbers to initialise the i2c display device and write output,
 * cribbed from various python drivers.
 */
var init = new Buffer([0x03, 0x03, 0x03, 0x02, 0x28, 0x0c, 0x01, 0x06]);
var LCD_LINE1 = 0x80, LCD_LINE2 = 0xc0;
var LCD_ENABLE = 0x04, LCD_BACKLIGHT = 0x08;

/*
 * Data is written 4 bits at a time with the lower 4 bits containing the mode.
 */
function lcdwrite4(data)
{
        rpio.i2cWrite(Buffer([(data | LCD_BACKLIGHT)]), 1);
        rpio.i2cWrite(Buffer([(data | LCD_ENABLE | LCD_BACKLIGHT)]), 1);
        rpio.i2cWrite(Buffer([((data & ~LCD_ENABLE) | LCD_BACKLIGHT)]), 1);
}
function lcdwrite(data, mode)
{
        lcdwrite4(mode | (data & 0xF0));
        lcdwrite4(mode | ((data << 4) & 0xF0));
}

/*
 * Write a string to the specified LCD line.
 */
function lineout(str, addr)
{
        lcdwrite(addr, 0);

        str.split('').forEach(function (c) {
                lcdwrite(c.charCodeAt(0), 1);
        });
}

/*
 * We can now start the program, talking to the i2c LCD at address 0x27.
 */
rpio.i2cBegin();
rpio.i2cSetSlaveAddress(0x27);
rpio.i2cSetBaudRate(10000);

for (var i = 0; i < init.length; i++)
        lcdwrite(init[i], 0);

lineout("node.js i2c LCD!", LCD_LINE1);
lineout("npm install rpio", LCD_LINE2);

rpio.i2cEnd();
```

### PWM

Pulse Width Modulation (PWM) allows you to create analog output from the
digital pins.  This can be used, for example, to make an LED appear to pulse
rather than be fully off or on.

On the 26-pin variants of the Raspberry Pi only GPIO18/Pin12 is available to be
configured for PWM.  On the 40-pin variants, you can use GPIO12/Pin32,
GPIO13/Pin33, GPIO18/Pin12, or GPIO19/Pin35.

To enable a PIN for PWM, use the `rpio.PWM` argument to `setFunction()`:

```js
rpio.setFunction(18, rpio.PWM); /* GPIO18/Pin12
```

Set the PWM refresh rate with `pwmSetClockDivider()`.  This is a power-of-two
divisor of the base 19.2MHz rate, with a minimum of 4096 (4.6875kHz).

```js
rpio.pwmSetClockDivider(64);    /* Set PWM refresh rate to 300kHz */
```

Set the PWM range for a pin with `pwmSetRange()`.  This determines the maximum
pulse width.

```js
rpio.pwmSetRange(18, 1024);
```

Finally, set the PWM width for a pin with `pwmSetData()`.

```js
rpio.pwmSetData(18, 512);
```

#### PWM Demo

The code below pulses an LED 5 times before exiting.

```js
var rpio = require('rpio');

var pin = 18;           /* GPIO18/P12 */
var range = 1024;       /* LEDs can quickly hit max brightness, so only use */
var max = 128;          /*   the bottom 8th of a larger scale */
var clockdiv = 8;       /* Clock divider (PWM refresh rate), 8 == 2.4MHz */
var interval = 5;       /* setInterval timer, speed of pulses */
var times = 5;          /* How many times to pulse before exiting */

/*
 * Enable PWM on the chosen pin and set the clock and range.
 */
rpio.setFunction(pin, rpio.PWM);
rpio.pwmSetClockDivider(clockdiv);
rpio.pwmSetRange(pin, range);

/*
 * Repeatedly pulse from low to high and back again until times runs out.
 */
var direction = 1;
var data = 0;
var pulse = setInterval(function() {
        rpio.pwmSetData(pin, data);
        if (data === 0) {
                direction = 1;
                if (times-- === 0) {
                        clearInterval(pulse);
                        rpio.setFunction(pin, rpio.INPUT);
                        return;
                }
        } else if (data === max) {
                direction = -1;
        }
        data += direction;
}, interval, data, direction, times);
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

```js
/*
 *  Value | Pin
 *  ------|----
 *    0   | SPI_CE0 (GPIO8)
 *    1   | SPI_CE1 (GPIO7)
 *    2   | Both
 */
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

```js
/*
 *  Mode | CPOL | CPHA
 *  -----|------|-----
 *    0  |  0   |  0
 *    1  |  0   |  1
 *    2  |  1   |  0
 *    3  |  1   |  1
 */
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

#### SPI Demo

The code below reads the 128x8 contents of an AT93C46 serial EEPROM.

```js
var rpio = require('rpio');

rpio.spiBegin();
rpio.spiChipSelect(0);                  /* Use CE0 */
rpio.spiSetCSPolarity(0, rpio.HIGH);    /* AT93C46 chip select is active-high */
rpio.spiSetClockDivider(128);           /* AT93C46 max is 2MHz, 128 == 1.95MHz */
rpio.spiSetDataMode(0);

/*
 * There are various magic numbers below.  A quick overview:
 *
 *   tx[0] is always 0x3, the EEPROM "READ" instruction.
 *   tx[1] is set to var i which is the EEPROM address to read from.
 *   tx[2] and tx[3] can be anything, at this point we are only interested in
 *     reading the data back from the EEPROM into our rx buffer.
 *
 * Once we have the data returned in rx, we have to do some bit shifting to
 * get the result in the format we want, as the data is not 8-bit aligned.
 */
var tx = new Buffer([0x3, 0x0, 0x0, 0x0]);
var rx = new Buffer(4);
var out;
var i, j = 0;

for (i = 0; i < 128; i++, ++j) {
        tx[1] = i;
        rx = rpio.spiTransfer(tx, 4);
        out = ((rx[2] << 1) | (rx[3] >> 7));
        process.stdout.write(out.toString(16) + ((j % 16 == 0) ? "\n" : " "));
}
rpio.spiEnd();
```

## Authors And Licenses

Mike McCauley wrote `src/bcm2835.{c,h}` which are under the GPL.

I wrote the rest, which is under the ISC license unless otherwise specified.
