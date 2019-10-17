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
#include <unistd.h>	/* usleep() */
#include "bcm2835.h"

#define RPIO_EVENT_LOW	0x1
#define RPIO_EVENT_HIGH	0x2

/* Avoid writing this monstrosity everywhere */
#define RPIO_BUFFER_OBJECT(i)	\
	node::Buffer::Data(Nan::To<v8::Object>(info[i]).ToLocalChecked())

using namespace Nan;

/*
 * GPIO function select.
 */
NAN_METHOD(gpio_function)
{
	if (info.Length() != 2 || !info[0]->IsUint32() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t pin(Nan::To<uint32_t>(info[0]).FromJust());
	uint32_t mode(Nan::To<uint32_t>(info[1]).FromJust());

	bcm2835_gpio_fsel(pin, mode);
}

/*
 * GPIO read/write
 */
NAN_METHOD(gpio_read)
{
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t pin(Nan::To<uint32_t>(info[0]).FromJust());

	info.GetReturnValue().Set(bcm2835_gpio_lev(pin));
}

NAN_METHOD(gpio_readbuf)
{
	if (info.Length() != 3 || !info[0]->IsUint32() ||
	    !info[1]->IsObject() || !info[2]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t pin(Nan::To<uint32_t>(info[0]).FromJust());
	char *buf = RPIO_BUFFER_OBJECT(1);
	uint32_t len(Nan::To<uint32_t>(info[2]).FromJust());

	for (uint32_t i = 0; i < len; i++)
		buf[i] = bcm2835_gpio_lev(pin);
}

NAN_METHOD(gpio_write)
{
	if (info.Length() != 2 || !info[0]->IsUint32() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t pin(Nan::To<uint32_t>(info[0]).FromJust());
	uint32_t val(Nan::To<uint32_t>(info[1]).FromJust());

	bcm2835_gpio_write(pin, val);
}

NAN_METHOD(gpio_writebuf)
{
	if (info.Length() != 3 ||
	    !info[0]->IsUint32() ||
	    !info[1]->IsObject() ||
	    !info[2]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t pin(Nan::To<uint32_t>(info[0]).FromJust());
	char *buf = RPIO_BUFFER_OBJECT(1);
	uint32_t len(Nan::To<uint32_t>(info[2]).FromJust());

	for (uint32_t i = 0; i < len; i++)
		bcm2835_gpio_write(pin, buf[i]);
}

NAN_METHOD(gpio_pad_read)
{
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t group(Nan::To<uint32_t>(info[0]).FromJust());

	info.GetReturnValue().Set(bcm2835_gpio_pad(group));
}

NAN_METHOD(gpio_pad_write)
{
	if (info.Length() != 2 || !info[0]->IsUint32() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t group(Nan::To<uint32_t>(info[0]).FromJust());
	uint32_t control(Nan::To<uint32_t>(info[1]).FromJust());

	bcm2835_gpio_set_pad(group, control);
}

NAN_METHOD(gpio_pud)
{
	if (info.Length() != 2 || !info[0]->IsUint32() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t pin(Nan::To<uint32_t>(info[0]).FromJust());
	uint32_t pud(Nan::To<uint32_t>(info[1]).FromJust());

	bcm2835_gpio_set_pud(pin, pud);
}

NAN_METHOD(gpio_event_set)
{
	if (info.Length() != 2 || !info[0]->IsUint32() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t pin(Nan::To<uint32_t>(info[0]).FromJust());
	uint32_t direction(Nan::To<uint32_t>(info[1]).FromJust());

	/* Clear all possible trigger events. */
	bcm2835_gpio_clr_ren(pin);
	bcm2835_gpio_clr_fen(pin);
	bcm2835_gpio_clr_hen(pin);
	bcm2835_gpio_clr_len(pin);
	bcm2835_gpio_clr_aren(pin);
	bcm2835_gpio_clr_afen(pin);

	/*
	 * Add the requested events, using the synchronous rising and
	 * falling edge detection bits.
	 */
	if (direction & RPIO_EVENT_HIGH)
		bcm2835_gpio_ren(pin);

	if (direction & RPIO_EVENT_LOW)
		bcm2835_gpio_fen(pin);
}

NAN_METHOD(gpio_event_poll)
{
	if ((info.Length() != 1) || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t rval = 0;
	uint32_t mask(Nan::To<uint32_t>(info[0]).FromJust());

	/*
	 * Interrupts are not supported, so this merely reports that an event
	 * happened in the time period since the last poll.  There is no way to
	 * know which trigger caused the event.
	 */
	if ((rval = bcm2835_gpio_eds_multi(mask)))
		bcm2835_gpio_set_eds_multi(rval);

	info.GetReturnValue().Set(rval);
}

NAN_METHOD(gpio_event_clear)
{
	if ((info.Length() != 1) || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t pin(Nan::To<uint32_t>(info[0]).FromJust());

	bcm2835_gpio_clr_fen(pin);
	bcm2835_gpio_clr_ren(pin);
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
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t divider(Nan::To<uint32_t>(info[0]).FromJust());

	bcm2835_i2c_setClockDivider(divider);
}

NAN_METHOD(i2c_set_baudrate)
{
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t baudrate(Nan::To<uint32_t>(info[0]).FromJust());

	bcm2835_i2c_set_baudrate(baudrate);
}

NAN_METHOD(i2c_set_slave_address)
{
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t addr(Nan::To<uint32_t>(info[0]).FromJust());

	bcm2835_i2c_setSlaveAddress(addr);
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
	if (info.Length() != 2 || !info[0]->IsObject() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint8_t rval;
	char *buf = RPIO_BUFFER_OBJECT(0);
	uint32_t len(Nan::To<uint32_t>(info[1]).FromJust());

	rval = bcm2835_i2c_read(buf, len);

	info.GetReturnValue().Set(rval);
}

NAN_METHOD(i2c_write)
{
	if (info.Length() != 2 || !info[0]->IsObject() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint8_t rval;
	char *buf = RPIO_BUFFER_OBJECT(0);
	uint32_t len(Nan::To<uint32_t>(info[1]).FromJust());

	rval = bcm2835_i2c_write(buf, len);

	info.GetReturnValue().Set(rval);
}

/*
 * PWM functions
 */
NAN_METHOD(pwm_set_clock)
{
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t divisor(Nan::To<uint32_t>(info[0]).FromJust());

	bcm2835_pwm_set_clock(divisor);
}

NAN_METHOD(pwm_set_mode)
{
	if (info.Length() != 3 || !info[0]->IsUint32() ||
	    !info[1]->IsUint32() || !info[2]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t channel(Nan::To<uint32_t>(info[0]).FromJust());
	uint32_t markspace(Nan::To<uint32_t>(info[1]).FromJust());
	uint32_t enabled(Nan::To<uint32_t>(info[2]).FromJust());

	bcm2835_pwm_set_mode(channel, markspace, enabled);
}

NAN_METHOD(pwm_set_range)
{
	if (info.Length() != 2 || !info[0]->IsUint32() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t channel(Nan::To<uint32_t>(info[0]).FromJust());
	uint32_t range(Nan::To<uint32_t>(info[1]).FromJust());

	bcm2835_pwm_set_range(channel, range);
}

NAN_METHOD(pwm_set_data)
{
	if (info.Length() != 2 || !info[0]->IsUint32() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t channel(Nan::To<uint32_t>(info[0]).FromJust());
	uint32_t data(Nan::To<uint32_t>(info[1]).FromJust());

	bcm2835_pwm_set_data(channel, data);
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
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t cs(Nan::To<uint32_t>(info[0]).FromJust());

	bcm2835_spi_chipSelect(cs);
}

NAN_METHOD(spi_set_cs_polarity)
{
	if (info.Length() != 2 || !info[0]->IsUint32() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t cs(Nan::To<uint32_t>(info[0]).FromJust());
	uint32_t active(Nan::To<uint32_t>(info[1]).FromJust());

	bcm2835_spi_setChipSelectPolarity(cs, active);
}

NAN_METHOD(spi_set_clock_divider)
{
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t divider(Nan::To<uint32_t>(info[0]).FromJust());

	bcm2835_spi_setClockDivider(divider);
}

NAN_METHOD(spi_set_data_mode)
{
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t mode(Nan::To<uint32_t>(info[0]).FromJust());

	bcm2835_spi_setDataMode(mode);
}

NAN_METHOD(spi_transfer)
{
	if (info.Length() != 3 || !info[0]->IsObject() ||
	    !info[1]->IsObject() || !info[2]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	char *tbuf = RPIO_BUFFER_OBJECT(0);
	char *rbuf = RPIO_BUFFER_OBJECT(1);
	uint32_t len(Nan::To<uint32_t>(info[2]).FromJust());

	bcm2835_spi_transfernb(tbuf, rbuf, len);
}

NAN_METHOD(spi_write)
{
	if (info.Length() != 2 || !info[0]->IsObject() || !info[1]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	char *buf = RPIO_BUFFER_OBJECT(0);
	uint32_t len(Nan::To<uint32_t>(info[1]).FromJust());

	bcm2835_spi_writenb(buf, len);
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
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t gpiomem(Nan::To<uint32_t>(info[0]).FromJust());

	if (!bcm2835_init(gpiomem))
		return ThrowError("Could not initialize bcm2835 GPIO library");
}

NAN_METHOD(rpio_close)
{
	bcm2835_close();
}

/*
 * Misc functions useful for simplicity
 */
NAN_METHOD(rpio_usleep)
{
	if (info.Length() != 1 || !info[0]->IsUint32())
		return ThrowTypeError("Incorrect arguments");

	uint32_t microseconds(Nan::To<uint32_t>(info[0]).FromJust());

	usleep(microseconds);
}

NAN_MODULE_INIT(setup)
{
	NAN_EXPORT(target, rpio_init);
	NAN_EXPORT(target, rpio_close);
	NAN_EXPORT(target, rpio_usleep);
	NAN_EXPORT(target, gpio_function);
	NAN_EXPORT(target, gpio_read);
	NAN_EXPORT(target, gpio_readbuf);
	NAN_EXPORT(target, gpio_write);
	NAN_EXPORT(target, gpio_writebuf);
	NAN_EXPORT(target, gpio_pad_read);
	NAN_EXPORT(target, gpio_pad_write);
	NAN_EXPORT(target, gpio_pud);
	NAN_EXPORT(target, gpio_event_set);
	NAN_EXPORT(target, gpio_event_poll);
	NAN_EXPORT(target, gpio_event_clear);
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
