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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "sunxi.h"

/*
 * The GPIO Port Controller is not aligned to a 4K page address, so we map in
 * the full page and calculate the offset.
 */
#define SUNXI_GPIO_BASE		0x01C20000
#define SUNXI_GPIO_OFFSET	0x00000800	/* 0x01C20800 - 0x01C20BFF */
#define SUNXI_GPIO_SIZE		0x00001000
#define SUNXI_PORT_SIZE		0x00000024	/* Port configuration size */

/*
 * The Port Controller is split into 9 ports (PA to PI).  Each port is
 * configured at the various offsets listed below.  These are also used as
 * offset input arguments for sunxi_port_regaddr().
 */
typedef enum {
	SUNXI_PORT_OFFSET_CFG = 0x00000000,	/* 4 configuration registers */
	SUNXI_PORT_OFFSET_DAT = 0x00000010,	/* 1 data register */
	SUNXI_PORT_OFFSET_DRV = 0x00000014,	/* 2 drive registers */
	SUNXI_PORT_OFFSET_PUL = 0x0000001C	/* 2 pull-up / pull-down regs */
} sunxi_port_offset_t;

/*
 * Drive level.
 */
#define SUNXI_DRIVE_0		0x00
#define SUNXI_DRIVE_1		0x01
#define SUNXI_DRIVE_2		0x02
#define SUNXI_DRIVE_3		0x03

/*
 * Internal pull-up/pull-down
 */
typedef enum {
	SUNXI_PULL_DISABLE =	0x00,
	SUNXI_PULL_UP =		0x01,
	SUNXI_PULL_DOWN =	0x02
} sunxi_pud_t;

/*
 * Supported functions selected by the configuration registers.
 */
#define SUNXI_FSEL_INPUT	0x00
#define SUNXI_FSEL_OUTPUT	0x01
#define SUNXI_FSEL_ALT0		0x02
#define SUNXI_FSEL_ALT1		0x03
#define SUNXI_FSEL_ALT2		0x04
#define SUNXI_FSEL_ALT3		0x05
#define SUNXI_FSEL_ALT4		0x06
#define SUNXI_FSEL_MASK		0x07

/*
 * Macros for calculating pin position and offsets across the port
 * configuration registers.
 */
#define SUNXI_PIN_PORT(pin)	(pin >> 5)
#define SUNXI_PIN_INDEX(pin)	(pin - (pin & 0xFFFFFFE0))
#define SUNXI_PIN_SELECT(pin)	((SUNXI_PIN_INDEX(pin) & 0x7) << 2)

/*
 * Offsets from base of each SUNXI_PORT_OFFSET_*
 */
#define SUNXI_REG_OFFSET_CFG(pin)	((SUNXI_PIN_INDEX(pin) >> 3) << 2)
#define SUNXI_REG_OFFSET_DRV(pin)	(SUNXI_PIN_INDEX(pin) << 2)
#define SUNXI_REG_OFFSET_PUL(pin)	(SUNXI_PIN_INDEX(pin) << 2)

volatile uint32_t *sunxi_gpio = (uint32_t *)MAP_FAILED;

/*
 * Return the register address for port configuration of a selected pin and
 * register combination.
 */
static uint32_t
sunxi_port_regaddr(uint32_t pin, sunxi_port_offset_t regoff)
{
	uint32_t addr;

	/*
	 * Start at the base of the GPIO port configuration area for the
	 * selected pin.
	 */
	addr = (uint32_t)sunxi_gpio + SUNXI_GPIO_OFFSET
		+ (SUNXI_PIN_PORT(pin) * SUNXI_PORT_SIZE);

	/*
	 * Calculate the remaining offset to add for each of the supported
	 * register areas.
	 */
	switch (regoff) {
	case SUNXI_PORT_OFFSET_CFG:
		addr += SUNXI_PORT_OFFSET_CFG + SUNXI_REG_OFFSET_CFG(pin);
		break;
	case SUNXI_PORT_OFFSET_DAT:
		/* Only 1 data register, no further offset required */
		addr += SUNXI_PORT_OFFSET_DAT;
		break;
	case SUNXI_PORT_OFFSET_DRV:
		addr += SUNXI_PORT_OFFSET_DRV + SUNXI_REG_OFFSET_DRV(pin);
		break;
	case SUNXI_PORT_OFFSET_PUL:
		addr += SUNXI_PORT_OFFSET_PUL + SUNXI_REG_OFFSET_PUL(pin);
		break;
	}

	return addr;
}

