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
module.exports = new rpio;

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
 * GPIO Pad Control
 */
rpio.prototype.PAD_GROUP_0_27     = 0x0;
rpio.prototype.PAD_GROUP_28_45    = 0x1;
rpio.prototype.PAD_GROUP_46_53    = 0x2;
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
 * Default pin mode is 'physical'.  Other option is 'gpio'
 */
var rpio_inited = false;
var rpio_options = {
	gpiomem: true,
	mapping: 'physical',
	mock: false,
};

/* Default mock mode if hardware is unsupported. */
var defmock = 'raspi-3';

/*
 * Wrapper functions.
 */
function bindcall(bindfunc, optarg)
{
	if (rpio_options.mock)
		return;

	return bindfunc(optarg);
}

function bindcall2(bindfunc, arg1, arg2)
{
	if (rpio_options.mock)
		return;

	return bindfunc(arg1, arg2);
}

function bindcall3(bindfunc, arg1, arg2, arg3)
{
	if (rpio_options.mock)
		return;

	return bindfunc(arg1, arg2, arg3);
}

function warn(msg)
{
	console.error('WARNING: ' + msg);
}

/*
 * Map physical pin to BCM GPIOxx numbering.  There are currently three
 * layouts:
 *
 * 	PINMAP_26_R1:	26-pin early original models A and B (PCB rev 1.0)
 * 	PINMAP_26:	26-pin standard models A and B (PCB rev 2.0)
 * 	PINMAP_40:	40-pin models
 *
 * A -1 indicates an unusable pin.  Each table starts with a -1 so that we
 * can index into the array by pin number.
 */
var pincache = {};
var pinmap = null;
var pinmaps = {
	/*
	 * Original Raspberry Pi, PCB revision 1.0
	 */
	PINMAP_26_R1: [
		-1,
		-1, -1,		/*  P1  P2 */
		 0, -1,		/*  P3  P4 */
		 1, -1,		/*  P5  P6 */
		 4, 14,		/*  P7  P8 */
		-1, 15,		/*  P9  P10 */
		17, 18,		/* P11  P12 */
		21, -1,		/* P13  P14 */
		22, 23,		/* P15  P16 */
		-1, 24,		/* P17  P18 */
		10, -1,		/* P19  P20 */
		 9, 25,		/* P21  P22 */
		11,  8,		/* P23  P24 */
		-1,  7		/* P25  P26 */
	],
	/*
	 * Original Raspberry Pi, PCB revision 2.0.
	 *
	 * Differs to R1 on pins 3, 5, and 13.
	 *
	 * XXX: no support yet for the P5 header pins.
	 */
	PINMAP_26: [
		-1,
		-1, -1,		/*  P1  P2 */
		 2, -1,		/*  P3  P4 */
		 3, -1,		/*  P5  P6 */
		 4, 14,		/*  P7  P8 */
		-1, 15,		/*  P9  P10 */
		17, 18,		/* P11  P12 */
		27, -1,		/* P13  P14 */
		22, 23,		/* P15  P16 */
		-1, 24,		/* P17  P18 */
		10, -1,		/* P19  P20 */
		 9, 25,		/* P21  P22 */
		11,  8,		/* P23  P24 */
		-1,  7		/* P25  P26 */
	],
	/*
	 * Raspberry Pi 40-pin models.
	 *
	 * First 26 pins are the same as PINMAP_26.
	 */
	PINMAP_40: [
		-1,
		-1, -1,		/*  P1  P2 */
		 2, -1,		/*  P3  P4 */
		 3, -1,		/*  P5  P6 */
		 4, 14,		/*  P7  P8 */
		-1, 15,		/*  P9  P10 */
		17, 18,		/* P11  P12 */
		27, -1,		/* P13  P14 */
		22, 23,		/* P15  P16 */
		-1, 24,		/* P17  P18 */
		10, -1,		/* P19  P20 */
		 9, 25,		/* P21  P22 */
		11,  8,		/* P23  P24 */
		-1,  7,		/* P25  P26 */
		 0,  1,		/* P27  P28 */
		 5, -1,		/* P29  P30 */
		 6, 12,		/* P31  P32 */
		13, -1,		/* P33  P34 */
		19, 16,		/* P35  P36 */
		26, 20,		/* P37  P38 */
		-1, 21		/* P39  P40 */
	]
}

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
	var active = bindcall(binding.gpio_event_poll, event_mask);

	for (gpiopin in event_pins) {
		if (active & (1 << gpiopin))
			module.exports.emit('pin' + gpiopin);
	}
}

