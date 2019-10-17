var rpio = require('../lib/rpio');

/*
 * Watch a button switch attached to the configured pin for changes.
 *
 * This example uses the default physical P01-P40 numbering, therefore physical
 * pin 15 also known as GPIO/BCM pin 22.  This pin is connected to one side of
 * the switch, and the other side of the switch is connected to ground.
 */
var pin = 15;

/*
 * Configure the pin for input, and enable the internal pullup resistor which
 * pulls the current high.  Pushing the button connects the pin to ground and
 * pulls it low.  We poll for state changes and print a message when it does.
 */
rpio.open(pin, rpio.INPUT, rpio.PULL_UP);

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
	 *
	 * If we read 1 then the pullup resistor is pulling the current high,
	 * i.e. the switch is not pressed.  If we read 0 then the switch is
	 * pulling the current low, i.e. it has been pressed.
	 */
	var state = rpio.read(cbpin) ? 'released' : 'pressed';
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
