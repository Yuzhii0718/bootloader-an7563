/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <mtk_an7563_gpioconfig_parser.h>
#include <mtk_an7563_gpio.h>
#include <mtk_an7563_pin.h>

int isDigit(char c) {
	return (c >= '0' && c <= '9') ? 1 : 0;
}

int atoi(const char* str) {
	int sign = 1;
	int res = 0;
	int i = 0;

	while(str[i] == ' ')
		i++;

	if(str[i] == '-' || str[i] == '+')
		sign = (str[i++] == '-') ? -1 : 1;

	while(isDigit(str[i]))
		res = res * 10 + (str[i++] - '0');

	return res * sign;
}

void transfer_config(uint8_t *str, int size) {
	int end = size - 1;
	int i;

	for(; end > 0; end--) {
		if (str[end] == '\0')
			continue;
		break;
	}

	for(i = 0; i <= end; i++) {
		if (str[i] == '\0') {
			str[i] = ';';
		}
	}

	return;
}

int env_get_from_linear(const unsigned char *env, const char *name, char *buf)
{
	const unsigned char *p, *end;
	size_t name_len;

	if (name == NULL || *name == '\0')
		return 0;

	name_len = strlen(name);
	for (p = env; *p != '\0'; p = end + 1) {
		const unsigned char *value;
		unsigned res;
		for (end = p; *end != ';'; ++end) {}

		if (strncmp(name,(char *) p, name_len) || p[name_len] != '=')
			continue;
		value = &p[name_len + 1];

		res = end - value;
		memcpy(buf, value, res);

		return res;
	}

	return 0;
}

int an_parse_config(char* str, int* pin) {
	const char *p, *end;
	int count = 0;

	for (p = str; *p != '\0'; p = end + 1) {
		char pinInChar[MAX_PIN_BIT];
		const char *value;
		unsigned res;

		for (end = p; *end != ',' && *end != '\0'; ++end) {
			if(!isDigit(*end)) {
				printf("Not a digital number!\n");
				return (count > 0) ? 1 : 0;
			}
		}

		value = p;
		res = end - value;
		if(res > MAX_PIN_BIT) {
			continue;
		}
		memcpy(pinInChar, value, res);
		pinInChar[res] = '\0';

		pin[count++] = atoi(pinInChar);	
	}

	return count;
}