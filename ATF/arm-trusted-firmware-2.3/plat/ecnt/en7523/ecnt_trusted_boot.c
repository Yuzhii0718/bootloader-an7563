/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <common/debug.h>
#include <plat/common/platform.h>
#include <tools_share/firmware_image_package.h>
#include <tools_share/firmware_encrypted.h>

#define SHA256_DER { 0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, \
			    0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20 }

#define SHA512_DER { 0x30, 0x51, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, \
			    0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40 }

#define	ROTPK_HASH_MAX_LEN	(ROTPK_HEADER_LEN + FILE_SIZE_SHA512)

static secure_efuse_t secure_data;
#ifndef TCSUPPORT_CPU_AN7552
static unsigned char ecnt_sha512_der[ROTPK_HEADER_LEN] = SHA512_DER;
#endif
static unsigned char ecnt_rotpk_hash[ROTPK_HASH_MAX_LEN] = SHA256_DER;

#if defined(IMAGE_BL31)
int get_hash_mode_by_efuse(void)
{
#ifndef TCSUPPORT_CPU_AN7552
	return secure_data.hash_mode;
#else
	return 0;
#endif
}
#endif

int get_asic_mode_by_efuse(void)
{
	return secure_data.asic_mode;
}
#ifndef TCSUPPORT_CPU_AN7552
int get_into_inic(void)
{
	return secure_data.into_inic;
}
#ifndef TCSUPPORT_CPU_EN7581
int get_freq_by_efuse(void)
{
	return secure_data.freq_by_efuse;
}

int get_freq_sel(void)
{
	return secure_data.freq_sel;
}
#endif
#endif

void fill_secure_data(uint8_t *p_data, uint8_t offset, size_t len)
{
	if (sizeof(secure_efuse_t) >= (offset + len ))
	{
		memcpy((void *) (((uint8_t *) &secure_data) + offset), (void *) p_data, len);
	}
	else
	{
		NOTICE("secure_data overflow\n");
		plat_error_handler(-EINVAL);
	}
}

int plat_get_enc_key_info(enum fw_enc_status_t fw_enc_status, uint8_t *key,
			  size_t *key_len, unsigned int *flags,
			  const uint8_t *img_id, size_t img_id_len)
{
	size_t len = 0;

#ifndef TCSUPPORT_CPU_AN7552
	if (secure_data.enc_mode == ENC_MODE_AES256)
		len = KEY_SIZE_256;
	else
#endif
		len = KEY_SIZE_128;

	if (*key_len >= len)
	{
		*key_len = len;
		memcpy(key, secure_data.ssk, *key_len);
		*flags = 0;

		return 0;
	}
	else
	{
		NOTICE("enc_key_info overflow\n");
		return -EINVAL;
	}
}

int plat_get_rotpk_info(void *cookie, void **key_ptr, unsigned int *key_len,
			unsigned int *flags)
{
#ifndef TCSUPPORT_CPU_AN7552
	if (secure_data.hash_mode == HASH_MODE_SHA512)
	{
		memcpy(ecnt_rotpk_hash, ecnt_sha512_der, ROTPK_HEADER_LEN);
		*key_len = FILE_SIZE_SHA512;
	}
	else
#endif
	{
		*key_len = FILE_SIZE_SHA256;		
	}

	memcpy(&ecnt_rotpk_hash[ROTPK_HEADER_LEN], secure_data.rotpk, *key_len);
	*key_ptr = ecnt_rotpk_hash;
	*key_len += ROTPK_HEADER_LEN;	

	*flags = ROTPK_IS_HASH;

	return 0;
}

int plat_get_nv_ctr(void *cookie, unsigned int *nv_ctr)
{
	*nv_ctr = 0;

	return 0;
}

int plat_set_nv_ctr(void *cookie, unsigned int nv_ctr)
{
	return 1;
}

int plat_get_mbedtls_heap(void **heap_addr, size_t *heap_size)
{
	return get_mbedtls_heap_helper(heap_addr, heap_size);
}
