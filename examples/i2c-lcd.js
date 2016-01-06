var rpio = require('../lib/rpio');

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
	rpio.i2cWrite(Buffer([(data | LCD_BACKLIGHT)]));
	rpio.i2cWrite(Buffer([(data | LCD_ENABLE | LCD_BACKLIGHT)]));
	rpio.i2cWrite(Buffer([((data & ~LCD_ENABLE) | LCD_BACKLIGHT)]));
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

lineout('node.js i2c LCD!', LCD_LINE1);
lineout('npm install rpio', LCD_LINE2);

rpio.i2cEnd();
