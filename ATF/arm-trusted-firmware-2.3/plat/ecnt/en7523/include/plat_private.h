/*
 * Copyright (c) 2014-2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_PRIVATE_H
#define PLAT_PRIVATE_H

#include <stdint.h>
#include <stddef.h>

typedef struct hw_trap {
	uint32_t skip_fw_upgrade		:1;
	uint32_t fw_upgrade_mode		:1;
	uint32_t inc_mode				:1;
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7552)
	uint32_t hw_trap_decode			:7;
	uint32_t inc_mdio_mode			:1;
	uint32_t is_emmc				:1;
	uint32_t is_parallel_nand		:1;
	uint32_t is_spi_nor				:1;
	uint32_t is_spi_nand_device_ecc	:1;
	uint32_t is_spi_nand_ctrl_ecc	:1;
	uint32_t reserved				:16;
#else //TCSUPPORT_CPU_EN7523
	uint32_t hw_trap_decode 		:6;
	uint32_t reserved				:23;
#endif
} hw_trap_t;

/*******************************************************************************
 * Function and variable prototypes
 ******************************************************************************/
#if defined(IMAGE_BL31)
int efuse_write_pkgid(uint8_t id, uint8_t remark);
int efuse_write_secure_key(uint8_t *p_buf);
int efuse_check_secure_key(unsigned int disableMsg);
int efuse_test(unsigned int mode, unsigned int sub);
int get_hash_mode_by_efuse(void);
#endif
int ecnt_system_init(unsigned long long *p_dram_size);
unsigned char ef_read_byte(unsigned int index);
unsigned int is_asic(void);
int get_asic_mode_by_efuse(void);
int get_into_inic(void);
#ifndef TCSUPPORT_CPU_EN7581
int get_freq_by_efuse(void);
int get_freq_sel(void);
#endif
void fill_secure_data(uint8_t *p_data, uint8_t offset, size_t len);
int efuse_init(void);
void plat_ecnt_io_setup(void);
int plat_check_bypass(void);
int plat_check_header(uint8_t *base);
void plat_invaild_clean_cache();
#if !defined(IMAGE_BL31) && !defined(IMAGE_BL21) && !defined(IMAGE_BL22)
int plat_get_dual_boot(void);
int plat_get_hw_bypass(void);
#endif

void jumparch64(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
#ifdef AARCH32
void plat_configure_mmu_svc_mon(unsigned long total_base,
			    unsigned long total_size,
			    unsigned long,
			    unsigned long,
			    unsigned long,
			    unsigned long);
#else
void plat_configure_mmu_el3(unsigned long total_base,
			    unsigned long total_size,
			    unsigned long,
			    unsigned long,
			    unsigned long,
			    unsigned long);
#endif
/* Declarations for plat_topology.c */
int mt_setup_topology(void);
#endif /* PLAT_PRIVATE_H */
