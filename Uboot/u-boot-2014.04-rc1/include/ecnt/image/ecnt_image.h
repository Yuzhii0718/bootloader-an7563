/*
 * (C) Copyright 2000-2009
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ECNT_IMAGE_H
#define _ECNT_IMAGE_H

#include <linux/ctype.h>
#include <common.h>

struct tclinux_imginfo {
	u32 kernel_off;
	u32 kernel_size;
	u32 rootfs_off;
	u32 rootfs_size;
	u32 tclinux_size;
};

typedef enum {  
	NOT_BOOT_IMG, 
	BOOT_IMG 
} IMG_BOOT_STATUS;

int boot_from_another_image(void);
int get_tclinux_imginfo(const unsigned long addr, struct tclinux_imginfo *info, IMG_BOOT_STATUS boot, unsigned char *buf);
#endif

