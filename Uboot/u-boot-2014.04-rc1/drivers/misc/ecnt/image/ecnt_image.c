/*
 * (C) Copyright 2000-2009
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <environment.h>
#include <linux/ctype.h>
#include <asm/io.h>
#include <libfdt.h>
#include <fdt.h>
#include <flashhal.h>
#include <ecnt/image/ecnt_image.h>
#include <ecnt/image/ecnt_mtd.h>
#include <trx.h>

#define DBG_ECNT_IMAGE 0
extern unsigned int get_fip_offset(void);
extern unsigned int image_read_mode;

enum
{
	VERIFY_SUCCESS = 0,
	RESERVED,
	VERIFY_ENCRYPTED_SUCCESS,
};

#ifdef TCSUPPORT_LITTLE_ENDIAN
#define STORE32_LE(X)   (X)
#else
#define STORE32_LE(X)		bswap_32(X)
#endif

int boot_from_another_image(void)
{
	static int isSwap = 0;

	if(isSwap == 0) {
		isSwap = 1;
		/* swap bootflag */
		if(run_command(CONFIG_BOOTFLAG_SWAP_COMMAND, 0)) {
			printf("bootflag swap fail.\n");
			return -1;
		}
		/* run boot command, flash imgread;bootm */
		return run_command(CONFIG_BOOTCOMMAND, 0);
	}
	return -1;
}

static int get_fdt_node_offset_len(unsigned char *buf, int images_noffset, const char *node, void **offset, u32 *len)
{
	int		noffset;

#if DBG_ECNT_IMAGE
	printf("check %s buf=0x%lx, images_noffset=0x%lx\n", node, buf, images_noffset);
#endif

	noffset = fdt_subnode_offset(buf, images_noffset, node);
#if DBG_ECNT_IMAGE
	printf("images_noffset=0x%lx, noffset=0x%lx\n", images_noffset, noffset);
#endif
	if (noffset < 0) {
		printf("Can't get node offset for image unit name: '%s' (%s)\n",
			  node, fdt_strerror(noffset));
		return -1;
	}

	*offset = (void *)fdt_getprop(buf, noffset, "data", len);
#if DBG_ECNT_IMAGE
	printf("offset=0x%lx, len=0x%lx\n", *offset, *len);
#endif
	if (*offset == NULL) {
		printf("get fdt data error:0x%08u\n", (u32)(*offset));
		return -1;
	}

	return 0;
}

#if DBG_ECNT_IMAGE
void dump_buffer(ulong addr, int size, ulong length)
{
	ulong bytes = size * length;
	const void *buf = map_sysmem(addr, bytes);

	/* Print the lines. */
	print_buffer(addr, buf, size, length, 16);
	unmap_sysmem(buf);
}
#endif

