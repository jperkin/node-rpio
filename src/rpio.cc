/*
 * Copyright (c) 2020 Jonathan Perkin <jonathan@perkin.org.uk>
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

/*
 * This addon only makes sense on Linux, given that the bcm2835 library
 * requires /dev/{,gpio}mem support and /proc is used to determine hardware.
 *
 * We still build it as an empty addon on other platforms, however, so that
 * mock mode works for application logic testing.
 */
#if defined(__linux__)

#include <unistd.h>	/* usleep() */
#include "bcm2835.h"
#include "sunxi.h"

#define RPIO_EVENT_LOW	0x1
#define RPIO_EVENT_HIGH	0x2

/* Avoid writing these monstrosities everywhere */
#define IS_OBJ(i)	info[i]->IsObject()
#define IS_U32(i)	info[i]->IsUint32()
#define FROM_OBJ(i) \
	node::Buffer::Data(Nan::To<v8::Object>(info[i]).ToLocalChecked())
#define FROM_U32(i)	Nan::To<uint32_t>(info[i]).FromJust()
#define NAN_ARGC	info.Length()
#define NAN_RETURN	info.GetReturnValue().Set

#define ASSERT_ARGC1(t0)						\
	do {								\
		if (NAN_ARGC != 1)					\
			return ThrowTypeError("Invalid argc");		\
		if (!t0(0))						\
			return ThrowTypeError("Invalid arg1");		\
	} while (0)

#define ASSERT_ARGC2(t0, t1)						\
	do {								\
		if (NAN_ARGC != 2)					\
			return ThrowTypeError("Invalid argc");		\
		if (!t0(0))						\
			return ThrowTypeError("Invalid arg1");		\
		if (!t1(1))						\
			return ThrowTypeError("Invalid arg2");		\
	} while (0)

#define ASSERT_ARGC3(t0, t1, t2)					\
	do {								\
		if (NAN_ARGC != 3)					\
			return ThrowTypeError("Invalid argc");		\
		if (!t0(0))						\
			return ThrowTypeError("Invalid arg1");		\
		if (!t1(1))						\
			return ThrowTypeError("Invalid arg2");		\
		if (!t2(2))						\
			return ThrowTypeError("Invalid arg3");		\
	} while (0)

#define ASSERT_ARGC4(t0, t1, t2, t3)					\
	do {								\
		if (NAN_ARGC != 4)					\
			return ThrowTypeError("Invalid argc");		\
		if (!t0(0))						\
			return ThrowTypeError("Invalid arg1");		\
		if (!t1(1))						\
			return ThrowTypeError("Invalid arg2");		\
		if (!t2(2))						\
			return ThrowTypeError("Invalid arg3");		\
		if (!t3(3))						\
			return ThrowTypeError("Invalid arg4");		\
	} while (0)

using namespace Nan;

#define RPIO_SOC_BCM2835	0x0
#define RPIO_SOC_SUNXI		0x1

uint32_t soctype = RPIO_SOC_BCM2835;

/*
 * GPIO function select.
 */
NAN_METHOD(gpio_function)
{
	ASSERT_ARGC2(IS_U32, IS_U32);

	uint32_t pin = FROM_U32(0);
	uint32_t mode = FROM_U32(1);

	switch (soctype) {
	case RPIO_SOC_BCM2835:
		bcm2835_gpio_fsel(pin, mode);
		break;
	case RPIO_SOC_SUNXI:
		sunxi_gpio_fsel(pin, mode);
		break;
	}
}

/*
 * GPIO read/write
 */
NAN_METHOD(gpio_read)
{
	ASSERT_ARGC2(IS_U32, IS_U32);

	uint32_t pin = FROM_U32(0);
	uint32_t mode = FROM_U32(1);


	switch (soctype) {
	case RPIO_SOC_BCM2835:
		if (mode) {
			bcm2835_gpio_fsel(pin, 0);
		}
		NAN_RETURN(bcm2835_gpio_lev(pin));
		break;
	case RPIO_SOC_SUNXI:
		if (mode) {
			sunxi_gpio_fsel(pin, 0);
		}
		NAN_RETURN(sunxi_gpio_lev(pin));
		break;
	}
}

