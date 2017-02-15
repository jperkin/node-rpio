/*
 * Copyright (c) 2015 Jonathan Perkin <jonathan@perkin.org.uk>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

var binding = require('bindings')('rpio');
var fs = require('fs');
var util = require('util');
var EventEmitter = require('events').EventEmitter;

/*
 * Event Emitter gloop.
 */
function rpio()
{
	EventEmitter.call(this);
}
util.inherits(rpio, EventEmitter);

/*
 * Constants.
 */
rpio.prototype.LOW = 0x0;
rpio.prototype.HIGH = 0x1;

/*
 * Supported function select modes.  INPUT and OUTPUT match the bcm2835
 * function select integers.  PWM is handled specially.
 */
rpio.prototype.INPUT = 0x0;
rpio.prototype.OUTPUT = 0x1;
rpio.prototype.PWM = 0x2;

/*
 * Configure builtin pullup/pulldown resistors.
 */
rpio.prototype.PULL_OFF = 0x0;
rpio.prototype.PULL_DOWN = 0x1;
rpio.prototype.PULL_UP = 0x2;

/*
 * Pin edge detect events.  Default to both.
 */
rpio.prototype.POLL_LOW = 0x1;	/* Falling edge detect */
rpio.prototype.POLL_HIGH = 0x2;	/* Rising edge detect */
rpio.prototype.POLL_BOTH = 0x3;	/* POLL_LOW | POLL_HIGH */

/*
 * Reset pin status on close (default), or preserve current status.
 */
rpio.prototype.PIN_PRESERVE = 0x0;
rpio.prototype.PIN_RESET = 0x1;

/*
 * Pin event polling.  We track which pins are being monitored, and create a
 * bitmask for efficient checks.  The event_poll function is executed in a
 * setInterval() context whenever any pins are being monitored, and emits
 * events when their EDS bit is set.
 */
var event_pins = {};
var event_mask = 0x0;
var event_running = false;

function event_poll()
{
	var active = binding.gpio_event_poll(event_mask);

	for (gpiopin in event_pins) {
		if (active & (1 << gpiopin))
			rpio.prototype.emit('pin' + gpiopin);
	}
}

/*
 * GPIO Pad Control
 */
rpio.prototype.PAD_GROUP_0_27  = 0x0;
rpio.prototype.PAD_GROUP_28_45 = 0x1;
rpio.prototype.PAD_GROUP_46_53 = 0x2;

rpio.prototype.PAD_DRIVE_2mA      = 0x00;
rpio.prototype.PAD_DRIVE_4mA      = 0x01;
rpio.prototype.PAD_DRIVE_6mA      = 0x02;
rpio.prototype.PAD_DRIVE_8mA      = 0x03;
rpio.prototype.PAD_DRIVE_10mA     = 0x04;
rpio.prototype.PAD_DRIVE_12mA     = 0x05;
rpio.prototype.PAD_DRIVE_14mA     = 0x06;
rpio.prototype.PAD_DRIVE_16mA     = 0x07;
rpio.prototype.PAD_HYSTERESIS     = 0x08;
rpio.prototype.PAD_SLEW_UNLIMITED = 0x10;

/*
 * Set up GPIO mapping based on board revision.
 */
var gpiomap;
function setup_board()
{
	var cpuinfo, boardrev, match;

	cpuinfo = fs.readFileSync('/proc/cpuinfo', 'ascii', function(err) {
		if (err)
			throw err;
	});

	cpuinfo.toString().split(/\n/).forEach(function (line) {
		match = line.match(/^Revision.*(.{4})/);
		if (match) {
			boardrev = parseInt(match[1], 16);
			return;
		}
	});

	switch (boardrev) {
	case 0x2:
	case 0x3:
		gpiomap = "v1rev1";
		break;
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
	case 0x8:
	case 0x9:
	case 0xd:
	case 0xe:
	case 0xf:
		gpiomap = "v1rev2";
		break;
	case 0x10:
	case 0x12:
	case 0x13:
	case 0x15:
	case 0x92:
	case 0x93:
	case 0xc1:
	case 0x1041:
	case 0x2042:
	case 0x2082:
		gpiomap = "v2plus";
		break;
	default:
		throw "Unable to determine board revision";
		break;
	}
}
setup_board();


