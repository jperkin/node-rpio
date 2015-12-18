/*
 * Copyright (c) 2015 Jonathan Perkin <jonathan@perkin.org.uk>
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

#include <nan.h>
#include <unistd.h>	/* geteuid() */
#include "bcm2835.h"

using namespace Nan;

/*
 * GPIO function select.  Pass through all values supported by bcm2835.
 */
NAN_METHOD(gpio_function)
{
	if ((info.Length() != 2) ||
	    !info[0]->IsNumber() ||
	    !info[1]->IsNumber() ||
	    (info[1]->NumberValue() > 7))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_gpio_fsel(info[0]->NumberValue(), info[1]->NumberValue());
}

/*
 * GPIO read/write
 */
NAN_METHOD(gpio_read)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	info.GetReturnValue().Set(bcm2835_gpio_lev(info[0]->NumberValue()));
}

NAN_METHOD(gpio_write)
{
	if ((info.Length() != 2) ||
	    !info[0]->IsNumber() ||
	    !info[1]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

	bcm2835_gpio_write(info[0]->NumberValue(), info[1]->NumberValue());
}

/*
 * i2c setup
 */
NAN_METHOD(i2c_begin)
{
	bcm2835_i2c_begin();
}

NAN_METHOD(i2c_set_clock_divider)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_i2c_setClockDivider(info[0]->NumberValue());
}

NAN_METHOD(i2c_set_baudrate)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_i2c_set_baudrate(info[0]->NumberValue());
}

NAN_METHOD(i2c_set_slave_address)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_i2c_setSlaveAddress(info[0]->NumberValue());
}

NAN_METHOD(i2c_end)
{
	bcm2835_i2c_end();
}


/*
 * i2c read/write.  The underlying bcm2835_i2c_read/bcm2835_i2c_write functions
 * do not return the number of bytes read/written, only a status code.  The JS
 * layer handles ensuring that the buffer is large enough to accommodate the
 * requested length.
 */
NAN_METHOD(i2c_read)
{
	uint8_t rval;

	if ((info.Length() != 2) ||
	    (!info[0]->IsObject()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	rval = bcm2835_i2c_read(node::Buffer::Data(info[0]->ToObject()),
				info[1]->NumberValue());

	info.GetReturnValue().Set(rval);
}

NAN_METHOD(i2c_write)
{
	uint8_t rval;

	if ((info.Length() != 2) ||
	    (!info[0]->IsObject()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	rval = bcm2835_i2c_write(node::Buffer::Data(info[0]->ToObject()),
				 info[1]->NumberValue());

	info.GetReturnValue().Set(rval);
}

/*
 * PWM functions
 */
NAN_METHOD(pwm_set_clock)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_pwm_set_clock(info[0]->NumberValue());
}

NAN_METHOD(pwm_set_mode)
{
	if ((info.Length() != 3) ||
	    (!info[0]->IsNumber()) ||
	    (!info[1]->IsNumber()) ||
	    (!info[2]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_pwm_set_mode(info[0]->NumberValue(), info[1]->NumberValue(),
			     info[2]->NumberValue());
}

NAN_METHOD(pwm_set_range)
{
	if ((info.Length() != 2) ||
	    (!info[0]->IsNumber()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_pwm_set_range(info[0]->NumberValue(), info[1]->NumberValue());
}

NAN_METHOD(pwm_set_data)
{
	if ((info.Length() != 2) ||
	    (!info[0]->IsNumber()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_pwm_set_data(info[0]->NumberValue(), info[1]->NumberValue());
}

/*
 * SPI functions.
 */
NAN_METHOD(spi_begin)
{
	bcm2835_spi_begin();
}

NAN_METHOD(spi_chip_select)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_spi_chipSelect(info[0]->NumberValue());
}

NAN_METHOD(spi_set_cs_polarity)
{
	if ((info.Length() != 2) ||
	    (!info[0]->IsNumber()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_spi_setChipSelectPolarity(info[0]->NumberValue(),
					  info[1]->NumberValue());
}

NAN_METHOD(spi_set_clock_divider)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_spi_setClockDivider(info[0]->NumberValue());
}

NAN_METHOD(spi_set_data_mode)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_spi_setDataMode(info[0]->NumberValue());
}

NAN_METHOD(spi_transfer)
{
	if ((info.Length() != 3) ||
	    (!info[0]->IsObject()) ||
	    (!info[1]->IsObject()) ||
	    (!info[2]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_spi_transfernb(node::Buffer::Data(info[0]->ToObject()),
			       node::Buffer::Data(info[1]->ToObject()),
			       info[2]->NumberValue());
}

NAN_METHOD(spi_write)
{
	if ((info.Length() != 2) ||
	    (!info[0]->IsObject()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	bcm2835_spi_writenb(node::Buffer::Data(info[0]->ToObject()),
			    info[1]->NumberValue());
}

NAN_METHOD(spi_end)
{
	bcm2835_spi_end();
}

/*
 * Initialize the bcm2835 interface and check we have permission to access it.
 */
NAN_METHOD(rpio_init)
{
	if (geteuid() != 0)
		return ThrowError("You must be root to access GPIO via mmap()");

	if (!bcm2835_init())
		return ThrowError("Could not initialize bcm2835 GPIO library");
}

NAN_METHOD(rpio_close)
{
	bcm2835_close();
}

NAN_MODULE_INIT(setup)
{
	NAN_EXPORT(target, rpio_init);
	NAN_EXPORT(target, rpio_close);
	NAN_EXPORT(target, gpio_function);
	NAN_EXPORT(target, gpio_read);
	NAN_EXPORT(target, gpio_write);
	NAN_EXPORT(target, i2c_begin);
	NAN_EXPORT(target, i2c_set_clock_divider);
	NAN_EXPORT(target, i2c_set_baudrate);
	NAN_EXPORT(target, i2c_set_slave_address);
	NAN_EXPORT(target, i2c_end);
	NAN_EXPORT(target, i2c_read);
	NAN_EXPORT(target, i2c_write);
	NAN_EXPORT(target, pwm_set_clock);
	NAN_EXPORT(target, pwm_set_mode);
	NAN_EXPORT(target, pwm_set_range);
	NAN_EXPORT(target, pwm_set_data);
	NAN_EXPORT(target, spi_begin);
	NAN_EXPORT(target, spi_chip_select);
	NAN_EXPORT(target, spi_set_cs_polarity);
	NAN_EXPORT(target, spi_set_clock_divider);
	NAN_EXPORT(target, spi_set_data_mode);
	NAN_EXPORT(target, spi_transfer);
	NAN_EXPORT(target, spi_write);
	NAN_EXPORT(target, spi_end);
}

NODE_MODULE(rpio, setup)