NAN_METHOD(gpio_readbuf)
{
	ASSERT_ARGC4(IS_U32, IS_OBJ, IS_U32, IS_U32);

	uint32_t pin = FROM_U32(0);
	char *buf = FROM_OBJ(1);
	uint32_t len = FROM_U32(2);
	uint32_t mode = FROM_U32(3);

	switch (soctype) {
	case RPIO_SOC_BCM2835:
		if (mode) {
			bcm2835_gpio_fsel(pin, 0);
		}
		for (uint32_t i = 0; i < len; i++) {
			buf[i] = bcm2835_gpio_lev(pin);
		}
		break;
	case RPIO_SOC_SUNXI:
		if (mode) {
			sunxi_gpio_fsel(pin, 0);
		}
		for (uint32_t i = 0; i < len; i++) {
			buf[i] = sunxi_gpio_lev(pin);
		}
		break;
	}
}

NAN_METHOD(gpio_write)
{
	ASSERT_ARGC2(IS_U32, IS_U32);

	uint32_t pin = FROM_U32(0);
	uint32_t val = FROM_U32(1);

	switch (soctype) {
	case RPIO_SOC_BCM2835:
		bcm2835_gpio_write(pin, val);
		break;
	case RPIO_SOC_SUNXI:
		sunxi_gpio_write(pin, val);
		break;
	}

	NAN_RETURN(val);
}

NAN_METHOD(gpio_writebuf)
{
	ASSERT_ARGC3(IS_U32, IS_OBJ, IS_U32);

	uint32_t pin = FROM_U32(0);
	char *buf = FROM_OBJ(1);
	uint32_t len = FROM_U32(2);

	switch (soctype) {
	case RPIO_SOC_BCM2835:
		for (uint32_t i = 0; i < len; i++) {
			bcm2835_gpio_write(pin, buf[i]);
		}
		break;
	case RPIO_SOC_SUNXI:
		for (uint32_t i = 0; i < len; i++) {
			sunxi_gpio_write(pin, buf[i]);
		}
		break;
	}
}

NAN_METHOD(gpio_pad_read)
{
	ASSERT_ARGC1(IS_U32);

	uint32_t group = FROM_U32(0);

	NAN_RETURN(bcm2835_gpio_pad(group));
}

NAN_METHOD(gpio_pad_write)
{
	ASSERT_ARGC2(IS_U32, IS_U32);

	uint32_t group = FROM_U32(0);
	uint32_t control = FROM_U32(1);

	bcm2835_gpio_set_pad(group, control);
}

NAN_METHOD(gpio_pud)
{
	ASSERT_ARGC2(IS_U32, IS_U32);

	uint32_t pin = FROM_U32(0);
	uint32_t pud = FROM_U32(1);

	switch (soctype) {
	case RPIO_SOC_BCM2835:
		bcm2835_gpio_set_pud(pin, pud);
		break;
	case RPIO_SOC_SUNXI:
		sunxi_gpio_set_pud(pin, pud);
		break;
	}
}

NAN_METHOD(gpio_event_set)
{
	ASSERT_ARGC2(IS_U32, IS_U32);

	uint32_t pin = FROM_U32(0);
	uint32_t direction = FROM_U32(1);

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
	if (direction & RPIO_EVENT_HIGH) {
		bcm2835_gpio_ren(pin);
	}

	if (direction & RPIO_EVENT_LOW) {
		bcm2835_gpio_fen(pin);
	}
}

NAN_METHOD(gpio_event_poll)
{
	ASSERT_ARGC1(IS_U32);

	uint32_t rval = 0;
	uint32_t mask = FROM_U32(0);

	/*
	 * Interrupts are not supported, so this merely reports that an event
	 * happened in the time period since the last poll.  There is no way to
	 * know which trigger caused the event.
	 */
	if ((rval = bcm2835_gpio_eds_multi(mask))) {
		bcm2835_gpio_set_eds_multi(rval);
	}

	NAN_RETURN(rval);
}

NAN_METHOD(gpio_event_clear)
{
	ASSERT_ARGC1(IS_U32);

	uint32_t pin = FROM_U32(0);

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
	ASSERT_ARGC1(IS_U32);

	uint32_t divider = FROM_U32(0);

	bcm2835_i2c_setClockDivider(divider);
}

NAN_METHOD(i2c_set_baudrate)
{
	ASSERT_ARGC1(IS_U32);

	uint32_t baudrate = FROM_U32(0);

	bcm2835_i2c_set_baudrate(baudrate);
}

NAN_METHOD(i2c_set_slave_address)
{
	ASSERT_ARGC1(IS_U32);

	uint32_t addr = FROM_U32(0);

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
	ASSERT_ARGC2(IS_OBJ, IS_U32);

	char *buf = FROM_OBJ(0);
	uint32_t len = FROM_U32(1);

	NAN_RETURN(bcm2835_i2c_read(buf, len));
}

