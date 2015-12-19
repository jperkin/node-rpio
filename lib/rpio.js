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
	case 0x1041:
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
var layout = 'physical';

/*
 * Valid GPIO pins, using GPIOxx BCM numbering.
 */
var validgpio = {
	'v1rev1': [
		0, 1, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 21, 22, 23, 24, 25
	],
	'v1rev2': [
		2, 3, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 22, 23, 24, 25, 27
	],
	'v2plus': [
		2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
		19, 20, 21, 22, 23, 24, 25, 26, 27
	]
}
/*
 * Map physical pin to BCM GPIOxx numbering.
 */
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
	switch (layout) {
	case 'physical':
		if (!(pin in pinmap[gpiomap]))
			throw "Invalid pin";
		return pinmap[gpiomap][pin];
	case 'gpio':
		if (validgpio[gpiomap].indexOf(pin) === -1)
			throw "Invalid pin";
		return pin;
	default:
		throw "Unsupported GPIO mode";
	}
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
		throw "Unknown PWM function for pin" + pin;
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
		throw "Unknown PWM channel for pin" + pin;
	}
}

function set_pin_pwm(pin)
{
	var gpiopin = pin_to_gpio(pin);
	var channel, altfunc;

	check_sys_gpio(gpiopin);

	/*
	 * PWM channels and alternate functions differ from pin to pin, set
	 * them up appropriately based on selected pin.
	 */
	channel = get_pwm_channel(pin);
	fsel = get_pwm_function(pin);

	binding.gpio_function(gpiopin, fsel);

	/*
	 * For now we assume mark-space mode, balanced is unsupported.
	 */
	binding.pwm_set_mode(channel, 1, 1);
}

/*
 * Open the bcm2835 driver.
 */
binding.rpio_init();

/*
 * API
 */
rpio.prototype.setMode = function(mode)
{
	throw "setMode is deprecated (setLayout), 'physical' is the new default";
}

rpio.prototype.setLayout = function(arg)
{
	switch (arg) {
	case 'gpio':
	case 'physical':
		layout = arg;
		break;
	default:
		throw "Unknown GPIO layout";
	}
}

/*
 * Legacy function select functions.
 */
rpio.prototype.setInput = function(pin)
{
	var gpiopin = pin_to_gpio(pin);

	check_sys_gpio(gpiopin);

	binding.gpio_function(gpiopin, rpio.prototype.INPUT);
}

rpio.prototype.setOutput = function(pin, init)
{
	var gpiopin = pin_to_gpio(pin);

	check_sys_gpio(gpiopin);

	if (init !== undefined)
		binding.gpio_write(gpiopin, parseInt(init));

	binding.gpio_function(gpiopin, rpio.prototype.OUTPUT);
}

/*
 * Current function select entry point.
 */
rpio.prototype.setFunction = function(pin, fsel)
{
	switch (fsel) {
	case rpio.prototype.INPUT:
		return rpio.prototype.setInput(pin);
	case rpio.prototype.OUTPUT:
		return rpio.prototype.setOutput(pin);
	case rpio.prototype.PWM:
		return set_pin_pwm(pin);
	default:
		throw "Unsupported function select " + fsel;
	}
}

rpio.prototype.read = function(pin)
{
	return binding.gpio_read(pin_to_gpio(pin));
}

rpio.prototype.write = function(pin, value)
{
	return binding.gpio_write(pin_to_gpio(pin), parseInt(value));
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
 * i2c
 */
rpio.prototype.i2cBegin = function()
{
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
	if (divider !== 0 && (divider & (divider - 1)) !== 0)
		throw "Clock divider must be zero or power of two";

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
