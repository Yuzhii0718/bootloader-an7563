/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

 #define PINMUX_FORCE_GPIO_BASE		(0x1FA20228)

/* GPIO CR */
#define GPIO_BASE		(0x1FBF0200)
#define GPIO_CTRL		(GPIO_BASE + 0x00)
#define GPIO_DATA		(GPIO_BASE + 0x04)
#define GPIO_INT		(GPIO_BASE + 0x08)
#define GPIO_OE			(GPIO_BASE + 0x14)
#define GPIO_LEDPRD		(GPIO_BASE + 0x18)
#define GPIO_LEDCTRL		(GPIO_BASE + 0x1C)
#define GPIO_CTRL1		(GPIO_BASE + 0x20)

#define GPIO_CTRL2		(GPIO_BASE + 0x60)
#define GPIO_CTRL3		(GPIO_BASE + 0x64)
#define GPIO_DATA1		(GPIO_BASE + 0x70)
#define GPIO_OE1		(GPIO_BASE + 0x78)
#define GPIO_CTRL1		(GPIO_BASE + 0x20)

/* bit per pin, pin per reg */
#define GPIO_CTRL_BITS		2
#define GPIO_DATA_BITS		1
#define GPIO_OE_BITS		1
#define MAX_GPIO_CTRL_PER_REG	16
#define MAX_GPIO_DATA_PER_REG	32
#define MAX_GPIO_OE_PER_REG	32

/* GPIO_CTRL CR LIST */
typedef enum {
	CTRL_INVALID = -1,
	CTRL,
	CTRL1,
	CTRL2,
	CTRL3,
} GPIO_CTRL_LST;

/* GPIO_DATA CR LIST */
typedef enum {
	DATA_INVALID = -1,
	DATA,
	DATA1,
} GPIO_DATA_LST;

/* GPIO_OE CR LIST */
typedef enum {
	OE_INVALID = -1,
	OE,
	OE1,
} GPIO_OE_LST;

/* GPIO_MODE LOW/HIGH */
typedef enum {
	LOW = 0,
	HIGH,
	MAX_PULL_MODE
} AN_GPIO_PULL_MODE;

/* GPIO_MODE input, output */
typedef enum {
	CTRL_INPUT,
	CTRL_OUTPUT,
	MAX_GPIO_CTRL
} AN_GPIO_CTRL;

/* GPIO_MODE OE */
typedef enum {
	OE_DISABLE,
	OE_ENABLE
} GPIOMODE_OE;

unsigned int an_gpio_get_value(unsigned int pin);
void an_gpio_set_value(unsigned int pin, unsigned val);
void ecnt_mtk_gpio_init(void);