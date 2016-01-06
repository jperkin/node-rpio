var rpio = require('../lib/rpio');

/*
 * Read data from an SPI-attached AT93C46 EEPROM.
 */
rpio.spiBegin();
rpio.spiChipSelect(0);			/* Use CE0 */
rpio.spiSetCSPolarity(0, rpio.HIGH);	/* AT93C46 chip select is active-high */
rpio.spiSetClockDivider(128);		/* AT93C46 max is 2MHz, 128 == 1.95MHz */
rpio.spiSetDataMode(0);

/*
 * There are various magic numbers below.  A quick overview:
 *
 *   tx[0] is always 0x3, the EEPROM READ instruction.
 *   tx[1] is set to var i which is the EEPROM address to read from.
 *   tx[2] and tx[3] can be anything, at this point we are only interested in
 *     reading the data back from the EEPROM into our rx buffer.
 *
 * Once we have the data returned in rx, we have to do some bit shifting to
 * get the result in the format we want, as the data is not 8-bit aligned.
 */
var tx = new Buffer([0x3, 0x0, 0x0, 0x0]);
var rx = new Buffer(4);
var out;
var i, j = 0;

for (i = 0; i < 128; i++, ++j) {
	tx[1] = i;
	rpio.spiTransfer(tx, rx, 4);
	out = ((rx[2] << 1) | (rx[3] >> 7));
	process.stdout.write(out.toString(16) + ((j % 16 == 0) ? '\n' : ' '));
}
rpio.spiEnd();
