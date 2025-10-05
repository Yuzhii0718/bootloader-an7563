/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#include <asm/tc3162.h>
#include <asm/io.h>
#include <common.h>
#include <ecnt/gpio/en7523_evb_mtk_pinmux.h>
#include <ecnt/gpio/en7523_pin.h>

unsigned int an_pinmux_get_force_gpio_cr(unsigned int pin) {
	unsigned int ret = GPIO_PIN_INVALID;
	unsigned int pos;

	pos = pin / MAX_GPIO_FORCE_PER_REG;

	switch(pos) {
		case GPIO_PIN_0_31:
			ret = GPIO_FORCE_GPIO0_31;
			break;
		case GPIO_PIN_32_63:
			ret = GPIO_FORCE_GPIO32_64;
			break;
		default:
			break;
	}

	return ret;
}

void an_pinmux_force_gpio(unsigned int pin) {
	unsigned int mask = (1L << GPIO_FORCE_BITS) - 1;
	unsigned int mode = ENBALE_FORCE_GPIO_MODE;
	unsigned int cr, bit, val;

	cr = an_pinmux_get_force_gpio_cr(pin);
	if(cr == GPIO_PIN_INVALID) {
		printf("[MTK] Pin%d CR doesn't exist! Skip!\n", pin);
		return;
	}

	mode &= mask;
	bit = pin % MAX_GPIO_FORCE_PER_REG;

	val = regRead32(cr);

	if (((val >> (bit * GPIO_FORCE_BITS)) & mask) == mode)
		return;

	val &= ~(mask << (bit * GPIO_FORCE_BITS));
	val |= (mode << (bit * GPIO_FORCE_BITS));

	regWrite32(cr, val);

	return;
}