/*
 * Fallback detection based on device-tree when /proc/cpuinfo is incomplete.
 *
 * XXX: At some point we should switch so that this is the default, need a
 * complete list of where this is supported and what the valid values are.
 */
function detect_pinmap_devicetree()
{
	var model;

	try {
		model = fs.readFileSync('/proc/device-tree/model', 'ascii');
	} catch (err) {
		return false;
	}

	if (!model)
		return false;

	model.toString().split(/\n/).forEach(function(line) {
		match = line.match(/^Raspberry Pi (.*) Model/);
		if (match) {
			boardrev = parseInt(match[1], 10);
			return;
		}
	});

	switch (boardrev) {
	case 2:
	case 3:
	case 4:
		pinmap = 'PINMAP_40';
		break;
	default:
		return false;
	}

	return true;
}

/*
 * Set up GPIO mapping based on board revision.
 */
function detect_pinmap()
{
	var cpuinfo, boardrev, match;

	try {
		cpuinfo = fs.readFileSync('/proc/cpuinfo', 'ascii');
	} catch (err) {
		return false;
	}

	if (!cpuinfo)
		return false;

	cpuinfo.toString().split(/\n/).forEach(function(line) {
		match = line.match(/^Revision.*(.{4})/);
		if (match) {
			boardrev = parseInt(match[1], 16);
			return;
		}
	});

	switch (boardrev) {
	case 0x2:
	case 0x3:
		pinmap = 'PINMAP_26_R1';
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
		pinmap = 'PINMAP_26';
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
	case 0x20a0: // Compute Module 3, needs separate pinmap?
	case 0x20d3:
	case 0x20e0: // Raspberry Pi 3 Model A+ 
	case 0x2100: // Compute Module 3
	case 0x3111: // Raspberry Pi 4 Model B
		pinmap = 'PINMAP_40';
		break;
	default:
		return detect_pinmap_devicetree();
	}

	return true;
}

function set_mock_pinmap()
{
	switch (rpio_options.mock) {
	case 'raspi-b-r1':
		pinmap = 'PINMAP_26_R1';
		break;
	case 'raspi-a':
	case 'raspi-b':
		pinmap = 'PINMAP_26';
		break;
	case 'raspi-a+':
	case 'raspi-b+':
	case 'raspi-2':
	case 'raspi-3':
	case 'raspi-4':
	case 'raspi-zero':
	case 'raspi-zero-w':
		pinmap = 'PINMAP_40';
		break;
	default:
		return false;
	}

	return true;
}

function pin_to_gpio(pin)
{
	if (pincache[pin])
		return pincache[pin];

	switch (rpio_options.mapping) {
	case 'physical':
		if (pinmaps[pinmap][pin] == -1 || pinmaps[pinmap][pin] == null)
			throw new Error('Invalid pin: ' + pin);
		pincache[pin] = pinmaps[pinmap][pin];
		break;
	case 'gpio':
		if (pinmaps[pinmap].indexOf(pin) === -1)
			throw new Error('Invalid pin: ' + pin);
		pincache[pin] = pin;
		break;
	default:
		throw new Error('Unsupported GPIO mode');
	}

	return pincache[pin];
}

function check_sys_gpio(pin)
{
	if (fs.existsSync('/sys/class/gpio/gpio' + pin))
		throw new Error('GPIO' + pin + ' is currently in use by /sys/class/gpio');
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
		throw new Error('Pin ' + pin + ' does not support hardware PWM');
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
		throw new Error('Unknown PWM channel for pin ' + pin);
	}
}

function set_pin_pwm(pin)
{
	var gpiopin = pin_to_gpio(pin);
	var channel, func;

	if (rpio_options.gpiomem)
		throw new Error('PWM not available in gpiomem mode');

	check_sys_gpio(gpiopin);

	/*
	 * PWM channels and alternate functions differ from pin to pin, set
	 * them up appropriately based on selected pin.
	 */
	channel = get_pwm_channel(pin);
	func = get_pwm_function(pin);

	bindcall2(binding.gpio_function, gpiopin, func);

	/*
	 * For now we assume mark-space mode, balanced is unsupported.
	 */
	bindcall3(binding.pwm_set_mode, channel, 1, 1);
}

