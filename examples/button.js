var rpio = require('../lib/rpio');

/*
 * Watch a button switch attached to the configured pin for changes.
 *
 * This example uses the default P01-P40 numbering, and thus defaults to
 * P11 / GPIO17.
 */
var pin = 11;

/*
 * Use the internal pulldown resistor to default to off.  Pressing the button
 * causes the input to go high, releasing it leaves the pulldown resistor to
 * pull it back down to low.
 */
rpio.open(pin, rpio.INPUT, rpio.PULL_DOWN);

/*
 * This callback will be called every time a configured event is detected on
 * the pin.  The argument is the pin that triggered the event, so you can use
 * the same callback for multiple pins.
 */
function pollcb(cbpin)
{
	/*
	 * It cannot be guaranteed that the current value of the pin is the
	 * same that triggered the event, so the best we can do is notify the
	 * user that an event happened and print what the value is currently
	 * set to.
	 *
	 * Unless you are pressing the button faster than 1ms (the default
	 * setInterval() loop which polls for events) this shouldn't be a
	 * problem.
	 */
	var state = rpio.read(cbpin) ? 'pressed' : 'released';
	console.log('Button event on P%d (button currently %s)', cbpin, state);

	/*
	 * By default this program will run forever.  If you want to cancel the
	 * poll after the first event and end the program, uncomment this line.
	 */
	// rpio.poll(cbpin, null);
}

/*
 * Configure the pin for the default set of events.  If you wanted to only
 * watch for high or low events, then you'd use the third argument to specify
 * either rpio.POLL_LOW or rpio.POLL_HIGH.
 */
rpio.poll(pin, pollcb);
