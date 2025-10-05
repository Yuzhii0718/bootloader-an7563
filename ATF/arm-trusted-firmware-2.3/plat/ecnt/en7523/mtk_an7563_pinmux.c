/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <mtk_an7563_pin.h>
#include <mtk_an7563_pinmux.h>

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
	unsigned int cr, bit, val;
	unsigned int mode = ENBALE_FORCE_GPIO_MODE;

	if(pin >= MAX_GPIO_PIN) {
		printf("[MTK] Pin%d doesn't exist! Skip!\n", pin);
		return;
	}

	cr = an_pinmux_get_force_gpio_cr(pin);
	if(cr == GPIO_PIN_INVALID) {
		printf("[MTK] Pin%d CR doesn't exist! Skip!\n", pin);
		return;
	}

	mode &= mask;
	bit = pin % MAX_GPIO_FORCE_PER_REG;

	val = mmio_read_32(cr);

	if (((val >> (bit * GPIO_FORCE_BITS)) & mask) == mode)
		return;

	val &= ~(mask << (bit * GPIO_FORCE_BITS));
	val |= (mode << (bit * GPIO_FORCE_BITS));

	mmio_write_32(cr, val);

	return;
}