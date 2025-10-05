#
# Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
NEED_INC				:=	0
CPU_BUS_BL2_TEST		:=	0

NEED_BL31				:=	yes
NEED_BL33				:=	yes
ifneq ($(TCSUPPORT_ARM_SECURE_BOOT),)
TRUSTED_FW				:=	1
else
TRUSTED_FW				:=	0
endif
AES						:=	none
AES_FW					:=	none
MBEDTLS_DIR				:=	../mbedtls-2.18.0
TESTFILE_NAME			:=	${BUILD_PLAT}/test.bin
EFUSE_NAME				:=	${BUILD_PLAT}/ecntefuse.bin
BYPASS_FWUPGRADE		:=	0

FIP_NAME				:=	preloader.bin
FIP_ALIGN				:=	1024

ifneq (${BYPASS_FWUPGRADE},0)
FIP_NAME				:=	preloader_bypass.bin
TESTFILE_NAME			:=	${BUILD_PLAT}/test_bypass.bin
endif

ifeq ($(IMAGE_BL21),1)
$(eval $(call add_define,IMAGE_BL21))
IMAGE_BL21				:= 1
endif
ifeq ($(IMAGE_BL22),1)
$(eval $(call add_define,IMAGE_BL22))
IMAGE_BL22				:= 1
endif
ifeq ($(IMAGE_BL23),1)
$(eval $(call add_define,IMAGE_BL23))
IMAGE_BL23				:= 1
endif

ECNT_PLAT				:=	plat/ecnt
ECNT_PLAT_SOC			:=	${ECNT_PLAT}/${PLAT}

PLAT_INCLUDES			:=	-I${ECNT_PLAT}/common/					\
				-I${ECNT_PLAT}/common/drivers/uart/					\
				-I${ECNT_PLAT}/common/drivers/flash/				\
				-I${ECNT_PLAT}/common/drivers/xmodem/				\
				-I${ECNT_PLAT_SOC}/include/							\
				-Iinclude/plat/arm/common/							\
				-I$(GLOBAL_INC_DIR)									\
				-I$(GLOBAL_INC_DIR)/modules							\

ifeq ($(CPU_BUS_BL2_TEST),1)
PLAT_INCLUDES += -DCPU_BUS_BL2_TEST
endif

	include lib/xlat_tables_v2/xlat_tables.mk
ifeq ($(IMAGE_BL21)$(IMAGE_BL22),)
    include drivers/auth/mbedtls/mbedtls_crypto.mk
    include drivers/auth/mbedtls/mbedtls_x509.mk
endif
    include plat/ecnt/common/drivers/flash/flash.mk
	include lib/lzma/lzma.mk
	include drivers/arm/gic/v3/gicv3.mk


ifeq ($(IMAGE_BL21)$(IMAGE_BL22),)
				AUTH_SOURCES		:=	drivers/auth/auth_mod.c					\
				drivers/auth/crypto_mod.c							\
				drivers/auth/img_parser_mod.c						\
				drivers/auth/tbbr/tbbr_cot.c
endif
	BL1_SOURCES			+= 	lib/cpus/${ARCH}/cortex_a53.S			\
				lib/cpus/${ARCH}/aem_generic.S						\
				${ECNT_PLAT_SOC}/${ARCH}/plat_helpers.S				\

ifeq ($(TCSUPPORT_BL2_OPTIMIZATION),)
	BL2_SOURCES			+=	${AUTH_SOURCES}							\
				${XLAT_TABLES_LIB_SRCS}								\
				drivers/delay_timer/delay_timer.c					\
				drivers/delay_timer/generic_delay_timer.c			\
				lib/cpus/${ARCH}/aem_generic.S						\
				lib/cpus/${ARCH}/cortex_a53.S						\
				${ECNT_PLAT_SOC}/platform_common.c					\
				${ECNT_PLAT_SOC}/${ARCH}/plat_helpers.S				\
				${ECNT_PLAT}/common/drivers/uart/ecnt_uart.S		\
				drivers/io/io_storage.c								\
				drivers/io/io_fip.c									\
				drivers/io/io_memmap.c								\
				drivers/io/io_encrypted.c							\
				common/image_decompress.c							\
				common/desc_image_load.c							\
				$(ECNT_PLAT_SOC)/ecnt_npu_img.S						\
				${ECNT_PLAT_SOC}/ecnt_trusted_boot.c				\
				${ECNT_PLAT_SOC}/ecnt_io_storage.c					\
				${ECNT_PLAT_SOC}/ecnt_bl2_setup.c					\
				${ECNT_PLAT_SOC}/ecnt_scu.c							\
				${ECNT_PLAT_SOC}/ecnt_system.c						\
				${ECNT_PLAT_SOC}/ecnt_cpufreq.c						\
				${ECNT_PLAT}/common/ecnt_image_load.c				\
				${ECNT_PLAT}/common/ecnt_bl2_mem_params_desc.c		\
				${ECNT_PLAT}/common/drivers/xmodem/xmodem.c			\
				${LZMA_SOURCES}
