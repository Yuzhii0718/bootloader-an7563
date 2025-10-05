/*
 * (C) Copyright 2000-2009
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ECNT_IMAGE_H
#define _ECNT_IMAGE_H

#include <ecnt/image/ecnt_image.h>

int bootargs_init(struct tclinux_imginfo *first_imginfo, struct tclinux_imginfo *second_imginfo, unsigned int bootflag);
char readBootFlagFromFlash(void);
int writeBootFlagtoFlash(char flag);
int swap_bootflag(void);

#endif

