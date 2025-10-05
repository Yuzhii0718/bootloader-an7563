/*
 * (C) Copyright 2000-2009
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifdef TCSUPPORT_CPU_EN7581
#include <configs/en7581_evb.h>
#else
#include <configs/en7523_evb.h>
#endif
#include <ecnt/image/ecnt_image.h>
#include <flashhal.h>
#include <spi/spi_nand_flash.h>
#include <bmt.h>
#include "flash_layout/tc_partition.h"
#include <asm/io.h>

#define DBG_MI_CONF 0
#define DBG_BOOTARGS 0


extern unsigned int get_fip_offset(void);
extern unsigned long long ecnt_memparse(const char *ptr, char **retptr, unsigned int blocksize);

extern struct mtd_info mtd;

char *mi_conf[] = {"sdram_conf",
					"vendor_name",
					"product_name",
					"ethaddr",
					"snmp_sysobjid",
					"country_code",
					"ether_gpio",
					"power_gpio",
					"username",
					"password",
					"dsl_gpio",
					"internet_gpio",
					"multi_upgrade_gpio",
					"onu_type",
					"qdma_init",
					"root",
					"console",
					"bootflag",
					"serdes_sel",
					"iommu",
					"mem",
					"swiotlb",
					"dram_limit",
					"iommu.passthrough",
					"serdes_pon",
					"serdes_ethernet",
					"serdes_wifi1",
					"serdes_wifi2",
					"serdes_usb1",
					"serdes_usb2",
					"sn_test",
					"board_args",
					TCLINUX_IMG_INFO_STR};
#if defined(TCSUPPORT_AUTOBENCH) && defined(TCSUPPORT_CPU_EN7581)
static void set_serdes_gpio(void)
{
	unsigned int val;
	//set usb gpio
	printf("==== set usb gpio \n");
	if(isAN7581PT || isAN7581FP || isAN7581FD || isAN7581CT || isAN7581ST || isAN7581IT || isAN7566PT || isAN7551PT)
	{
		//usb0: usb3.0 gpio:45=1, gpio:46=0
		val = readl(0x1fbf0260);
		val |= (0x1400 << 16);
		writel(val,0x1fbf0260);
		val = readl(0x1fbf0278);
		val |= (0x6 << 12);
		writel(val,0x1fbf0278);
		val = readl(0x1fbf0270);
		val &= ~(1 << 14);
		val |= (1 << 13);
		writel(val,0x1fbf0270);
		
	}
	else if(isAN7581DT)
	{
		//usb0 : HSGMII gpio:45=1, gpio:46=1
		val = readl(0x1fbf0260);
		val |= (0x1400 << 16);
		writel(val,0x1fbf0260);
		val = readl(0x1fbf0278);
		val |= (0x6 << 12);
		writel(val,0x1fbf0278);
		val = readl(0x1fbf0270);
		val |= (1 << 14);
		val |= (1 << 13);
		writel(val,0x1fbf0270);
	}
	else if(isAN7581GT || isAN7581FG || isAN7551GT || isAN7566GT)
	{
		//usb0: usb3.0 gpio:45=1, gpio:46=0
		val = readl(0x1fbf0260);
		val |= (0x1400 << 16);
		writel(val,0x1fbf0260);
		val = readl(0x1fbf0278);
		val |= (0x6 << 12);
		writel(val,0x1fbf0278);
		val = readl(0x1fbf0270);
		val &= ~(1 << 14);
		val |= (1 << 13);
		writel(val,0x1fbf0270);
		//usb1 : pcie gpio:15=0 gpio:16=1
		val = readl(0x1fbf0200);
		val |= 0x40000000;
		writel(val,0x1fbf0200);
		val = readl(0x1fbf0220);
		val |= 0x1;
		writel(val,0x1fbf0220);
		val = readl(0x1fbf0214);
		val |= 0x18000;
		writel(val,0x1fbf0214);
		val = readl(0x1fbf0204);
		val |= 0x10000;
		val &= ~(0x8000);
		writel(val,0x1fbf0204);
		
	}
	//set pon gpio
	printf("==== set pon gpio \n");
	if(isAN7581PT || isAN7581FP || isAN7581FD || isAN7581CT || isAN7581ST || isAN7581IT || isAN7581DT || isAN7581GT || isAN7581FG ||isAN7551PT ||isAN7551GT)
	{
		//pon : 7573 gpio:35=1, gpio:42=1
		val = readl(0x1fbf0260);
		val |= 0x100040;
		writel(val,0x1fbf0260);
		val = readl(0x1fbf0278);
		val |= 0x408;
		writel(val,0x1fbf0278);
		val = readl(0x1fbf0270);
		val |= 0x400;
		val |= 0x8;
		writel(val,0x1fbf0270);
	}
	else if(isAN7566PT || isAN7566GT)
	{
		//pon : USXGMII gpio:35=0, gpio:42=1
		val = readl(0x1fbf0260);
		val |= 0x100040;
		writel(val,0x1fbf0260);
		val = readl(0x1fbf0278);
		val |= 0x408;
		writel(val,0x1fbf0278);
		val = readl(0x1fbf0270);
		val |= 0x400;
		val &= ~(0x8);
		writel(val,0x1fbf0270);
	}
	//set pcie gpio
	printf("==== set pcie gpio \n");
	if(isAN7581PT || isAN7581FP || isAN7581IT || isAN7566PT || isAN7551PT || isAN7581CT)
	{
		//pcie0 : MINI CARD gpio:3=0, gpio:4=1
		val = readl(0x1fbf0200);
		val |= 0x140;
		writel(val,0x1fbf0200);
		val = readl(0x1fbf0214);
		val |= 0x18;
		writel(val,0x1fbf0214);
		val = readl(0x1fbf0204);
		val |= 0x10;
		val &= ~(0x8);
		writel(val,0x1fbf0204);
		
		//pcie1 : MINI CARD gpio:13=0, gpio:14=1
		val = readl(0x1fbf0200);
		val |= 0x14000000;
		writel(val,0x1fbf0200);
		val = readl(0x1fbf0214);
		val |= 0x6000;
		writel(val,0x1fbf0214);
		val = readl(0x1fbf0204);
		val |= 0x4000;
		val &= ~(0x2000);
		writel(val,0x1fbf0204);
		
	}	
	else if(isAN7581DT)  
	{
		//pcie0 : NA gpio:3=0, gpio:4=0
		val = readl(0x1fbf0200);
		val |= 0x140;
		writel(val,0x1fbf0200);
		val = readl(0x1fbf0214);
		val |= 0x18;
		writel(val,0x1fbf0214);
		val = readl(0x1fbf0204);
		val &= ~(0x10);
		val &= ~(0x8);
		writel(val,0x1fbf0204);
		
		//pcie1 : NA gpio:13=0, gpio:14=0
		val = readl(0x1fbf0200);
		val |= 0x14000000;
		writel(val,0x1fbf0200);
		val = readl(0x1fbf0214);
		val |= 0x6000;
		writel(val,0x1fbf0214);
		val = readl(0x1fbf0204);
		val &= ~(0x4000);
		val &= ~(0x2000);
		writel(val,0x1fbf0204);
		
	}
	else if(isAN7581FD || isAN7581ST)  
	{
		//pcie0 : USXGMII gpio:3=1, gpio:4=1 
		val = readl(0x1fbf0200);
		val |= 0x140;
		writel(val,0x1fbf0200);
		val = readl(0x1fbf0214);
		val |= 0x18;
		writel(val,0x1fbf0214);
		val = readl(0x1fbf0204);
		val |= (0x10); // use pcie 1 need pcie 0 up
		val |= (0x8);
		writel(val,0x1fbf0204);
		
		//pcie1 : USXGMII gpio:13=1, gpio:14=1
		val = readl(0x1fbf0200);
		val |= 0x14000000;
		writel(val,0x1fbf0200);
		val = readl(0x1fbf0214);
		val |= 0x6000;
		writel(val,0x1fbf0214);
		val = readl(0x1fbf0204);
		val |= 0x4000;
		val |= 0x2000;
		writel(val,0x1fbf0204);

	}
	else if(isAN7581GT || isAN7581FG || isAN7551GT || isAN7566GT)
	{
		//pcie0 : Lan2 gpio:3=0, gpio:4=1
		val = readl(0x1fbf0200);
		val |= 0x140;
		writel(val,0x1fbf0200);
		val = readl(0x1fbf0214);
		val |= 0x18;
		writel(val,0x1fbf0214);
		val = readl(0x1fbf0204);
		val |= 0x10;
		val &= ~(0x8);
		writel(val,0x1fbf0204);
		
		//pcie0 : Lan2 gpio:13=0, gpio:14=1
		val = readl(0x1fbf0200);
		val |= 0x14000000;
		writel(val,0x1fbf0200);
		val = readl(0x1fbf0214);
		val |= 0x6000;
		writel(val,0x1fbf0214);
		val = readl(0x1fbf0204);
		val |= 0x4000;
		val &= ~(0x2000);
		writel(val,0x1fbf0204);
		
	}
	
}

static void set_serdes_config(void)
{
	int i;
	int n_mi_conf;
	char *val;
	
	n_mi_conf = (sizeof(mi_conf) / sizeof(const char *));

	for(i = 0; i < n_mi_conf; i++){
		/*wifi serdes*/
		if(isAN7581PT|| isAN7581CT|| isAN7551PT || isAN7566PT||isAN7581FP)
		{
			if(strcmp(mi_conf[i],"serdes_wifi1")==0)// pt
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"01")!=0)
				{
					printf("set wifi1 serdes to 1 lane\n");
					setenv("serdes_wifi1", "01"); //lane 1
				}
			}
			if(strcmp(mi_conf[i],"serdes_wifi2")==0)//  pt
			{	
				val = getenv(mi_conf[i]);
				
				if(strcmp(val,"01")!=0)
				{
					printf("set wifi2 serdes to 1 lane\n");
					setenv("serdes_wifi2", "01"); // lane 1
				}
			}
		}
		else if(isAN7581DT)
		{
			if(strcmp(mi_conf[i],"serdes_wifi1")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"05")!=0)
				{
					printf("set wifi1 serdes to NA\n");
					setenv("serdes_wifi1", "05"); 
				}
			}
			if(strcmp(mi_conf[i],"serdes_wifi2")==0)
			{	
				val = getenv(mi_conf[i]);
				
				if(strcmp(val,"05")!=0)
				{
					printf("set wifi2 serdes to NA\n");
					setenv("serdes_wifi2", "05"); 
				}
			}
		}
		else if(isAN7581ST||isAN7581FD)
		{
			if(strcmp(mi_conf[i],"serdes_wifi1")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"13")!=0)
				{
					printf("set wifi1 serdes to USXGMII\n"); // use pcie 1 need pcie 0 up
					setenv("serdes_wifi1", "13"); 
				}
			}
			if(strcmp(mi_conf[i],"serdes_wifi2")==0)
			{	
				val = getenv(mi_conf[i]);
				
				if(strcmp(val,"13")!=0)
				{
					printf("set wifi2 serdes to USXGMII\n");
					setenv("serdes_wifi2", "13"); 
				}
			}
		}
		else if(isAN7581GT || isAN7581FG || isAN7551GT || isAN7566GT)
		{
			//lan2
			if(strcmp(mi_conf[i],"serdes_wifi1")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"00")!=0)
				{
					printf("set wifi1 serdes to PCIE 2-lane\n");
					setenv("serdes_wifi1", "00"); 
				}
			}
			if(strcmp(mi_conf[i],"serdes_wifi2")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"00")!=0)
				{
					printf("set wifi2 serdes to PCIE 2-lane\n");
					setenv("serdes_wifi2", "00"); 
				}
			}
		}

		/*usb serdes*/
		if(isAN7581DT)
		{
			if(strcmp(mi_conf[i],"serdes_usb1")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"11")!=0)
				{
					printf("set usb1 serdes to HSGMII\n");
					setenv("serdes_usb1", "11"); 
				}
			}
		}
		else if(isAN7581GT || isAN7581FG || isAN7551GT || isAN7566GT)
		{
			if(strcmp(mi_conf[i],"serdes_usb2")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"01")!=0)
				{
					printf("set usb2 serdes to PCIE 1-lane\n");
					setenv("serdes_usb2", "01"); 
				}
			}
		}
		
		/*pon serdes*/
		if(isAN7566PT || isAN7566GT)
		{
			if(strcmp(mi_conf[i],"serdes_pon")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"13")!=0)
				{
					printf("set pon serdes to USXGMII\n");
					setenv("serdes_pon", "13"); 
				}
			}
		}

		/*ethernet serdes*/
		if(isAN7581CT || isAN7581DT || isAN7551PT || isAN7551GT)
		{
			if(strcmp(mi_conf[i],"serdes_ethernet")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"12")!=0)
				{
					printf("set ethernet serdes to USGMII\n");
					setenv("serdes_ethernet", "12"); 
				}
			}
		}
		else if(isAN7581FP || isAN7581PT || isAN7581IT ||isAN7581GT ||isAN7581ST||isAN7566PT ||isAN7581FD || isAN7581FG || isAN7566GT )
		{
			if(strcmp(mi_conf[i],"serdes_ethernet")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"11")!=0)
				{
					printf("set ethernet serdes to USXGMII\n");
					setenv("serdes_ethernet", "11"); 
				}
			}
		}

