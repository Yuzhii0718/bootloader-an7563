/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>

#include <platform_def.h>

#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_encrypted.h>
#include <tools_share/firmware_image_package.h>

#if TRUSTED_BOARD_BOOT
#define TRUSTED_BOOT_FW_CERT_NAME	"tb_fw.crt"
#define TRUSTED_KEY_CERT_NAME		"trusted_key.crt"
#define SOC_FW_KEY_CERT_NAME		"soc_fw_key.crt"
#define TOS_FW_KEY_CERT_NAME		"tos_fw_key.crt"
#define NT_FW_KEY_CERT_NAME			"nt_fw_key.crt"
#define SOC_FW_CONTENT_CERT_NAME	"soc_fw_content.crt"
#define TOS_FW_CONTENT_CERT_NAME	"tos_fw_content.crt"
#define NT_FW_CONTENT_CERT_NAME		"nt_fw_content.crt"
#endif /* TRUSTED_BOARD_BOOT */

/* IO devices */
static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_handle;
static const io_dev_connector_t *memmap_dev_con;
static uintptr_t memmap_dev_handle;
static const io_dev_connector_t *enc_dev_con;
static uintptr_t enc_dev_handle;

#if defined(IMAGE_BL31)
io_block_spec_t fip_block_spec = {
	.offset = 0,
	.length = 0
};

void set_fip_block_spec(size_t offset, size_t length)
{
	fip_block_spec.offset = offset;
	fip_block_spec.length = length;
}
#else
static const io_block_spec_t fip_block_spec = {
	.offset = PLAT_ECNT_FIP_BASE,
	.length = PLAT_ECNT_FIP_MAX_SIZE
};
#endif

static const io_uuid_spec_t bl2_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
};
#if !defined(IMAGE_BL1)
static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};
#ifdef TCSUPPORT_OPTEE
static const io_uuid_spec_t bl32_uuid_spec = {
		.uuid = UUID_SECURE_PAYLOAD_BL32,
};
#endif
static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};
#endif
#if TRUSTED_BOARD_BOOT
static const io_uuid_spec_t tb_fw_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FW_CERT,
};
#if !defined(IMAGE_BL1)
static const io_uuid_spec_t trusted_key_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_KEY_CERT,
};
static const io_uuid_spec_t soc_fw_key_cert_uuid_spec = {
	.uuid = UUID_SOC_FW_KEY_CERT,
};

static const io_uuid_spec_t nt_fw_key_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FW_KEY_CERT,
};
static const io_uuid_spec_t soc_fw_cert_uuid_spec = {
	.uuid = UUID_SOC_FW_CONTENT_CERT,
};

static const io_uuid_spec_t nt_fw_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FW_CONTENT_CERT,
};
#endif
#endif /* TRUSTED_BOARD_BOOT */

static int open_fip(const uintptr_t spec);
static int open_memmap(const uintptr_t spec);
static int open_enc_fip(const uintptr_t spec);

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

/* By default, load images from the FIP */
static const struct plat_io_policy policies[] = {
	[FIP_IMAGE_ID] = {
		&memmap_dev_handle,
		(uintptr_t)&fip_block_spec,
		open_memmap
	},
	[ENC_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)NULL,
		open_fip
	},
	[BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl2_uuid_spec,
		open_fip
	},
#if !defined(IMAGE_BL1)
	[BL31_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		open_fip
	},
#ifdef TCSUPPORT_OPTEE
	[BL32_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		open_fip
	},
#endif
	[BL33_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_uuid_spec,
		open_fip
	},
#endif
#if TRUSTED_BOARD_BOOT
	[TRUSTED_BOOT_FW_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&tb_fw_cert_uuid_spec,
		open_fip
	},
