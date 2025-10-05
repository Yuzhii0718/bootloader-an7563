/*
 * Copyright (c) 2013, Google Inc.
 *
 * Copyright (C) 2011
 * Corscience GmbH & Co. KG - Simon Schwarz <schwarz@corscience.de>
 *  - Added prep subcommand support
 *  - Reorganized source - modeled after powerpc version
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fdt_support.h>
#if defined(CONFIG_ECNT)
#include <asm/tc3162.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

int arch_fixup_memory_node(void *blob)
{
	bd_t *bd = gd->bd;
	int bank;
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	for (bank = 0; bank < CONFIG_NR_DRAM_BANKS; bank++) {
		start[bank] = bd->bi_dram[bank].start;
#if defined(CONFIG_ECNT)
		/* set the real dram size and pass it to kernel*/
		size[bank] = ((u64)GET_DRAM_SIZE * (u64)SZ_1M);
#if defined(CONFIG_MTK_ATF)
		size[bank] -= SZ_16M;
#endif
#else
		size[bank] = bd->bi_dram[bank].size;
#endif
#if defined(CONFIG_MTK_ATF)
		/*return 16M dram to kernel , dram_init() furnction borrow 16M*/
		size[bank] += SZ_16M;
#endif
	}

	return fdt_fixup_memory_banks(blob, start, size, CONFIG_NR_DRAM_BANKS);
}
