/*
 * (C) Copyright 2012 Stephen Warren
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

/**********************************************************************************************
 *                                      ARM Cortex A7
 **********************************************************************************************/

#if 0
#define CONFIG_ARMV7_LPAE
#endif
#if 1
#define CONFIG_CPU_V7
#define CONFIG_SYS_ARM_CACHE_WRITEALLOC
#endif

#define CONFIG_CMD_DHCP
/* Over write */
#define CONFIG_ENV_OVERWRITE

/* bootargs */
#define BOOTARGS_STR_MAX_LEN	1024
#define TCLINUX_IMG_INFO_STR	("tclinux_info")

/* mi.conf */
#define CONFIG_ENV_SDRAM_CONF			"0x00108893"
#define CONFIG_ENV_VENDOR_NAME			"ECONET Technologies Corp."
#define CONFIG_ENV_PRODUCT_NAME			"xPON ONU"
#define CONFIG_ENV_SNMP_SYSOBJID		"1.2.3.4.5"
#define CONFIG_ENV_COUNTRY_CODE			"ff"
#define CONFIG_ENV_ETHER_GPIO			"0c"
#define CONFIG_ENV_POWER_GPIO			"1515"
#define CONFIG_ENV_USERNAME				"telecomadmin"
#define CONFIG_ENV_PASSWORD				"nE7jA%5m"
#define CONFIG_ENV_DSL_GPIO				"0b"
#define CONFIG_ENV_INTERNET_GPIO		"02"
#define CONFIG_ENV_MULTI_UPGRADE_GPIO	"0b020400000000000000000000000000"
#define CONFIG_ENV_ONU_TYPE				"2"
#define CONFIG_ENV_QDMA_INIT			"11"
#define CONFIG_ENV_ROOT					"/dev/mtdblock5 ro"
#define CONFIG_ENV_ROOT_SLAVE			"/dev/mtdblock8 ro"
#define CONFIG_ENV_CONSOLE				"ttyS0,115200n8 earlycon"
#define CONFIG_ENV_SERDES_SEL			"0"
#define CONFIG_ENV_BOARD_ARGS 			""
#define CONFIG_RAMFS_BASE               0x8B000000

/* Machine ID */
#define CONFIG_MACH_TYPE                    7523
#define CONFIG_ECNT_UBOOT
#define CONFIG_ECNT
#define CONFIG_ECNT_FLASH
#define CONFIG_ECNT_NET
#define CONFIG_PHY_AIROHA_EN8811H
#define CONFIG_ECNT_BOOTARGS
#define CONFIG_ECNT_IMAGE
#define CONFIG_ECNT_TFTP_AUTO_UPGRADE
#define CONFIG_ECNT_MULTIUPGRADE



#define CONFIG_ECNT_EFUSE
#if 0
/* enable IRQ*/
#define CONFIG_USE_IRQ
#endif

#if 0
#define CONFIG_ECNT_FLASH_TEST
#endif
#if 0 /* test in ASIC Only */
#define CONFIG_ECNT_CPUFREQ_TEST
#endif
#if 0
#define BOOT_ON_L2C_SRAM        (1)
#define TEST_BLOCK_CNT_IN_SRAM  (1)
#endif
#if 0
#define CONFIG_SPL
#define CONFIG_TPL
#endif
#ifdef CONFIG_SPL
#define CONFIG_SPL_LIBGENERIC_SUPPORT   /* string.c */
#ifdef CONFIG_TPL_BUILD
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_YMODEM_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SPL_MAX_SIZE					0x1F800
#ifdef BOOT_ON_L2C_SRAM
#define CONFIG_SPL_TEXT_BASE				0x1EFC0800
#else
#define CONFIG_SPL_TEXT_BASE				0x1F000800
#endif
#define CONFIG_SPL_STACK					(CONFIG_SPL_TEXT_BASE + \
                                                CONFIG_SPL_MAX_SIZE - 4)
