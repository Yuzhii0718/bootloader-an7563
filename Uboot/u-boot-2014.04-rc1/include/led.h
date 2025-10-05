/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#ifndef __LED_H
#define __LED_H

typedef enum{
	LEDST_OFF = 0,
	LEDST_ON = 1,
	LEDST_TOGGLE,
#ifdef CONFIG_LED_BLINK
	LEDST_BLINK,
#endif

	LEDST_COUNT,
} led_state_t;

#define LED_POWER_RED		8
#define LED_POWER_GREEN		9
#define LED_WPS_GREEN		10

#ifdef CONFIG_LED_BLINK
struct led_gpio_priv {
	//int period_ms;
	int delay_on;
	int delay_off;
	int passed_ms;
	led_state_t state;
};

//struct led_gpio_priv ledctl[64];
void gpio_led_tick(ulong tick_ms);
int gpio_led_set_period(int pinId, int delay_on, int delay_off);
#endif
#endif