/*
 * Modelled on the bcm2835 equivalents, just without the debug.  The same rules
 * apply, the *_nb variants must only be used if the next access is to the same
 * peripheral.
 */
static uint32_t
sunxi_peri_read(volatile uint32_t *paddr)
{
	uint32_t val;

	__sync_synchronize();
	val = *paddr;
	__sync_synchronize();

	return val;
}

static void
sunxi_peri_write(volatile uint32_t *paddr, uint32_t val)
{
	__sync_synchronize();
	*paddr = val;
	__sync_synchronize();
}

#ifdef NOTYET
static uint32_t
sunxi_peri_read_nb(volatile uint32_t *paddr)
{
	uint32_t val;

	val = *paddr;

	return val;
}

static void
sunxi_peri_write_nb(volatile uint32_t *paddr, uint32_t val)
{
	*paddr = val;
}
#endif

/*
 * Read/Set/Clear pin
 */
uint8_t
sunxi_gpio_lev(uint32_t pin)
{
	volatile uint32_t *paddr;
	uint32_t value;

	paddr = (uint32_t *)sunxi_port_regaddr(pin, SUNXI_PORT_OFFSET_DAT);
	value = sunxi_peri_read(paddr);

	return (value & (1 << (pin % 32))) ? 1 : 0;
}

void
sunxi_gpio_set(uint32_t pin)
{
	volatile uint32_t *paddr;
	uint32_t value;

	paddr = (uint32_t *)sunxi_port_regaddr(pin, SUNXI_PORT_OFFSET_DAT);
	value = sunxi_peri_read(paddr);

	sunxi_peri_write(paddr, (value | (1 << (pin % 32))));
}

void
sunxi_gpio_clr(uint32_t pin)
{
	volatile uint32_t *paddr;
	uint32_t value;

	paddr = (uint32_t *)sunxi_port_regaddr(pin, SUNXI_PORT_OFFSET_DAT);
	value = sunxi_peri_read(paddr);

	sunxi_peri_write(paddr, (value & ~(1 << (pin % 32))));
}

/*
 * Pull-up / pull-down register
 */
void
sunxi_gpio_set_pud(uint32_t pin, uint8_t status)
{
	volatile uint32_t *paddr;
	uint32_t value;

	paddr = (uint32_t *)sunxi_port_regaddr(pin, SUNXI_PORT_OFFSET_PUL);
	value = sunxi_peri_read(paddr);

	sunxi_peri_write(paddr, (value & ~(1 << (pin % 32))));
}

/*
 * Function select
 */
void
sunxi_gpio_fsel(uint32_t pin, uint8_t mode)
{
	volatile uint32_t *paddr;
	uint32_t value;

	/*
	 * Read from configuration register for the selected pin, then clear
	 * function select bits for that pin.
	 */
	paddr = (uint32_t *)sunxi_port_regaddr(pin, SUNXI_PORT_OFFSET_CFG);
	value = sunxi_peri_read(paddr);
	value &= ~(SUNXI_FSEL_MASK << SUNXI_PIN_SELECT(pin));

	switch (mode) {
	case SUNXI_FSEL_INPUT:
		sunxi_peri_write(paddr, value);
		break;
	case SUNXI_FSEL_OUTPUT:
		value |= (1 << SUNXI_PIN_SELECT(pin));
		sunxi_peri_write(paddr, value);
		break;
	default:
		fprintf(stderr, "mode %d unsupported\n", mode);
		break;
	}
}

/*
 * Convenience functions.
 */
void
sunxi_gpio_write(uint32_t pin, uint32_t on)
{
	if (on)
		sunxi_gpio_set(pin);
	else
		sunxi_gpio_clr(pin);
}

int
sunxi_init(int unused)
{
	int fd;

	if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0) {
		fprintf(stderr, "sunxi_init: open /dev/mem failed: %s\n",
		    strerror(errno));
		return 0;
	}

	sunxi_gpio = (uint32_t *)mmap(0, SUNXI_GPIO_SIZE,
	    PROT_READ | PROT_WRITE, MAP_SHARED, fd, SUNXI_GPIO_BASE);

	if (sunxi_gpio == MAP_FAILED) {
		fprintf(stderr, "sunxi_init: mmap failed: %s\n",
		    strerror(errno));
		return 0;
	}

	return 1;
}
