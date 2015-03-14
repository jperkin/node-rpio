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

var binding = require('bindings')('shim');
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
 * Set up GPIO layout based on board revision.
 */
var layout;
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
		layout = "v1";
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
		layout = "v2";
		break;
	case 0x10:
	case 0x1041:
		layout = "b+";
		break;
	default:
		throw "Unable to determine board revision";
		break;
	}
}
setup_board();


/*
 * Default pin mode is 'gpio'.  Other option is 'physical'
 */
var pinmode = 'gpio';

/*
 * Valid GPIO pins, using GPIOxx BCM numbering.
 */
var validgpio = {
	'v1': [
		0, 1, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 21, 22, 23, 24, 25
	],
	'v2': [
		2, 3, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 22, 23, 24, 25, 27
	],
	'b+': [
		2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
		19, 20, 21, 22, 23, 24, 25, 26, 27
	]
}
/*
 * Map physical pin to BCM GPIOxx numbering.
 */
var pinmap = {
	'v1': {
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
	'v2': {
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
	'b+': {
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
	switch (pinmode) {
	case 'physical':
		if (!(pin in pinmap[layout]))
			throw "Invalid pin";
		return pinmap[layout][pin];
	case 'gpio':
		if (validgpio[layout].indexOf(pin) === -1)
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

function set_pin_pwm(pin, cb)
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

	if (binding.pin_function(gpiopin, fsel) !== 0)
		throw "Unable to configure pin" + pin + " for PWM";

	/*
	 * For now we assume mark-space mode, balanced is unsupported.
	 */
	if (binding.pwm_set_mode(channel, 1, 1) !== 0)
		throw "Unable to set PWM mode for pin" + pin;

	if (cb)
		return cb();
}

/*
 * Open the bcm2835 driver.
 */
binding.bcm_init();

/*
 * API
 */
rpio.prototype.setMode = function(mode, cb)
{
	switch (mode) {
	case 'gpio':
	case 'physical':
		pinmode = mode;
		break;
	default:
		throw "Unknown GPIO mode";
	}

	if (cb)
		return cb();
}

/*
 * Legacy function select functions.
 */
rpio.prototype.setInput = function(pin, cb)
{
	var gpiopin = pin_to_gpio(pin);

	check_sys_gpio(gpiopin);

	if (binding.pin_function(gpiopin, rpio.prototype.INPUT) !== 0)
		throw "Unable to configure pin" + pin + " for input";

	if (cb)
		return cb();
}

rpio.prototype.setOutput = function(pin, cb)
{
	var gpiopin = pin_to_gpio(pin);

	check_sys_gpio(gpiopin);

	if (binding.pin_function(gpiopin, rpio.prototype.OUTPUT) !== 0)
		throw "Unable to configure pin" + pin + " for output";

	if (cb)
		return cb();
}

/*
 * Current function select entry point.
 */
rpio.prototype.setFunction = function(pin, fsel, cb)
{
	switch (fsel) {
	case rpio.prototype.INPUT:
		return rpio.prototype.setInput(pin, cb);
	case rpio.prototype.OUTPUT:
		return rpio.prototype.setOutput(pin, cb);
	case rpio.prototype.PWM:
		return set_pin_pwm(pin, cb);
	default:
		throw "Unsupported function select " + fsel;
	}
}

rpio.prototype.read = function(pin)
{
	return binding.pin_read(pin_to_gpio(pin));
}

rpio.prototype.write = function(pin, value)
{
	binding.pin_write(pin_to_gpio(pin), parseInt(value));
}

rpio.prototype.pwmSetClockDivider = function (divider)
{
	if (divider !== 0 && (divider & (divider - 1)) !== 0)
		throw "Clock divider must be zero or power of two";

	return binding.pwm_set_clock(divider);
}

rpio.prototype.pwmSetData = function (pin, data)
{
	var channel = get_pwm_channel(pin);

	return binding.pwm_set_data(channel, data);
}

rpio.prototype.pwmSetRange = function (pin, range)
{
	var channel = get_pwm_channel(pin);

	return binding.pwm_set_range(channel, range);
}

/*
 * i2c
 */
rpio.prototype.i2cBegin = function(cb)
{
	binding.i2c_begin();

	if (cb)
		return cb();
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

rpio.prototype.i2cRead = function(len)
{
	return binding.i2c_read(len);
}

rpio.prototype.i2cWrite = function(data, len)
{
	return binding.i2c_write(data, len);
}

rpio.prototype.i2cEnd = function(cb)
{
	binding.i2c_end();

	if (cb)
		return cb();
}

/*
 * SPI
 */
rpio.prototype.spiBegin = function(cb)
{
	binding.spi_begin();

	if (cb)
		return cb();
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

rpio.prototype.spiTransfer = function(data, len)
{
	return binding.spi_transfer(data, len);
}

rpio.prototype.spiEnd = function(cb)
{
	binding.spi_end();

	if (cb)
		return cb();
}

process.on('exit', function (code) {
	binding.bcm_close();
});

module.exports = new rpio;