#if defined(TCSUPPORT_CPU_AN7552)
		/*pon serdes*/
		if(isAN7563PT || isAN7563CT)
		{
			if(strcmp(mi_conf[i],"serdes_pon")==0)
			{	
				val = getenv(mi_conf[i]);
			
				if(strcmp(val,"11")!=0)
				{
					printf("set pon serdes to HSGMII\n");
					setenv("serdes_pon", "11"); 
				}
			}
		}
#endif

	}	
}
#endif

static int parse_env_config(char *var)
{
	int i;
	int n_mi_conf;
	char *val;
	unsigned int totalLen = 0;

	n_mi_conf = (sizeof(mi_conf) / sizeof(const char *));
	totalLen = strlen(var);

	for(i = 0; i < n_mi_conf; i++) {
		val = getenv(mi_conf[i]);
#if DBG_MI_CONF
		printf("=== totalLen:%d ===\n", totalLen);
		printf("parse %s=%s\n", mi_conf[i], val);
#endif
		if(val) {
			totalLen += (strlen(mi_conf[i]) + strlen("=")
						+ strlen(val) + strlen(" "));
			if(totalLen >= (BOOTARGS_STR_MAX_LEN - 1)) {
				printf("bootargs len:%d more than %d ===\n", totalLen, BOOTARGS_STR_MAX_LEN);
				return -1;
			}
			
			strncat(var, mi_conf[i], BOOTARGS_STR_MAX_LEN - 1);
			strncat(var, "=", BOOTARGS_STR_MAX_LEN - 1);
			strncat(var, val, BOOTARGS_STR_MAX_LEN - 1);
			strncat(var, " ", BOOTARGS_STR_MAX_LEN - 1);
		}
	}
	
	return 0;
}

