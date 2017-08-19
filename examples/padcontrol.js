var rpio = require('../lib/rpio');

rpio.init({gpiomem: false});

/* Print the current pad control settings for the primary GPIO pins. */
var curpad = rpio.readpad(rpio.PAD_GROUP_0_27);

/* Test bits for settings. */
var slew = ((curpad & rpio.PAD_SLEW_UNLIMITED) == rpio.PAD_SLEW_UNLIMITED);
var hysteresis = ((curpad & rpio.PAD_HYSTERESIS) == rpio.PAD_HYSTERESIS)
var drive = (curpad & 0x7);

console.log('GPIO Pad Control for GPIO0 - GPIO27 is currently set to:');
console.log('\tSlew rate: ' + (slew ? 'unlimited' : 'limited'));
console.log('\tInput hysteresis: ' + (hysteresis ? 'enabled' : 'disabled'));
console.log('\tDrive rate: ' + (drive * 2 + 2));
