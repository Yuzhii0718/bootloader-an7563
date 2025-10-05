/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */
#include <asm/tc3162.h>
#include <asm/io.h>
#include <common.h>
#include <ecnt/en7523_evb_mtk_env_parser.h>

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

void mtk_transfer_config(uint8_t *str, int size) {
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

	str[end] = '\0';

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
		buf[res] = '\0';

		return res;
	}

	return 0;
}

int mtk_parse_config_to_bool(char* str) {
	const char *p, *end;
	char config[10];
	int ret = 0;

	for (p = str; *p != '\0'; p = end + 1) {
		const char *value;
		unsigned res;

		for (end = p; *end != '\0'; ++end) {
			if(!isDigit(*end)) {
				printf("[MTK] Parsing env config fail!\n");
				return ret;
			}
		}

		value = p;
		res = end - value;
		memcpy(config, value, res);

		ret = atoi(config);

		return ret;
	}
}