static int set_tclinux_img_env(struct tclinux_imginfo *firstInfo, struct tclinux_imginfo *secondInfo)
{
	char info_str[86 * 2] = ""; /* 86 = 16 * 5 + 5 char ("," * 4 + " ") */
	char tclinux_size_str[16] = {0};
	char kernel_off_str[16] = {0};
	char kernel_size_str[16] = {0};
	char rootfs_off_str[16] = {0};
	char rootfs_size_str[16] = {0};

	if(firstInfo) {
		sprintf(tclinux_size_str, "0x%x", firstInfo->tclinux_size);
		sprintf(kernel_off_str, "0x%x", (firstInfo->kernel_off + get_fip_offset()));
		sprintf(kernel_size_str, "0x%x", firstInfo->kernel_size);
		sprintf(rootfs_off_str, "0x%x", (firstInfo->rootfs_off + get_fip_offset()));
		sprintf(rootfs_size_str, "0x%x", firstInfo->rootfs_size);
		strncat(info_str, tclinux_size_str, sizeof(info_str) - 1);
		strncat(info_str, ",", sizeof(info_str) - 1);
		strncat(info_str, kernel_off_str, sizeof(info_str) - 1);
		strncat(info_str, ",", sizeof(info_str) - 1);
		strncat(info_str, kernel_size_str, sizeof(info_str) - 1);
		strncat(info_str, ",", sizeof(info_str) - 1);
		strncat(info_str, rootfs_off_str, sizeof(info_str) - 1);
		strncat(info_str, ",", sizeof(info_str) - 1);
		strncat(info_str, rootfs_size_str, sizeof(info_str) - 1);
	}

	if(secondInfo) {
		strncat(info_str, ",", sizeof(info_str) - 1);
		sprintf(tclinux_size_str, "0x%x", secondInfo->tclinux_size);
		sprintf(kernel_off_str, "0x%x", (secondInfo->kernel_off + get_fip_offset()));
		sprintf(kernel_size_str, "0x%x", secondInfo->kernel_size);
		sprintf(rootfs_off_str, "0x%x", (secondInfo->rootfs_off + get_fip_offset()));
		sprintf(rootfs_size_str, "0x%x", secondInfo->rootfs_size);
		strncat(info_str, tclinux_size_str, sizeof(info_str) - 1);
		strncat(info_str, ",", sizeof(info_str) - 1);
		strncat(info_str, kernel_off_str, sizeof(info_str) - 1);
		strncat(info_str, ",", sizeof(info_str) - 1);
		strncat(info_str, kernel_size_str, sizeof(info_str) - 1);
		strncat(info_str, ",", sizeof(info_str) - 1);
		strncat(info_str, rootfs_off_str, sizeof(info_str) - 1);
		strncat(info_str, ",", sizeof(info_str) - 1);
		strncat(info_str, rootfs_size_str, sizeof(info_str) - 1);
	}

	setenv(TCLINUX_IMG_INFO_STR, info_str);

	return 0;
}

