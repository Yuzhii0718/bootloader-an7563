#
# Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include drivers/auth/mbedtls/mbedtls_common.mk

MBEDTLS_SOURCES	+=	drivers/auth/mbedtls/mbedtls_x509_parser.c
ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifeq ($(TCSUPPORT_ATF_RELEASE),)
BL2_UNOPEN_SOURCES += drivers/auth/mbedtls/mbedtls_x509_parser.c
endif
else
BL2_UNOPEN_SOURCES += drivers/auth/mbedtls/mbedtls_x509_parser.c
endif