/*
 * Default pin mode is 'physical'.  Other option is 'gpio'
 */
var rpio_inited = false;
var rpio_options = {
	gpiomem: true,
	mapping: 'physical',
};

/*
 * Valid GPIO pins, using GPIOxx BCM numbering.
 */
var validgpio = {
	'v1rev1': [
		0, 1, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 21, 22, 23, 24, 25
	],
	'v1rev2': [
		2, 3, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 22, 23, 24, 25, 27,
		28, 29, 30, 31
	],
	'v2plus': [
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
		18, 19, 20, 21, 22, 23, 24, 25, 26, 27
	]
}
/*
 * Map physical pin to BCM GPIOxx numbering.
 */
var pincache = {};
var pinmap = {
	'v1rev1': {
		3: 0,
		5: 1,
		7: 4,
		8: 14,
		10: 15,
		11: 17,
		12: 18,
		13: 21,
		15: 22,
		16: 23,
		18: 24,
		19: 10,
		21: 9,
		22: 25,
		23: 11,
		24: 8,
		26: 7
	},
	'v1rev2': {
		3: 2,
		5: 3,
		7: 4,
		8: 14,
		10: 15,
		11: 17,
		12: 18,
		13: 27,
		15: 22,
		16: 23,
		18: 24,
		19: 10,
		21: 9,
		22: 25,
		23: 11,
		24: 8,
		26: 7
		/* XXX: no support for the P5 header pins. */
	},
	'v2plus': {
		3: 2,
		5: 3,
		7: 4,
		8: 14,
		10: 15,
		11: 17,
		12: 18,
		13: 27,
		15: 22,
		16: 23,
		18: 24,
		19: 10,
		21: 9,
		22: 25,
		23: 11,
		24: 8,
		26: 7,
		27: 0,
		28: 1,
		29: 5,
		31: 6,
		32: 12,
		33: 13,
		35: 19,
		36: 16,
		37: 26,
		38: 20,
		40: 21
	},
}

function pin_to_gpio(pin)
{
	if (pincache[pin])
		return pincache[pin];

	switch (rpio_options.mapping) {
	case 'physical':
		if (!(pin in pinmap[gpiomap]))
			throw "Invalid pin";
		pincache[pin] = pinmap[gpiomap][pin];
		break;
	case 'gpio':
		if (validgpio[gpiomap].indexOf(pin) === -1)
			throw "Invalid pin";
		pincache[pin] = pin;
		break;
	default:
		throw "Unsupported GPIO mode";
	}

	return pincache[pin];
}

function check_sys_gpio(pin)
{
	if (fs.existsSync('/sys/class/gpio/gpio' + pin))
		throw "GPIO" + pin + " is currently in use by /sys/class/gpio";
}

function get_pwm_function(pin)
{
	var gpiopin = pin_to_gpio(pin);

	switch (gpiopin) {
	case 12:
	case 13:
		return 4; /* BCM2835_GPIO_FSEL_ALT0 */
	case 18:
	case 19:
		return 2; /* BCM2835_GPIO_FSEL_ALT5 */
	default:
		throw "Pin " + pin + " does not support hardware PWM";
	}
}

function get_pwm_channel(pin)
{
	var gpiopin = pin_to_gpio(pin);

	switch (gpiopin) {
	case 12:
	case 18:
		return 0;
	case 13:
	case 19:
		return 1;
	default:
		throw "Unknown PWM channel for pin " + pin;
	}
}

function set_pin_pwm(pin)
{
	var gpiopin = pin_to_gpio(pin);
	var channel, func;

	if (rpio_options.gpiomem)
		throw "PWM not available in gpiomem mode";

	check_sys_gpio(gpiopin);

	/*
	 * PWM channels and alternate functions differ from pin to pin, set
	 * them up appropriately based on selected pin.
	 */
	channel = get_pwm_channel(pin);
	func = get_pwm_function(pin);

	binding.gpio_function(gpiopin, func);

	/*
	 * For now we assume mark-space mode, balanced is unsupported.
	 */
	binding.pwm_set_mode(channel, 1, 1);
}

