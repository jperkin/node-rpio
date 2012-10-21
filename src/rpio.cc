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
 * Set a pin as input
 */
Handle<Value> SetInput(const Arguments& args)
{
	HandleScope scope;

	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Incorrect number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Incorrect argument type(s)")));
		return scope.Close(Undefined());
	}

	bcm2835_gpio_fsel(args[0]->NumberValue(), BCM2835_GPIO_FSEL_INPT);

	return scope.Close(Integer::New(0));
}

/*
 * Set a pin as output
 */
Handle<Value> SetOutput(const Arguments& args)
{
	HandleScope scope;

	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Incorrect number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Incorrect argument type(s)")));
		return scope.Close(Undefined());
	}

	bcm2835_gpio_fsel(args[0]->NumberValue(), BCM2835_GPIO_FSEL_OUTP);

	return scope.Close(Integer::New(0));
}

/*
 * Read from a pin
 */
Handle<Value> Read(const Arguments& args)
{
	HandleScope scope;
	uint8_t value;

	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Incorrect number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Incorrect argument type(s)")));
		return scope.Close(Undefined());
	}

	value = bcm2835_gpio_lev(args[0]->NumberValue());

	return scope.Close(Integer::New(value));
}

/*
 * Write to a pin
 */
Handle<Value> Write(const Arguments& args)
{
	HandleScope scope;

	if (args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Incorrect number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Incorrect argument type(s)")));
		return scope.Close(Undefined());
	}

	bcm2835_gpio_write(args[0]->NumberValue(), args[1]->NumberValue());

	return scope.Close(Integer::New(0));
}

/*
 * Initialize the bcm2835 interface and check we have permission to access it.
 */
Handle<Value> Start(const Arguments& args)
{
	HandleScope scope;

	if (geteuid() != 0) {
		ThrowException(Exception::Error(String::New("You must be root to access GPIO")));
		return scope.Close(Undefined());
	}

	if (!bcm2835_init()) {
		ThrowException(Exception::Error(String::New("Could not initialize GPIO")));
		return scope.Close(Undefined());
	}

	return scope.Close(Undefined());
}

void init(Handle<Object> target)
{

	target->Set(String::NewSymbol("start"),
	    FunctionTemplate::New(Start)->GetFunction());

	target->Set(String::NewSymbol("setInput"),
	    FunctionTemplate::New(SetInput)->GetFunction());

	target->Set(String::NewSymbol("setOutput"),
	    FunctionTemplate::New(SetOutput)->GetFunction());

	target->Set(String::NewSymbol("read"),
	    FunctionTemplate::New(Read)->GetFunction());

	target->Set(String::NewSymbol("write"),
	    FunctionTemplate::New(Write)->GetFunction());
}

NODE_MODULE(rpio, init)