/*
 * GPIO
 */

/*
 * Default warning handler, if the user registers their own then this one
 * is cancelled.
 */
function default_warn_handler(msg)
{
	if (module.exports.listenerCount('warn') > 1) {
		module.exports.removeListener('warn', default_warn_handler);
		return;
	}
	warn(msg);
}

module.exports.on('warn', default_warn_handler);

rpio.prototype.init = function(opts)
{
	opts = opts || {};

	for (var k in rpio_options) {
		if (k in opts)
			rpio_options[k] = opts[k];
	}

	/*
	 * Invalidate the pin cache and mapping as we may be in the process
	 * of changing them.
	 */
	pincache = {};
	pinmap = null;

	/*
	 * Allow the user to specify a mock board to emulate, otherwise try
	 * to autodetect the board, and fall back to mock mode if running on
	 * an unsupported platform.
	 */
	if (rpio_options.mock) {
		if (!set_mock_pinmap())
			throw new Error('Unsupported mock mode ' + rpio_options.mock);
	} else {
		if (!detect_pinmap()) {
			module.exports.emit('warn',
			    'Hardware auto-detect failed, running in ' +
			    defmock + ' mock mode');
			rpio_options.mock = defmock;
			set_mock_pinmap();
		}
	}

	/*
	 * Open the bcm2835 driver.
	 */
	bindcall(binding.rpio_init, Number(rpio_options.gpiomem));
	rpio_inited = true;
}

rpio.prototype.open = function(pin, mode, init)
{
	if (!rpio_inited) {
		/* PWM requires full /dev/mem */
		if (mode === rpio.prototype.PWM)
			rpio_options.gpiomem = false;
		rpio.prototype.init();
	}

	var gpiopin = pin_to_gpio(pin);

	check_sys_gpio(gpiopin);

	switch (mode) {
	case rpio.prototype.INPUT:
		bindcall2(binding.gpio_function, gpiopin, rpio.prototype.INPUT);
		if (init !== undefined)
			bindcall2(binding.gpio_pud, gpiopin, init);
		return;
	case rpio.prototype.OUTPUT:
		if (init !== undefined)
			bindcall2(binding.gpio_write, gpiopin, init);
		return bindcall2(binding.gpio_function, gpiopin, rpio.prototype.OUTPUT);
	case rpio.prototype.PWM:
		return set_pin_pwm(pin);
	default:
		throw new Error('Unsupported mode ' + mode);
	}
}

rpio.prototype.mode = function(pin, mode)
{
	var gpiopin = pin_to_gpio(pin);

	switch (mode) {
	case rpio.prototype.INPUT:
		return bindcall2(binding.gpio_function, gpiopin, rpio.prototype.INPUT);
	case rpio.prototype.OUTPUT:
		return bindcall2(binding.gpio_function, gpiopin, rpio.prototype.OUTPUT);
	case rpio.prototype.PWM:
		return set_pin_pwm(pin);
	default:
		throw new Error('Unsupported mode ' + mode);
	}
}

rpio.prototype.read = function(pin)
{
	return bindcall(binding.gpio_read, pin_to_gpio(pin));
}

rpio.prototype.readbuf = function(pin, buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw new Error('Buffer not large enough to accommodate request');

	return bindcall3(binding.gpio_readbuf, pin_to_gpio(pin), buf, len);
}

rpio.prototype.write = function(pin, value)
{
	return bindcall2(binding.gpio_write, pin_to_gpio(pin), value);
}

rpio.prototype.writebuf = function(pin, buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw new Error('Buffer not large enough to accommodate request');

	return bindcall3(binding.gpio_writebuf, pin_to_gpio(pin), buf, len);
}

rpio.prototype.readpad = function(group)
{
	if (rpio_options.gpiomem)
		throw new Error('Pad control not available in gpiomem mode');

	return bindcall(binding.gpio_pad_read, group);
}

rpio.prototype.writepad = function(group, control)
{
	if (rpio_options.gpiomem)
		throw new Error('Pad control not available in gpiomem mode');

	bindcall2(binding.gpio_pad_write, group, control);
}

