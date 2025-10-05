/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#include <asm/tc3162.h>
#include <asm/io.h>
#include <common.h>
#include <ecnt/gpio/en7523_pin.h>
#include <ecnt/gpio/en7523_evb_mtk_gpio.h>
#include <ecnt/gpio/en7523_evb_mtk_pinmux.h>
#include <led.h>

#ifdef CONFIG_LED_BLINK
struct led_gpio_priv ledctl[64];
void gpio_led_tick(ulong tick_ms)
{
	static ulong s_latest_tick_ms;
	int pinId = 0;

	for (pinId = 0; pinId < MAX_GPIO_PIN; pinId++)
	{
		if ((ledctl[pinId].delay_on +  ledctl[pinId].delay_off) > 0)
		{
			if (tick_ms > s_latest_tick_ms)
			{
				ledctl[pinId].passed_ms += (tick_ms - s_latest_tick_ms);
			}

			if (ledctl[pinId].passed_ms < ledctl[pinId].delay_off)
			{
				if (ledctl[pinId].state == LEDST_ON)
				{
					ledctl[pinId].state = LEDST_OFF;
					an_gpio_set_value(pinId, HIGH);
				}
			}
			else if (ledctl[pinId].passed_ms < (ledctl[pinId].delay_on + ledctl[pinId].delay_off))
			{
				if (ledctl[pinId].state == LEDST_OFF)
				{
					ledctl[pinId].state = LEDST_ON;
					an_gpio_set_value(pinId, LOW);
				}
			}
			else
			{
				ledctl[pinId].state = LEDST_OFF;
				an_gpio_set_value(pinId, HIGH);
				do 
				{
					ledctl[pinId].passed_ms = ledctl[pinId].passed_ms - ledctl[pinId].delay_on - ledctl[pinId].delay_off;
				} while (ledctl[pinId].passed_ms > (ledctl[pinId].delay_on + ledctl[pinId].delay_off));
			}
		}
	}

	s_latest_tick_ms = tick_ms;
}

int gpio_led_set_period(int pinId, int delay_on, int delay_off)
{
	if (delay_on < 0 || delay_off < 0)
		return -1;

	if (delay_on != ledctl[pinId].delay_on || delay_off != ledctl[pinId].delay_off)
	{
		ledctl[pinId].delay_on = delay_on;
		ledctl[pinId].delay_off = delay_off;
		ledctl[pinId].passed_ms = 0;
	}

	return 0;
}
#endif



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

	val = regRead32(cr);

	if (((val >> (bit * GPIO_CTRL_BITS)) & mask) == ctrl)
		return;

	val &= ~(mask << (GPIO_CTRL_BITS * bit));
	val |= (ctrl << (GPIO_CTRL_BITS * bit));

	regWrite32(cr, val);

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

	val = regRead32(cr);

	if (((val >> (bit * GPIO_DATA_BITS)) & mask) == data)
		return;

	val &= ~(mask << (bit * GPIO_DATA_BITS));
	val |= (data << (bit * GPIO_DATA_BITS));

	regWrite32(cr, val);

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

	val = regRead32(cr);

	if (((val >> (bit * GPIO_OE_BITS)) & mask) == oe)
		return;

	val &= ~(mask << (bit * GPIO_OE_BITS));
	val |= (oe << (bit * GPIO_OE_BITS));

	regWrite32(cr, val);

	return;
}

void an_gpio_set_dir(unsigned int pin, unsigned int gpio_dir) {
	an_gpio_set_ctrl(pin, gpio_dir);

	if(gpio_dir == CTRL_OUTPUT)
		an_gpio_set_oe(pin, OE_ENABLE);
	if(gpio_dir == CTRL_INPUT)
		an_gpio_set_oe(pin, OE_DISABLE);
}


unsigned int an_gpio_get_value(unsigned int pin) {

	unsigned int cr, val;

	/* Check pin valid */
	if (pin < MAX_GPIO_PIN) {
		an_pinmux_force_gpio(pin);
		an_gpio_set_dir(pin, CTRL_INPUT);
		cr = gpio_get_data_cr(pin);
		val = (regRead32(cr) >> (pin % MAX_GPIO_DATA_PER_REG)) & 1;
	} else {
		printf("WARNING: GPIO%d not exist, skip setting\n", pin);
		return;
	}

	return val;
}

void an_gpio_set_value(unsigned int pin, unsigned val) {

	/* Check val valid */
	if (val != HIGH && val != LOW) {
		printf("WARNING: Cannot set GPIO%d to %d, skip setting!\n", pin, val);
		return;
	}

	/* Check pin valid */
	if (pin < MAX_GPIO_PIN) {
		an_pinmux_force_gpio(pin);
		an_gpio_set_dir(pin, CTRL_OUTPUT);
		an_gpio_set_data(pin, val);
	} else {
		printf("WARNING: GPIO%d not exist, skip setting\n", pin);
		return;
	}
}

void ecnt_mtk_gpio_init() {
	printf("[MTK] Start uboot gpio init...\n");
#ifdef CONFIG_LED_BLINK
	memset(ledctl, 0, sizeof(ledctl));
#endif
	//an_gpio_set_value(PIN12, LOW);
	//an_gpio_set_value(PIN16, HIGH);
	return;
}