static void set_bootflag_env(unsigned int bootflag)
{
	if(bootflag == 0) {
		setenv("bootflag", "0");
	} else {
		setenv("bootflag", "1");
	}
}

// coverity[ +tainted_string_sanitize_content : arg-0]
static inline void __coverity_taint_getenv_check(char *s)
{
	(void)s;
}
static void check_dram_limit(void)
{
	char *val;
	unsigned long long size = 0;
	unsigned long long cal_size = 0;

	cal_size =  ((u64)GET_DRAM_SIZE * (u64)SZ_1M);

	val = getenv("dram_limit");
	if(val) {
		__coverity_taint_getenv_check(val);
		size = ecnt_memparse(val, 0, 0);

		if (cal_size >= size)
		{
			SET_DRAM_SIZE(size >> 20);
			printf("dram size is limited to %s\n", val);
		}
		else
		{
			printf("ERROR: dram limit value is exceed the calibrated dram size !! Use calibrated dram size as default.\n");
		}
	}
	else 
	{
		/* does not need limit the dram size, do nothing*/
	}
}

int bootargs_init(struct tclinux_imginfo *first_imginfo, struct tclinux_imginfo *second_imginfo, unsigned int bootflag)
{
	char var[BOOTARGS_STR_MAX_LEN] = {0};
	char *bootargs;
	
	if(set_tclinux_img_env(first_imginfo, second_imginfo)) {
		return -1;
	}

	set_bootflag_env(bootflag);
#if defined(TCSUPPORT_AUTOBENCH) && defined(TCSUPPORT_CPU_EN7581)
		set_serdes_gpio();
		set_serdes_config();
#endif

	/* set MAC address to kernel */
	if(parse_env_config(var)) {
		return -1;
	}

	check_dram_limit();

#if DBG_BOOTARGS
	printf("%s, var:%s\n", __func__, var);
#endif

	bootargs = getenv("bootargs");
	if(bootargs) {
		strncat(var, bootargs, BOOTARGS_STR_MAX_LEN - 1);
	}

	setenv("bootargs", var);
	return 0;
}