int get_tclinux_imginfo(const unsigned long orig_addr, struct tclinux_imginfo *info, IMG_BOOT_STATUS boot, unsigned char *buf)
{
	int				noffset, images_noffset;
	u32				len;
	const void		*data;
	unsigned long	retlen = 0;
	unsigned long	img_size = 0;
	image_header_t	*img_hdr = (image_header_t *)buf;
	u32 fdt_read_length = 0;
	const int FLASH_READ_HEADER_LEN = 0x800;
	fdt32_t dt_strings_off, dt_strings_size;
	unsigned long	addr = orig_addr + get_fip_offset();

	if (image_read_mode == 1)
		addr = 0x100;
#if DBG_ECNT_IMAGE
	printf("read: src=0x%lx, len=0x%lx, dst=0x%lx\n", addr, FLASH_READ_HEADER_LEN, buf);
#endif

	/* Read a SPI NAND Page for parse image header */
	if(image_read(addr, FLASH_READ_HEADER_LEN, &retlen, buf)) {
		printf("%s, image_read error! src=0x%lx, len=0x%lx, dst=0x%lx\n", __func__, addr, (unsigned long)2048, (unsigned long)buf);
		return -1;
	}
	fdt_read_length += FLASH_READ_HEADER_LEN;

	/* Print the lines. */
#if DBG_ECNT_IMAGE
	dump_buffer((ulong)buf, 1, 0x100);
#endif

	if (FDT_MAGIC == (unsigned long) fdt_magic(buf)) {
		info->tclinux_size = fdt_totalsize(buf);
		images_noffset = fdt_path_offset(buf, "/images");
		if (images_noffset < 0) {
			printf("Can't find images parent node '%s' (%s)\n",
				  "/images", fdt_strerror(images_noffset));
			return -1;
		}

		/* get dt strings offset */
		dt_strings_off = fdt_off_dt_strings(buf);
		dt_strings_size = fdt_size_dt_strings(buf);
		/* Read dt strings */
		if(image_read(addr + dt_strings_off, dt_strings_size, &retlen, buf + dt_strings_off)) {
			printf("%s, image_read error! src=0x%lx, len=0x%lx, dst=0x%lx\n", __func__, addr, (unsigned long)2048,(unsigned long) buf);
			return -1;
		}

		if(get_fdt_node_offset_len(buf, images_noffset, "fdt@1", &data, &len)) {
			return -1;
		}

#if DBG_ECNT_IMAGE
		printf("fdt@1: data=0x%lx, buf=0x%lx, off=0x%lx, len=0x%lx\n", data, buf, (u32)data - (u32)buf, len);
#endif

		/* find kernel dt struct header off */
		len += ((u32)data - (u32)buf) + FLASH_READ_HEADER_LEN;

		/* Read kernel dt struct header */
		if(fdt_read_length < len) {
#if DBG_ECNT_IMAGE
			printf("kd, flash read 0x%lx\n", len - fdt_read_length);
#endif
			if(image_read(addr + fdt_read_length, len - fdt_read_length, &retlen, buf + fdt_read_length)) {
				printf("%s, image_read error! src=0x%lx, len=0x%lx, dst=0x%lx\n", __func__, addr, (unsigned long)2048, (unsigned long)buf);
				return -1;
			}
			fdt_read_length = len;
		}

		if(get_fdt_node_offset_len(buf, images_noffset, "kernel@1", &data, &len)) {
			return -1;
		}

#if DBG_ECNT_IMAGE
		printf("kernel@1: data=0x%lx, buf=0x%lx, off=0x%lx, len=0x%lx\n", data, buf, (u32)data - (u32)buf, len);
#endif

		info->kernel_off = (u32)data - (u32)buf;
		info->kernel_size = len;

		if(boot == BOOT_IMG) {
#if DBG_ECNT_IMAGE
			printf("BOOT_IMG: off=0x%x, len=0x%x, fdtlen=0x%x\n", (addr + fdt_read_length), (info->kernel_off + info->kernel_size - fdt_read_length), fdt_read_length);
#endif
			/* Read kernel to DRAM */
			printf("Reading kernel...\n");
			if(fdt_read_length < (info->kernel_off + info->kernel_size)) {
#if DBG_ECNT_IMAGE
				printf("flash read 0x%lx\n", len - fdt_read_length);
#endif
				image_read((addr + fdt_read_length), (info->kernel_off + info->kernel_size - fdt_read_length), &retlen, (buf + fdt_read_length));
			}
		}
		
		/* Read rootfs header */
		image_read((addr + (info->kernel_off + info->kernel_size)), FLASH_READ_HEADER_LEN, &retlen, (buf + (info->kernel_off + info->kernel_size)));

		if (image_read_mode == 1)
		{
			if(get_fdt_node_offset_len(buf, images_noffset, "ramdisk@1", &data, &len)) {
				return -1;
			}
			setenv("root", "/dev/ram rw");

		}
		else
		{
			if(get_fdt_node_offset_len(buf, images_noffset, "filesystem@1", &data, &len)) {
				return -1;
			}			
		}

		info->rootfs_off = (u32)data - (u32)buf;
		info->rootfs_size = len;
		if (image_read_mode == 1)
		{
			
			image_read((addr + (info->kernel_off + info->kernel_size) + FLASH_READ_HEADER_LEN), info->rootfs_size, &retlen, (buf + (info->kernel_off + info->kernel_size) + FLASH_READ_HEADER_LEN));
			
		}
		/* Read configurations */
		if(boot == BOOT_IMG) {
				image_read((addr + (info->rootfs_off + info->rootfs_size)), FLASH_READ_HEADER_LEN, &retlen, (buf + (info->rootfs_off + info->rootfs_size)));
		}
	} else if (IH_MAGIC == uimage_to_cpu(img_hdr->ih_magic)) {
		img_size = uimage_to_cpu(img_hdr->ih_size)+sizeof(image_header_t);
		printf("This is a uImage, img_size = 0x%lx\n", img_size);
		return -1;
	} else {
		printf("FDT_MAGIC or IH_MAGIC check fail.\n");
		return -1;
	}

#if DBG_ECNT_IMAGE
	printf("=== tclinux_size:0x%08x ===\n", info->tclinux_size);
	printf("=== kernel_off:0x%08x ===\n", info->kernel_off);
	printf("=== kernel_size:0x%08x ===\n", info->kernel_size);
	printf("=== rootfs_off:0x%08x ===\n", info->rootfs_off);
	printf("=== rootfs_size:0x%08x ===\n", info->rootfs_size);
#endif

	return 0;
}