/*
 * GPIO
 */
rpio.prototype.init = function(opts)
{
	opts = opts || {};

	for (var k in rpio_options) {
		if (k in opts)
			rpio_options[k] = opts[k];
	}

	/* Invalidate the pin cache as we may have changed mapping. */
	pincache = {};

	/*
	 * Open the bcm2835 driver.
	 */
	binding.rpio_init(Number(rpio_options.gpiomem));
	rpio_inited = true;
}

rpio.prototype.open = function(pin, mode, init)
{
	var gpiopin = pin_to_gpio(pin);

	if (!rpio_inited) {
		/* PWM requires full /dev/mem */
		if (mode === rpio.prototype.PWM)
			rpio_options.gpiomem = false;
		rpio.prototype.init();
	}

	check_sys_gpio(gpiopin);

	switch (mode) {
	case rpio.prototype.INPUT:
		if (init !== undefined)
			binding.gpio_pud(gpiopin, init);
		return binding.gpio_function(gpiopin, rpio.prototype.INPUT);
	case rpio.prototype.OUTPUT:
		if (init !== undefined)
			binding.gpio_write(gpiopin, init);
		return binding.gpio_function(gpiopin, rpio.prototype.OUTPUT);
	case rpio.prototype.PWM:
		return set_pin_pwm(pin);
	default:
		throw "Unsupported mode " + mode;
	}
}

rpio.prototype.mode = function(pin, mode)
{
	var gpiopin = pin_to_gpio(pin);

	switch (mode) {
	case rpio.prototype.INPUT:
		return binding.gpio_function(gpiopin, rpio.prototype.INPUT);
	case rpio.prototype.OUTPUT:
		return binding.gpio_function(gpiopin, rpio.prototype.OUTPUT);
	case rpio.prototype.PWM:
		return set_pin_pwm(pin);
	default:
		throw "Unsupported mode " + mode;
	}
}

rpio.prototype.read = function(pin)
{
	return binding.gpio_read(pin_to_gpio(pin));
}

rpio.prototype.readbuf = function(pin, buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw "Buffer not large enough to accommodate request";

	return binding.gpio_readbuf(pin_to_gpio(pin), buf, len);
}

rpio.prototype.write = function(pin, value)
{
	return binding.gpio_write(pin_to_gpio(pin), value);
}

rpio.prototype.writebuf = function(pin, buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw "Buffer not large enough to accommodate request";

	return binding.gpio_writebuf(pin_to_gpio(pin), buf, len);
}

rpio.prototype.readpad = function(group)
{
	if (rpio_options.gpiomem)
		throw "Pad control not available in gpiomem mode";

	return binding.gpio_pad_read(group);
}

rpio.prototype.writepad = function(group, control)
{
	if (rpio_options.gpiomem)
		throw "Pad control not available in gpiomem mode";

	binding.gpio_pad_write(group, control);
}

rpio.prototype.pud = function(pin, state)
{
	binding.gpio_pud(pin_to_gpio(pin), state);
}

rpio.prototype.poll = function(pin, cb, direction)
{
	var gpiopin = pin_to_gpio(pin);

	if (direction === undefined)
		direction = rpio.prototype.POLL_BOTH;

	/*
	 * If callback is a function, set up pin for polling, otherwise
	 * clear it.
	 */
	if (typeof(cb) === 'function') {
		if (gpiopin in event_pins)
			throw "Pin " + pin + " is already listening for events."

		binding.gpio_event_set(gpiopin, direction);

		var pincb = function() {
			cb(pin);
		};
		rpio.prototype.addListener('pin' + gpiopin, pincb);

		event_pins[gpiopin] = pincb;
		event_mask |= (1 << gpiopin);

		if (!(event_running))
			event_running = setInterval(event_poll, 1);
	} else {
		if (!(gpiopin in event_pins))
			throw "Pin " + pin + " is not listening for events."

		binding.gpio_event_clear(gpiopin);

		rpio.prototype.removeListener('pin' + gpiopin, event_pins[gpiopin]);

		delete event_pins[gpiopin];
		event_mask &= ~(1 << gpiopin);

		if (Object.keys(event_pins).length === 0) {
			clearInterval(event_running);
			event_running = false;
		}
	}
}

