/*
 * (C) Copyright 2000-2009
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ECNT_MTD_H
#define _ECNT_MTD_H

extern unsigned int ecnt_get_tclinux_mtd_offset(void);
extern unsigned int ecnt_get_tclinux_slave_mtd_offset(void);
extern unsigned int ecnt_get_tclinux_flash_offset(void);
extern unsigned int ecnt_get_tclinux_slave_flash_offset(void);
extern unsigned int ecnt_get_reservearea_flash_offset(void);
extern unsigned int ecnt_get_tclinux_size(void);
extern unsigned int ecnt_get_tclinux_slave_size(void);
extern unsigned int ecnt_get_kernel_size(unsigned int erasesize);
extern unsigned int ecnt_get_kernel_slave_size(unsigned int erasesize);
extern unsigned int ecnt_get_reservearea_size(void);
extern unsigned int ecnt_get_boot_size(void);
extern unsigned int ecnt_get_romfile_size(void);
extern unsigned int ecnt_get_ubootenv_size(void);
extern unsigned int ecnt_get_factory_size(void);
extern int ecnt_parse_cmdline_partitions(void);
extern int do_get_mtd_info(void);
extern unsigned int do_get_mtd_rootfsdata_info(ulong *addr, ulong *size);
extern unsigned int ecnt_get_ubootenv_mtd_offset(void);
extern unsigned int ecnt_get_factory_mtd_offset(void);

#endif

