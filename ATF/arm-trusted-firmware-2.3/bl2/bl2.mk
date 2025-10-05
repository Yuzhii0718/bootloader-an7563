#
# Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifneq (${IMAGE_BL21}${IMAGE_BL22},)
BL2_SOURCES		+=	bl2/bl2_main.c				\
				bl2/${ARCH}/bl2_arch_setup.c		\
				lib/locks/exclusive/${ARCH}/spinlock.S	\
				plat/common/${ARCH}/platform_up_stack.S
else
BL2_SOURCES		+=	bl2/bl2_image_load_v2.c			\
				bl2/bl2_main.c				\
				bl2/${ARCH}/bl2_arch_setup.c		\
				lib/locks/exclusive/${ARCH}/spinlock.S	\
				plat/common/${ARCH}/platform_up_stack.S	\
				${MBEDTLS_SOURCES}
endif
				
ifeq (${ARCH},aarch64)
BL2_SOURCES		+=	common/aarch64/early_exceptions.S
endif

ifeq (${BL2_AT_EL3},0)
BL2_SOURCES		+=	bl2/${ARCH}/bl2_entrypoint.S
BL2_LINKERFILE		:=	bl2/bl2.ld.S

else
BL2_SOURCES		+=	bl2/${ARCH}/bl2_el3_entrypoint.S	\
				bl2/${ARCH}/bl2_el3_exceptions.S	\
				lib/cpus/${ARCH}/cpu_helpers.S		\
				lib/cpus/errata_report.c

ifeq (${ARCH},aarch64)
BL2_SOURCES		+=	lib/cpus/aarch64/dsu_helpers.S
endif

BL2_LINKERFILE		:=	bl2/bl2_el3.ld.S
endif

ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifeq ($(TCSUPPORT_ATF_RELEASE),)
BL2_UNOPEN_SOURCES += bl2_el3_entrypoint.S
endif
else
BL2_UNOPEN_SOURCES += bl2_el3_entrypoint.S
endif