unsigned int get_boot_flag_addr(void)
{
	unsigned int boot_flag_addr = 0;
	unsigned int is_EMMC = 0;

#ifdef TCSUPPORT_EMMC
	is_EMMC = (unsigned int)isEMMC;
#endif

		if (IS_NANDFLASH || is_EMMC) {
        #if 0
			printf("\nflash_base:0x%x; avalable_size:0x%x, reservearea_size:0x%x, block size:%d, IMG_BOOT_FLAG_OFFSET:0x%x\n",
				flash_base, nand_flash_avalable_size, reservearea_size, TCSUPPORT_RESERVEAREA_BLOCK, IMG_BOOT_FLAG_OFFSET);
        #endif
#if defined(TCSUPPORT_OPENWRT)
			boot_flag_addr =  flash_base + ecnt_get_reservearea_flash_offset() +  reservearea_size*(TCSUPPORT_RESERVEAREA_BLOCK-1); 	
#else
			boot_flag_addr =  flash_base + ecnt_get_reservearea_flash_offset() + IMG_BOOT_FLAG_OFFSET;
#endif
		}
#ifdef TCSUPPORT_NEW_SPIFLASH	
		else {
        #if 0
			printf("\nflash_base: %x; mtd.size: %x, erasesize: %x, block size: %d, IMG_BOOT_FLAG_OFFSET: %x\n",
				flash_base, mtd.size, mtd.erasesize, TCSUPPORT_RESERVEAREA_BLOCK, IMG_BOOT_FLAG_OFFSET);
        #endif
			boot_flag_addr =  flash_base + mtd.size - mtd.erasesize * TCSUPPORT_RESERVEAREA_BLOCK + IMG_BOOT_FLAG_OFFSET;
		}
#endif

	return boot_flag_addr;
}