#ifdef CONFIG_USE_IRQ

void ecnt_ImageUpgrade(int fw_type)
{
	ulong img_size;
	char *s;
	ulong addr = 0xFFFFFFFF;;
	unsigned long	retlen = 0;
	unsigned int check_value = 0;
	unsigned int checksize = 0;
	unsigned int crc = 0;
	unsigned int cal_check_sum = 0;
	int stat = -1;
	struct trx_header* trx_H = NULL;
	ulong mtd_rootfs_data_addr, mtd_rootfs_data_size;

	/* MTK allinone */
	unsigned int crc_len;
	ulong trxH_offset;

	unsigned int bl_check = 0;
	unsigned int main_check = 0;
	unsigned int second_check = 0;

	ulong mtk_tcboot_size = ecnt_get_boot_size();
	ulong mtk_uboot_size = ecnt_get_ubootenv_size();
	ulong mtk_factory_size = ecnt_get_factory_size();
	ulong mtk_main_image_offset = mtk_tcboot_size + mtk_uboot_size + mtk_factory_size;
	ulong mtk_second_image_offset = mtk_main_image_offset + ecnt_get_tclinux_size();

	img_size = NetBootFileXferSize;
	/* flush cache */
	flush_cache(load_addr, img_size);

	stat = upgrade_verify((char *)load_addr);

	if ((stat != VERIFY_SUCCESS) && (stat != VERIFY_ENCRYPTED_SUCCESS))
	{
		printf("Verify image!!!\r\n");
		return;
	}

#if defined(CONFIG_ECNT_TFTP_AUTO_UPGRADE)
		s = getenv("filename");
		if (!strcmp(s, getenv("uboot_filename")))
		{
			addr = CONFIG_TCBOOT_OFFSET;
			checksize = img_size;
			check_value = crc32_no_comp(DEFAULT_CRC,(const unsigned char*)load_addr , checksize-CONFIG_ENV_SIZE);

		}
		else if (!strcmp(s, getenv("kernel_filename")))
		{
			addr = ecnt_get_tclinux_flash_offset();
			trx_H = (struct trx_header*)((unsigned char*)load_addr + get_fip_offset());

			/* check tclinux hash if the image does not encrypted*/
			if (stat == VERIFY_SUCCESS)
			{
				if (fit_all_image_verify((const void *) (load_addr+trx_H->header_len+get_fip_offset()))!= 1)
				{
					printf("hash check error!!! \r\n");
					check_value = -1;
				}
			}
			if (img_size < (trx_H->len))
			{
				printf("receive image size 0x%lx is less than the length 0x%lx which stored in trx !!!",img_size, (unsigned long)trx_H->len);
				check_value = -1;
			}
			else
			{
				/* get image len from trx*/
				checksize = trx_H->len - trx_H->header_len;
				cal_check_sum = crc32_no_comp(DEFAULT_CRC,(const unsigned char*)load_addr + trx_H->header_len + get_fip_offset(), checksize);
				crc = __swab32(ntohl(trx_H->crc32));
					if (cal_check_sum != crc)
				{
					printf("crc check error!!! \r\n");
					check_value = -1;
				}

			}
		}
        	else if (!strcmp(s, getenv("singleImage_filename"))) {
			addr = CONFIG_MTK_ALLINONE_OFFSET;

			/* Check tcboot CRC */
			crc_len = ecnt_get_boot_size() - CONFIG_ENV_SIZE;
			bl_check = crc32_no_comp(DEFAULT_CRC,(const unsigned char*)load_addr,
						 crc_len);

			/* Check Main image CRC */
			trxH_offset = mtk_main_image_offset + mtk_get_fip_offset();
			trx_H = (struct trx_header*)((unsigned char*)load_addr + trxH_offset);
			if (stat == VERIFY_SUCCESS) {
				if (fit_all_image_verify((const void *) (load_addr + trxH_offset +
							 trx_H->header_len))!= 1) {
					printf("hash check error on main img!!! \r\n");
					main_check = -1;
				}
			}
			crc_len = trx_H->len - trx_H->header_len;
			cal_check_sum = crc32_no_comp(DEFAULT_CRC,(const unsigned char*)load_addr +
						      trxH_offset + trx_H->header_len, crc_len);
			crc = __swab32(ntohl(trx_H->crc32));
			if (cal_check_sum != crc) {
				printf("crc check error on main img!!! \r\n");
				main_check = -1;
			}

			/* Check Second image CRC */
			trxH_offset = mtk_second_image_offset + mtk_get_fip_offset();
			trx_H = (struct trx_header*)((unsigned char*)load_addr + trxH_offset);
			if (stat == VERIFY_SUCCESS) {
				if (fit_all_image_verify((const void *) (load_addr + trxH_offset +
							 trx_H->header_len))!= 1) {
					printf("hash check error on 2nd img!!! \r\n");
					second_check = -1;
				}
			}
			crc_len = trx_H->len - trx_H->header_len;
			cal_check_sum = crc32_no_comp(DEFAULT_CRC,(const unsigned char*)load_addr +
						      trxH_offset + trx_H->header_len, crc_len);
			crc = __swab32(ntohl(trx_H->crc32));
			if (cal_check_sum != crc) {
				printf("crc check error on 2nd img!!! \r\n");
				second_check = -1;
			}

			/* Check above verification flag */
			if (bl_check != 0 || main_check != 0 || second_check != 0)
				check_value = -1;
        	}

		setEtherRateLimit(0);

		if (addr == 0xFFFFFFFF)
		{
			printf("ERROR: Unknown file name !\n");
			return;
		}

		if (check_value != 0)
		{
			printf("ERROR: CRC/Hash check failed !\n");

			return;
		}

		printf("CRC check success, start imageUpgrade\n");
		printf("erase: addr=0x%lx, len=0x%lx\n", addr, img_size);
		flash_erase(addr, img_size);
		printf("write: src=0x%lx, len=0x%lx, dst=0x%lx\n", load_addr, img_size, addr);
		flash_write(addr, img_size, &retlen, (unsigned char *) load_addr);
		printf("upgrade finished !\n");

		/* MTK erases rootfs_data while kernel upgrading for convenience for wifi team */
		if (!strcmp(s, getenv("kernel_filename"))) {
			if (do_get_mtd_rootfsdata_info(&mtd_rootfs_data_addr, &mtd_rootfs_data_size)) {
				printf("[MTK] Erase rootfs_data: addr=0x%lx, len=0x%lx\n", mtd_rootfs_data_addr, mtd_rootfs_data_size);
				flash_erase(mtd_rootfs_data_addr, mtd_rootfs_data_size);
				printf("Done...\n");
				run_command("go", 0);
			} else
				printf("No rootfs_data partition found!\n");
		}

		if (!strcmp(s, getenv("uboot_filename")))
		{
			printf("reset...\n");
			run_command("reset", 0);
		}
#endif


}
#endif

int get_tclinux_img_version(const unsigned long addr, struct trx_header	*trxHdr)
{
	unsigned long		retlen = 0;
	unsigned char		*buf = (unsigned char *)trxHdr;
	const u32			len = sizeof(struct trx_header);

#if DBG_ECNT_IMAGE
	printf("read: src=0x%lx, len=0x%lx, dst=0x%lx\n", addr, len, buf);
#endif

	memset(buf, 0, len);

	/* Read trx header */
	if(image_read(addr + get_fip_offset(), len, &retlen, buf)) {
		printf("%s, image_read error! src=0x%lx, len=0x%lx, dst=0x%lx\n", __func__, addr, (unsigned long)len, (unsigned long)buf);
		return -1;
	}

	/* compare trx magic */
	if(trxHdr->magic != STORE32_LE(TRX_MAGIC2)) {
		printf("%s, read trx magic(0x%x) error.\n", __func__, STORE32_LE(TRX_MAGIC2));
		return -1;
	}

	return 0;
}

