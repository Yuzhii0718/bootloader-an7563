/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <flashhal.h>
#include <lib/mmio.h>
#include <mtk_an7563_pin.h>
#include <mtk_an7563_gpio.h>
#include <mtk_an7563_pinmux.h>
#include <mtk_an7563_gpioconfig_parser.h>

unsigned int gpio_get_ctrl_cr(unsigned int pin)
{
	unsigned int ret = CTRL_INVALID;
	unsigned int pos;

	pos = pin / MAX_GPIO_CTRL_PER_REG;

	switch(pos) {
		case CTRL:
			ret = GPIO_CTRL;
			break;
		case CTRL1:
			ret = GPIO_CTRL1;
			break;
		case CTRL2:
			ret = GPIO_CTRL2;
			break;
		case CTRL3:
			ret = GPIO_CTRL3;
			break;
		default:
			break;
	}

	return ret;
}

unsigned int gpio_get_data_cr(unsigned int pin)
{
	unsigned int ret = DATA_INVALID;
	unsigned int pos;

	pos = pin / MAX_GPIO_DATA_PER_REG;

	switch(pos) {
		case DATA:
			ret = GPIO_DATA;
			break;
		case DATA1:
			ret = GPIO_DATA1;
			break;
		default:
			break;
	}

	return ret;
}

unsigned int gpio_get_oe_cr(unsigned int pin)
{
	unsigned int ret = OE_INVALID;
	unsigned int pos;

	pos = pin / MAX_GPIO_OE_PER_REG;

	switch(pos) {
		case OE:
			ret = GPIO_OE;
			break;
		case OE1:
			ret = GPIO_OE1;
			break;
		default:
			break;
	}

	return ret;
}

void an_gpio_set_ctrl(unsigned int pin, int ctrl)
{
	unsigned int cr, bit, val;
	unsigned int mask = (1L << GPIO_CTRL_BITS) - 1;

	cr = gpio_get_ctrl_cr(pin);
	if(cr == CTRL_INVALID) {
		printf("WARNING: Cannot find GPIO%d ctrl CR, skip setting\n", pin);
		return;
	}

	ctrl &= mask;
	bit = pin % MAX_GPIO_CTRL_PER_REG;

	val = mmio_read_32(cr);

	if (((val >> (bit * GPIO_CTRL_BITS)) & mask) == ctrl)
		return;

	val &= ~(mask << (GPIO_CTRL_BITS * bit));
	val |= (ctrl << (GPIO_CTRL_BITS * bit));

	mmio_write_32(cr, val);

	return;
}

void an_gpio_set_data(unsigned int pin, int data)
{
	unsigned int cr, bit, val;
	unsigned int mask = (1L << GPIO_DATA_BITS) - 1;

	cr = gpio_get_data_cr(pin);
	if(cr == DATA_INVALID) {
		printf("WARNING: Cannot find GPIO%d data CR, skip setting\n", pin);
		return;
	}

	data &= mask;
	bit = pin % MAX_GPIO_DATA_PER_REG;

	val = mmio_read_32(cr);

	if (((val >> (bit * GPIO_DATA_BITS)) & mask) == data)
		return;

	val &= ~(mask << (bit * GPIO_DATA_BITS));
	val |= (data << (bit * GPIO_DATA_BITS));

	mmio_write_32(cr, val);

	return;
}

void an_gpio_set_oe(unsigned int pin, int oe)
{
	unsigned int cr, bit, val;
	unsigned int mask = (1L << GPIO_OE_BITS) - 1;

	cr = gpio_get_oe_cr(pin);
	if(cr == OE_INVALID) {
		printf("WARNING: Cannot find GPIO%d OE CR, skip setting\n", pin);
		return;
	}

	oe &= mask;
	bit = pin % MAX_GPIO_OE_PER_REG;

	val = mmio_read_32(cr);

	if (((val >> (bit * GPIO_OE_BITS)) & mask) == oe)
		return;

	val &= ~(mask << (bit * GPIO_OE_BITS));
	val |= (oe << (bit * GPIO_OE_BITS));

	mmio_write_32(cr, val);

	return;
}

void an_gpio_set_dir(unsigned int pin, unsigned int gpio_dir) {
	an_gpio_set_ctrl(pin, gpio_dir);

	if(gpio_dir == CTRL_OUTPUT)
		an_gpio_set_oe(pin, OE_ENABLE);
}

void an_gpio_set_value(unsigned int pin, unsigned val) {
	if (pin < MAX_GPIO_PIN) {
		printf("[MTK] Start to set GPIO%d=%d\n", pin, val);
		an_pinmux_force_gpio(pin);
		an_gpio_set_dir(pin, CTRL_OUTPUT);
		an_gpio_set_data(pin, val);
	} else {
        	printf("WARNING: GPIO%d not exist, skip setting\n", pin);
    	}
}

static bool an_gpio_find_config(char* config, char* sig) {
	unsigned int read_len = MTK_FIP_READ_LEN;
	unsigned int fip_addr = MTK_FIP_BASE;
	uint8_t fip[MTK_FIP_READ_LEN];
	int flash_read_status;
	int ret = 0;

	while (fip_addr <= MTK_FIP_END) {
		/* Check addr boundary */
		if (fip_addr + read_len > MTK_FIP_END)
			read_len = MTK_FIP_END - fip_addr;

		/* Fetch uboot env saved in flash 0x7C004 offset*/
		flash_read_status = flash_read(fip_addr, read_len, (uint8_t *) fip);
		if ((flash_read_status != FLASH_READ_STATUS_CORRECT))
			return ret;

		/* Transfer '\0' to ';' for parsing */
		transfer_config(fip, read_len);

		/* Find Signature */
		if (env_get_from_linear(fip, sig, config)) {
			printf("[MTK] Sig %s found! (%s)\n", sig, config);
			ret = 1;
			break;
		}

		fip_addr += MTK_FIP_READ_LEN;
	}

	return ret;
}

void an_gpio_set(char *config, AN_GPIO_PULL_MODE mode) {
	int gpio_list[MAX_GPIO_PIN] = {};
	int count, idx;

	if(config == NULL) {
		printf("Config empty, skip setting GPIO to %d\n", mode);
		return;
	}

	count = an_parse_config(config, gpio_list);

	for(idx = 0; idx < count;  idx++)
		an_gpio_set_value((MTK_AN7563_GPIO_PIN) gpio_list[idx], mode);

	return;
}

void init_gpio(void)
{
	char low_gpio_config[MTK_GPIO_CONFIG_MAXSIZE] = "";
	char high_gpio_config[MTK_GPIO_CONFIG_MAXSIZE] = "";

	printf("[MTK] Start initializing gpio setting from 0x%x...\n", MTK_FIP_BASE);

	/* Parse gpio low config in uboot env */
	if (!an_gpio_find_config(low_gpio_config, MKT_GPIO_LOW_SIG)) {
		printf("[MTK] No GPIO low config found in uboot env!\n");
	} else {
		if(low_gpio_config != NULL)
			an_gpio_set(low_gpio_config, LOW);
	}

	/* Parse gpio high config in uboot env */
	if (!an_gpio_find_config(high_gpio_config, MKT_GPIO_HIGH_SIG)) {
		printf("[MTK] No GPIO high config found in uboot env!\n");
	} else {
		if(high_gpio_config != NULL)
			an_gpio_set(high_gpio_config, HIGH);
	}

	return;
}