/*
 * Copyright (c) 2012 Jonathan Perkin <jonathan@perkin.org.uk>
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

#include <unistd.h>

#include <node.h>
#include <v8.h>

#include "bcm2835.h"

using namespace v8;

/*
 * flash(pin, freq, count)
 */
Handle<Value> Flash(const Arguments& args)
{
	HandleScope scope;
	int i;

	if (args.Length() != 3) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if (geteuid() != 0) {
		ThrowException(Exception::Error(String::New("You must be root to access GPIO")));
		return scope.Close(Undefined());
	}

	if (!bcm2835_init()) {
		ThrowException(Exception::Error(String::New("Could not initialize GPIO")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Wrong arguments")));
		return scope.Close(Undefined());
	}

	bcm2835_gpio_fsel(args[0]->NumberValue(), BCM2835_GPIO_FSEL_OUTP);

	for (i = 0; i < args[2]->NumberValue(); i++) {
		bcm2835_gpio_write(args[0]->NumberValue(), HIGH);
		bcm2835_delay(args[1]->NumberValue());
		bcm2835_gpio_write(args[0]->NumberValue(), LOW);
		bcm2835_delay(args[1]->NumberValue());
	}

	return scope.Close(Undefined());
}

void init(Handle<Object> target) {
	target->Set(String::NewSymbol("flash"),
	    FunctionTemplate::New(Flash)->GetFunction());
}

NODE_MODULE(rpio, init)