char readBootFlagFromFlash(void)
{
	unsigned long retlen = 0;
	unsigned int boot_flag_addr = 0;
	char flag = 0;
	unsigned int is_EMMC = 0;

	boot_flag_addr=get_boot_flag_addr();

#ifdef TCSUPPORT_EMMC
	is_EMMC = (unsigned int)isEMMC;
#endif

	if (IS_NANDFLASH || is_EMMC) {
        flash_read(boot_flag_addr, 1, &retlen, &flag);
    }
#ifdef TCSUPPORT_NEW_SPIFLASH	
    else {
        memcpy(&flag, boot_flag_addr, 1);
    }
#endif

	flag = flag - '0';
	//printf("boot_flag_addr:0x%x; flag:0x%x\n", boot_flag_addr, flag);
	if (flag != 0 && flag != 1){
		flag = 0;
        printf("(flag != 0 && flag != 1) --> boot from Master\n");
	}

	return flag;
}

int writeBootFlagtoFlash(char flag)
{
	unsigned int boot_flag_addr = 0;
    char old_flag;
	int retVal;

	if (flag != 0 && flag != 1){
		printf("\nError: flag:%d !=0 && !=1\n", flag);
		return -1;
	}

    old_flag = readBootFlagFromFlash();
    if (old_flag==flag) {
        printf("\nboot_flag is %d already, so just exit\n", flag);
        return -1;
    }
    
	flag = flag + '0';
	boot_flag_addr=get_boot_flag_addr();

    printf("write %c to boot_flag_addr:0x%x\n", flag, boot_flag_addr);
    
	retVal = flash_partial_write(boot_flag_addr, 1, &flag);
    
    printf("\nnew bootflag==%d\n", readBootFlagFromFlash());

	return retVal;
}

int swap_bootflag(void)
{
    char bootflag = readBootFlagFromFlash();

    if (bootflag==0)
        bootflag=1;
    else
        bootflag=0;

    return writeBootFlagtoFlash(bootflag);
}

