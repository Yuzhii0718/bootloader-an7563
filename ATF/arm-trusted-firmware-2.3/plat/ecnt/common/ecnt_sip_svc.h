/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECNT_SIP_SVC_H
#define ECNT_SIP_SVC_H

#include <stdint.h>

/* SMC function IDs for SiP Service queries */
#define SIP_SVC_CALL_COUNT		0x8200ff00
#define SIP_SVC_UID			0x8200ff01
/*					0x8200ff02 is reserved */
#define SIP_SVC_VERSION			0x8200ff03

/* ECNT SiP Service Calls version numbers */
#define ECNT_SIP_SVC_VERSION_MAJOR	0x0
#define ECNT_SIP_SVC_VERSION_MINOR	0x1

#define SMC_AARCH64_BIT		0x40000000

/* Number of ECNT SiP Calls implemented */
#define ECNT_COMMON_SIP_NUM_CALLS			4

/* ECNT SiP Service Calls function IDs */
#define ECNT_SIP_EFUSE_HANDLE				0x82000001
#define ECNT_SIP_TEST_HANDLE				0x82000002
#define ECNT_SIP_GET_ALG_HANDLE				0x82000003
#define ECNT_SIP_VERIFY_BL2_HANDLE			0x82000201	/* & 0xF is IMAGE_ID, don't modify it */
#define ECNT_SIP_VERIFY_BL33_HANDLE			0x82000205	/* & 0xF is IMAGE_ID, don't modify it */
#define ECNT_SIP_DCRYPT_VERIFY_BL2_HANDLE	0x82000211
#define ECNT_SIP_DCRYPT_VERIFY_BL33_HANDLE	0x82000215
#define ECNT_SIP_AVS_HANDLE				  	0x82000301

/* For ECNT SMC from Secure OS */
/* 0x82000000 - 0x820000FF & 0xC2000000 - 0xC20000FF */
#define ECNT_SIP_KERNEL_BOOT_AARCH32		0x82000200
#define ECNT_SIP_KERNEL_BOOT_AARCH64		0xC2000200

/* ECNT SiP Calls error code */
enum {
	ECNT_SIP_E_SUCCESS = 0,
	ECNT_SIP_E_INVALID_PARAM = -1,
	ECNT_SIP_E_NOT_SUPPORTED = -2,
	ECNT_SIP_E_INVALID_RANGE = -3,
	ECNT_SIP_E_PERMISSION_DENY = -4,
	ECNT_SIP_E_LOCK_FAIL = -5
};

#endif /* ECNT_SIP_SVC_H */
