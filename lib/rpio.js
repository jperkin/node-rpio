/*
 * Copyright (c) 2020 Jonathan Perkin <jonathan@perkin.org.uk>
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
 * Supported SoC types.  Must be kept in sync with rpio.cc.
 */
rpio.prototype.SOC_BCM2835 = 0x0;
rpio.prototype.SOC_SUNXI = 0x1;

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
	close_on_exit: true
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

function bindcall4(bindfunc, arg1, arg2, arg3, arg4)
{
	if (rpio_options.mock)
		return;

	return bindfunc(arg1, arg2, arg3, arg4);
}

function warn(msg)
{
	console.error('WARNING: ' + msg);
}

/*
 * Map physical pin to BCM GPIOxx numbering.  There are currently three
 * layouts:
 *
 * 	PINMAP_26_R1:	26-pin original model B (PCB rev 1.0)
 * 	PINMAP_26:	26-pin models A and B (PCB rev 2.0)
 * 	PINMAP_40:	40-pin models
 *
 * A -1 indicates an unusable pin.  Each table starts with a -1 so that we
 * can index into the array by pin number.
 */
var pincache = {};
var soctype = null;
var pinmap = null;
var mockmap = {};
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
	],
	/*
	 * Compute Modules.
	 *
	 * https://www.raspberrypi.org/documentation/hardware/computemodule/datasheets/rpi_DATA_CM_1p0.pdf
	 * https://datasheets.raspberrypi.org/cm4/cm4-datasheet.pdf
	 */
	PINMAP_CM1: [
		-1,
		-1, -1,		/*  P1  P2 */
		 0, -1,		/*  P3  P4 */
		 1, -1,		/*  P5  P6 */
		-1, -1,		/*  P7  P8 */
		 2, -1,		/*  P9  P10 */
		 3, -1,		/* P11  P12 */
		-1, -1,		/* P13  P14 */
		 4, -1,		/* P15  P16 */
		 5, -1,		/* P17  P18 */
		-1, -1,		/* P19  P20 */
		 6, -1,		/* P21  P22 */
		 7, -1,		/* P23  P24 */
		-1, -1,		/* P25  P26 */
		 8, 28,		/* P27  P28 */
		 9, 29,		/* P29  P30 */
		-1, -1,		/* P31  P32 */
		10, 30,		/* P33  P34 */
		11, 31,		/* P35  P36 */
		-1, -1,		/* P37  P38 */
		-1, -1,		/* P39  P40 */
		-1, -1,		/* P41  P42 */
		-1, -1,		/* P43  P44 */
		12, 32,		/* P45  P46 */
		13, 33,		/* P47  P48 */
		-1, -1,		/* P49  P50 */
		14, 34,		/* P51  P52 */
		15, 35,		/* P53  P54 */
		-1, -1,		/* P55  P56 */
		16, 36,		/* P57  P58 */
		17, 37,		/* P59  P60 */
		-1, -1,		/* P61  P62 */
		18, 38,		/* P63  P64 */
		19, 39,		/* P65  P66 */
		-1, -1,		/* P67  P68 */
		20, 40,		/* P69  P70 */
		21, 41,		/* P71  P72 */
		-1, -1,		/* P73  P74 */
		22, 42,		/* P75  P76 */
		23, 43,		/* P77  P78 */
		-1, -1,		/* P79  P80 */
		24, 44,		/* P81  P82 */
		25, 45,		/* P83  P84 */
		-1, -1,		/* P85  P86 */
		26, 46,		/* P87  P88 */
		27, 47,		/* P89  P90 */
		-1, -1,		/* P91  P92 */
		-1, -1,		/* P93  P94 */
		-1, -1,		/* P95  P96 */
		-1, -1,		/* P97  P98 */
		-1, -1,		/* P99  P100 */
		-1, -1,		/* P101 P102 */
		-1, -1,		/* P103 P104 */
		-1, -1,		/* P105 P106 */
		-1, -1,		/* P107 P108 */
		-1, -1,		/* P109 P110 */
		-1, -1,		/* P111 P112 */
		-1, -1,		/* P113 P114 */
		-1, -1,		/* P115 P116 */
		-1, -1,		/* P117 P118 */
		-1, -1,		/* P119 P120 */
		-1, -1,		/* P121 P122 */
		-1, -1,		/* P123 P124 */
		-1, -1,		/* P125 P126 */
		-1, -1,		/* P127 P128 */
		-1, -1,		/* P129 P130 */
		-1, -1,		/* P131 P132 */
		-1, -1,		/* P133 P134 */
		-1, -1,		/* P135 P136 */
		-1, -1,		/* P137 P138 */
		-1, -1,		/* P139 P140 */
		-1, -1,		/* P141 P142 */
		-1, -1,		/* P143 P144 */
		-1, -1,		/* P145 P146 */
		-1, -1,		/* P147 P148 */
		-1, -1,		/* P149 P150 */
		-1, -1,		/* P151 P152 */
		-1, -1,		/* P153 P154 */
		-1, -1,		/* P155 P156 */
		-1, -1,		/* P157 P158 */
		-1, -1,		/* P159 P160 */
		-1, -1,		/* P161 P162 */
		-1, -1,		/* P163 P164 */
		-1, -1,		/* P165 P166 */
		-1, -1,		/* P167 P168 */
		-1, -1,		/* P169 P170 */
		-1, -1,		/* P171 P172 */
		-1, -1,		/* P173 P174 */
		-1, -1,		/* P175 P176 */
		-1, -1,		/* P177 P178 */
		-1, -1,		/* P179 P180 */
		-1, -1,		/* P181 P182 */
		-1, -1,		/* P183 P184 */
		-1, -1,		/* P185 P186 */
		-1, -1,		/* P187 P188 */
		-1, -1,		/* P189 P190 */
		-1, -1,		/* P191 P192 */
		-1, -1,		/* P193 P194 */
		-1, -1,		/* P195 P196 */
		-1, -1,		/* P197 P198 */
		-1, -1		/* P199 P200 */
	],
	PINMAP_CM4: [
		-1,
		-1, -1,		/*  P1  P2 */
		-1, -1,		/*  P3  P4 */
		-1, -1,		/*  P5  P6 */
		-1, -1,		/*  P7  P8 */
		-1, -1,		/*  P9  P10 */
		-1, -1,		/* P11  P12 */
		-1, -1,		/* P13  P14 */
		-1, -1,		/* P15  P16 */
		-1, -1,		/* P17  P18 */
		-1, -1,		/* P19  P20 */
		-1, -1,		/* P21  P22 */
		-1, 26,		/* P23  P24 */
		21, 19,		/* P25  P26 */
		20, 13,		/* P27  P28 */
		16,  6,		/* P29  P30 */
		12, -1,		/* P31  P32 */
		-1,  5,		/* P33  P34 */
		 1,  0,		/* P35  P36 */
		 7, 11,		/* P37  P38 */
		 8,  9,		/* P39  P40 */
		25, -1,		/* P41  P42 */
		-4, 10,		/* P43  P44 */
		24, 22,		/* P45  P46 */
		23, 27,		/* P47  P48 */
		18, 17,		/* P49  P50 */
		15, -1,		/* P51  P52 */
		-1,  4,		/* P53  P54 */
		14,  3,		/* P55  P56 */
		-1,  2,		/* P57  P58 */
		-1, -1,		/* P59  P60 */
		-1, -1,		/* P61  P62 */
		-1, -1,		/* P63  P64 */
		-1, -1,		/* P65  P66 */
		-1, -1,		/* P67  P68 */
		-1, -1,		/* P69  P70 */
		-1, -1,		/* P71  P72 */
		-1, -1,		/* P73  P74 */
		-1, -1,		/* P75  P76 */
		-1, -1,		/* P77  P78 */
		-1, 45,		/* P79  P80 */
		-1, 44,		/* P81  P82 */
		-1, -1,		/* P83  P84 */
		-1, -1,		/* P85  P86 */
		-1, -1,		/* P87  P88 */
		-1, -1,		/* P89  P90 */
		-1, -1,		/* P91  P92 */
		-1, -1,		/* P93  P94 */
		-1, -1,		/* P95  P96 */
		-1, -1,		/* P97  P98 */
		-1, -1,		/* P99  P100 */
		-1, -1,		/* P101 P102 */
		-1, -1,		/* P103 P104 */
		-1, -1,		/* P105 P106 */
		-1, -1,		/* P107 P108 */
		-1, -1,		/* P109 P110 */
		-1, -1,		/* P111 P112 */
		-1, -1,		/* P113 P114 */
		-1, -1,		/* P115 P116 */
		-1, -1,		/* P117 P118 */
		-1, -1,		/* P119 P120 */
		-1, -1,		/* P121 P122 */
		-1, -1,		/* P123 P124 */
		-1, -1,		/* P125 P126 */
		-1, -1,		/* P127 P128 */
		-1, -1,		/* P129 P130 */
		-1, -1,		/* P131 P132 */
		-1, -1,		/* P133 P134 */
		-1, -1,		/* P135 P136 */
		-1, -1,		/* P137 P138 */
		-1, -1,		/* P139 P140 */
		-1, -1,		/* P141 P142 */
		-1, -1,		/* P143 P144 */
		-1, -1,		/* P145 P146 */
		-1, -1,		/* P147 P148 */
		-1, -1,		/* P149 P150 */
		-1, -1,		/* P151 P152 */
		-1, -1,		/* P153 P154 */
		-1, -1,		/* P155 P156 */
		-1, -1,		/* P157 P158 */
		-1, -1,		/* P159 P160 */
		-1, -1,		/* P161 P162 */
		-1, -1,		/* P163 P164 */
		-1, -1,		/* P165 P166 */
		-1, -1,		/* P167 P168 */
		-1, -1,		/* P169 P170 */
		-1, -1,		/* P171 P172 */
		-1, -1,		/* P173 P174 */
		-1, -1,		/* P175 P176 */
		-1, -1,		/* P177 P178 */
		-1, -1,		/* P179 P180 */
		-1, -1,		/* P181 P182 */
		-1, -1,		/* P183 P184 */
		-1, -1,		/* P185 P186 */
		-1, -1,		/* P187 P188 */
		-1, -1,		/* P189 P190 */
		-1, -1,		/* P191 P192 */
		-1, -1,		/* P193 P194 */
		-1, -1,		/* P195 P196 */
		-1, -1,		/* P197 P198 */
		-1, -1		/* P199 P200 */
	],
	/*
	 * Orange Pi Zero (26 pin)
	 */
	PINMAP_OPI_26: [
		-1,
		-1,  -1,	/*  P1  P2 */
		12,  -1,	/*  P3  P4 */
		11,  -1,	/*  P5  P6 */
		 6, 198,	/*  P7  P8 */
		-1, 199,	/*  P9  P10 */
		 1,   7,	/* P11  P12 */
		 0,  -1,	/* P13  P14 */
		 3,  19,	/* P15  P16 */
		-1,  18,	/* P17  P18 */
		15,  -1,	/* P19  P20 */
		16,   2,	/* P21  P22 */
		14,  13,	/* P23  P24 */
		-1,  10,	/* P25  P26 */
	],
	/*
	 * Banana Pi M2 Zero (40 pin)
	 */
	PINMAP_BPI_M2Z: [
		 -1,
		 -1,  -1,	/*  P1  P2 */
		 12,  -1,	/*  P3  P4 */
		 11,  -1,	/*  P5  P6 */
		  6,  13,	/*  P7  P8 */
		 -1,  14,	/*  P9  P10 */
		  1,  16,	/* P11  P12 */
		  0,  -1,	/* P13  P14 */
		  3,  15,	/* P15  P16 */
		 -1,  68,	/* P17  P18 */
		 64,  -1,	/* P19  P20 */
		 65,   2,	/* P21  P22 */
		 66,  67,	/* P23  P24 */
		 -1,  71,	/* P25  P26 */
		 19,  18,	/* P27  P28 */
		  7,  -1,	/* P29  P30 */
		  8, 354,	/* P31  P32 */
		  9,  -1,	/* P33  P34 */
		 10, 356,	/* P35  P36 */
		 17,  21,	/* P37  P38 */
		 -1,  20	/* P39  P40 */
	],
	/*
	 * Banana Pi M2 Berry / Ultra (40 pin)
	 */
	PINMAP_BPI_M2U: [
		 -1,
		 -1,  -1,	/*  P1  P2 */
		 53,  -1,	/*  P3  P4 */
		 52,  -1,	/*  P5  P6 */
		 35, 274,	/*  P7  P8 */
		 -1, 275,	/*  P9  P10 */
		276, 273,	/* P11  P12 */
		277,  -1,	/* P13  P14 */
		249, 272,	/* P15  P16 */
		 -1, 250,	/* P17  P18 */
		 64,  -1,	/* P19  P20 */
		 65, 251,	/* P21  P22 */
		 66,  87,	/* P23  P24 */
		 -1, 248,	/* P25  P26 */
		257, 256,	/* P27  P28 */
		224,  -1,	/* P29  P30 */
		225, 116,	/* P31  P32 */
		226,  -1,	/* P33  P34 */
		227, 231,	/* P35  P36 */
		228, 230,	/* P37  P38 */
		 -1, 229	/* P39  P40 */
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
 * Detect Raspberry Pi model and the pinmap in use using device tree.
 */
function detect_pinmap()
{
	var model;

	try {
		model = fs.readFileSync('/proc/device-tree/model', 'ascii');
		model = model.replace(/\0+$/, '');
	} catch (err) {
		return false;
	}

	if (m = model.match(/^Raspberry Pi (.*)$/)) {
		soctype = rpio.prototype.SOC_BCM2835;

		/*
		 * Original 26-pin PCB 1.0.  Note that older kernels do not
		 * expose the "Rev 1" string here, affecting e.g. Debian 7,
		 * but that release is way past EOL at this point.
		 *
		 * An open question is what DT returns for the newer revisions,
		 * as we may be able to reverse the logic here.  Answers on a
		 * postcard to my "raspberry pi cpuinfo vs device-tree" gist.
		 */
		if (m[1] == 'Model B Rev 1') {
			pinmap = 'PINMAP_26_R1';
			return true;
		}

		/*
		 * Regular 26-pin variants with the P5 header.
		 */
		if (m[1].indexOf('Model') === 0 && m[1].indexOf('Plus') === -1) {
			pinmap = 'PINMAP_26';
			return true;
		}

		/*
		 * 200-pin Compute Modules.
		 */
		if (m[1].indexOf('Compute Module 4') === 0) {
			pinmap = 'PINMAP_CM4';
			return true;
		}
		if (m[1].indexOf('Compute Module') === 0) {
			pinmap = 'PINMAP_CM1';
			return true;
		}

		/*
		 * All other models are 40-pin variants with the same pinmap.
		 */
		pinmap = 'PINMAP_40';
		return true;
	}

	/*
	 * Banana Pi M2 Zero (H2+)
	 * Banana Pi M2P Zero (H2+) (40 pin)
	 */
	if ((m = model.match(/^sun8iw7p1/)) ||
			(m = model.match(/Banana Pi BPI-M2-Zero/))) {
		soctype = rpio.prototype.SOC_SUNXI;
		pinmap = 'PINMAP_BPI_M2Z';
		return true;
	}
	/*
	 * Orange Pi Zero (H2+)
	 *
	 * XXX: According to linux-sunxi.org sun8iw7p may match 40-pin models?
	 * Zw1d: And it does. That's why BPI-M2-Zero definition was moved above
	 * and sun8iw7p1 was added.
	 */
	if ((m = model.match(/^sun8iw7p/)) ||
	    (m = model.match(/Orange Pi Zero/))) {
		soctype = rpio.prototype.SOC_SUNXI;
		pinmap = 'PINMAP_OPI_26';
		return true;
	}
	/*
	 * Banana Pi M2 Ultra (R40, V40)
	 */
	if (m = model.match(/^sun8iw11p/)) {
		soctype = rpio.prototype.SOC_SUNXI;
		pinmap = 'PINMAP_BPI_M2U';
		return true;
	}

	return false;
}

function set_mock_pinmap()
{
	switch (rpio_options.mock) {
	case 'raspi-b-r1':
		soctype = rpio.prototype.SOC_BCM2835;
		pinmap = 'PINMAP_26_R1';
		break;
	case 'raspi-a':
	case 'raspi-b':
		soctype = rpio.prototype.SOC_BCM2835;
		pinmap = 'PINMAP_26';
		break;
	case 'raspi-a+':
	case 'raspi-b+':
	case 'raspi-2':
	case 'raspi-3':
	case 'raspi-4':
	case 'raspi-400':
	case 'raspi-zero':
	case 'raspi-zero-w':
		soctype = rpio.prototype.SOC_BCM2835;
		pinmap = 'PINMAP_40';
		break;
	case 'cm':
	case 'cm1':
	case 'cm3':
	case 'cm3-lite':
	case 'compute-module':
	case 'compute-module-3':
	case 'compute-module-3-lite':
		pinmap = 'PINMAP_CM1';
		break;
	case 'cm4':
	case 'compute-module-4':
		pinmap = 'PINMAP_CM4';
		break;
	case 'orangepi-zero':
		soctype = rpio.prototype.SOC_SUNXI;
		pinmap = 'PINMAP_OPI_26';
		break;
	case 'bananapi-m2-berry':
	case 'bananapi-m2-ultra':
		soctype = rpio.prototype.SOC_SUNXI;
		pinmap = 'PINMAP_BPI_M2U';
		break;
	case 'bananapi-m2-zero':
		soctype = rpio.prototype.SOC_SUNXI;
		pinmap = 'PINMAP_BPI_M2Z';
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

	errstr = util.format('Pin %d is not valid when using %s mapping',
			     pin, rpio_options.mapping);

	switch (rpio_options.mapping) {
	case 'physical':
		if (pinmaps[pinmap][pin] == -1 ||
		    pinmaps[pinmap][pin] == null) {
			throw new Error(errstr);
		}
		pincache[pin] = pinmaps[pinmap][pin];
		break;
	case 'gpio':
		if (pinmaps[pinmap].indexOf(pin) === -1)
			throw new Error(errstr);
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

	/*
	 * Invalidate the pin cache and mapping as we may be in the process
	 * of changing them.
	 */
	pincache = {};
	pinmap = null;
	soctype = null;

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
	 * Allow the user to suppress automatic close of rpio on exit.
	 */
	if (rpio_options.close_on_exit) {
		process.on('exit', function(code) {
			rpio.prototype.exit();
		});
	}

	/*
	 * Open the SoC driver.
	 */
	bindcall2(binding.rpio_init, soctype, rpio_options.gpiomem ? 1 : 0);
	rpio_inited = true;
}

rpio.prototype.mode = function(pin, mode, init)
{
	var gpiopin = pin_to_gpio(pin);

	switch (mode) {
	case rpio.prototype.INPUT:
		bindcall2(binding.gpio_function, gpiopin, rpio.prototype.INPUT);
		if (init !== undefined)
			bindcall2(binding.gpio_pud, gpiopin, init);
		return;
	case rpio.prototype.OUTPUT:
		if (init !== undefined) {
			if (rpio_options.mock)
				mockmap[gpiopin] = init;
			bindcall2(binding.gpio_write, gpiopin, init);
		}
		return bindcall2(binding.gpio_function, gpiopin, rpio.prototype.OUTPUT);
	case rpio.prototype.PWM:
		return set_pin_pwm(pin);
	default:
		throw new Error('Unsupported mode ' + mode);
	}
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

	return rpio.prototype.mode(pin, mode, init);
}

rpio.prototype.read = function(pin, mode)
{
	if (rpio_options.mock) {
		return mockmap[pin_to_gpio(pin)]
		     ? mockmap[pin_to_gpio(pin)]
		     : mockmap[pin_to_gpio(pin)] = 0
	}

	return bindcall2(binding.gpio_read, pin_to_gpio(pin), mode ? 1 : 0);
}

rpio.prototype.readbuf = function(pin, buf, len, mode)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw new Error('Buffer not large enough to accommodate request');

	return bindcall4(binding.gpio_readbuf, pin_to_gpio(pin), buf, len, mode ? 1 : 0);
}

rpio.prototype.write = function(pin, value)
{
	if (rpio_options.mock)
		return mockmap[pin_to_gpio(pin)] = value

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

	if (rpio_options.mock)
		mockmap[pin_to_gpio(pin)] = undefined

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

rpio.prototype.exit = function()
{
	bindcall(binding.rpio_close);
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

rpio.prototype.i2cReadRegisterRestart = function(reg, buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw new Error('Buffer not large enough to accommodate request');

	return bindcall3(binding.i2c_read_register_rs, reg, buf, len);
}

rpio.prototype.i2cWrite = function(buf, len)
{
	if (len === undefined)
		len = buf.length;

	if (len > buf.length)
		throw new Error('Buffer not large enough to accommodate request');

	return bindcall2(binding.i2c_write, buf, len);
}

rpio.prototype.i2cWriteReadRestart = function(cmdbuf, cmdlen, rbuf, rlen)
{
	if (cmdlen === undefined)
		cmdlen = cmdbuf.length;

	if (cmdlen > cmdbuf.length)
		throw new Error('Write buffer not large enough to accommodate request');

	if (rlen === undefined)
		rlen = rbuf.length;

	if (rlen > rbuf.length)
		throw new Error('Read buffer not large enough to accommodate request');

	return bindcall4(binding.i2c_write_read_rs, cmdbuf, cmdlen, rbuf, rlen);
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

