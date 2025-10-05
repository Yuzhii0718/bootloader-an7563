/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#define MAX_CR_LIST		10

/* bit per pin, pin per reg */
#define GPIO_CTRL_BITS		2
#define GPIO_DATA_BITS		1
#define GPIO_OE_BITS		1
#define MAX_GPIO_CTRL_PER_REG	16
#define MAX_GPIO_DATA_PER_REG	32
#define MAX_GPIO_OE_PER_REG	32

/* UBOOT ENV GPIO config setting */
#define MTK_FIP_BASE		0x7C004
#define MTK_FIP_END		0x80000
#define MTK_FIP_READ_LEN	0xA00
#define MTK_GPIO_CONFIG_MAXSIZE 0xA0
#define MKT_GPIO_LOW_SIG	"MTK_GPIO_LOW"
#define MKT_GPIO_HIGH_SIG	"MTK_GPIO_HIGH"

/* GPIO CR */
#define GPIO_BASE		(0x1FBF0200)
#define GPIO_CTRL		(GPIO_BASE + 0x00)
#define GPIO_DATA		(GPIO_BASE + 0x04)
#define GPIO_INT		(GPIO_BASE + 0x08)
#define GPIO_OE			(GPIO_BASE + 0x14)
#define GPIO_LEDPRD		(GPIO_BASE + 0x18)
#define GPIO_LEDCTRL		(GPIO_BASE + 0x1C)
#define GPIO_CTRL1		(GPIO_BASE + 0x20)
#define GPIO_SGPIO_LEDDATA	(GPIO_BASE + 0x24)
#define GPIO_SGPIO_CLKDIVR	(GPIO_BASE + 0x28)
#define GPIO_SGPIO_CLKDLY	(GPIO_BASE + 0x2C)
#define GPIO_SGPIO_CFG		(GPIO_BASE + 0x30)
#define GPIO_FLASH_MODE_CFG	(GPIO_BASE + 0x34)
#define GPIO_RSP_MODE_CFG	(GPIO_BASE + 0x38)
#define GPIO_FLASH_PRD_SET0	(GPIO_BASE + 0x3C)
#define GPIO_FLASH_PRD_SET1	(GPIO_BASE + 0x40)
#define GPIO_FLASH_PRD_SET2	(GPIO_BASE + 0x44)
#define GPIO_FLASH_PRD_SET3	(GPIO_BASE + 0x48)
#define GPIO_FLASH_MAP_CFG0	(GPIO_BASE + 0x4C)
#define GPIO_FLASH_MAP_CFG1	(GPIO_BASE + 0x50)
#define GPIO_FLASH_MODE_CFG_EXT	(GPIO_BASE + 0x68)
#define GPIO_INT1		(GPIO_BASE + 0x7C)
#define CYCLE_CFG_VALUE0	(GPIO_BASE + 0x98)
#define CYCLE_CFG_VALUE1	(GPIO_BASE + 0x9C)
#define AUTO_FLASH_STEP1_0	(GPIO_BASE + 0xA0)
#define AUTO_FLASH_STEP3_2	(GPIO_BASE + 0xA4)
#define AUTO_FLASH_STEP5_4	(GPIO_BASE + 0xA8)
#define AUTO_FLASH_STEP7_6	(GPIO_BASE + 0xAC)
#define AUTO_FLASH_AMP3_0	(GPIO_BASE + 0xB0)
#define AUTO_FLASH_AMP7_4	(GPIO_BASE + 0xB4)
#define AUTO_FLASH_EN		(GPIO_BASE + 0xB8)
#define FAN_EN			(GPIO_BASE + 0xBC)

#define GPIO_CTRL2		(GPIO_BASE + 0x60)
#define GPIO_CTRL3		(GPIO_BASE + 0x64)
#define GPIO_DATA1		(GPIO_BASE + 0x70)
#define GPIO_OE1		(GPIO_BASE + 0x78)

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
	LOW,
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

extern void init_gpio(void);