rpio.prototype.pud = function(pin, state)
{
	bindcall2(binding.gpio_pud, pin_to_gpio(pin), state);
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
			throw new Error('Pin ' + pin + ' is already listening for events.');

		bindcall2(binding.gpio_event_set, gpiopin, direction);

		var pincb = function() {
			cb(pin);
		};
		module.exports.on('pin' + gpiopin, pincb);

		event_pins[gpiopin] = pincb;
		event_mask |= (1 << gpiopin);

		if (!(event_running))
			event_running = setInterval(event_poll, 1);
	} else {
		if (!(gpiopin in event_pins))
			throw new Error('Pin ' + pin + ' is not listening for events.');

		bindcall(binding.gpio_event_clear, gpiopin);

		module.exports.removeListener('pin' + gpiopin, event_pins[gpiopin]);

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
rpio.prototype.pwmSetClockDivider = function(divider)
{
	if (divider !== 0 && (divider & (divider - 1)) !== 0)
		throw new Error('Clock divider must be zero or power of two');

	return bindcall(binding.pwm_set_clock, divider);
}

rpio.prototype.pwmSetRange = function(pin, range)
{
	var channel = get_pwm_channel(pin);

	return bindcall2(binding.pwm_set_range, channel, range);
}

rpio.prototype.pwmSetData = function(pin, data)
{
	var channel = get_pwm_channel(pin);

	return bindcall2(binding.pwm_set_data, channel, data);
}

/*
 * i2c
 */
rpio.prototype.i2cBegin = function()
{
	if (!rpio_inited) {
		/* i2c requires full /dev/mem */
		rpio_options.gpiomem = false;
		rpio.prototype.init();
	}

	if (rpio_options.gpiomem)
		throw new Error('i2c not available in gpiomem mode');

	bindcall(binding.i2c_begin);
}

rpio.prototype.i2cSetSlaveAddress = function(addr)
{
	return bindcall(binding.i2c_set_slave_address, addr);
}

rpio.prototype.i2cSetClockDivider = function(divider)
{
	if ((divider % 2) !== 0)
		throw new Error('Clock divider must be an even number');

	return bindcall(binding.i2c_set_clock_divider, divider);
}

rpio.prototype.i2cSetBaudRate = function(baud)
{
	return bindcall(binding.i2c_set_baudrate, baud);
}

rpio.prototype.i2cRead = function(buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw new Error('Buffer not large enough to accommodate request');

	return bindcall2(binding.i2c_read, buf, len);
}

rpio.prototype.i2cWrite = function(buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw new Error('Buffer not large enough to accommodate request');

	return bindcall2(binding.i2c_write, buf, len);
}

rpio.prototype.i2cEnd = function()
{
	bindcall(binding.i2c_end);
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
		throw new Error('SPI not available in gpiomem mode');

	bindcall(binding.spi_begin);
}

rpio.prototype.spiChipSelect = function(cs)
{
	return bindcall(binding.spi_chip_select, cs);
}

rpio.prototype.spiSetCSPolarity = function(cs, active)
{
	return bindcall2(binding.spi_set_cs_polarity, cs, active);
}

rpio.prototype.spiSetClockDivider = function(divider)
{
	if ((divider % 2) !== 0 || divider < 0 || divider > 65536)
		throw new Error('Clock divider must be an even number between 0 and 65536');

	return bindcall(binding.spi_set_clock_divider, divider);
}

rpio.prototype.spiSetDataMode = function(mode)
{
	return bindcall(binding.spi_set_data_mode, mode);
}

rpio.prototype.spiTransfer = function(txbuf, rxbuf, len)
{
	return bindcall3(binding.spi_transfer, txbuf, rxbuf, len);
}

rpio.prototype.spiWrite = function(buf, len)
{
	return bindcall2(binding.spi_write, buf, len);
}

rpio.prototype.spiEnd = function()
{
	bindcall(binding.spi_end);
}

/*
 * Misc functions.
 */
rpio.prototype.sleep = function(secs)
{
	bindcall(binding.rpio_usleep, secs * 1000000);
}

rpio.prototype.msleep = function(msecs)
{
	bindcall(binding.rpio_usleep, msecs * 1000);
}

rpio.prototype.usleep = function(usecs)
{
	bindcall(binding.rpio_usleep, usecs);
}

process.on('exit', function(code) {
	bindcall(binding.rpio_close);
});