rpio.prototype.close = function(pin, reset)
{
	var gpiopin = pin_to_gpio(pin);

	if (reset === undefined)
		reset = rpio.prototype.PIN_RESET;

	if (gpiopin in event_pins)
		rpio.prototype.poll(pin, null);

	if (reset) {
		if (!rpio_options.gpiomem)
			rpio.prototype.pud(pin, rpio.prototype.PULL_OFF);
		rpio.prototype.mode(pin, rpio.prototype.INPUT);
	}
}

/*
 * PWM
 */
rpio.prototype.pwmSetClockDivider = function (divider)
{
	if (divider !== 0 && (divider & (divider - 1)) !== 0)
		throw "Clock divider must be zero or power of two";

	return binding.pwm_set_clock(divider);
}

rpio.prototype.pwmSetRange = function (pin, range)
{
	var channel = get_pwm_channel(pin);

	return binding.pwm_set_range(channel, range);
}

rpio.prototype.pwmSetData = function (pin, data)
{
	var channel = get_pwm_channel(pin);

	return binding.pwm_set_data(channel, data);
}

/*
 * i²c
 */
rpio.prototype.i2cBegin = function()
{
	if (!rpio_inited) {
		/* i²c requires full /dev/mem */
		rpio_options.gpiomem = false;
		rpio.prototype.init();
	}

	if (rpio_options.gpiomem)
		throw "i²c not available in gpiomem mode";

	binding.i2c_begin();
}

rpio.prototype.i2cSetSlaveAddress = function(addr)
{
	return binding.i2c_set_slave_address(addr);
}

rpio.prototype.i2cSetClockDivider = function(divider)
{
	if ((divider % 2) !== 0)
		throw "Clock divider must be an even number";

	return binding.i2c_set_clock_divider(divider);
}

rpio.prototype.i2cSetBaudRate = function(baud)
{
	return binding.i2c_set_baudrate(baud);
}

rpio.prototype.i2cRead = function(buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw "Buffer not large enough to accommodate request";

	return binding.i2c_read(buf, len);
}

rpio.prototype.i2cWrite = function(buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw "Buffer not large enough to accommodate request";

	return binding.i2c_write(buf, len);
}

rpio.prototype.i2cEnd = function()
{
	binding.i2c_end();
}

/*
 * SPI
 */
rpio.prototype.spiBegin = function()
{
	if (!rpio_inited) {
		/* SPI requires full /dev/mem */
		rpio_options.gpiomem = false;
		rpio.prototype.init();
	}

	if (rpio_options.gpiomem)
		throw "SPI not available in gpiomem mode";

	binding.spi_begin();
}

rpio.prototype.spiChipSelect = function(cs)
{
	return binding.spi_chip_select(cs);
}

rpio.prototype.spiSetCSPolarity = function(cs, active)
{
	return binding.spi_set_cs_polarity(cs, active);
}

rpio.prototype.spiSetClockDivider = function(divider)
{
	if ((divider % 2) !== 0 || divider < 0 || divider > 65536)
		throw "Clock divider must be an even number between 0 and 65536";

	return binding.spi_set_clock_divider(divider);
}

rpio.prototype.spiSetDataMode = function(mode)
{
	return binding.spi_set_data_mode(mode);
}

rpio.prototype.spiTransfer = function(txbuf, rxbuf, len)
{
	return binding.spi_transfer(txbuf, rxbuf, len);
}

rpio.prototype.spiWrite = function(buf, len)
{
	return binding.spi_write(buf, len);
}

rpio.prototype.spiEnd = function()
{
	binding.spi_end();
}

/*
 * Misc functions.
 */
rpio.prototype.sleep = function(secs)
{
	binding.rpio_usleep(secs * 1000000);
}

rpio.prototype.msleep = function(msecs)
{
	binding.rpio_usleep(msecs * 1000);
}

rpio.prototype.usleep = function(usecs)
{
	binding.rpio_usleep(usecs);
}

process.on('exit', function (code) {
	binding.rpio_close();
});

module.exports = new rpio;
