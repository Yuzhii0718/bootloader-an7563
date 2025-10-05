/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#include <stdbool.h>
#include <stdint.h>

#define MAX_PIN_BIT 	4

void transfer_config(uint8_t *str, int end);
void uint8ToChar(uint8_t *uint8Str, char *charStr, int len);
int env_get_from_linear(const unsigned char *env, const char *name, char *buf);
int an_parse_config(char* str, int* pin);