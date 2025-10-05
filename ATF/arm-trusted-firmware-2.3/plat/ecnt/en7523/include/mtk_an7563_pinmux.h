/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#include <stdbool.h>
#include <stdint.h>
#include <lib/mmio.h>

/* bit per pin, pin per reg */
#define GPIO_FORCE_BITS		1
#define MAX_GPIO_FORCE_PER_REG	32

/* PINMUX base */
#define PINMUX_FORCE_GPIO_BASE		(0x1FA20000)

#define GPIO_FORCE_GPIO0_31	(PINMUX_FORCE_GPIO_BASE + 0x228)
#define GPIO_FORCE_GPIO32_64	(PINMUX_FORCE_GPIO_BASE + 0x22C)

/* FORCE_GPIO cr list, mode */
typedef enum {
	FGPIO_INVALID = -1,
	FGPIO1,
	FGPIO2,
} PINMUX_FGPIO_LST;

typedef enum {
	DISABLE_FORCE_GPIO_MODE,
	ENBALE_FORCE_GPIO_MODE
} PINMUX_FGPIO_MODE;

/* Pin catagory */
typedef enum {
	GPIO_PIN_INVALID = -1,
	GPIO_PIN_0_31,
	GPIO_PIN_32_63,
} GPIO_PIN_ONE_BIT;

extern void an_pinmux_force_gpio(unsigned int pin);