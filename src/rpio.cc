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
#include "sunxi.h"

#define BCM2835	0x1
#define SUNXI	0x2

using namespace Nan;

uint8_t type = 0;

/*
 * GPIO function select.  Pass through all values supported by wiringPi.
 */
NAN_METHOD(gpio_function)
{
	if ((info.Length() != 2) ||
	    !info[0]->IsNumber() ||
	    !info[1]->IsNumber() ||
	    (info[1]->NumberValue() > 7))
		return ThrowTypeError("Incorrect arguments");

    uint8_t pin = info[0]->NumberValue();
    uint8_t mode = info[1]->NumberValue();

    if (type == BCM2835) {
        bcm2835_gpio_fsel(pin, mode);
    } else if (type == SUNXI) {
    	sunxi_gpio_fsel(pin, mode);
    }
}

/*
 * GPIO read/write
 */
NAN_METHOD(gpio_read)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint8_t pin = info[0]->NumberValue();
    uint8_t value = 0;

    if (type == BCM2835) {
        value = bcm2835_gpio_lev(pin);
    } else if (type == SUNXI) {
        value = sunxi_gpio_lev(pin);
    }

	info.GetReturnValue().Set(value);
}

NAN_METHOD(gpio_readbuf)
{
	uint32_t i;
	char *buf;

	if ((info.Length() != 3) ||
	    !info[0]->IsNumber() ||
	    !info[1]->IsObject() ||
	    !info[2]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

	buf = node::Buffer::Data(info[1]->ToObject());

    uint8_t pin = info[0]->NumberValue();

	for (i = 0; i < info[2]->NumberValue(); i++)
        if (type == BCM2835) {
            buf[i] = bcm2835_gpio_lev(pin);
        } else if (type == SUNXI) {
            buf[i] = sunxi_gpio_lev(pin);
        }
}

NAN_METHOD(gpio_write)
{
	if ((info.Length() != 2) ||
	    !info[0]->IsNumber() ||
	    !info[1]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

    uint8_t pin = info[0]->NumberValue();
    uint8_t on = info[1]->NumberValue();

    if (type == BCM2835) {
        bcm2835_gpio_write(pin, on);
    } else if (type == SUNXI) {
        sunxi_gpio_write(pin, on);
    }

}

NAN_METHOD(gpio_writebuf)
{
	uint32_t i;
	char *buf;

	if ((info.Length() != 3) ||
	    !info[0]->IsNumber() ||
	    !info[1]->IsObject() ||
	    !info[2]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

	buf = node::Buffer::Data(info[1]->ToObject());

    uint8_t pin = info[0]->NumberValue();

	for (i = 0; i < info[2]->NumberValue(); i++)
        if (type == BCM2835) {
		    bcm2835_gpio_write(pin, buf[i]);
        } else if (type == SUNXI) {
		    sunxi_gpio_write(pin, buf[i]);
        }
}

NAN_METHOD(gpio_pad_read)
{
	if ((info.Length() != 1) || !info[0]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

    uint8_t group = info[0]->NumberValue();
    uint32_t state = 0;

    if (type == BCM2835) {
        state = bcm2835_gpio_pad(group);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support PAD control");
    }

	info.GetReturnValue().Set(state);
}

NAN_METHOD(gpio_pad_write)
{
	if ((info.Length() != 2) ||
	    !info[0]->IsNumber() ||
	    !info[1]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

    uint8_t group = info[0]->NumberValue();
    uint32_t control = info[1]->NumberValue();

    if (type == BCM2835) {
    	bcm2835_gpio_set_pad(group, control);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support PAD control");
    }
}

NAN_METHOD(gpio_pud)
{
	if ((info.Length() != 2) ||
	    !info[0]->IsNumber() ||
	    !info[1]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

    uint8_t pin = info[0]->NumberValue();
    uint8_t pud = info[1]->NumberValue();

    if (type == BCM2835) {
        bcm2835_pullUpDnControl(pin, pud);
    } else if (type == SUNXI) {
        sunxi_pullUpDnControl(pin, pud);
    }
}

NAN_METHOD(gpio_event_set)
{
	if ((info.Length() != 2) ||
	    !info[0]->IsNumber() ||
	    !info[1]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

    uint8_t pin = info[0]->NumberValue();
    uint32_t direction = info[1]->NumberValue();

    if (type == BCM2835) {
        bcm2835_gpio_event_set(pin, direction);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support poll events");
    }

}

NAN_METHOD(gpio_event_poll)
{
	if ((info.Length() != 1) || !info[0]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

    if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support poll events");
    }

	uint32_t rval = 0;

    uint32_t mask = info[0]->NumberValue();

	/*
	 * Interrupts are not supported, so this merely reports that an event
	 * happened in the time period since the last poll.  There is no way to
	 * know which trigger caused the event.
	 */

    if (type == BCM2835) {
    	if ((rval = bcm2835_gpio_eds_multi(mask)))
	    	bcm2835_gpio_set_eds_multi(rval);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support poll events");
    }

	info.GetReturnValue().Set(rval);
}

NAN_METHOD(gpio_event_clear)
{
	if ((info.Length() != 1) || !info[0]->IsNumber())
		return ThrowTypeError("Incorrect arguments");

    uint8_t pin = info[0]->NumberValue();

    if (type == BCM2835) {
    	bcm2835_gpio_clr_fen(pin);
    	bcm2835_gpio_clr_ren(pin);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support poll events");
    }
}

/*
 * i2c setup
 */
NAN_METHOD(i2c_begin)
{
    if (type == BCM2835) {
    	bcm2835_i2c_begin();
    } else if (type == SUNXI) {
        sunxi_i2c_begin(1);
    }
}

NAN_METHOD(i2c_set_clock_divider)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint16_t divider = info[0]->NumberValue();

    if (type == BCM2835) {
    	bcm2835_i2c_setClockDivider(divider);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support i2c clock divider");
    }
}

NAN_METHOD(i2c_set_baudrate)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint32_t baudrate = info[0]->NumberValue();

    if (type == BCM2835) {
    	bcm2835_i2c_set_baudrate(baudrate);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support i2c baudrate");
    }
}

NAN_METHOD(i2c_set_slave_address)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint8_t addr = info[0]->NumberValue();

    if (type == BCM2835) {
	    bcm2835_i2c_setSlaveAddress(addr);
    } else if (type == SUNXI) {
	    sunxi_i2c_setSlaveAddress(addr);
    }
}

NAN_METHOD(i2c_end)
{
    if (type == BCM2835) {
    	bcm2835_i2c_end();
    } else if (type == SUNXI) {
        // no implementation for sunxi
    }
}


/*
 * i2c read/write.  The underlying bcm2835_i2c_read/bcm2835_i2c_write functions
 * do not return the number of bytes read/written, only a status code.  The JS
 * layer handles ensuring that the buffer is large enough to accommodate the
 * requested length.
 */
NAN_METHOD(i2c_read)
{
	uint8_t rval = 0;

	if ((info.Length() != 2) ||
	    (!info[0]->IsObject()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    char* buf = node::Buffer::Data(info[0]->ToObject());
    uint32_t len = info[1]->NumberValue();

    if (type == BCM2835) {
    	rval = bcm2835_i2c_read(buf, len);
    } else if (type == SUNXI) {
    	rval = sunxi_i2c_read(buf, len);
    }

	info.GetReturnValue().Set(rval);
}

NAN_METHOD(i2c_write)
{
	uint8_t rval;

	if ((info.Length() != 2) ||
	    (!info[0]->IsObject()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    char* buf = node::Buffer::Data(info[0]->ToObject());
    uint32_t len = info[1]->NumberValue();

    if (type == BCM2835) {
        rval = bcm2835_i2c_write(buf, len);
    } else if (type == SUNXI) {
    	rval = sunxi_i2c_write(buf, len);
    }

	info.GetReturnValue().Set(rval);
}

/*
 * PWM functions
 */
NAN_METHOD(pwm_set_clock)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint32_t divisor = info[0]->NumberValue();
    if (type == BCM2835) {
    	bcm2835_pwm_set_clock(divisor);
    } else if (type == SUNXI) {
    	sunxi_pwm_set_clock(divisor);
    }
}

NAN_METHOD(pwm_set_mode)
{
	if ((info.Length() != 3) ||
	    (!info[0]->IsNumber()) ||
	    (!info[1]->IsNumber()) ||
	    (!info[2]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint8_t channel = info[0]->NumberValue();
    uint8_t markspace = info[1]->NumberValue();
    uint8_t enabled = info[2]->NumberValue();

    if (type == BCM2835) {
	    bcm2835_pwm_set_mode(channel, markspace, enabled);
    } else if (type == SUNXI) {
    	sunxi_pwm_set_mode(channel, markspace, enabled);
    }
}

NAN_METHOD(pwm_set_range)
{
	if ((info.Length() != 2) ||
	    (!info[0]->IsNumber()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint8_t channel = info[0]->NumberValue();
    uint32_t range = info[1]->NumberValue();

    if (type == BCM2835) {
	    bcm2835_pwm_set_range(channel, range);
    } else if (type == SUNXI) {
    	sunxi_pwm_set_range(range);
    }
}

NAN_METHOD(pwm_set_data)
{
	if ((info.Length() != 2) ||
	    (!info[0]->IsNumber()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint8_t channel = info[0]->NumberValue();
    uint32_t data = info[1]->NumberValue();

    if (type == BCM2835) {
    	bcm2835_pwm_set_data(channel, data);
    } else if (type == SUNXI) {
    	sunxi_pwm_set_data(data);
    }
}

/*
 * SPI functions.
 */
NAN_METHOD(spi_begin)
{
    if (type == BCM2835) {
    	bcm2835_spi_begin();
    } else if (type == SUNXI) {
    	sunxi_spi_begin();
    }
}

NAN_METHOD(spi_chip_select)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint8_t cs = info[0]->NumberValue();

    if (type == BCM2835) {
	    bcm2835_spi_chipSelect(cs);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support selecting chips");
    }
}

NAN_METHOD(spi_set_cs_polarity)
{
	if ((info.Length() != 2) ||
	    (!info[0]->IsNumber()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint8_t cs = info[0]->NumberValue();
    uint8_t active = info[1]->NumberValue();

    if (type == BCM2835) {
	    bcm2835_spi_setChipSelectPolarity(cs, active);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support polarity selections");
    }
}

NAN_METHOD(spi_set_clock_divider)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint16_t divider = info[0]->NumberValue();

    if (type == BCM2835) {
	    bcm2835_spi_setClockDivider(divider);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support clock divider");
    }
}

NAN_METHOD(spi_set_data_mode)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint8_t mode = info[0]->NumberValue();

    if (type == BCM2835) {
    	bcm2835_spi_setDataMode(mode);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support changing mode");
    }
}

NAN_METHOD(spi_transfer)
{
	if ((info.Length() != 3) ||
	    (!info[0]->IsObject()) ||
	    (!info[1]->IsObject()) ||
	    (!info[2]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    char* tbuf = node::Buffer::Data(info[0]->ToObject());
    char* rbuf = node::Buffer::Data(info[1]->ToObject());
    uint32_t len = info[2]->NumberValue();

    if (type == BCM2835) {
	    bcm2835_spi_transfernb(tbuf, rbuf, len);
    } else if (type == SUNXI) {
	    sunxi_spi_transfernb(tbuf, rbuf, len);
    }
}

NAN_METHOD(spi_write)
{
	if ((info.Length() != 2) ||
	    (!info[0]->IsObject()) ||
	    (!info[1]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    char* tbuf = node::Buffer::Data(info[0]->ToObject());
    uint32_t len = info[1]->NumberValue();

    if (type == BCM2835) {
	    bcm2835_spi_writenb(tbuf, len);
    } else if (type == SUNXI) {
		return ThrowTypeError("SUNXI doesn't support writing only mode");
    }
}

NAN_METHOD(spi_end)
{
    if (type == BCM2835) {
    	bcm2835_spi_end();
    } else if (type == SUNXI) {
    	//not implemented yet
    }
}

/*
 * Initialize the bcm2835 interface and check we have permission to access it.
 */
NAN_METHOD(rpio_init)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

    uint8_t gpiomem = info[0]->NumberValue();
    if (gpiomem<2) {
        type = BCM2835;
        if (!bcm2835_init(gpiomem))
		    return ThrowError("Could not initialize bcm2835 GPIO library");
    } else {
        type = SUNXI;
        if (!sunxi_init(gpiomem-2))
		    return ThrowError("Could not initialize sunxi GPIO library");
    }
}

NAN_METHOD(rpio_close)
{
    if (type == BCM2835) {
    	bcm2835_close();
    } else if (type == SUNXI) {
    	//sunxi doesn't implement this
    }
}

/*
 * Misc functions useful for simplicity
 */
NAN_METHOD(rpio_usleep)
{
	if ((info.Length() != 1) || (!info[0]->IsNumber()))
		return ThrowTypeError("Incorrect arguments");

	usleep(info[0]->NumberValue());
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
