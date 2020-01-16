var rpio = require('../lib/rpio');

/*
 * Watch an LM393-based sound detection module on Pin 7 / GPIO 4 and print a
 * message whenever the dB level reaches the set threshold, indicated by the
 * pin going low.
 */
var pin = 7;

rpio.open(pin, rpio.INPUT);

rpio.poll(pin, function() {
	console.log("Could you please be a little quieter?");
}, rpio.POLL_LOW);