#if !defined(IMAGE_BL1)
	[TRUSTED_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&trusted_key_cert_uuid_spec,
		open_fip
	},
	[SOC_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&soc_fw_key_cert_uuid_spec,
		open_fip
	},
	[NON_TRUSTED_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&nt_fw_key_cert_uuid_spec,
		open_fip
	},
	[SOC_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&soc_fw_cert_uuid_spec,
		open_fip
	},
	[NON_TRUSTED_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&nt_fw_cert_uuid_spec,
		open_fip
	},
#endif
#endif /* TRUSTED_BOARD_BOOT */
};

static int open_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if ((result == 0) && (spec != (uintptr_t)NULL)) {
		result = io_open(fip_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using FIP\n");
			result = fip_file_enc(local_image_handle);
			io_close(local_image_handle);
		}
	}
	return result;
}

static int open_enc_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	/* See if an encrypted FIP is available */
	result = io_dev_init(enc_dev_handle, (uintptr_t)ENC_IMAGE_ID);
	if (result == 0) {
		result = io_open(enc_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using encrypted FIP\n");
			io_close(local_image_handle);
		}
	}
	return result;
}

static int open_memmap(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(memmap_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(memmap_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using Memmap\n");
			io_close(local_image_handle);
		}
	}
	return result;
}

void plat_ecnt_io_setup(void)
{
	int io_result;

	io_result = register_io_dev_fip(&fip_dev_con);
	assert(io_result == 0);

	io_result = register_io_dev_memmap(&memmap_dev_con);
	assert(io_result == 0);

	/* Open connections to devices and cache the handles */
	io_result = io_dev_open(fip_dev_con, (uintptr_t)NULL,
				&fip_dev_handle);
	assert(io_result == 0);

	io_result = io_dev_open(memmap_dev_con, (uintptr_t)NULL,
				&memmap_dev_handle);
	assert(io_result == 0);

	io_result = register_io_dev_enc(&enc_dev_con);
	assert(io_result == 0);

	io_result = io_dev_open(enc_dev_con, (uintptr_t)NULL,
				&enc_dev_handle);
	assert(io_result == 0);

	/* Ignore improbable errors in release builds */
	(void)io_result;
}

/*
 * Return an IO device handle and specification which can be used to access
 * an image. Use this to enforce platform load policy
 */
int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	int result;
	const struct plat_io_policy *policy;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	result = policy->check(policy->image_spec);
	if (result == 0)
	{
		if ((image_id == BL2_IMAGE_ID) || (image_id == BL31_IMAGE_ID) || (image_id == BL33_IMAGE_ID))
			NOTICE("FW UN-ENCRYPTION\n");

		*image_spec = policy->image_spec;
		*dev_handle = *(policy->dev_handle);
	}
	else if (result == FW_ENCRYPTION)
	{
		if ((image_id == BL2_IMAGE_ID) || (image_id == BL31_IMAGE_ID) || (image_id == BL33_IMAGE_ID))
			NOTICE("FW ENCRYPTION\n");

		result = open_enc_fip(policy->image_spec);
		if (result == 0)
		{
			*image_spec = policy->image_spec;
			*dev_handle = enc_dev_handle;
		}
	}

	return result;
}

int plat_check_bypass(void)
{
	int result = 0;
	uint16_t plat_toc_flag = 0;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if ((result == 0))
	{
		fip_dev_get_plat_toc_flag((io_dev_info_t *)fip_dev_handle , &plat_toc_flag);
		result = plat_toc_flag & BYPASS_FWUPGRADE;
		if (result == BYPASS_FWUPGRADE)
			NOTICE("3-3-4\n");
		else
			NOTICE("3-3-3\n");
	}
	else
	{
		NOTICE("3-3-2\n");
	}

	return result;
}

int plat_check_header(uint8_t *base)
{
	fip_toc_header_t *header = (fip_toc_header_t *) base;

	if ((header->name == TOC_HEADER_NAME) && (header->serial_number != 0))
	{
		return 1;
	} else
	{
		return 0;
	}
}