#elif defined(CONFIG_SPL_BUILD)
#define CONFIG_SPL_MAX_SIZE					0x800
#define CONFIG_SPL_TEXT_BASE				0x0
#else
#define CONFIG_TPL_PAD_TO					0x1F800
#define CONFIG_SPL_PAD_TO					0x800
#endif
#define CONFIG_SPL_TARGET					"u-boot-with-spl.bin"
#endif

/*enable IRQ if multiupgrade is enabled. */
#ifdef CONFIG_ECNT_MULTIUPGRADE
#define CONFIG_USE_IRQ
#endif

/* enable the following CONFIG when CONFIG_USE_IRQ is enabled*/
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ (4*1024)
#define CONFIG_STACKSIZE_FIQ (4*1024)
#endif

#define CONFIG_CMD_NMRP	1
#define CONFIG_LED_BLINK 1
#define CONFIG_AES 1

/**********************************************************************************************
 *                                          Memory
 **********************************************************************************************/
/* Memory layout */
/* DRAM definition */
/*
 * Iverson 20140521 : We detect ram size automatically.
 *      CONFIG_SYS_SDRAM_SIZE define max uboot size.
 *      The max size that auto detection support is 256MB.
 */
#define CONFIG_NR_DRAM_BANKS		        1
#define CONFIG_SYS_SDRAM_BASE		        0x80000000

#define CONFIG_SYS_SDRAM_SIZE               SZ_1G

/* Code Layout */
#ifdef BOOT_ON_L2C_SRAM
#define CONFIG_TPL_TEXT_BASE				0x1EFC0800
#else
#define CONFIG_TPL_TEXT_BASE				0x1F000800
#endif
#define CONFIG_SYS_TEXT_BASE		        0x81E00000

/* Uboot definition */
#define CONFIG_SYS_UBOOT_BASE		        CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_UBOOT_MAX_SIZE           SZ_2M
#define CONFIG_SYS_INIT_SP_ADDR             (CONFIG_SYS_TEXT_BASE + \
                                                CONFIG_SYS_UBOOT_MAX_SIZE - \
                                                GENERATED_GBL_DATA_SIZE)


#define CONFIG_SYS_MALLOC_LEN               SZ_32M


/* RichOS memory partitions */
#define CONFIG_SYS_DECOMP_ADDR				0x80080000
#define CONFIG_SYS_LOAD_ADDR				0x81800000

/* Linux DRAM definition */
#define CONFIG_LOADADDR			            CONFIG_SYS_LOAD_ADDR

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 64 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CONFIG_SYS_BOOTM_LEN	            0x4000000

/**********************************************************************************************
 *                                           Board
 **********************************************************************************************/

/* Board */
#define CONFIG_BOARD_LATE_INIT

/**********************************************************************************************
 *                                          Devices
 **********************************************************************************************/

/********************** Flash *************************/
#define CONFIG_SYS_NO_FLASH
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_SIZE                     SZ_16K
#if 0
#define CONFIG_BOARD_SIZE_LIMIT				 $$((240 * 1024))
#endif

#define CONFIG_ATF_OFFSET					0x20000
#define CONFIG_ATF_MAX_SIZE					0x20000
#define CONFIG_UBOOT_OFFSET					0x40000
#define CONFIG_UBOOT_MAX_SIZE				0x3C000
#define CONFIG_ENV_MTK_OFFSET				ecnt_get_ubootenv_mtd_offset()
#define CONFIG_ENV_AIROHA_OFFSET			0x7C000
#define CONFIG_TCBOOT_OFFSET				0x0
#define CONFIG_TCLINUX_OFFSET				ecnt_get_tclinux_flash_offset()
#define CONFIG_MTK_ALLINONE_OFFSET			0x0

#define ENV_BOOT_WRITE_IMAGE "boot_wr_img=none\0"
#define ENV_BOOT_READ_IMAGE "boot_rd_img=none\0"
#define ENV_WRITE_UBOOT "wr_uboot=none\0"
#define ENV_WRITE_ATF
#define ENV_WRITE_PRELOADER
#define ENV_WRITE_ROM_HEADER
#define ENV_WRITE_CTP
#define ENV_BOOT_READ_CTP
#define ENV_WRITE_FLASHIMAGE
#define ENV_MOVE_DISABLE	"0"
#define ENV_MOVE_ENABLE		"1"

