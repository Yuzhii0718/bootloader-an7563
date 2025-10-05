/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author : Tim.Kuo <Tim.kuo@mediatek.com>
 */

#define ENV_READ_LEN	0xA00

void mtk_transfer_config(uint8_t *str, int end);
int env_get_from_linear(const unsigned char *env, const char *name, char *buf);
int mtk_parse_config_to_bool(char* str);