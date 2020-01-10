var rpio = require('../lib/rpio');

/*
 * Blink an LED connected to pin 15 a few times.
 *
 * This example uses the default physical P01-P40 numbering, therefore physical
 * pin 15 which is also known as GPIO/BCM pin 22.
 */
var pin = 15;

/*
 * Configure the pin for output, setting it low initially.  The state is set
 * prior to the pin being activated, so is suitable for devices which require
 * a stable setup.
 */
rpio.open(pin, rpio.OUTPUT, rpio.LOW);

/*
 * Blink the LED 5 times.  The sleep functions block, but for a trivial example
 * like this that isn't a problem and simplifies things.
 */
for (var i = 0; i < 5; i++) {

	/* On for 1 second */
	rpio.write(pin, rpio.HIGH);
	rpio.sleep(1);

	/* Off for half a second (500ms) */
	rpio.write(pin, rpio.LOW);
	rpio.msleep(500);
}