#define CONFIG_ENV_VARS_UBOOT_CONFIG


/********************** Watchdog *************************/
#define CFG_HW_WATCHDOG 1
#define CONFIG_WATCHDOG_OFF


/********************** Console UART *************************/
/* Uart baudrate */
#define CONFIG_BAUDRATE                     115200

/* Console configuration */
#define CONFIG_SYS_CBSIZE		            1024
#define CONFIG_SYS_PBSIZE		            (CONFIG_SYS_CBSIZE +		\
					                            sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#define ENV_DEVICE_SETTINGS \
	"stdin=serial\0" \
	"stdout=serial\0" \
	"stderr=serial\0"

/********************** Ethernet *************************/
#define CONFIG_ETHADDR                      00:AA:BB:01:23:40
#define CONFIG_IPADDR                       192.168.1.1
#define CONFIG_SERVERIP                     192.168.1.126
#define CONFIG_BOOTFILE                     "tclinux.bin"
#define CONFIG_CMD_NET
#define CONFIG_CMD_TFTPSRV

#define RALINK_REG(x)		(*((volatile u32 *)(x)))

/**********************************************************************************************
 *                                          Dual Image
 **********************************************************************************************/
#define CONFIG_BOOTFLAG_SWAP_COMMAND	"bootflag swap"

/**********************************************************************************************
 *                                       Boot Menu
 **********************************************************************************************/
#ifdef TCSUPPORT_AUTOBENCH
#define CONFIG_BOOTDELAY                    1
#else
#ifdef CONFIG_ECNT_MULTIUPGRADE
#define CONFIG_BOOTDELAY                    3
#else
#define CONFIG_BOOTDELAY                    1
#endif
#endif
#define CONFIG_BOOTCOMMAND_READ_LEN			2048
#define CONFIG_BOOTCOMMAND                  "flash imgread "__stringify(CONFIG_BOOTCOMMAND_READ_LEN)";bootm"

#define CONFIG_EXTRA_ENV_SETTINGS \
	"kernel_filename=tclinux.bin\0" \
	"uboot_filename=tcboot.bin\0" \
	"singleImage_filename=mtk_allinone.bin\0" \
	"fdt_high=0xac000000\0" \
	"invaild_env=no\0" \
	"serdes_pon=121\0" \
	ENV_DEVICE_SETTINGS

/**********************************************************************************************
 *                                      FDT
 **********************************************************************************************/
#define CONFIG_FIT
#define CONFIG_OF_LIBFDT
#define CONFIG_FDT_DEBUG

/**********************************************************************************************
 *                                      ATF
 **********************************************************************************************/
#define CONFIG_MTK_ATF

/**********************************************************************************************
 *                                KERNEL CheckSum
 **********************************************************************************************/
#define CONFIG_SPL_SHA1_SUPPORT

/**********************************************************************************************
 *                                       UBoot Command
 **********************************************************************************************/
/* Shell */
#define CONFIG_SYS_MAXARGS		            8
#define CONFIG_SYS_PROMPT		            "ECNT> "
#define CONFIG_COMMAND_HISTORY

/* Commands */
#include <config_cmd_default.h>

/* Device tree support */
#define CONFIG_OF_BOARD_SETUP
/* ATAGs support for bootm/bootz */
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG

#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_NFS

#define CONFIG_SYS_LONGHELP
#define CONFIG_CMD_PING

#define CONFIG_CMD_ECNT

#define CONFIG_CMD_NAND
#define CONFIG_SYS_MAX_NAND_DEVICE 1
#define CONFIG_SYS_NAND_BASE 0

#define CONFIG_SYS_HUSH_PARSER

#define CONFIG_CMD_CACHE

/**********************************************************************************************
 *                                      Compression
 **********************************************************************************************/
#define CONFIG_LZMA

#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_CMD_MII
#define CONFIG_PHYLIB

#endif
