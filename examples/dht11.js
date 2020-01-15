var rpio = require('../lib/rpio');

/*
 * Repeatedly read a DHT11 attached to Pin 7 / GPIO 4 and print out the current
 * temperature and relative humidity if the read was successful.
 */
var pin = 7;

/*
 * The DHT11 transmission is made up of 40 "bits", where the length of each
 * high output determines whether it is a 0 or 1.  Per the datasheet, "0" is
 * 26-28us in length, and "1" 70us.  However, in a JS environment we can't be
 * so precise, and so we determine what is high and low based on a sampling of
 * the values we received.
 *
 * These devices are notoriously unreliable, so this program will run forever
 * attempting to retrieve and print valid data until the user kills the process.
 *
 * For the most portable implementation we read all bits after initialisation
 * into a big buffer as fast as possible, and then parse afterwards.  This is
 * necessary on e.g. the original Raspberry Pi 1 as the CPU is not fast enough
 * to do something like:
 *
 *	while ((curstate = rpio.read(...)) == laststate)
 *		count++;
 *
 * for each read, which would be the normal way of doing something like this.
 */
function read_dht11(vals)
{
	/* Data bits received from the DHT11 */
	var data = new Array(40);

	/*
	 * Our read buffer of all the bits sent.  Should be plenty.  On a
	 * Raspberry Pi 4 a successful read uses up about half of this.
	 */
	var buf = new Buffer(50000);

	/*
	 * The initialisation sequence as per the datasheet is to start high,
	 * pull low for at least 18ms, then back high and read.  The JavaScript
	 * function call overhead ensures that we'll actually be sleeping
	 * longer than that, but it doesn't appear to be a problem.
	 */
	rpio.open(pin, rpio.OUTPUT, rpio.HIGH);
	rpio.write(pin, rpio.LOW);
	rpio.msleep(18);

	/*
	 * Switch to input mode with a pull-up as per the datasheet, and read
	 * as fast as possible into the buffer.  Configuring the pull-up is the
	 * same as setting the line high first, so we don't explicitly do that,
	 * to reduce the chance of missing the start of the transmission.
	 */
	rpio.mode(pin, rpio.INPUT, rpio.PULL_UP);
	rpio.readbuf(pin, buf);
	rpio.close(pin);

	/*
	 * The data has been received, split the buffer into groups of "1"s.
	 */
	var dlen = 0;
	buf.join('').replace(/0+/g, '0').split('0').forEach(function(bits, n) {
		/*
		 * Skip past the first two values that are part of the
		 * initialisation sequence sent by the DHT11 to indicate the
		 * transmission is about to begin, and log the next 40 data
		 * bits.
		 */
		if (n < 2 || n > 41)
			return;

		data[dlen++] = bits.length;
	});

	/*
	 * In theory we should be able to test for exactly 40 bits here, but
	 * sometimes only 39 bits are returned and the checksum is still valid,
	 * so we allow it, but return early if any fewer than that.
	 */
	if (dlen < 39)
		return false;

	/*
	 * Calculate the low and high water marks.  As each model of Raspberry
	 * Pi will run at different speeds, the length of each high bit will
	 * vary, so calculate the average and use that to determine what is
	 * "high" and "low".
	 *
	 * The longest "low" seen on a Raspberry Pi 4 is around 135, so the
	 * default low here should be more than sufficient.
	 */
	var low = 10000;
	var high = 0;
	for (var i = 0; i < dlen; i++) {
		if (data[i] < low)
			low = data[i];
		if (data[i] > high)
			high = data[i];
	}
	var avg = (low + high) / 2;

	/*
	 * The data received from the DHT11 is in 5 groups of 8-bits:
	 *
	 *	[0:7] integral relative humidity
	 *	[8:15] decimal relative humidity
	 *	[16:23] integral temperature
	 *	[24:31] decimal temperature
	 *	[32:39] checksum
	 *
	 * Parse the bitstream into the supplied "vals" buffer.
	 */
	vals.fill(0);
	for (var i = 0; i < dlen; i++) {
		var group = parseInt(i/8)

		/* The data is in big-endian format, shift it in. */
		vals[group] <<= 1;

		/* This should be a high bit, based on the average. */
		if (data[i] >= avg)
			vals[group] |= 1;
	}

	/*
	 * Validate the checksum and return whether successful or not.  The
	 * checksum is simply the value of the other 4 groups combined, masked
	 * off to 8-bits.
	 */
	return (vals[4] == ((vals[0] + vals[1] + vals[2] + vals[3]) & 0xFF));
}

/*
 * Run until the user kills the process, skipping any bad reads.
 */
var v = Buffer(5);

while (true) {
	/*
	 * On the DHT11 the fractional parts are always 0 so just print the
	 * integrals.
	 */
	if (read_dht11(v)) {
		console.log("Temperature = %dC, Humidity = %d%%", v[2], v[0]);
	}

	/*
	 * The datasheet specifies that the sampling period should not be less
	 * than 1 second, so be generous and give it 2.
	 */
	rpio.sleep(2);
}
