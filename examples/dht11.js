var rpio = require('../lib/rpio');

/*
 * Read a DHT11 attached to P07 / GPIO4 and print out the current temperature
 * and relative humidity if the read was successful.
 */
var pin = 7;

var data = new Array(40);
var buf = new Buffer(100000);
var i;

/*
 * Initiate the MCU sequence.
 */
rpio.open(pin, rpio.OUTPUT);
rpio.write(pin, rpio.HIGH);
rpio.msleep(500);
rpio.write(pin, rpio.LOW);
/*
 * The datasheet says 18us, but we need to account for the JavaScript function
 * call overhead.  Trial and error suggests this is a working value.
 */
rpio.msleep(3);
rpio.write(pin, rpio.HIGH);

/*
 * Switch to input mode and read as fast as possible into our buffer.
 */
rpio.mode(pin, rpio.INPUT);
rpio.readbuf(pin, buf);

/*
 * Parse the buffer for lengths of each high section.  The length determines
 * whether it's a low, high, or control bit.
 */
var tmp = new Array(buf.length); /* 0.8 compat */
for (i = 0; i < buf.length; i++) {
	/* Convert buffer to array for node.js 0.8 compat */
	tmp[i] = buf[i];
}
i = 0;
tmp.join('').replace(/0+/g, '0').split('0').forEach(function(bits) {
	/*
	 * These are magic numbers.  If they don't work then uncomment this
	 * line instead, it should give you the rough numbers required to
	 * differentiate 0's and 1's.
	 *
	 * console.log(bits.length);
	 */
	if (bits.length > 320)
		return;
	data[i++] = (bits.length > 150) ? 1 : 0;
});

/*
 * Pull out the individual value octets
 */
var rh_int = parseInt(data.slice(0, 8).join(''), 2);
var rh_dec = parseInt(data.slice(8, 16).join(''), 2);
var tm_int = parseInt(data.slice(16, 24).join(''), 2);
var tm_dec = parseInt(data.slice(24, 32).join(''), 2);
var chksum = parseInt(data.slice(32, 40).join(''), 2);

/* chksum should match this calculation */
var chk = ((rh_int + rh_dec + tm_int + tm_dec) & 0xFF);

/* Sometimes the checksum will be correct but the values obviously wrong. */
if (chksum == chk && rh_int <= 100) {
	console.log("Temperature = %dC, Humidity = %d%%", tm_int, rh_int);
} else {
	console.log("Read failed:");
	console.log("    chk = %d, chksum = %d", chk, chksum);
	console.log("    %d %d %d %d (%d = %d)", rh_int, rh_dec, tm_int, tm_dec, chk, chksum);
}