NAN_METHOD(i2c_read_register_rs)
{
	ASSERT_ARGC3(IS_OBJ, IS_OBJ, IS_U32);

	char *reg = FROM_OBJ(0);
	char *buf = FROM_OBJ(1);
	uint32_t len = FROM_U32(2);

	NAN_RETURN(bcm2835_i2c_read_register_rs(reg, buf, len));
}

NAN_METHOD(i2c_write_read_rs)
{
	ASSERT_ARGC4(IS_OBJ, IS_U32, IS_OBJ, IS_U32);

	char *cmds = FROM_OBJ(0);
	uint32_t cmdlen = FROM_U32(1);
	char *buf = FROM_OBJ(2);
	uint32_t buflen = FROM_U32(3);

	NAN_RETURN(bcm2835_i2c_write_read_rs(cmds, cmdlen, buf, buflen));
}

NAN_METHOD(i2c_write)
{
	ASSERT_ARGC2(IS_OBJ, IS_U32);

	char *buf = FROM_OBJ(0);
	uint32_t len = FROM_U32(1);

	NAN_RETURN(bcm2835_i2c_write(buf, len));
}

/*
 * PWM functions
 */
NAN_METHOD(pwm_set_clock)
{
	ASSERT_ARGC1(IS_U32);

	uint32_t divisor = FROM_U32(0);

	bcm2835_pwm_set_clock(divisor);
}

NAN_METHOD(pwm_set_mode)
{
	ASSERT_ARGC3(IS_U32, IS_U32, IS_U32);

	uint32_t channel = FROM_U32(0);
	uint32_t markspace = FROM_U32(1);
	uint32_t enabled = FROM_U32(2);

	bcm2835_pwm_set_mode(channel, markspace, enabled);
}

NAN_METHOD(pwm_set_range)
{
	ASSERT_ARGC2(IS_U32, IS_U32);

	uint32_t channel = FROM_U32(0);
	uint32_t range = FROM_U32(1);

	bcm2835_pwm_set_range(channel, range);
}

NAN_METHOD(pwm_set_data)
{
	ASSERT_ARGC2(IS_U32, IS_U32);

	uint32_t channel = FROM_U32(0);
	uint32_t data = FROM_U32(1);

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
	ASSERT_ARGC1(IS_U32);

	uint32_t cs = FROM_U32(0);

	bcm2835_spi_chipSelect(cs);
}

NAN_METHOD(spi_set_cs_polarity)
{
	ASSERT_ARGC2(IS_U32, IS_U32);

	uint32_t cs = FROM_U32(0);
	uint32_t active = FROM_U32(1);

	bcm2835_spi_setChipSelectPolarity(cs, active);
}

NAN_METHOD(spi_set_clock_divider)
{
	ASSERT_ARGC1(IS_U32);

	uint32_t divider = FROM_U32(0);

	bcm2835_spi_setClockDivider(divider);
}

NAN_METHOD(spi_set_data_mode)
{
	ASSERT_ARGC1(IS_U32);

	uint32_t mode = FROM_U32(0);

	bcm2835_spi_setDataMode(mode);
}

NAN_METHOD(spi_transfer)
{
	ASSERT_ARGC3(IS_OBJ, IS_OBJ, IS_U32);

	char *tbuf = FROM_OBJ(0);
	char *rbuf = FROM_OBJ(1);
	uint32_t len = FROM_U32(2);

	bcm2835_spi_transfernb(tbuf, rbuf, len);
}

NAN_METHOD(spi_write)
{
	ASSERT_ARGC2(IS_OBJ, IS_U32);

	char *buf = FROM_OBJ(0);
	uint32_t len = FROM_U32(1);

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
	ASSERT_ARGC2(IS_U32, IS_U32);

	soctype = FROM_U32(0);
	uint32_t gpiomem = FROM_U32(1);

	switch (soctype) {
	case RPIO_SOC_BCM2835:
		if (!bcm2835_init(gpiomem)) {
			return ThrowError("Could not initialize bcm2835");
		}
		break;
	case RPIO_SOC_SUNXI:
		if (!sunxi_init(gpiomem)) {
			return ThrowError("Could not initialize sunxi");
		}
		break;
	}
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
	ASSERT_ARGC1(IS_U32);

	uint32_t microseconds = FROM_U32(0);

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
	NAN_EXPORT(target, i2c_write_read_rs);
	NAN_EXPORT(target, i2c_read_register_rs);
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

#else /* __linux__ */

/*
 * On non-Linux this is just a dummy addon.
 */
NAN_MODULE_INIT(setup)
{
}

#endif

NODE_MODULE(rpio, setup)