ifeq ($(TCSUPPORT_CPU_EN7581),1)
	BL2_SOURCES			+= ${ECNT_PLAT}/common/drivers/ddr_cal/an7581/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc.c			\
				${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c				\
				${ECNT_PLAT}/common/drivers/efuse_load/an7581/efuse_load.c	\
				${ECNT_PLAT}/common/drivers/serdes/an7581/serdes_config.c
else
	BL2_SOURCES			+= ${ECNT_PLAT}/common/drivers/ddr_cal/en7523/hal_io.c			\
				${ECNT_PLAT}/common/drivers/ddr_cal/en7523/dramc_pi_basic_api.c			\
				${ECNT_PLAT}/common/drivers/ddr_cal/en7523/dramc_pi_calibration_api.c			\
				${ECNT_PLAT}/common/drivers/ddr_cal/en7523/dramc_pi_main.c			\
				${ECNT_PLAT}/common/drivers/ddr_cal/en7523/dramc.c				\
				${ECNT_PLAT}/common/drivers/efuse/efuse.c						\
				${ECNT_PLAT_SOC}/ecnt_avs.c							\
				${ECNT_PLAT_SOC}/ecnt_scu_phy.c						\
				${ECNT_PLAT}/common/drivers/efuse_load/en7523/efuse_load.c
endif
else
			
ifeq ($(IMAGE_BL21),1)
	BL2_SOURCES			+= 	lib/cpus/${ARCH}/cortex_a53.S			\
				lib/cpus/${ARCH}/aem_generic.S						\
				${ECNT_PLAT_SOC}/${ARCH}/plat_helpers.S				\
				${XLAT_TABLES_LIB_SRCS}								\
				drivers/delay_timer/delay_timer.c					\
				drivers/delay_timer/generic_delay_timer.c			\
				${ECNT_PLAT}/common/drivers/uart/ecnt_uart.S		\
				${ECNT_PLAT_SOC}/platform_common.c					\
				common/image_decompress.c							\
				common/desc_image_load.c							\
				${ECNT_PLAT_SOC}/ecnt_bl2_setup.c					\
				${ECNT_PLAT}/common/ecnt_image_load.c				\
				${ECNT_PLAT}/common/ecnt_bl2_mem_params_desc.c		\
				${LZMA_SOURCES}						\
				${ECNT_PLAT_SOC}/mtk_an7563_pinmux.c			\
				${ECNT_PLAT_SOC}/mtk_an7563_gpio.c			\
				${ECNT_PLAT_SOC}/mtk_an7563_gpioconfig_parser.c

endif
ifeq ($(IMAGE_BL22),1)
	BL2_SOURCES			+= 	lib/cpus/${ARCH}/cortex_a53.S			\
				lib/cpus/${ARCH}/aem_generic.S						\
				${ECNT_PLAT_SOC}/${ARCH}/plat_helpers.S				\
				${XLAT_TABLES_LIB_SRCS}								\
				drivers/delay_timer/delay_timer.c					\
				drivers/delay_timer/generic_delay_timer.c			\
				${ECNT_PLAT}/common/drivers/uart/ecnt_uart.S		\
				${ECNT_PLAT_SOC}/platform_common.c					\
				${ECNT_PLAT_SOC}/ecnt_scu.c							\
				${ECNT_PLAT_SOC}/ecnt_trusted_boot.c				\
				${ECNT_PLAT_SOC}/ecnt_bl2_setup.c					\
				${ECNT_PLAT}/common/ecnt_bl2_mem_params_desc.c				\
				${ECNT_PLAT_SOC}/mtk_an7563_pinmux.c					\
				${ECNT_PLAT_SOC}/mtk_an7563_gpio.c					\
				${ECNT_PLAT_SOC}/mtk_an7563_gpioconfig_parser.c

ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL2_UNOPEN_SOURCES	+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c			\
				${ECNT_PLAT_SOC}/ecnt_system.c
else
	BL2_SOURCES			+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c			\
				${ECNT_PLAT_SOC}/ecnt_system.c
endif
else
	BL2_SOURCES			+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c			\
				${ECNT_PLAT_SOC}/ecnt_system.c
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL2_UNOPEN_SOURCES	+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c			\
				${ECNT_PLAT_SOC}/ecnt_system.c
endif
endif

ifeq ($(TCSUPPORT_CPU_EN7581),1)
	BL2_SOURCES			+= ${ECNT_PLAT}/common/drivers/ddr_cal/an7581/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc.c
ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL2_UNOPEN_SOURCES	+= ${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c				\
				${ECNT_PLAT}/common/drivers/efuse_load/an7581/efuse_load.c
else
	BL2_SOURCES			+= ${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c				\
				${ECNT_PLAT}/common/drivers/efuse_load/an7581/efuse_load.c				\
				${ECNT_PLAT}/common/drivers/serdes/an7581/serdes_config.c
	BL2_UNOPEN_SOURCES	+= ${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc.c
endif
else
	BL2_SOURCES			+= ${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c				\
				${ECNT_PLAT}/common/drivers/efuse_load/an7581/efuse_load.c				\
				${ECNT_PLAT}/common/drivers/serdes/an7581/serdes_config.c
	BL2_UNOPEN_SOURCES	+= ${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc.c
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL2_UNOPEN_SOURCES	+= ${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c				\
				${ECNT_PLAT}/common/drivers/efuse_load/an7581/efuse_load.c
endif
endif
else
ifeq ($(TCSUPPORT_CPU_AN7552),1)
ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL2_UNOPEN_SOURCES			+= ${ECNT_PLAT}/common/drivers/ddr_cal/an7552/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc.c	\
				${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c				\
				${ECNT_PLAT}/common/drivers/efuse_load/an7552/efuse_load.c
else
	BL2_SOURCES			+= ${ECNT_PLAT}/common/drivers/ddr_cal/an7552/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc.c	\
				${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c				\
				${ECNT_PLAT}/common/drivers/efuse_load/an7552/efuse_load.c
endif
else
	BL2_SOURCES			+= ${ECNT_PLAT}/common/drivers/ddr_cal/an7552/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc.c	\
				${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c				\
				${ECNT_PLAT}/common/drivers/efuse_load/an7552/efuse_load.c
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL2_UNOPEN_SOURCES			+= ${ECNT_PLAT}/common/drivers/ddr_cal/an7552/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc.c	\
				${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c				\
				${ECNT_PLAT}/common/drivers/efuse_load/an7552/efuse_load.c
endif
endif
else
	BL2_SOURCES			+= ${ECNT_PLAT}/common/drivers/ddr_cal/en7523/hal_io.c			\
				${ECNT_PLAT}/common/drivers/ddr_cal/en7523/dramc_pi_basic_api.c			\
				${ECNT_PLAT}/common/drivers/ddr_cal/en7523/dramc_pi_calibration_api.c			\
				${ECNT_PLAT}/common/drivers/ddr_cal/en7523/dramc_pi_main.c			\
				${ECNT_PLAT}/common/drivers/ddr_cal/en7523/dramc.c				\
				${ECNT_PLAT}/common/drivers/efuse/efuse.c						\
				${ECNT_PLAT}/common/drivers/efuse_load/en7523/efuse_load.c
endif
endif
endif
ifeq ($(IMAGE_BL23),1)
	BL2_SOURCES			+=	${AUTH_SOURCES}							\
				${XLAT_TABLES_LIB_SRCS}								\
				drivers/delay_timer/delay_timer.c					\
				drivers/delay_timer/generic_delay_timer.c			\
				lib/cpus/${ARCH}/aem_generic.S						\
				lib/cpus/${ARCH}/cortex_a53.S						\
				${ECNT_PLAT_SOC}/platform_common.c					\
				${ECNT_PLAT_SOC}/${ARCH}/plat_helpers.S				\
				${ECNT_PLAT}/common/drivers/uart/ecnt_uart.S		\
				drivers/io/io_storage.c								\
				drivers/io/io_fip.c									\
				drivers/io/io_memmap.c								\
				drivers/io/io_encrypted.c							\
				common/image_decompress.c							\
				common/desc_image_load.c							\
				${ECNT_PLAT_SOC}/ecnt_trusted_boot.c				\
				${ECNT_PLAT_SOC}/ecnt_io_storage.c					\
				${ECNT_PLAT_SOC}/ecnt_bl2_setup.c					\
				${ECNT_PLAT_SOC}/ecnt_scu.c							\
				${ECNT_PLAT}/common/ecnt_image_load.c				\
				${ECNT_PLAT}/common/ecnt_bl2_mem_params_desc.c		\
				${ECNT_PLAT}/common/drivers/xmodem/xmodem.c			\
				${LZMA_SOURCES}							\
				${ECNT_PLAT_SOC}/mtk_an7563_pinmux.c				\
				${ECNT_PLAT_SOC}/mtk_an7563_gpio.c				\
				${ECNT_PLAT_SOC}/mtk_an7563_gpioconfig_parser.c

ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL2_UNOPEN_SOURCES	+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c			\
				${ECNT_PLAT_SOC}/ecnt_system.c
else
	BL2_SOURCES			+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c			\
				${ECNT_PLAT_SOC}/ecnt_system.c
endif
else
	BL2_SOURCES			+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c			\
				${ECNT_PLAT_SOC}/ecnt_system.c
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL2_UNOPEN_SOURCES	+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c			\
				${ECNT_PLAT_SOC}/ecnt_system.c
endif
endif

ifeq ($(TCSUPPORT_CPU_EN7581),1)
ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifneq ($(TCSUPPORT_ATF_RELEASE),)
				BL2_UNOPEN_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
							${ECNT_PLAT}/common/drivers/efuse_load/an7581/efuse_load.c

else
				BL2_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
							${ECNT_PLAT}/common/drivers/efuse_load/an7581/efuse_load.c	\
							$(ECNT_PLAT_SOC)/ecnt_npu_img.S
endif
else
				BL2_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
							${ECNT_PLAT}/common/drivers/efuse_load/an7581/efuse_load.c	\
							$(ECNT_PLAT_SOC)/ecnt_npu_img.S
ifneq ($(TCSUPPORT_ATF_RELEASE),)
				BL2_UNOPEN_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
							${ECNT_PLAT}/common/drivers/efuse_load/an7581/efuse_load.c
endif
endif
else
ifeq ($(TCSUPPORT_CPU_AN7552),1)
ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifneq ($(TCSUPPORT_ATF_RELEASE),)
				BL2_UNOPEN_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
							${ECNT_PLAT}/common/drivers/efuse_load/an7552/efuse_load.c

else
				BL2_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
							${ECNT_PLAT}/common/drivers/efuse_load/an7552/efuse_load.c	\
							$(ECNT_PLAT_SOC)/ecnt_npu_img.S
endif
else
				BL2_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
							${ECNT_PLAT}/common/drivers/efuse_load/an7552/efuse_load.c	\
							$(ECNT_PLAT_SOC)/ecnt_npu_img.S
ifneq ($(TCSUPPORT_ATF_RELEASE),)
				BL2_UNOPEN_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
							${ECNT_PLAT}/common/drivers/efuse_load/an7552/efuse_load.c
endif
endif
else
				BL2_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse.c	\
							${ECNT_PLAT_SOC}/ecnt_avs.c							\
							${ECNT_PLAT_SOC}/ecnt_scu_phy.c						\
							${ECNT_PLAT}/common/drivers/efuse_load/en7523/efuse_load.c
endif
endif				
endif
endif

ifeq ($(CPU_BUS_BL2_TEST),1)
	BL2_SOURCES			+= ${ECNT_PLAT_SOC}/cpu_bus_bl2_test.c
endif
ifeq ($(TCSUPPORT_CPU_AN7552),1)
ifdef TCSUPPORT_AUTOBENCH
	BL2_SOURCES			+= ${ECNT_PLAT_SOC}/mbist_test.c
endif
endif
ifeq ($(IMAGE_BL21)$(IMAGE_BL22),)
ifeq ($(NEED_INC),1)
	BL2_SOURCES			+=	${ECNT_PLAT}/common/drivers/ephy/ephy.c	\
				${ECNT_PLAT}/common/drivers/sgmii/sgmii.c			\
				${ECNT_PLAT}/common/drivers/sgmii/sgmii_api.c		\
				${ECNT_PLAT}/common/drivers/ether/ecnt_eth.c		\
				${ECNT_PLAT}/common/drivers/iNIC/iNIC.c

	BL2_SOURCES			+=	${FLASH_SRCS}
	BL2_REBUILD_SOURCES +=  ${FLASH_REBUILD_SRCS}
	BL2_UNOPEN_SOURCES += lib/cpus/${ARCH}/cortex_a53.S
$(eval $(call add_define,INC_MODE))
else

	BL2_SOURCES			+=	${FLASH_SRCS}
	BL2_REBUILD_SOURCES +=  ${FLASH_REBUILD_SRCS}
	BL2_UNOPEN_SOURCES += lib/cpus/${ARCH}/cortex_a53.S
endif
endif

ifeq ($(TCSUPPORT_BL2_OPTIMIZATION),)
ifeq ($(TCSUPPORT_CPU_EN7581),1)
	BL2_UNOPEN_SOURCES += ${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc.c
else
	BL2_UNOPEN_SOURCES += ${ECNT_PLAT}/common/drivers/ddr_cal/en7523/dramc.c
endif
endif

	BL31_SOURCES		+=					${GICV3_SOURCES}		\
				${XLAT_TABLES_LIB_SRCS}								\
				plat/arm/common/arm_gicv3.c							\
				plat/common/plat_gicv3.c							\
				plat/common/plat_psci_common.c						\
				drivers/delay_timer/delay_timer.c					\
				drivers/delay_timer/generic_delay_timer.c			\
				lib/cpus/${ARCH}/aem_generic.S						\
				lib/cpus/${ARCH}/cortex_a53.S						\
				${ECNT_PLAT}/common/ecnt_sip_svc.c					\
				${ECNT_PLAT}/common/drivers/uart/ecnt_uart.S		\
				${ECNT_PLAT_SOC}/ecnt_scu.c						\
				${ECNT_PLAT}/common/ecnt_plat_common.c				\
				${ECNT_PLAT_SOC}/${ARCH}/plat_helpers.S				\
				${ECNT_PLAT_SOC}/platform_common.c					\
				${ECNT_PLAT_SOC}/bl31_plat_setup.c					\
				${ECNT_PLAT_SOC}/plat_topology.c					\
				${ECNT_PLAT_SOC}/plat_pm.c							\
				${ECNT_PLAT_SOC}/ecnt_trusted_boot.c

ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL31_UNOPEN_SOURCES	+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c
else
	BL31_SOURCES			+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c
endif
else
	BL31_SOURCES			+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL31_UNOPEN_SOURCES	+= ${ECNT_PLAT_SOC}/ecnt_cpufreq.c
endif
endif

ifeq ($(TCSUPPORT_CPU_EN7581),1)
	BL31_SOURCES		+=	${ECNT_PLAT}/common/drivers/ddr_cal/an7581/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7581/dramc.c
ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL31_UNOPEN_SOURCES	+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c
else
	BL31_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c
endif
else
	BL31_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL31_UNOPEN_SOURCES	+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c
endif
endif
else
ifeq ($(TCSUPPORT_CPU_AN7552),1)
ifneq ($(TCSUPPORT_BB_FIX_UNOPEN),0)
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL31_UNOPEN_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc.c
else
	BL31_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc.c
endif
else
	BL31_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc.c
ifneq ($(TCSUPPORT_ATF_RELEASE),)
	BL31_UNOPEN_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse_28nm.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/Hal_io.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR3_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DDR4_dram_init.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR3.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/IPM_actiming_setting_DDR4.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/RX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_path_auto_gen.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/TX_RX_auto_gen_library.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_actiming.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/MD32_initial.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dv_dut.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_utility.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_NONSHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DIG_SHUF_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/DRAMC_SUBSYS_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/HW_FUNC_MANAGE.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/ANA_init_config.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_basic_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_calibration_api.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_dvfs.c		\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc_pi_main.c	\
				${ECNT_PLAT}/common/drivers/ddr_cal/an7552/dramc.c
endif
endif
else
				BL31_SOURCES		+=	${ECNT_PLAT}/common/drivers/efuse/efuse.c		\
							${ECNT_PLAT_SOC}/ecnt_avs.c							\
							${ECNT_PLAT_SOC}/ecnt_scu_phy.c
endif
endif
	BL31_SOURCES		+=					${AUTH_SOURCES}			\
				${MBEDTLS_SOURCES}									\
				common/desc_image_load.c							\
				drivers/io/io_storage.c								\
				drivers/io/io_fip.c									\
				drivers/io/io_memmap.c								\
				drivers/io/io_encrypted.c							\
				${ECNT_PLAT_SOC}/ecnt_io_storage.c					\
				${ECNT_PLAT}/common/ecnt_bl31_mem_params_desc.c


ifeq (${TRUSTED_FW},1)
KEY_SIZE				:=	4096
ifneq ($(TCSUPPORT_CPU_AN7552),)
HASH_ALG				:=	sha256
AES						:=	aes128
else
HASH_ALG				:=	sha512
AES						:=	aes256
endif

ENC_BL2					:= 0
ENC_BL31				:= 0
ENC_BL33				:= 0
ENC_TCLINUX				:= 0

ifneq (${KEY_SIZE}, )
ROTPK_HASH				:=	${BUILD_PLAT}/rotpk_${KEY_SIZE}_${HASH_ALG}.bin
ROT_KEY					:=	${ECNT_PLAT}/key/rot_key_${KEY_SIZE}.pem
TESTFILE_NAME			:=	${BUILD_PLAT}/test_${KEY_SIZE}_${HASH_ALG}.bin
EFUSE_NAME				:=	${ECNT_PLAT}/key/ecntefuse_${KEY_SIZE}_${HASH_ALG}.bin

FIP_NAME				:=	preloader_${KEY_SIZE}_${HASH_ALG}.bin

EFUSE_ARGS				:=	-s ${HASH_ALG} -r ${ROTPK_HASH}

ifneq (${AES}, none)
TESTFILE_NAME			:=	${BUILD_PLAT}/test_${KEY_SIZE}_${HASH_ALG}_${AES}.bin
EFUSE_NAME				:=	${ECNT_PLAT}/key/ecntefuse_${KEY_SIZE}_${HASH_ALG}_${AES}.bin

FIP_NAME				:=	preloader_${KEY_SIZE}_${HASH_ALG}_${AES}.bin

AES_FILE				:=	${ECNT_PLAT}/key/aes_${AES}
AES_KEY					:=	${BUILD_PLAT}/aes_key_${AES}
AES_IV					:=	${BUILD_PLAT}/aes_iv_${AES}

AES_FW					:=	${AES}
ENC_KEY					:=	`cat ${AES_KEY}`
ENC_NONCE				:=	`cat ${AES_IV}`

EFUSE_ARGS				+=	-a ${AES} -k ${ENC_KEY}

ifeq (${ENC_BL2},1)
ENCRYPT_BL2				:=	1
endif

ifeq (${ENC_BL31},1)
ENCRYPT_BL31			:=	1
endif

ifeq (${ENC_BL33},1)
ENCRYPT_BL33			:=	1
endif
endif
endif

EFUSE_ARGS				+= 	-o ${EFUSE_NAME}

key: $(ROT_KEY) $(AES_FILE)

ecnt_efuse: efusetool certificate
	$(ECHO) "$(EFUSETOOL) $(EFUSE_ARGS)"
	$(Q)$(EFUSETOOL) $(EFUSE_ARGS)

certificate: $(ROTPK_HASH) $(AES_KEY) $(AES_IV)

$(AES_FILE):
ifneq (${AES_FW}, none)
	@echo "  OPENSSL $@"
ifeq (${AES}, aes128)
	$(Q)$(ATF_DIR)/openssl enc -aes-128-gcm -P -nosalt -k `$(ATF_DIR)/openssl rand 32 -base64`  > $@ 2>/dev/null
else ifeq (${AES}, aes256)
	$(Q)$(ATF_DIR)/openssl enc -aes-256-gcm -P -nosalt -k `$(ATF_DIR)/openssl rand 32 -base64`  > $@ 2>/dev/null
endif
else
	@echo "  AES_FW=${AES_FW}"
endif

$(AES_KEY):
ifneq (${AES_FW}, none)
	$(Q)awk -F= 'NR==1{print $$2}' $(AES_FILE) > $(AES_KEY)
else
	@echo "  AES_FW=${AES_FW}"
endif

$(AES_IV):
ifneq (${AES_FW}, none)
	$(Q)awk -F= 'NR==2{print $$2}' $(AES_FILE) > $(AES_IV)
else
	@echo "  AES_FW=${AES_FW}"
endif

$(ROT_KEY):
	@echo "  OPENSSL $@"
	$(Q)$(ATF_DIR)/openssl genrsa ${KEY_SIZE} > $@ 2>/dev/null

$(ROTPK_HASH):
	@echo "  OPENSSL $@"
	$(Q)$(ATF_DIR)/openssl rsa -in $(ROT_KEY) -pubout -outform DER 2>/dev/null |       \
	$(ATF_DIR)/openssl dgst -${HASH_ALG} -binary > $@ 2>/dev/null

secure_script:
	@echo 'cp $$1 tclinux.fip' > $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh

ifneq (${ENC_TCLINUX}, 1)
	@echo '$(TOOLS_UBOOT_DIR)/cert_create -n --tfw-nvctr 0 --ntfw-nvctr 0 --trusted-key-cert trusted_key.crt  --key-alg rsa --key-size ${KEY_SIZE} --hash-alg ${HASH_ALG} --rot-key $(ATF_DIR)/arm-trusted-firmware-2.3/${ROT_KEY} --nt-fw-cert nt_fw_content.crt  --nt-fw-key-cert nt_fw_key.crt  --nt-fw tclinux.fip' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
	@echo '$(TOOLS_UBOOT_DIR)/fiptool create --trusted-key-cert trusted_key.crt --nt-fw-cert nt_fw_content.crt --nt-fw-key-cert nt_fw_key.crt --align 1024 --nt-fw tclinux.fip $$1' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
else
	@echo $(ENC_ARGS) > $(BSP_EXT_TCLINUX_BUILDER)/enc_args
	@echo '$(TOOLS_UBOOT_DIR)/cert_create -n --tfw-nvctr 0 --ntfw-nvctr 0 --trusted-key-cert trusted_key.crt  --key-alg rsa --key-size ${KEY_SIZE} --hash-alg ${HASH_ALG} --rot-key $(ATF_DIR)/arm-trusted-firmware-2.3/${ROT_KEY} --nt-fw-cert nt_fw_content.crt  --nt-fw-key-cert nt_fw_key.crt  --nt-fw $$1' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
	@echo '$(TOOLS_UBOOT_DIR)/encrypt_fw `cat $(BSP_EXT_TCLINUX_BUILDER)/enc_args` -i tclinux.fip -o tclinux_enc.bin' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
	@echo '$(TOOLS_UBOOT_DIR)/fiptool create --trusted-key-cert trusted_key.crt --nt-fw-cert nt_fw_content.crt --nt-fw-key-cert nt_fw_key.crt  --align 1024 --nt-fw tclinux_enc.bin tclinux.fip' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
	@echo 'mv tclinux.fip tclinux_tmp' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
	@echo '${TOOLS_TRX_DIR}/trx -f tclinux_tmp -o tclinux_encrypted -c ${TOOLS_TRX_DIR}/trx_config' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
	@echo '$(TOOLS_UBOOT_DIR)/cert_create -n --tfw-nvctr 0 --ntfw-nvctr 0 --trusted-key-cert trusted_key.crt  --key-alg rsa --key-size ${KEY_SIZE} --hash-alg ${HASH_ALG} --rot-key $(ATF_DIR)/arm-trusted-firmware-2.3/${ROT_KEY} --nt-fw-cert nt_fw_content.crt  --nt-fw-key-cert nt_fw_key.crt  --nt-fw tclinux_encrypted' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
	@echo '$(TOOLS_UBOOT_DIR)/fiptool create --trusted-key-cert trusted_key.crt --nt-fw-cert nt_fw_content.crt --nt-fw-key-cert nt_fw_key.crt  --align 1024 --nt-fw tclinux_encrypted tclinux.fip' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
	@echo 'cp tclinux.fip $$1' >> $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh
endif
	@chmod 777 $(BSP_EXT_TCLINUX_BUILDER)/secure_firmware.sh

tclinux.fip:
ifneq (${ENC_TCLINUX}, 1)
	${Q}${CRTTOOL} -n --tfw-nvctr 0 --ntfw-nvctr 0 --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt  --key-alg rsa --key-size ${KEY_SIZE} --hash-alg ${HASH_ALG} --rot-key ${ROT_KEY} --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt  --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt  --nt-fw ${IMAGE_DIR}/tclinux.bin
	${Q}${FIPTOOL} create --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt --align 1024 --nt-fw ${IMAGE_DIR}/tclinux.bin ${IMAGE_DIR}/tclinux.fip
	${Q}mv ${IMAGE_DIR}/tclinux.fip ${IMAGE_DIR}/tclinux.bin
else
	${Q}${CRTTOOL} -n --tfw-nvctr 0 --ntfw-nvctr 0 --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt  --key-alg rsa --key-size ${KEY_SIZE} --hash-alg ${HASH_ALG} --rot-key ${ROT_KEY} --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt  --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt  --nt-fw ${IMAGE_DIR}/tclinux.bin
	$(Q)$(ENCTOOL) $(ENC_ARGS) -i ${IMAGE_DIR}/tclinux.bin -o ${IMAGE_DIR}/tclinux_enc.bin
	${Q}${FIPTOOL} create --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt --align 1024 --nt-fw ${IMAGE_DIR}/tclinux_enc.bin ${IMAGE_DIR}/tclinux.fip
	${Q}mv ${IMAGE_DIR}/tclinux.fip ${IMAGE_DIR}/tclinux_tmp
ifeq ($(TCSUPPORT_OPENWRT),)
	$(Q)${TOOLS_TRX_DIR}/trx -f ${IMAGE_DIR}/tclinux_tmp -o ${IMAGE_DIR}/tclinux_encrypted -c ${TOOLS_TRX_DIR}/trx_config
else
	$(STAGING_DIR_HOST)/bin/ecnt_trx -f ${IMAGE_DIR}/tclinux_tmp -o ${IMAGE_DIR}/tclinux_encrypted -c $(PLATFORM_DIR)/image/trx_config
endif
	${Q}${CRTTOOL} -n --tfw-nvctr 0 --ntfw-nvctr 0 --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt  --key-alg rsa --key-size ${KEY_SIZE} --hash-alg ${HASH_ALG} --rot-key ${ROT_KEY} --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt  --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt  --nt-fw ${IMAGE_DIR}/tclinux_encrypted
	${Q}${FIPTOOL} create --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt --align 1024 --nt-fw ${IMAGE_DIR}/tclinux_encrypted ${IMAGE_DIR}/tclinux.fip
	${Q}mv ${IMAGE_DIR}/tclinux.fip ${IMAGE_DIR}/tclinux.bin
	${Q}rm ${IMAGE_DIR}/tclinux_enc.bin
endif

linux.fip:
	${Q}${CRTTOOL} -n --tfw-nvctr 0 --ntfw-nvctr 0 --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt  --key-alg rsa --key-size ${KEY_SIZE} --hash-alg ${HASH_ALG} --rot-key ${ROT_KEY} --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt  --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt  --nt-fw ${IMAGE_DIR}/linux.7z
	${Q}${FIPTOOL} create --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt --align 1024 --nt-fw ${IMAGE_DIR}/linux.7z ${IMAGE_DIR}/linux.fip
	${Q}mv ${IMAGE_DIR}/linux.fip ${IMAGE_DIR}/linux.7z
rootfs.fip:
	${Q}${CRTTOOL} -n --tfw-nvctr 0 --ntfw-nvctr 0 --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt  --key-alg rsa --key-size ${KEY_SIZE} --hash-alg ${HASH_ALG} --rot-key ${ROT_KEY} --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt  --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt  --nt-fw ${IMAGE_DIR}/rootfs
	${Q}${FIPTOOL} create --trusted-key-cert ${ECNT_PLAT}/key/trusted_key.crt --nt-fw-cert ${ECNT_PLAT}/key/nt_fw_content.crt --nt-fw-key-cert ${ECNT_PLAT}/key/nt_fw_key.crt --align 1024 --nt-fw ${IMAGE_DIR}/rootfs ${IMAGE_DIR}/rootfs.fip
	${Q}mv ${IMAGE_DIR}/rootfs.fip ${IMAGE_DIR}/rootfs
else
ecnt_efuse:
	@echo "  TRUSTED_FW=${TRUSTED_FW}"
endif

NPU_IMAGE_BIN		=	$(BOOTROM_UNOPEN_IMG_DIR)/npu_rv32.bin
NPU_DATA_BIN		=	$(BOOTROM_UNOPEN_IMG_DIR)/npu_data.bin

$(eval $(call add_define_val,NPU_IMAGE_BIN,'"$(NPU_IMAGE_BIN)"'))
$(eval $(call add_define_val,NPU_DATA_BIN,'"$(NPU_DATA_BIN)"'))

# Enable workarounds for selected Cortex-A53 erratas.
ERRATA_A53_836870		:=	1
ERRATA_A53_855873		:=	1

# indicate the reset vector address can be programmed
PROGRAMMABLE_RESET_ADDRESS	:=	1
ENABLE_SVE_FOR_NS			:=	0

ECNT_SIP_KERNEL_BOOT_ENABLE := 1
$(eval $(call add_define,ECNT_SIP_KERNEL_BOOT_ENABLE))

PLAT_XLAT_TABLES_DYNAMIC := 1
$(eval $(call add_define,PLAT_XLAT_TABLES_DYNAMIC))
