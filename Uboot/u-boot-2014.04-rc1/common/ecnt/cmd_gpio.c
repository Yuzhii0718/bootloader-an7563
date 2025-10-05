/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#include <common.h>
#include <command.h>
#include <ecnt/gpio/en7523_pin.h>
#include <ecnt/gpio/en7523_evb_mtk_gpio.h>
#include <ecnt/gpio/en7523_evb_mtk_pinmux.h>

static int gpio_command(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long pin, val;

	if (!strncmp(argv[1], "set", 4)) {
		if(argc != 4)
			return CMD_RET_USAGE;

		pin = simple_strtoul(argv[2], NULL, 10);
		val = simple_strtoul(argv[3], NULL, 10);

		/* Check val */
		if (val != HIGH && val != LOW)
			goto INVALID_CMD;

		/* Check pin */
		if (pin >= MAX_GPIO_PIN)
			goto INVALID_CMD;

		an_gpio_set_value(pin, val);
	} else if (!strncmp(argv[1], "get", 3)) {
		if(argc != 3)
			return CMD_RET_USAGE;

		pin = simple_strtoul(argv[2], NULL, 10);

		/* Check pin */
		if (pin >= MAX_GPIO_PIN)
			goto INVALID_CMD;

		val = an_gpio_get_value(pin);
		printf("GPIO%d input val = %d\n", pin, val);
	} else {
		return CMD_RET_USAGE;
	}

	return 0;

INVALID_CMD:
	printf("WARNING: Cannot set GPIO%d to %d, skip setting!\n", pin, val);
	return CMD_RET_USAGE;
}

U_BOOT_CMD(
		gpio,   4,      0,      gpio_command,
		"gpio command\n",
		"usage:\n"
		"	gpio set <pin> <val>\n"
		"		,<pin> : 0 ~ 35\n"
		"		,<val> : 0 or 1\n"
		"	gpio get <pin>\n"
		"		,<pin> : 0~35\n"
);
