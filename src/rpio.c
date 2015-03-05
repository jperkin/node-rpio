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

#include <unistd.h>

#include "bcm2835.h"
#include "shim.h"

/*
 * Function select.  Currently supported:
 *
 *	BCM2835_GPIO_FSEL_INPT = 0b000
 *	BCM2835_GPIO_FSEL_OUTP = 0b001
 *
 * None of the alternate functions are currently supported.
 *
 */
static int
pin_function(shim_ctx_t* ctx, shim_args_t* args)
{
	uint8_t pin, function;

	if (!shim_unpack(ctx, args,
	    SHIM_TYPE_UINT32, &pin,
	    SHIM_TYPE_UINT32, &function,
	    SHIM_TYPE_UNKNOWN)) {
		shim_throw_error(ctx, "Incorrect arguments");
		return FALSE;
	}

	switch (function) {
	case 0:
		bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
		break;
	case 1:
		bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
		break;
	default:
		shim_throw_error(ctx, "Unsupported function select");
		return FALSE;
	}

	return TRUE;
}

/*
 * Read from a pin
 */
static int
pin_read(shim_ctx_t* ctx, shim_args_t* args)
{
	uint8_t pin, rval;

	if (!shim_unpack(ctx, args,
	    SHIM_TYPE_UINT32, &pin,
	    SHIM_TYPE_UNKNOWN)) {
		shim_throw_error(ctx, "Incorrect arguments");
		return FALSE;
	}

	rval = bcm2835_gpio_lev(pin);

	shim_args_set_rval(ctx, args, shim_integer_new(ctx, rval));

	return TRUE;
}

/*
 * Write to a pin
 */
static int
pin_write(shim_ctx_t* ctx, shim_args_t* args)
{
	uint8_t pin, on;

	if (!shim_unpack(ctx, args,
	    SHIM_TYPE_UINT32, &pin,
	    SHIM_TYPE_UINT32, &on,
	    SHIM_TYPE_UNKNOWN)) {
		shim_throw_error(ctx, "Incorrect arguments");
		return FALSE;
	}

	bcm2835_gpio_write(pin, on);

	return TRUE;
}

/*
 * Initialize the bcm2835 interface and check we have permission to access it.
 */
static int
init(shim_ctx_t* ctx, shim_args_t* args)
{
	if (geteuid() != 0) {
		shim_throw_error(ctx, "You must be root to access GPIO");
		return FALSE;
	}

	if (!bcm2835_init()) {
		shim_throw_error(ctx, "Could not initialize GPIO");
		return FALSE;
	}

	return TRUE;
}

int
setup(shim_ctx_t* ctx, shim_val_t* exports, shim_val_t* module)
{
	shim_fspec_t funcs[] = {
		SHIM_FS(init),
		SHIM_FS(pin_function),
		SHIM_FS(pin_read),
		SHIM_FS(pin_write),
		SHIM_FS_END
	};

	shim_obj_set_funcs(ctx, exports, funcs);

	return TRUE;
}

SHIM_MODULE(rpio, setup)
