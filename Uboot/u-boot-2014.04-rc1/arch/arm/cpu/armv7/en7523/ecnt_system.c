/***************************************************************
Copyright Statement:

This software/firmware and related documentation (��EcoNet Software��) 
are protected under relevant copyright laws. The information contained herein 
is confidential and proprietary to EcoNet (HK) Limited (��EcoNet��) and/or 
its licensors. Without the prior written permission of EcoNet and/or its licensors, 
any reproduction, modification, use or disclosure of EcoNet Software, and 
information contained herein, in whole or in part, shall be strictly prohibited.

EcoNet (HK) Limited  EcoNet. ALL RIGHTS RESERVED.

BY OPENING OR USING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY 
ACKNOWLEDGES AND AGREES THAT THE SOFTWARE/FIRMWARE AND ITS 
DOCUMENTATIONS (��ECONET SOFTWARE��) RECEIVED FROM ECONET 
AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON AN ��AS IS�� 
BASIS ONLY. ECONET EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, 
WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, 
OR NON-INFRINGEMENT. NOR DOES ECONET PROVIDE ANY WARRANTY 
WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTIES WHICH 
MAY BE USED BY, INCORPORATED IN, OR SUPPLIED WITH THE ECONET SOFTWARE. 
RECEIVER AGREES TO LOOK ONLY TO SUCH THIRD PARTIES FOR ANY AND ALL 
WARRANTY CLAIMS RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES 
THAT IT IS RECEIVER��S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD 
PARTY ALL PROPER LICENSES CONTAINED IN ECONET SOFTWARE.

ECONET SHALL NOT BE RESPONSIBLE FOR ANY ECONET SOFTWARE RELEASES 
MADE TO RECEIVER��S SPECIFICATION OR CONFORMING TO A PARTICULAR 
STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND 
ECONET'S ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE ECONET 
SOFTWARE RELEASED HEREUNDER SHALL BE, AT ECONET'S SOLE OPTION, TO 
REVISE OR REPLACE THE ECONET SOFTWARE AT ISSUE OR REFUND ANY SOFTWARE 
LICENSE FEES OR SERVICE CHARGES PAID BY RECEIVER TO ECONET FOR SUCH 
ECONET SOFTWARE.
***************************************************************/

/* ToDo: make releasebsp need to delete ecnt_system.c */

#include <common.h>
#include <asm/tc3162.h>

#define FPGA_SYS_HCLK 40

static void check_fpga(void)
{
	int _isFPGA;
	
	_isFPGA = (((VPint(0x1fb0009c) & 0x1) == 0) ? 1 : 0);
	SET_IS_FPGA(_isFPGA);
}

static void check_spi_ctrl_ecc(void)
{
//#ifdef TCSUPPORT_SPI_CONTROLLER_ECC
//	SET_IS_SPI_CONTROLLER_ECC(1);
//#else
	SET_IS_SPI_CONTROLLER_ECC(0);
//#endif
}

#if defined(TCSUPPORT_CPU_EN7523)
static void check_pkg_id(void)
{
	/* Only valid package id can be pass */
}

static void en7523_init(void)
{
	if (isFPGA) {
		SET_SYS_CLK(FPGA_SYS_HCLK);
	} else {
		//en7580_setup_xtal_driving();
		//en7580_setup_clk(EN7580_CPU_CLOCK_1300MHZ);
		//en7580_packageID_init();

		/* only allow existing package ID */
		check_pkg_id();
	}
}
#endif

int ecnt_system_init(void)
{
	check_fpga();
	check_spi_ctrl_ecc();

#if defined(TCSUPPORT_CPU_EN7523)
	en7523_init();
#endif

	return 0;
}
