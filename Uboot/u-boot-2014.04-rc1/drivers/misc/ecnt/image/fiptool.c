
#include <common.h>
#include <command.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/stddef.h>
#include "firmware_image_package.h"

#define MV_DATA_OFFSET		(0x800)
#define BOOT_FIP_MAX_SIZE	(0x1F800)

/*
 * Because airoha will recalculate fip_offset dynamically
 * and will cause unexpected problem for mtk_allinone,
 * MTK seperate fip_offset for mtk_allinone.
 */
#ifdef TCSUPPORT_ARM_SECURE_BOOT
#define MTK_FIP_OFFSET      (0x2000)
#else
#define MTK_FIP_OFFSET      (0x0)
#endif

uuid_t img_uuid[2] = {
	UUID_NON_TRUSTED_FIRMWARE_BL33,
	UUID_TRUSTED_BOOT_FIRMWARE_BL2,
};

unsigned int img_smc[2] = {
	SIP_VERIFY_BL33,
	SIP_VERIFY_BL2,
};

#ifdef TCSUPPORT_ARM_SECURE_BOOT
static uint32_t fip_offset = 0x2000;
#else
static uint32_t fip_offset = 0;

#endif

static inline int compare_uuids(const uuid_t *uuid1, const uuid_t *uuid2)
{
	return memcmp(uuid1, uuid2, sizeof(uuid_t));
}

unsigned int get_fip_offset(void)
{
	return fip_offset;
}

unsigned int mtk_get_fip_offset(void)
{
	return MTK_FIP_OFFSET;
}

unsigned int parse_fip(char **buf, unsigned int *buf_size, unsigned int *offset, unsigned int *size, unsigned int *flag)
{
	char *bufend = NULL;
	int tcboot = 0;
	fip_toc_header_t *toc_header = NULL;
	fip_toc_entry_t *toc_entry = NULL;

	toc_header = (fip_toc_header_t *) *buf;

	if (toc_header->name != TOC_HEADER_NAME)
	{
		*buf += MV_DATA_OFFSET;
		toc_header = (fip_toc_header_t *) *buf;
		if (toc_header->name != TOC_HEADER_NAME)
		{
			return 0;
		}
		tcboot = 1;
		*buf_size = BOOT_FIP_MAX_SIZE;
	}

	toc_entry = (fip_toc_entry_t *)(toc_header + 1);
	bufend = *buf + *buf_size;

	/* Walk through each ToC entry in the file. */
	while ((char *)toc_entry + sizeof(*toc_entry) - 1 < bufend)
	{
		if (compare_uuids(&(toc_entry->uuid), &img_uuid[tcboot]) == 0)
		{
			*offset = (unsigned int) toc_entry->offset_address;
			*size = (unsigned int) toc_entry->size;
			if (toc_entry->flags != 0)
			{
				*flag = (unsigned int) toc_entry->flags;
			}
			fip_offset = *offset;

			return img_smc[tcboot];
		}

		toc_entry++;
	}

	return 0;
}
