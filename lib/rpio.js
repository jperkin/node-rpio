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

var binding = require('../build/Release/rpio');
var fs = require('fs');
var util = require('util');
var EventEmitter = require('events').EventEmitter;

/*
 * Supported pin functions.
 */
var funcsel = {
	INPUT: 0,
	OUTPUT: 1
}

/*
 * Event Emitter gloop.
 */
function rpio()
{
	EventEmitter.call(this);
}
util.inherits(rpio, EventEmitter);

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

function set_pin_function(pin, direction)
{
	var gpiopin = pin_to_gpio(pin);

	check_sys_gpio(gpiopin);

	if (binding.pin_function(gpiopin, direction) !== 0)
		throw "Unable to configure pin " + pin + " for "
		      + (direction === 0 ? "input" : "output");
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

rpio.prototype.setInput = function(pin, cb)
{
	set_pin_function(pin, funcsel.INPUT);

	if (cb)
		return cb();
}

rpio.prototype.setOutput = function(pin, cb)
{
	set_pin_function(pin, funcsel.OUTPUT);

	if (cb)
		return cb();
}

rpio.prototype.read = function(pin)
{
	return binding.pin_read(pin_to_gpio(pin));
}

rpio.prototype.write = function(pin, value)
{
	binding.pin_write(pin_to_gpio(pin), parseInt(value));
}

process.on('exit', function (code) {
	binding.bcm_close();
});

rpio.prototype.LOW = 0x0;
rpio.prototype.HIGH = 0x1;

module.exports = new rpio;
