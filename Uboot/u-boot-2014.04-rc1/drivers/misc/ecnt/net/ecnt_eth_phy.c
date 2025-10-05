/***************************************************************
Copyright Statement:

This software/firmware and related documentation (EcoNet Software) 
are protected under relevant copyright laws. The information contained herein 
is confidential and proprietary to EcoNet (HK) Limited (EcoNet) and/or 
its licensors. Without the prior written permission of EcoNet and/or its licensors, 
any reproduction, modification, use or disclosure of EcoNet Software, and 
information contained herein, in whole or in part, shall be strictly prohibited.

EcoNet (HK) Limited  EcoNet. ALL RIGHTS RESERVED.

BY OPENING OR USING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY 
ACKNOWLEDGES AND AGREES THAT THE SOFTWARE/FIRMWARE AND ITS 
DOCUMENTATIONS (ECONET SOFTWARE) RECEIVED FROM ECONET 
AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON AN AS IS 
BASIS ONLY. ECONET EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, 
WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, 
OR NON-INFRINGEMENT. NOR DOES ECONET PROVIDE ANY WARRANTY 
WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTIES WHICH 
MAY BE USED BY, INCORPORATED IN, OR SUPPLIED WITH THE ECONET SOFTWARE. 
RECEIVER AGREES TO LOOK ONLY TO SUCH THIRD PARTIES FOR ANY AND ALL 
WARRANTY CLAIMS RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES 
THAT IT IS RECEIVERS SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD 
PARTY ALL PROPER LICENSES CONTAINED IN ECONET SOFTWARE.

ECONET SHALL NOT BE RESPONSIBLE FOR ANY ECONET SOFTWARE RELEASES 
MADE TO RECEIVERS SPECIFICATION OR CONFORMING TO A PARTICULAR 
STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND 
ECONET'S ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE ECONET 
SOFTWARE RELEASED HEREUNDER SHALL BE, AT ECONET'S SOLE OPTION, TO 
REVISE OR REPLACE THE ECONET SOFTWARE AT ISSUE OR REFUND ANY SOFTWARE 
LICENSE FEES OR SERVICE CHARGES PAID BY RECEIVER TO ECONET FOR SUCH 
ECONET SOFTWARE.
***************************************************************/
#include <common.h>
#include <command.h>
/////////
#include <asm/errno.h>
#include <linux/types.h>
#include <asm/bitops.h>
#include <asm/system.h>
#include <asm/tc3162.h>
#include <asm/io.h>
#include "ecnt_eth.h"
#include "ecnt_eth_phy.h"

#include "H/air_eth_xsgmii.h"
#include "H/air_eth_xsgmii_config.h"

#include "H/en7581_xfi_pma_hal_reg.h"
#include "H/en7581_xfi_pma_reg.h"
#include "sgmii_globaldef.h"
#include "H/xfi_ana_pxp_hal_reg.h"
#include "H/xfi_ana_pxp_reg.h"


#define pcs1_base  0x1fa75900
#define USXGMII_PCS1_BASE_OFFSET    			(RgAddr) 0x0900
#define pma_base   0x1fa7b000
#define PMA_BASE_OFFSET   						(RgAddr) 0xB000

u8 dbg_print                    =  0;
#if 0
int XSGMII_Linkdn_Ignone_SD_EN   =  0;
int linkup_sta                   =  0;
int linkdn_sta                   =  0;
#endif
extern uint16_t miiStationRead45(uint32_t phy_addr, uint32_t dev_addr, uint32_t phy_reg);
extern int miiStationWrite45(uint32_t phy_addr, uint32_t dev_addr, uint32_t phy_reg, uint32_t phy_data);

extern u32 xsgmii_api(u8 serdes,u8 xsgmii,u8 mod,u8 rate,u8 an);

static void RG_W_PL(u32 Base,u32 Base_start,u32 Addr,u32 Data){

 write_reg_word( Base + (Addr - Base_start),Data);

}
	
static u32 RG_R_PL(u32 Base,u32 Base_start,u32 Addr){

  return read_reg_word(Base + (Addr - Base_start));	
}  

int XSI_MAC_LOGIC_RESET(void){
	u32 mac_lock;
	//unlock
	mac_lock = read_reg_word(0x1fa09000);
	write_reg_word(0x1fa09000, (mac_lock | 0xf));
	//delay 1ms
	udelay(1000);
	//reset
	write_reg_word(0x1fa09010,0);
	write_reg_word(0x1fa09010,1);
	//lock
	write_reg_word(0x1fa09000, (mac_lock & 0xfffffff0));		
}

static u8 RX_CDR_LFP_L2D_sta(void){
	rg_type_t(HAL_rg_force_da_pxp_cdr_lpf_lck2data) rg_force_da_pxp_cdr_lpf_lck2data;
	u8 sta; 
	rg_force_da_pxp_cdr_lpf_lck2data.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _rg_force_da_pxp_cdr_lpf_lck2data);
	sta = rg_force_da_pxp_cdr_lpf_lck2data.hal.rg_force_sel_da_pxp_cdr_lpf_lck2data ;	
	if(dbg_print) printf("RX_CDR_LFP_L2D_sta %x\n",sta);
	return sta;
}

static void RX_CDR_LFP_L2D(u8 mod,u8 sel){
	rg_type_t(HAL_rg_force_da_pxp_cdr_lpf_lck2data) rg_force_da_pxp_cdr_lpf_lck2data;	
	if(dbg_print) printf("RX_CDR_LFP_L2D mode %x, sel %x\n",mod,sel);
	rg_force_da_pxp_cdr_lpf_lck2data.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _rg_force_da_pxp_cdr_lpf_lck2data);

	//rg_force_da_pxp_cdr_lpf_lck2data.hal.rg_force_sel_da_pxp_cdr_lpf_lck2data = 1;	
	rg_force_da_pxp_cdr_lpf_lck2data.hal.rg_force_sel_da_pxp_cdr_lpf_lck2data = mod;
	rg_force_da_pxp_cdr_lpf_lck2data.hal.rg_force_da_pxp_cdr_lpf_lck2data = sel;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _rg_force_da_pxp_cdr_lpf_lck2data,rg_force_da_pxp_cdr_lpf_lck2data.dat.value);
}


static void RX_CDR_LPF_RSTB(u8 mod,u8 sel){
	rg_type_t(HAL_rg_force_da_pxp_cdr_lpf_lck2data) rg_force_da_pxp_cdr_lpf_lck2data;	
	if(dbg_print) printf("RX_CDR_LPF_RSTB mode %x, sel%x\n",mod,sel);
	rg_force_da_pxp_cdr_lpf_lck2data.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _rg_force_da_pxp_cdr_lpf_lck2data);

	//rg_force_da_pxp_cdr_lpf_lck2data.hal.rg_force_sel_da_pxp_cdr_lpf_rstb = 1;
	rg_force_da_pxp_cdr_lpf_lck2data.hal.rg_force_sel_da_pxp_cdr_lpf_rstb = mod;
	rg_force_da_pxp_cdr_lpf_lck2data.hal.rg_force_da_pxp_cdr_lpf_rstb = sel;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _rg_force_da_pxp_cdr_lpf_lck2data,rg_force_da_pxp_cdr_lpf_lck2data.dat.value);
}

static void RX_CDR_RST(void){
	if(dbg_print) printf("RX_CDR_RST\n");
	RX_CDR_LFP_L2D(1,0);
	RX_CDR_LPF_RSTB(1,0);
	udelay(700);
	RX_CDR_LPF_RSTB(1,1);
	udelay(100);
	RX_CDR_LFP_L2D(1,1);

	//switch to auto
	RX_CDR_LPF_RSTB(0,1);
	RX_CDR_LFP_L2D(0,1);
}

static u8 RX_RDY_Sta(void){
	rg_type_t(HAL_RX_CTRL_SEQUENCE_DISB_CTRL_1) RX_CTRL_SEQUENCE_DISB_CTRL_1;	
	RX_CTRL_SEQUENCE_DISB_CTRL_1.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_CTRL_SEQUENCE_DISB_CTRL_1);
	if(dbg_print) printf("RX_RDY_Sta %x \n",RX_CTRL_SEQUENCE_DISB_CTRL_1.hal.rg_disb_rx_rdy);
	return RX_CTRL_SEQUENCE_DISB_CTRL_1.hal.rg_disb_rx_rdy;
}

static void RX_RDY(u8 mod,u8 sel){
	rg_type_t(HAL_RX_CTRL_SEQUENCE_DISB_CTRL_1) RX_CTRL_SEQUENCE_DISB_CTRL_1;	
	rg_type_t(HAL_RX_CTRL_SEQUENCE_FORCE_CTRL_1) RX_CTRL_SEQUENCE_FORCE_CTRL_1;
	if(dbg_print) printf("RX_RDY %x, sel %x\n",mod,sel);
	RX_CTRL_SEQUENCE_DISB_CTRL_1.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_CTRL_SEQUENCE_DISB_CTRL_1);
	RX_CTRL_SEQUENCE_DISB_CTRL_1.hal.rg_disb_rx_rdy = mod;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_CTRL_SEQUENCE_DISB_CTRL_1,RX_CTRL_SEQUENCE_DISB_CTRL_1.dat.value);

	RX_CTRL_SEQUENCE_FORCE_CTRL_1.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_CTRL_SEQUENCE_FORCE_CTRL_1);
	RX_CTRL_SEQUENCE_FORCE_CTRL_1.hal.rg_force_rx_rdy = sel;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_CTRL_SEQUENCE_FORCE_CTRL_1,RX_CTRL_SEQUENCE_FORCE_CTRL_1.dat.value);
}



static void xsgmii_force_data(u8 xsgmii,u8 mod,u8 an, u8 rate){
	rg_type_t(HAL_rg_user_define_sel) rg_user_define_sel;	
	rg_type_t(HAL_rg_user_define_xgmii_control) rg_user_define_xgmii_control;
	rg_type_t(HAL_rg_user_define_xgmii_data_lsb) rg_user_define_xgmii_data_lsb;
	if(dbg_print)printf("USXGMII_force_data xsgmii %x, force %x\n",xsgmii,an);
	switch(xsgmii){
		case USXGMII:
			switch(rate){
				case 0:
					rg_user_define_xgmii_data_lsb.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr)_rg_user_define_xgmii_data_lsb);			
					if(rg_user_define_xgmii_data_lsb.hal.rg_user_define_rxd_lsb != USR_DATA){
						rg_user_define_xgmii_data_lsb.hal.rg_user_define_rxd_lsb = USR_DATA;
						RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr)_rg_user_define_xgmii_data_lsb,rg_user_define_xgmii_data_lsb.dat.value);
					}
					
						rg_user_define_xgmii_control.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr)_rg_user_define_xgmii_control);
					if(rg_user_define_xgmii_control.hal.rg_user_define_rxc != 0xff){
						rg_user_define_xgmii_control.hal.rg_user_define_rxc = 0xff;				
						RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr)_rg_user_define_xgmii_control,rg_user_define_xgmii_control.dat.value);				
					}
					
					rg_user_define_sel.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr)_rg_user_define_sel);
					rg_user_define_sel.hal.rg_user_define_sel = an;
					RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr)_rg_user_define_sel,rg_user_define_sel.dat.value);
					break;
				case 1:					
					rg_user_define_sel.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr)_rg_user_define_sel);
					rg_user_define_sel.hal.rg_user_define_sel = an;
					RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr)_rg_user_define_sel,rg_user_define_sel.dat.value);
					break;
			}
			break;
		case HSGMII:	
		case SGMII:
			break;
	}
}

static u8 RX_SigDet_Flag(void){
	RGDATA_t rg;	
	u8 i,cnt = 0;		
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _ADD_DIG_RESERVE_0,0x30000);	
	for (i=0;i<=5;i++){		
		rg.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _ADD_DIG_RO_RESERVE_2);
		cnt += rg.bit.b8;
	}
	if(dbg_print)printf("RX_SigDet_Flag, cnt %x\n",cnt);
	
	return cnt >= 4? 1:0;  
}

static u8 RX_SigDet_Flag_D(void){
	rg_type_t(HAL_XPON_INT_EN_3) XPON_INT_EN_3;		
	rg_type_t(HAL_SS_RX_SIGDET_1) SS_RX_SIGDET_1;		
	rg_type_t(HAL_XPON_INT_STA_3) XPON_INT_STA_3;
	rg_type_t(HAL_RX_RESET_1) RX_RESET_1;	

	XPON_INT_EN_3.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_EN_3);	
	XPON_INT_EN_3.hal.rg_rx_sigdet_int_en = 0;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_EN_3,XPON_INT_EN_3.dat.value);	

	SS_RX_SIGDET_1.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _SS_RX_SIGDET_1);	
	SS_RX_SIGDET_1.hal.rg_sigdet_en = 1;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _SS_RX_SIGDET_1,SS_RX_SIGDET_1.dat.value);

	RX_RESET_1.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_RESET_1);	
	RX_RESET_1.hal.rg_sigdet_rst_b = 0;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_RESET_1,RX_RESET_1.dat.value);
	
	RX_RESET_1.hal.rg_sigdet_rst_b = 1;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_RESET_1,RX_RESET_1.dat.value);

	XPON_INT_STA_3.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_STA_3);	
	XPON_INT_STA_3.hal.rx_sigdet_int = 1;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_STA_3,XPON_INT_STA_3.dat.value);	
	udelay(50);	
	XPON_INT_STA_3.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_STA_3);	

	SS_RX_SIGDET_1.hal.rg_sigdet_en = 0;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _SS_RX_SIGDET_1,SS_RX_SIGDET_1.dat.value);
	
	if(dbg_print)printf("RX_SigDet_Flag_D %x\n",XPON_INT_STA_3.hal.rx_sigdet_int);
	return XPON_INT_STA_3.hal.rx_sigdet_int;  
}

static void SigDet_Int_Init(u8 en){
	rg_type_t(HAL_XPON_INT_EN_3) XPON_INT_EN_3;		
	rg_type_t(HAL_SS_RX_SIGDET_1) SS_RX_SIGDET_1;		
	rg_type_t(HAL_XPON_INT_STA_3) XPON_INT_STA_3;	
	rg_type_t(HAL_RX_RESET_1) RX_RESET_1;	

	if(dbg_print)printf("SigDet_Int_Init, en %x\n",en);

	XPON_INT_STA_3.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_STA_3);	
	XPON_INT_STA_3.hal.rx_sigdet_int = 1;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_STA_3,XPON_INT_STA_3.dat.value);	

	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_STA_3,0x0);	
	XPON_INT_EN_3.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_EN_3);	
	XPON_INT_EN_3.hal.rg_rx_sigdet_int_en = en;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_EN_3,XPON_INT_EN_3.dat.value);	

	SS_RX_SIGDET_1.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _SS_RX_SIGDET_1);	
	SS_RX_SIGDET_1.hal.rg_sigdet_en = en;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _SS_RX_SIGDET_1,SS_RX_SIGDET_1.dat.value);	

	RX_RESET_1.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_RESET_1);	
	RX_RESET_1.hal.rg_sigdet_rst_b = 0;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_RESET_1,RX_RESET_1.dat.value);
	
	RX_RESET_1.hal.rg_sigdet_rst_b = 1;
	RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _RX_RESET_1,RX_RESET_1.dat.value);
}
static u8 SigDet_IntEn_sta(void){
	rg_type_t(HAL_XPON_INT_EN_3) XPON_INT_EN_3;	
	XPON_INT_EN_3.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_EN_3);
	return (u8) XPON_INT_EN_3.hal.rg_rx_sigdet_int_en;
}


void usxgmii_isr(void *args)
{	
	rg_type_t(HAL_xfi_pcs_int_sta_2) rg_xfi_pcs_int_sta_2; 	 
	rg_type_t(HAL_xfi_pcs_int_sta_3) rg_xfi_pcs_int_sta_3; 	 
	rg_type_t(HAL_xfi_pcs_int_sta_4) rg_xfi_pcs_int_sta_4; 	

	rg_type_t(HAL_rg_xfi_pcs_int_ctrl_2) rg_xfi_pcs_int_ctrl_2;    
	rg_type_t(HAL_rg_xfi_pcs_int_ctrl_3) rg_xfi_pcs_int_ctrl_3;	
	rg_type_t(HAL_rg_xfi_pcs_int_ctrl_4) rg_xfi_pcs_int_ctrl_4;

	rg_type_t(HAL_XPON_INT_EN_3) XPON_INT_EN_3;	
	rg_type_t(HAL_XPON_INT_STA_3) XPON_INT_STA_3;		

	u8 signal = XSGMII_SigDet_A_EN? RX_SigDet_Flag(): RX_SigDet_Flag_D();
	u16 sync = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _ro_base_r_10gb_t_pcs_stus1);
	u8 sd_int = 0;
	
	if(dbg_print) printf("eth xsgmii: usxgmii_isr signal %x, sync %x\n",signal,sync);
//SD int
	if(SigDet_IntEn_sta()){
		XPON_INT_STA_3.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_STA_3);	
		if(dbg_print)printf("rx_sigdet_int %x\n",XPON_INT_STA_3.hal.rx_sigdet_int);

		if(XPON_INT_STA_3.hal.rx_sigdet_int)
		{			
			sd_int = 1;
			if(dbg_print)printf("---------------rx_sigdet_int isr *** ---------------\n");
			if(RX_CDR_LFP_L2D_sta()== 1) RX_CDR_RST();
			if(RX_RDY_Sta()== 0) RX_RDY(1,0);
			SigDet_Int_Init(0);
			//if(XSGMII_SigDet_Wrapper_EN) MAC_SigDet_Wrapper();
		}
		
		RG_W_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_STA_3,XPON_INT_STA_3.dat.value);	
		if(dbg_print){
			XPON_INT_STA_3.dat.value = RG_R_PL(pma_base,PMA_BASE_OFFSET,(RgAddr) _XPON_INT_STA_3);	
			printf("_XPON_INT_STA_3 %x\n",XPON_INT_STA_3.dat.value);
		}
		if(dbg_print)printf("---------------rx_sigdet_int isr &&&---------------\n");
		
		if(sd_int){
			sd_int = 0;
			return;
		}
	}	

//Linkup int	
	rg_xfi_pcs_int_sta_2.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_2);	 
	rg_xfi_pcs_int_sta_3.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_3);	 
	rg_xfi_pcs_int_sta_4.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_4);	
	if(dbg_print) printf("Interrupt sta rg_xfi_pcs_int_sta_2= %x,rg_xfi_pcs_int_sta_3= %x,rg_xfi_pcs_int_sta_4= %x\n",rg_xfi_pcs_int_sta_2.dat.value,rg_xfi_pcs_int_sta_3.dat.value,rg_xfi_pcs_int_sta_4.dat.value);

	if(dbg_print)printf("link_up_st_int %x\n",rg_xfi_pcs_int_sta_3.hal.link_up_st_int);
	if(rg_xfi_pcs_int_sta_3.hal.link_up_st_int)	{
		if(dbg_print)printf("---------------link_up_st_int isr *** ---------------\n");
		if(signal) sync = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _ro_base_r_10gb_t_pcs_stus1);
		if(dbg_print)printf("link_up_st_int sync = %x\n",sync);
		if((0x100d == sync) && signal) {
			//if(XSGMII_Linkup_Wrapper_EN)MAC_Linkup_Wrapper();
			linkup_sta = 1;
			//if(USX_FORCE_USR_DATA) xsgmii_force_data(0,8,0,0);
			xsgmii_force_data(0,8,0,0);
			//printf("xsgmii_force_data 0 8 0 0\n");
		}	
		else linkup_sta = 0;
		if(dbg_print)printf("---------------link_up_st_int isr &&& ---------------\n");
	}
	else linkup_sta = 0;

//Linkdn int		
	if(dbg_print)printf("link_down_st_int %x\n",rg_xfi_pcs_int_sta_4.hal.link_down_st_int);
	if(rg_xfi_pcs_int_sta_4.hal.link_down_st_int){
		if(dbg_print)printf("---------------link_down_st_int isr *** ---------------\n");
		if(((0x100d != sync) && !signal )||((0x100d != sync) && XSGMII_Linkdn_Ignone_SD_EN)){
			linkdn_sta = 1;
			//if(USX_FORCE_USR_DATA)xsgmii_force_data(0,8,1,0);
			xsgmii_force_data(0,8,1,0);
			RX_RDY(0,0);
			RX_CDR_LFP_L2D(1,0);			
			#if 0
				TMR_INI();
			#else
				SigDet_Int_Init(1);
			#endif
			//if(XSGMII_Linkdn_Wrapper_EN)MAC_Linkdn_Wrapper();
			XSI_MAC_LOGIC_RESET();
			//printf("------XSI_MAC_LOGIC_RESET----------\n");

		}
		else linkdn_sta = 0;
		if(dbg_print)printf("---------------link_down_st_int isr &&& ---------------\n");
	}
	else linkdn_sta = 0;
		
	if(dbg_print)printf("usxgmii linkup_sta=%x,linkdn_sta=%x\n",linkup_sta,linkdn_sta);
	
	RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_2,0x1010101);
	RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_3,0x1010101);
	RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_4,0x1);
	
	if(dbg_print){
		rg_xfi_pcs_int_sta_2.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_2);	 
		rg_xfi_pcs_int_sta_3.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_3);	 
		rg_xfi_pcs_int_sta_4.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_4);	
		printf("After clear, rg_xfi_pcs_int_sta_2= %x,rg_xfi_pcs_int_sta_3= %x,rg_xfi_pcs_int_sta_4= %x\n",rg_xfi_pcs_int_sta_2.dat.value,rg_xfi_pcs_int_sta_3.dat.value,rg_xfi_pcs_int_sta_4.dat.value);
	}
	
}

static void usxgmii_pcs_int_init(u8 en){
	
	rg_type_t(HAL_rg_xfi_pcs_int_ctrl_2) rg_xfi_pcs_int_ctrl_2;    
	rg_type_t(HAL_rg_xfi_pcs_int_ctrl_3) rg_xfi_pcs_int_ctrl_3;	
	rg_type_t(HAL_rg_xfi_pcs_int_ctrl_4) rg_xfi_pcs_int_ctrl_4;

	printf("usxgmii_pcs_int en %x\n",en);
			RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_0,0x00); 				
			RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_1,0x00); 				
			RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_2,0x00); 				
			RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_3,0x00); 	
			RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_4,0x00);

	RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_2,0x1010101);
	RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_3,0x1010101);
	RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _xfi_pcs_int_sta_4,0x1);
	

			rg_xfi_pcs_int_ctrl_2.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_2);	
			rg_xfi_pcs_int_ctrl_3.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_3);	
			rg_xfi_pcs_int_ctrl_4.dat.value = RG_R_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_4);				

	rg_xfi_pcs_int_ctrl_2.hal.rg_r_type_e_int_en =0;			
	rg_xfi_pcs_int_ctrl_2.hal.rg_rxpcs_fsm_dec_err_int_en =0;	
	RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_2,rg_xfi_pcs_int_ctrl_2.dat.value);					
	rg_xfi_pcs_int_ctrl_3.hal.rg_hi_ber_st_int_en=0;	
	rg_xfi_pcs_int_ctrl_3.hal.rg_link_up_st_int_en=en;		
	rg_xfi_pcs_int_ctrl_3.hal.rg_rx_block_lock_st_int_en=0;
	rg_xfi_pcs_int_ctrl_3.hal.rg_fail_sync_xor_st_int_en=0;
	RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_3,rg_xfi_pcs_int_ctrl_3.dat.value);		
	rg_xfi_pcs_int_ctrl_4.hal.rg_link_down_st_int_en=en; 
	RG_W_PL(pcs1_base,USXGMII_PCS1_BASE_OFFSET,(RgAddr) _rg_xfi_pcs_int_ctrl_4,rg_xfi_pcs_int_ctrl_4.dat.value);
}

void serdes_phy_init()
{
#ifdef TCSUPPORT_CPU_AN7552
	unsigned char* serdes_intf[SERDES_INTF_MAX];
	int i = 0;
	serdes_intf[SERDES_PON] = getenv("serdes_pon");
	if((serdes_intf[4][1] == SERDES_ETHERWAN) && (serdes_intf[4][2] == SERDES_HSGMII_MODE))
	{
		printf("ETHERWAN & HSGMII mode\n");
		printf("pon-serdes setting\n");
		//8'h11: HSGMII 
		write_reg_word(0x1FB00070,0x11);
		write_reg_word(0x1FB0009C,0x409);
		write_reg_word(0x1FB00830,0x1);
		write_reg_word(0x1FB00830,0);
		write_reg_word(0x1FB00834,0x80000000);
		write_reg_word(0x1FB00834,0);

		//PHYA_INIT_START
		write_reg_word(0x1FAF4500,0x2004409);
		write_reg_word(0x1FAF4330,0x1771C1E);
		write_reg_word(0x1FAF4338,0x1771C1E);
		write_reg_word(0x1FAF4328,0x1771C1E);
		write_reg_word(0x1FAF4324,0xF60710);
		write_reg_word(0x1FAF3024,0x18000702);
		write_reg_word(0x1FAF3018,0x100);
		write_reg_word(0x1FAF3010,0x1F910001);
		write_reg_word(0x1FAF4600,0x779E1012);
		write_reg_word(0x1FAF464C,0x13);
		write_reg_word(0x1FAF4604,0x30004010);
		write_reg_word(0x1FAF4610,0x3E);
		write_reg_word(0x1FAF4228,0x3);

 		//freq
		write_reg_word(0x1FAF4364,0x3003);
		write_reg_word(0x1FAF436C,0x6180618);
		write_reg_word(0x1FAF3024,0x18000702);

		//CK
		write_reg_word(0x1FAF4300,0x2C0003C0);

		//RST
		write_reg_word(0x1FAF4310,0x3703F);
		write_reg_word(0x1FAF4608,0x56101801);
		write_reg_word(0x1FAF4530,0x380013);
		write_reg_word(0x1FAF3024,0x10000702);
		write_reg_word(0x1FAF3008,0x2400542);
		write_reg_word(0x1FAF4320,0x11);
		write_reg_word(0x1FAF4314,0x1);

		//NCPO
		write_reg_word(0x1FAF4600,0x779E0012);
		write_reg_word(0x1FAF4130,0x7D000000);
		write_reg_word(0x1FAF413C,0x10100);
		write_reg_word(0x1FAF4134,0x20000100);
		write_reg_word(0x1FAF413C,0x1010100);
		write_reg_word(0x1FAF3014,0x1000C87);
		write_reg_word(0x1FAF4200,0x100381);

		//PHYA_INIT_END
		write_reg_word(0x1FA80A00,0xC9CC000);
		write_reg_word(0x1FA80A24,0x0);
		write_reg_word(0x1FA84018,0x34);
		write_reg_word(0x1FA8602C,0x4);
		write_reg_word(0x1FA80000,0x140);
		write_reg_word(0x1FA86100,0);
		write_reg_word(0x1FA86000,0xC000C11);

		printf("pon-serdes setting done\n");
	}

#elif defined(TCSUPPORT_CPU_EN7581)
	unsigned char* serdes_intf[SERDES_INTF_MAX];
    int i = 0;

	rg_type_t(HAL_RG_PXP_CDR_PR_INJ_MODE) RG_PXP_CDR_PR_INJ_MODE;
	rg_type_t(HAL_rg_force_da_pxp_cdr_pr_lpf_c_en) rg_force_da_pxp_cdr_pr_lpf_c_en;
	rg_type_t(HAL_rg_force_da_pxp_cdr_pr_idac) rg_force_da_pxp_cdr_pr_idac;
	rg_type_t(HAL_rg_force_da_pxp_cdr_pr_pieye_pwdb) rg_force_da_pxp_cdr_pr_pieye_pwdb;
	rg_type_t(HAL_SS_RX_FLL_b) SS_RX_FLL_b;	
	rg_type_t(HAL_SS_RX_FLL_1) SS_RX_FLL_1;
	rg_type_t(HAL_RO_RX_FREQDET) RO_RX_FREQDET;	
	rg_type_t(HAL_SS_RX_FREQ_DET_2) SS_RX_FREQ_DET_2;
	rg_type_t(HAL_SS_RX_FREQ_DET_1) SS_RX_FREQ_DET_1;
	rg_type_t(HAL_SS_RX_FREQ_DET_4) SS_RX_FREQ_DET_4;	
	rg_type_t(HAL_SS_RX_FREQ_DET_3) SS_RX_FREQ_DET_3;
	rg_type_t(HAL_SW_RST_SET) SW_RST_SET; 	

	uint PrCal_Serach = 0 , RO_FL_Out = 0;
  	uint pr_idac = 0  , RO_pr_idac = 0;
  	int cdr_pr_idac_tmp = 0, RO_state_freqdet = 0, turn_pr_idac_bit_position = 0;
	int RO_FL_Out_diff = 0, RO_FL_Out_diff_tmp = 0xffff;
	int len_eth = 0;
	
	
	serdes_intf[SERDES_ETH] = getenv("serdes_ethernet");
    serdes_intf[SERDES_USB] = getenv("serdes_usb1");
    serdes_intf[SERDES_PCIE1] = getenv("serdes_wifi1");
    serdes_intf[SERDES_PCIE2] = getenv("serdes_wifi2");

	len_eth = strlen(serdes_intf[SERDES_ETH]);
	if(len_eth < 2)
	{
		printf("error eth_len=%d\n",len_eth);
		return;
	}

	/*only support eth_serdes phy*/
    if((serdes_intf[0][len_eth-2] == SERDES_ETHERLAN) && (serdes_intf[0][len_eth-1] == SERDES_XFI_MODE))
    {
		//printf("brown- %s:%d\n",__func__,__LINE__);
		printf("ETH VER = ETH.2.2.1-R\n");
		uint FL_Out_target = 0x9EDF ;
      		write_reg_word(0x1fb00094,0xe0002820);//add
    	//b''
		//b'echo "1 0 0 0 0">/proc/xsgmii\r\n'
		//b'op = 1,xsgmii 0,mod 0,an 0,rate 0,buf 31,num 5,count a\r\n'
		//b'xsgmii_api: serdes 1,xsgmii 0,mod 0,rate 0,an 0\r\n'
		//b'xsgmii_chg 1\r\n'
		write_reg_word(0x1fa7a000,0x10040001);
		//b'JCPLL BringUp 0\r\n'
		//b'JCPLL_LDO\r\n'
		write_reg_word(0x1fa7a048,0x1020ff);
		//b'JCPLL_RSTB\r\n'
		write_reg_word(0x1fa7a01c,0x3000004);
		write_reg_word(0x1fa7a01c,0x3000104);
		//b'JCPLL_EN\r\n'
		write_reg_word(0x1fa7b828,0x1000000);
		//b'JCPLL_SDM\r\n'
		write_reg_word(0x1fa7a01c,0x104);
		write_reg_word(0x1fa7a020,0x30000);
		write_reg_word(0x1fa7a024,0x5010100);
		//b'JCPLL_SSC\r\n'
		write_reg_word(0x1fa7a038,0x0);
		write_reg_word(0x1fa7a034,0x0);
		write_reg_word(0x1fa7a030,0x3018);
		//b'JCPLL_LPF\r\n'
		write_reg_word(0x1fa7a004,0x180000);
		write_reg_word(0x1fa7a008,0x101f0a);
		write_reg_word(0x1fa7a00c,0x2ff0000);
		//b'JCPLL_VCO\r\n'
		write_reg_word(0x1fa7a02c,0x4010100);
		write_reg_word(0x1fa7a030,0x301d);
		//b'JCPLL_PCW\r\n'
		write_reg_word(0x1fa7b800,0x25800000);
		write_reg_word(0x1fa7b79c,0x10000);
		//b'JCPLL_DIV\r\n'
		write_reg_word(0x1fa7a014,0x10000);
		write_reg_word(0x1fa7a02c,0x4010100);
		//b'JCPLL_KBand\r\n'
		write_reg_word(0x1fa7a010,0x1000300);
		write_reg_word(0x1fa7a00c,0x2e40000);
		//b'JCPLL_TCL\r\n'
		write_reg_word(0x1fa7a048,0xf20ff);
		write_reg_word(0x1fa7a024,0x5010100);
		write_reg_word(0x1fa7a028,0x10400);
		//b'JCPLL_EN\r\n'
		write_reg_word(0x1fa7b828,0x1010000);
		//b'JCPLL_Out\r\n'
		write_reg_word(0x1fa7b828,0x1010101);
		//b'TXPLL BringUp\r\n'
		//b'TXPLL_VCOLDO_Out\r\n'
		write_reg_word(0x1fa7a084,0x101031b);
		//b'TXPLL_RSTB\r\n'
		write_reg_word(0x1fa7a064,0x1040001);
		//b'TXPLL_EN\r\n'
		write_reg_word(0x1fa7b854,0x1000000);
		//b'TXPLL_SDM\r\n'
		write_reg_word(0x1fa7a068,0x0);
		write_reg_word(0x1fa7a06c,0x1010003);
		//b'TXPLL_SSC\r\n'
		write_reg_word(0x1fa7a080,0x0);
		write_reg_word(0x1fa7a07c,0x0);
		write_reg_word(0x1fa7a084,0x1010000);
		//b'TXPLL_LPF\r\n'
		write_reg_word(0x1fa7a050,0x1f05000f);
		write_reg_word(0x1fa7a054,0x180b02);
		//b'TXPLL_VCO\r\n'
		write_reg_word(0x1fa7a074,0x1000001);
		write_reg_word(0x1fa7a078,0x4040701);
		//b'TXPLL_PCW\r\n'
		write_reg_word(0x1fa7b798,0x8400000);
		write_reg_word(0x1fa7b794,0x1000000);
		//b'TXPLL_KBand\r\n'
		write_reg_word(0x1fa7a058,0x30004e4);
		write_reg_word(0x1fa7a05c,0x101);
		write_reg_word(0x1fa7a054,0x180b02);
		//b'TXPLL_DIV\r\n'
		write_reg_word(0x1fa7a05c,0x1);
		write_reg_word(0x1fa7a074,0x1000001);
		//b'TXPLL_TCL\r\n'
		write_reg_word(0x1fa7a094,0x1000f);
		write_reg_word(0x1fa7a070,0x4000b03);
		write_reg_word(0x1fa7a074,0x1000001);
		write_reg_word(0x1fa7a06c,0x1010003);
		//b'TXPLL_EN\r\n'
		write_reg_word(0x1fa7b854,0x1010000);
		//b'TXPLL_Out\r\n'
		write_reg_word(0x1fa7b854,0x1010101);
		//b'TX BringUp 0\r\n'
		//b'tx_rate_ctrl 0\r\n'
		write_reg_word(0x1fa7b580,0x2);
		//b'TX_CONFIG\r\n'
		write_reg_word(0x1fa7a0c4,0x1010401);
		write_reg_word(0x1fa7b874,0x1010000);
		write_reg_word(0x1fa7b77c,0x1050101);
		write_reg_word(0x1fa7b784,0x102);
		//b'TX_FIR_Load_Para swing 2, len 4, cn1 1, c0b 1, c1 b, en 1\r\n'
		//b'TX_FIR\r\n'
		write_reg_word(0x1fa7b778,0x1010101);
		write_reg_word(0x1fa7b780,0x10b);
		//b'TX_RSTB\r\n'
		write_reg_word(0x1fa7b260,0x101);
		//b'RX BringUp 0\r\n'
		//b'RX_INIT\r\n'
		//b'rx_rate_ctrl\r\n'
		write_reg_word(0x1fa7b374,0x2);
		//b'RX_Path_Init\r\n'
		write_reg_word(0x1fa7b184,0x40003ff);
		write_reg_word(0x1fa7a148,0x1010101);
		write_reg_word(0x1fa7a144,0x1000000);
		write_reg_word(0x1fa7a11c,0x2000401);
		write_reg_word(0x1fa7b004,0xc100a01);
		write_reg_word(0x1fa7a13c,0x20000);
		write_reg_word(0x1fa7a120,0x3ff08);
		write_reg_word(0x1fa7b320,0x10101);
		write_reg_word(0x1fa7b48c,0x1000202);
		write_reg_word(0x1fa7a0dc,0x0);
		write_reg_word(0x1fa7b80c,0x1000000);
		write_reg_word(0x1fa7b814,0x1010000);
		write_reg_word(0x1fa7a10c,0x70604);
		//b'RX_FE,force 0, Gain 1, Peaking 2\r\n'
		write_reg_word(0x1fa7b88c,0x0);
		write_reg_word(0x1fa7b768,0x0);
		//b'RX_FE_VOS\r\n'
		//write_reg_word(0x1fa7b79c,0x10000);
		//b'RX_IMP 1\r\n'
		write_reg_word(0x1fa7a114,0x1040000);
		//b'RX_FLL_PR_FMeter\r\n'
		write_reg_word(0x1fa7b390,0x100001);
		write_reg_word(0x1fa7b394,0xffff0000);
		write_reg_word(0x1fa7b39c,0x3107);
		//b'RX_REV\r\n'
		write_reg_word(0x1fa7a0d4,0xc8c31030);
		//b'RX_Rdy_TimeOut\r\n'
		write_reg_word(0x1fa7b100,0xa0005);
		//b'RX_CalBoundry_Init\r\n'
		write_reg_word(0x1fa7b08c,0x101);
		write_reg_word(0x1fa7b104,0x2);
		write_reg_word(0x1fa7b090,0x320002);
		write_reg_word(0x1fa7b09c,0x320002);
		write_reg_word(0x1fa7b094,0x320002);
		write_reg_word(0x1fa7b098,0x320002);
		//b'RX_BySerdes\r\n'
		//b'RX_OSR\r\n'
		write_reg_word(0x1fa7b76c,0x1000000);
		write_reg_word(0x1fa7a0dc,0x0);
		//b'CDR_LPF_RATIO\r\n'
		write_reg_word(0x1fa7a0e8,0x2000000);
		//b'CDR_PR\r\n'
		write_reg_word(0x1fa7a0f8,0x4010808);
		write_reg_word(0x1fa7a0fc,0x80606);
		//b'RX_EYE_Mon\r\n'
		write_reg_word(0x1fa7b120,0x103);
		write_reg_word(0x1fa7b088,0x1);
		//b'RX_SYS_En\r\n'
		write_reg_word(0x1fa7b38c,0x1);
		write_reg_word(0x1fa7b000,0x1000000);
		//b'RX_FLL_PR_FMeter\r\n'
		write_reg_word(0x1fa7b33c,0x1010100);
		write_reg_word(0x1fa7b330,0x1);
		//b'RX_CMLEQ_EN\r\n'
		write_reg_word(0x1fa7a118,0x1010100);
		//b'RX_CDR_PR\r\n'
		write_reg_word(0x1fa7a10c,0x70604);
		//b'RX_CDR_xxx_Pwdb\r\n'
		write_reg_word(0x1fa7b824,0x1000100);
		write_reg_word(0x1fa7b81c,0x100);
		write_reg_word(0x1fa7b894,0x100);
		write_reg_word(0x1fa7b84c,0x1000000);
		write_reg_word(0x1fa7b34c,0x0);
		//b'RX_SigDet\r\n'
		write_reg_word(0x1fa7a114,0x1020200);
		write_reg_word(0x1fa7a110,0x3000200);
		//b'RX_SigDet_Pwdb\r\n'
		write_reg_word(0x1fa7b350,0x0);
		//b'PXP_RX_PHYCK\r\n'
		write_reg_word(0x1fa7a0d8,0x10242);
		write_reg_word(0x1fa7a0cc,0x1000000);
		//b'RX_CDR_xxx_Pwdb\r\n'
		write_reg_word(0x1fa7b824,0x1010101);
		write_reg_word(0x1fa7b81c,0x101);
		write_reg_word(0x1fa7b894,0x101);
		write_reg_word(0x1fa7b84c,0x1010000);
		write_reg_word(0x1fa7b34c,0x1010101);
		//b'RX_SigDet_Pwdb\r\n'
		write_reg_word(0x1fa7b350,0x1);
		//b'RX_PR_CAL_SEQ 0\r\n'
		//b'RX_CDR_LFP_L2D mode 1, sel 0\r\n'
		write_reg_word(0x1fa7b818,0x100);
		//b'RX_PRCal 0\r\n'
        write_reg_word(0x1fa7b460,0x20);
        write_reg_word(0x1fa7b150,0x9f439e7b);
        write_reg_word(0x1fa7b14c,0x7fff7fff);
        write_reg_word(0x1fa7b158,0x3307);
        write_reg_word(0x1fa7b154,0x9f439e7b);
        write_reg_word(0x1fa7a0f4,0x1000000);
        write_reg_word(0x1fa7b820,0x1010100);
        write_reg_word(0x1fa7b794,0x1010000);
        write_reg_word(0x1fa7b824,0x1010101);
        write_reg_word(0x1fa7b824,0x1000101);
        write_reg_word(0x1fa7b824,0x1010101);
		
		for (PrCal_Serach = 1; PrCal_Serach < 8 ; PrCal_Serach++)
  		{

		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac = PrCal_Serach<<8;		
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,(u32)(rg_force_da_pxp_cdr_pr_idac.dat.value));

	 	write_reg_word(0x1fa7b158,0x3300);

		write_reg_word(0x1fa7b158,0x3303);

	 	udelay(5000);

		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);		
		RO_FL_Out = RO_RX_FREQDET.hal.ro_fl_out;
		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		RO_pr_idac = rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac;
	 	printf("pr_idac = 0x%x ,RO_FL_Out = 0x%x\n" ,RO_pr_idac , RO_FL_Out);

		//abs
		if (RO_FL_Out > FL_Out_target) RO_FL_Out_diff = RO_FL_Out - FL_Out_target;
		else if (RO_FL_Out < FL_Out_target) RO_FL_Out_diff = FL_Out_target - RO_FL_Out;
		else RO_FL_Out_diff = 0;
		
	 	if(RO_FL_Out > FL_Out_target)
	 	{        
			RO_FL_Out_diff_tmp = RO_FL_Out_diff;
			cdr_pr_idac_tmp = (PrCal_Serach<<8);
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);		 
	 	}
  		}

  		for (turn_pr_idac_bit_position = 7; turn_pr_idac_bit_position > -1 ; turn_pr_idac_bit_position--)
  		{
		pr_idac = cdr_pr_idac_tmp |(0x1<<turn_pr_idac_bit_position); 
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac = pr_idac;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);

	 	write_reg_word(0x1fa7b158,0x3300);

		write_reg_word(0x1fa7b158,0x3303);

		udelay(5000);	  
	    
		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);	  
		RO_FL_Out = RO_RX_FREQDET.hal.ro_fl_out;

		printf("pr_idac = 0x%x ,RO_FL_Out = 0x%x\n",pr_idac,RO_FL_Out);

		if(RO_FL_Out < FL_Out_target)
		{
	        pr_idac &= ~(0x1<<turn_pr_idac_bit_position);
			cdr_pr_idac_tmp = pr_idac;
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);
		}
		else
		{
			cdr_pr_idac_tmp = pr_idac;
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);
		}   
	  
  		}

		rg_force_da_pxp_cdr_pr_idac.dat.value = cdr_pr_idac_tmp;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);
		printf("sel_cdr_pr_idac = 0x%x\n",cdr_pr_idac_tmp);

		write_reg_word(0x1fa7b158,0x3300);
		write_reg_word(0x1fa7b158,0x3303);

		udelay(5000);
		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);	  
  		RO_state_freqdet = RO_RX_FREQDET.hal.ro_fbck_lock; 

		printf("RO_state_freqdet = 0x%x\n",RO_state_freqdet);

		//Load_Band
		RG_PXP_CDR_PR_INJ_MODE.dat.value = RG_R_PL(0x1fa7a000,PXP_BASE_OFFSET,(u32) _RG_PXP_CDR_PR_INJ_MODE);
		rg_force_da_pxp_cdr_pr_lpf_c_en.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_lpf_c_en);
		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		SS_RX_FLL_b.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_b);
		SS_RX_FLL_1.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_1);
		rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb);
		
		RG_PXP_CDR_PR_INJ_MODE.hal.rg_pxp_cdr_pr_inj_force_off = 0;
		RG_W_PL(0x1fa7a000,PXP_BASE_OFFSET,(u32) _RG_PXP_CDR_PR_INJ_MODE,RG_PXP_CDR_PR_INJ_MODE.dat.value);
		
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_da_pxp_cdr_pr_lpf_c_en = 0;	
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_sel_da_pxp_cdr_pr_lpf_c_en = 0;
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_da_pxp_cdr_pr_lpf_r_en = 1;
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_sel_da_pxp_cdr_pr_lpf_r_en = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_lpf_c_en,rg_force_da_pxp_cdr_pr_lpf_c_en.dat.value);
			
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_sel_da_pxp_cdr_pr_idac = 0;	
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);
		
		SS_RX_FLL_b.hal.rg_load_en = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_b,SS_RX_FLL_b.dat.value);
		SS_RX_FLL_1.hal.rg_ipath_idac = cdr_pr_idac_tmp;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_1,SS_RX_FLL_1.dat.value);
			
		
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_da_pxp_cdr_pr_pwdb = 0;	
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_da_pxp_cdr_pr_pwdb = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);	
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_sel_da_pxp_cdr_pr_pwdb = 0;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);
		
		SW_RST_SET.hal.rg_sw_ref_rst_n = RX_PRCal_REF_RESETB_HI_EN;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SW_RST_SET,SW_RST_SET.dat.value);
		//return RO_state_freqdet;
		//RX_PRCal_end--------------------------------
		//b'RX_CDR_LPF_RSTB mode 1, sel0\r\n'
		write_reg_word(0x1fa7b818,0x1000100);
		udelay(700);
		//b'RX_CDR_LPF_RSTB mode 1, sel1\r\n'
		write_reg_word(0x1fa7b818,0x1010100);
		udelay(100);
		//b'RX_CDR_LFP_L2D mode 1, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x1010101);
		
		//b'RX_CDR_LPF_RSTB mode 0, sel1\r\n'
		write_reg_word(0x1fa7b818,0x10101);
		//b'RX_CDR_LFP_L2D mode 0, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x10001);
		
		//b'RSTB sel8, val 0\r\n'
		write_reg_word(0x1fa7b460,0x20);
		//b'RSTB sel8, val 1\r\n'
		write_reg_word(0x1fa7b460,0xfff);
		//b'RX_CDR_RST\r\n'
		//b'RX_CDR_LFP_L2D mode 1, sel 0\r\n'
		write_reg_word(0x1fa7b818,0x10100);
		//b'RX_CDR_LPF_RSTB mode 1, sel0\r\n'
		write_reg_word(0x1fa7b818,0x1000100);
		udelay(700);
		//b'RX_CDR_LPF_RSTB mode 1, sel1\r\n'
		write_reg_word(0x1fa7b818,0x1010100);
		udelay(100);
		//b'RX_CDR_LFP_L2D mode 1, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x1010101);
		//b'RX_CDR_LPF_RSTB mode 0, sel1\r\n'
		write_reg_word(0x1fa7b818,0x10101);
		//b'RX_CDR_LFP_L2D mode 0, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x10001);
		//b'xSGMII_Solution : 0\r\n'
		//b'xsgmii_init\r\n'
		//b'usxgmii_init 1\r\n'
		write_reg_word(0x1fa74100,0x10010001);
		//b'PCS E0/E1 solution\r\n'
		write_reg_word(0x1fa75b2c,0x0);
		//b'usxgmii_pcs_int en 0\r\n'
		write_reg_word(0x1fa75bc0,0x0);
		write_reg_word(0x1fa75bc4,0x0);
		write_reg_word(0x1fa75bc8,0x0);
		write_reg_word(0x1fa75bcc,0x0);
		write_reg_word(0x1fa75be0,0x0);
		write_reg_word(0x1fa75bd8,0x0);
		write_reg_word(0x1fa75bdc,0x0);
		write_reg_word(0x1fa75be4,0x0);
		write_reg_word(0x1fa75bc8,0x0);
		write_reg_word(0x1fa75bcc,0x0);
		write_reg_word(0x1fa75be0,0x0);
		//b'xsgmii 0 int enable 0\r\n'
		//b'force mode\r\n'
		//b'xSGMII_AN_API 0, enable : 0\r\n'
		//b'usxgmii_an\r\n'
		write_reg_word(0x1fa75bf8,0x6330000);
		//b'_rg_usxgmii_an_control_0 6330000\r\n'
		//b'_rg_usxgmii_an_control_1 0\r\n'
		//b'usxgmii_rate_api rate 0 : 10GUSXGMII_10G\r\n'
		write_reg_word(0x1fa75bfc,0x1001);
		write_reg_word(0x1fa76000,0xc000c11);
		write_reg_word(0x1fa7602c,0x104);
      
      		//set mac reg init
      		write_reg_word(0x1fa09000,0x71082800);
      		printf("set mac reg init\n");
      
		//b'_rg_usxgmii_an_control_1 1001\r\n'
		//b'_rg_rate_adapt_ctrl_0 c000c11\r\n'
		//b'_rg_rate_adapt_ctrl_11 104\r\n'
		//b'USXGMII_10G exit\r\n'
		//b'usxgmii_pcs_an_ctrl7\r\n'
		write_reg_word(0x1fa75c20,0x1000);
		printf("XFI_10G exit\n");
		//b'xSGMII_Solution : 1\r\n'
		return ;
	}else if((serdes_intf[0][len_eth-2] == SERDES_ETHERLAN) && (serdes_intf[0][len_eth-1] == SERDES_HSGMII_MODE)) {
		//printf("brown- %s:%d\n",__func__,__LINE__);
		printf("ETH VER = ETH.2.2.1-R\n");
		uint FL_Out_target = 0xA000 ;
        //b''
        //b'echo "1 1 0 0 0">/proc/xsgmii\r\n'
        //b'op = 1,xsgmii 1,mod 0,an 0,rate 0,buf 31,num 5,count a\r\n'
        //b'xsgmii_api: serdes 1,xsgmii 1,mod 0,rate 0,an 0\r\n'
        //b'xsgmii_chg 1\r\n'
        write_reg_word(0x1fa7a000,0x10040001);
        //b'JCPLL BringUp 1\r\n'
        //b'JCPLL_LDO\r\n'
        write_reg_word(0x1fa7a048,0x1020ff);
        //b'JCPLL_RSTB\r\n'
        write_reg_word(0x1fa7a01c,0x3000004);
        write_reg_word(0x1fa7a01c,0x3000104);
        //b'JCPLL_EN\r\n'
        write_reg_word(0x1fa7b828,0x1000000);
        //b'JCPLL_SDM\r\n'
        write_reg_word(0x1fa7a01c,0x104);
        write_reg_word(0x1fa7a020,0x30000);
        write_reg_word(0x1fa7a024,0x5010100);
        //b'JCPLL_SSC\r\n'
        write_reg_word(0x1fa7a038,0x0);
        write_reg_word(0x1fa7a034,0x0);
        write_reg_word(0x1fa7a030,0x3018);
        //b'JCPLL_LPF\r\n'
        write_reg_word(0x1fa7a004,0x180000);
        write_reg_word(0x1fa7a008,0x101f0a);
        write_reg_word(0x1fa7a00c,0x2ff0000);
        //b'JCPLL_VCO\r\n'
        write_reg_word(0x1fa7a02c,0x4010100);
        write_reg_word(0x1fa7a030,0x301d);
        //b'JCPLL_PCW\r\n'
        write_reg_word(0x1fa7b800,0x25800000);
        write_reg_word(0x1fa7b79c,0x10000);
        //b'JCPLL_DIV\r\n'
        write_reg_word(0x1fa7a014,0x10000);
        write_reg_word(0x1fa7a02c,0x4010100);
        //b'JCPLL_KBand\r\n'
        write_reg_word(0x1fa7a010,0x1000300);
        write_reg_word(0x1fa7a00c,0x2e40000);
        //b'JCPLL_TCL\r\n'
        write_reg_word(0x1fa7a048,0x1020ff);
        write_reg_word(0x1fa7a024,0x5010100);
        write_reg_word(0x1fa7a028,0x10400);
        //b'JCPLL_EN\r\n'
        write_reg_word(0x1fa7b828,0x1010000);
        //b'JCPLL_Out\r\n'
        write_reg_word(0x1fa7b828,0x1010101);
        //b'TXPLL BringUp\r\n'
        //b'TXPLL_VCOLDO_Out\r\n'
        write_reg_word(0x1fa7a084,0x101031b);
        //b'TXPLL_RSTB\r\n'
        write_reg_word(0x1fa7a064,0x1040001);
        //b'TXPLL_EN\r\n'
        write_reg_word(0x1fa7b854,0x1000000);
        //b'TXPLL_SDM\r\n'
        write_reg_word(0x1fa7a068,0x0);
        write_reg_word(0x1fa7a06c,0x1000003);
        //b'TXPLL_SSC\r\n'
        write_reg_word(0x1fa7a080,0x0);
        write_reg_word(0x1fa7a07c,0x0);
        write_reg_word(0x1fa7a084,0x1010000);
        //b'TXPLL_LPF\r\n'
        write_reg_word(0x1fa7a050,0x1f05000a);
        write_reg_word(0x1fa7a054,0x5);
        //b'TXPLL_VCO\r\n'
        write_reg_word(0x1fa7a074,0x1);
        write_reg_word(0x1fa7a078,0x4040701);
        //b'TXPLL_PCW\r\n'
        write_reg_word(0x1fa7b798,0xa000000);
        write_reg_word(0x1fa7b794,0x1000000);
        //b'TXPLL_KBand\r\n'
        write_reg_word(0x1fa7a058,0x30004e4);
        write_reg_word(0x1fa7a05c,0x101);
        write_reg_word(0x1fa7a054,0x5);
        //b'TXPLL_DIV\r\n'
        write_reg_word(0x1fa7a05c,0x1);
        write_reg_word(0x1fa7a074,0x10001);
        //b'TXPLL_TCL\r\n'
        write_reg_word(0x1fa7a0940,0x1000f);
        write_reg_word(0x1fa7a0700,0x4000d03);
        write_reg_word(0x1fa7a0740,0x10001);
        write_reg_word(0x1fa7a06c0,0x1000003);
        //b'TXPLL_EN\r\n'
        write_reg_word(0x1fa7b854,0x1010000);
        //b'TXPLL_Out\r\n'
        write_reg_word(0x1fa7b854,0x1010101);
        //b'TX BringUp 1\r\n'
        //b'tx_rate_ctrl 1\r\n'
        write_reg_word(0x1fa7b580,0x1);
        //b'TX_CONFIG\r\n'
        write_reg_word(0x1fa7a0c4,0x1010401);
        write_reg_word(0x1fa7b874,0x1010000);
        write_reg_word(0x1fa7b77c,0x1040101);
        write_reg_word(0x1fa7b784,0x101);
        //b'TX_FIR_Load_Para swing 2, len 4, cn1 0, c0b b, c1 1, en 1\r\n'
        //b'TX_FIR\r\n'
        write_reg_word(0x1fa7b778,0x100010b);
        write_reg_word(0x1fa7b780,0x101);
        //b'TX_RSTB\r\n'
        write_reg_word(0x1fa7b260,0x101);
        //b'RX BringUp 1\r\n'
        //b'RX_INIT\r\n'
        //b'rx_rate_ctrl\r\n'
        write_reg_word(0x1fa7b374,0x0);
        //b'RX_Path_Init\r\n'
        write_reg_word(0x1fa7b184,0x40003ff);
        write_reg_word(0x1fa7a148,0x1010101);
        write_reg_word(0x1fa7a144,0x1000000);
        write_reg_word(0x1fa7a11c,0x2000401);
        write_reg_word(0x1fa7b004,0xc100a01);
        write_reg_word(0x1fa7a13c,0x20000);
        write_reg_word(0x1fa7a120,0x3ff08);
        write_reg_word(0x1fa7b320,0x10101);
        write_reg_word(0x1fa7b48c,0x1000202);
        write_reg_word(0x1fa7a0dc,0x0);
        write_reg_word(0x1fa7b80c,0x1000000);
        write_reg_word(0x1fa7b814,0x1010000);
        write_reg_word(0x1fa7a10c,0x70604);
        //b'RX_FE,force 0, Gain 1, Peaking 1\r\n'
        write_reg_word(0x1fa7b88c,0x0);
        write_reg_word(0x1fa7b768,0x0);
        //b'RX_FE_VOS\r\n'
        write_reg_word(0x1fa7b79c,0x10100);
        //b'RX_IMP 1\r\n'
        write_reg_word(0x1fa7a114,0x1040000);
        //b'RX_FLL_PR_FMeter\r\n'
        write_reg_word(0x1fa7b390,0x100001);
        write_reg_word(0x1fa7b394,0xffff0000);
        write_reg_word(0x1fa7b39c,0x3107);
        //b'RX_REV\r\n'
        write_reg_word(0x1fa7a0d4,0xc8c31030);
        //b'RX_Rdy_TimeOut\r\n'
        write_reg_word(0x1fa7b100,0xa0005);
        //b'RX_CalBoundry_Init\r\n'
        write_reg_word(0x1fa7b08c,0x101);
        write_reg_word(0x1fa7b104,0x2);
        write_reg_word(0x1fa7b090,0x320002);
        write_reg_word(0x1fa7b09c,0x320002);
        write_reg_word(0x1fa7b094,0x320002);
        write_reg_word(0x1fa7b098,0x320002);
        //b'RX_BySerdes\r\n'
        //b'RX_OSR\r\n'
        write_reg_word(0x1fa7b76c,0x1010000);
        write_reg_word(0x1fa7a0dc,0x100);
        //b'CDR_LPF_RATIO\r\n'
        write_reg_word(0x1fa7a0e8,0x2000001);
        //b'CDR_PR\r\n'
        write_reg_word(0x1fa7a0f8,0x4010806);
        write_reg_word(0x1fa7a0fc,0x60606);
        //b'RX_EYE_Mon\r\n'
        write_reg_word(0x1fa7b120,0x103);
        write_reg_word(0x1fa7b088,0x1);
        //b'RX_SYS_En\r\n'
        write_reg_word(0x1fa7b38c,0x1);
        write_reg_word(0x1fa7b000,0x1000000);
        //b'RX_FLL_PR_FMeter\r\n'
        write_reg_word(0x1fa7b33c,0x1010100);
        write_reg_word(0x1fa7b330,0x1);
        //b'RX_CMLEQ_EN\r\n'
        write_reg_word(0x1fa7a118,0x1010100);
        //b'RX_CDR_PR\r\n'
        write_reg_word(0x1fa7a10c,0xe0604);
        //b'RX_CDR_xxx_Pwdb\r\n'
        write_reg_word(0x1fa7b824,0x1000100);
        write_reg_word(0x1fa7b81c,0x100);
        write_reg_word(0x1fa7b894,0x100);
        write_reg_word(0x1fa7b84c,0x1000000);
        write_reg_word(0x1fa7b34c,0x0);
        //b'RX_SigDet\r\n'
        write_reg_word(0x1fa7a114,0x1020200);
        write_reg_word(0x1fa7a110,0x3000200);
        //b'RX_SigDet_Pwdb\r\n'
        write_reg_word(0x1fa7b350,0x0);
        //b'PXP_RX_PHYCK\r\n'
        write_reg_word(0x1fa7a0d8,0x1010b);
        write_reg_word(0x1fa7a0cc,0x1000000);
        //b'RX_CDR_xxx_Pwdb\r\n'
        write_reg_word(0x1fa7b824,0x1010101);
        write_reg_word(0x1fa7b81c,0x101);
        write_reg_word(0x1fa7b894,0x101);
        write_reg_word(0x1fa7b84c,0x1010000);
        write_reg_word(0x1fa7b34c,0x1010101);
        //b'RX_SigDet_Pwdb\r\n'
        write_reg_word(0x1fa7b350,0x1);
		//b'RX_PR_CAL_SEQ 1\r\n'
		//b'RX_CDR_LFP_L2D mode 1, sel 0\r\n'
		write_reg_word(0x1fa7b818,0x100);
		//b'RX_PRCal 1\r\n'
		write_reg_word(0x1fa7b460,0x20);
		write_reg_word(0x1fa7b150,0xa0649f9c);
		write_reg_word(0x1fa7b14c,0x4e204e20);
		write_reg_word(0x1fa7b158,0x3307);
		write_reg_word(0x1fa7b154,0xa0649f9c);
		write_reg_word(0x1fa7a0f4,0x1000000);
		write_reg_word(0x1fa7b820,0x1010100);
		write_reg_word(0x1fa7b794,0x1010000);
		write_reg_word(0x1fa7b824,0x1010101);
		write_reg_word(0x1fa7b824,0x1000101);
		write_reg_word(0x1fa7b824,0x1010101);

		
		for (PrCal_Serach = 1; PrCal_Serach < 8 ; PrCal_Serach++)
  		{

		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac = PrCal_Serach<<8;		
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,(u32)(rg_force_da_pxp_cdr_pr_idac.dat.value));

	 	write_reg_word(0x1fa7b158,0x3300);

		write_reg_word(0x1fa7b158,0x3303);

	 	udelay(5000);

		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);		
		RO_FL_Out = RO_RX_FREQDET.hal.ro_fl_out;
		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		RO_pr_idac = rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac;
	 	printf("pr_idac = 0x%x ,RO_FL_Out = 0x%x\n" ,RO_pr_idac , RO_FL_Out);

		//abs
		if (RO_FL_Out > FL_Out_target) RO_FL_Out_diff = RO_FL_Out - FL_Out_target;
		else if (RO_FL_Out < FL_Out_target) RO_FL_Out_diff = FL_Out_target - RO_FL_Out;
		else RO_FL_Out_diff = 0;
		
	 	if(RO_FL_Out > FL_Out_target)
	 	{        
			RO_FL_Out_diff_tmp = RO_FL_Out_diff;
			cdr_pr_idac_tmp = (PrCal_Serach<<8);
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);		 
	 	}
  		}

  		for (turn_pr_idac_bit_position = 7; turn_pr_idac_bit_position > -1 ; turn_pr_idac_bit_position--)
  		{
		pr_idac = cdr_pr_idac_tmp |(0x1<<turn_pr_idac_bit_position); 
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac = pr_idac;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);

	 	write_reg_word(0x1fa7b158,0x3300);

		write_reg_word(0x1fa7b158,0x3303);

		udelay(5000);	  
	    
		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);	  
		RO_FL_Out = RO_RX_FREQDET.hal.ro_fl_out;

		printf("pr_idac = 0x%x ,RO_FL_Out = 0x%x\n",pr_idac,RO_FL_Out);

		if(RO_FL_Out < FL_Out_target)
		{
	        pr_idac &= ~(0x1<<turn_pr_idac_bit_position);
			cdr_pr_idac_tmp = pr_idac;
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);
		}
		else
		{
			cdr_pr_idac_tmp = pr_idac;
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);
		}   
	  
  		}

		rg_force_da_pxp_cdr_pr_idac.dat.value = cdr_pr_idac_tmp;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);
		printf("sel_cdr_pr_idac = 0x%x\n",cdr_pr_idac_tmp);

		write_reg_word(0x1fa7b158,0x3300);
		write_reg_word(0x1fa7b158,0x3303);

		udelay(5000);
		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);	  
  		RO_state_freqdet = RO_RX_FREQDET.hal.ro_fbck_lock; 

		printf("RO_state_freqdet = 0x%x\n",RO_state_freqdet);

		//Load_Band
		RG_PXP_CDR_PR_INJ_MODE.dat.value = RG_R_PL(0x1fa7a000,PXP_BASE_OFFSET,(u32) _RG_PXP_CDR_PR_INJ_MODE);
		rg_force_da_pxp_cdr_pr_lpf_c_en.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_lpf_c_en);
		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		SS_RX_FLL_b.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_b);
		SS_RX_FLL_1.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_1);
		rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb);
		
		RG_PXP_CDR_PR_INJ_MODE.hal.rg_pxp_cdr_pr_inj_force_off = 0;
		RG_W_PL(0x1fa7a000,PXP_BASE_OFFSET,(u32) _RG_PXP_CDR_PR_INJ_MODE,RG_PXP_CDR_PR_INJ_MODE.dat.value);
		
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_da_pxp_cdr_pr_lpf_c_en = 0;	
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_sel_da_pxp_cdr_pr_lpf_c_en = 0;
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_da_pxp_cdr_pr_lpf_r_en = 1;
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_sel_da_pxp_cdr_pr_lpf_r_en = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_lpf_c_en,rg_force_da_pxp_cdr_pr_lpf_c_en.dat.value);
			
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_sel_da_pxp_cdr_pr_idac = 0;	
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);
		
		SS_RX_FLL_b.hal.rg_load_en = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_b,SS_RX_FLL_b.dat.value);
		SS_RX_FLL_1.hal.rg_ipath_idac = cdr_pr_idac_tmp;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_1,SS_RX_FLL_1.dat.value);
			
		
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_da_pxp_cdr_pr_pwdb = 0;	
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_da_pxp_cdr_pr_pwdb = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);	
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_sel_da_pxp_cdr_pr_pwdb = 0;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);
		
		SW_RST_SET.hal.rg_sw_ref_rst_n = RX_PRCal_REF_RESETB_HI_EN;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SW_RST_SET,SW_RST_SET.dat.value);
		//return RO_state_freqdet;
		//RX_PRCal_end--------------------------------
		//b'RX_CDR_LPF_RSTB mode 1, sel0\r\n'
		write_reg_word(0x1fa7b818,0x1000100);
		udelay(700);
		//b'RX_CDR_LPF_RSTB mode 1, sel1\r\n'
		write_reg_word(0x1fa7b818,0x1010100);
		udelay(100);
		//b'RX_CDR_LFP_L2D mode 1, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x1010101);
		
		//b'RX_CDR_LPF_RSTB mode 0, sel1\r\n'
		write_reg_word(0x1fa7b818,0x10101);
		//b'RX_CDR_LFP_L2D mode 0, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x10001);
		//---------------------------------------------
		//b'RSTB sel8, val 0\r\n'
		write_reg_word(0x1fa7b460,0x20);
		//b'RSTB sel8, val 1\r\n
		write_reg_word(0x1fa7b460,0xfff);
		//b'RX_CDR_RST\r\n'
        //b'RX_CDR_LFP_L2D mode 1, sel 0\r\n'
        write_reg_word(0x1fa7b818,0x10100);
        //b'RX_CDR_LPF_RSTB mode 1, sel0\r\n'
        write_reg_word(0x1fa7b818,0x1000100);
		udelay(700);
        //b'RX_CDR_LPF_RSTB mode 1, sel1\r\n'
        write_reg_word(0x1fa7b818,0x1010100);
		udelay(100);
        //b'RX_CDR_LFP_L2D mode 1, sel 1\r\n'
        write_reg_word(0x1fa7b818,0x1010101);
        //b'RX_CDR_LPF_RSTB mode 0, sel1\r\n'
        write_reg_word(0x1fa7b818,0x10101);
        //b'RX_CDR_LFP_L2D mode 0, sel 1\r\n'
        write_reg_word(0x1fa7b818,0x10001);
		//b'xSGMII_Solution : 0\r\n'
		//b'xsgmii_init\r\n'
		//b'usxgmii_init\r\n'
		write_reg_word(0x1fa70a00,0x4c9cc000);
		write_reg_word(0x1fa74018,0x34);
		//b'hsgmii_init\r\n'
		//b'xsgmii 1 int enable 0\r\n'
		//b'force mode\r\n'
		//b'xSGMII_AN_API 1, enable : 0\r\n'
		//b'sgmii_an 0\r\n'
		write_reg_word(0x1fa70000,0x140);
		//b'hsgmii\r\n'
		//b'HGMII_2.5G\r\n'
		write_reg_word(0x1fa76000,0xc000c11);
		//b'HSGMII_2.5 exit\r\n'
		printf("HSGMII_2.5 exit\n");
		//b'xSGMII_Solution : 1\r\n'

	}else if((serdes_intf[0][len_eth-2] == SERDES_ETHERLAN) && (serdes_intf[0][len_eth-1] == SERDES_SGMII_MODE)) {
		//printf("brown- %s:%d\n",__func__,__LINE__);
		printf("ETH VER = ETH.2.2.1-R\n");
		uint FL_Out_target = 0xA3D6 ;
        //b''
		//b'echo "1 2 0 0 0">/proc/xsgmii\r\n'
		//b'op = 1,xsgmii 2,mod 0,an 0,rate 0,buf 31,num 5,count a\r\n'
		//b'xsgmii_api: serdes 1,xsgmii 2,mod 0,rate 0,an 0\r\n'
		//b'xsgmii_chg 1\r\n'
		write_reg_word(0x1fa7a000,0x10040001);
		//b'JCPLL BringUp 2\r\n'
		//b'JCPLL_LDO\r\n'
		write_reg_word(0x1fa7a048,0x1020ff);
		//b'JCPLL_RSTB\r\n'
		write_reg_word(0x1fa7a01c,0x3000004);
		write_reg_word(0x1fa7a01c,0x3000104);
		//b'JCPLL_EN\r\n'
		write_reg_word(0x1fa7b828,0x1000000);
		//b'JCPLL_SDM\r\n'
		write_reg_word(0x1fa7a01c,0x104);
		write_reg_word(0x1fa7a020,0x30000);
		write_reg_word(0x1fa7a024,0x5010100);
		//b'JCPLL_SSC\r\n'
		write_reg_word(0x1fa7a038,0x0);
		write_reg_word(0x1fa7a034,0x0);
		write_reg_word(0x1fa7a030,0x3018);
		//b'JCPLL_LPF\r\n'
		write_reg_word(0x1fa7a004,0x180000);
		write_reg_word(0x1fa7a008,0x101f0a);
		write_reg_word(0x1fa7a00c,0x2ff0000);
		//b'JCPLL_VCO\r\n'
		write_reg_word(0x1fa7a02c,0x4010100);
		write_reg_word(0x1fa7a030,0x301d);
		//b'JCPLL_PCW\r\n'
		write_reg_word(0x1fa7b800,0x25800000);
		write_reg_word(0x1fa7b79c,0x10000);
		//b'JCPLL_DIV\r\n'
		write_reg_word(0x1fa7a014,0x10000);
		write_reg_word(0x1fa7a02c,0x4010100);
		//b'JCPLL_KBand\r\n'
		write_reg_word(0x1fa7a010,0x1000300);
		write_reg_word(0x1fa7a00c,0x2e40000);
		//b'JCPLL_TCL\r\n'
		write_reg_word(0x1fa7a048,0x1020ff);
		write_reg_word(0x1fa7a024,0x5010100);
		write_reg_word(0x1fa7a028,0x10400);
		//b'JCPLL_EN\r\n'
		write_reg_word(0x1fa7b828,0x1010000);
		//b'JCPLL_Out\r\n'
		write_reg_word(0x1fa7b828,0x1010101);
		//b'TXPLL BringUp\r\n'
		//b'TXPLL_VCOLDO_Out\r\n'
		write_reg_word(0x1fa7a084,0x101031b);
		//b'TXPLL_RSTB\r\n'
		write_reg_word(0x1fa7a064,0x1040001);
		//b'TXPLL_EN\r\n'
		write_reg_word(0x1fa7b854,0x1000000);
		//b'TXPLL_SDM\r\n'
		write_reg_word(0x1fa7a068,0x0);
		write_reg_word(0x1fa7a06c,0x1000003);
		//b'TXPLL_SSC\r\n'
		write_reg_word(0x1fa7a080,0x0);
		write_reg_word(0x1fa7a07c,0x0);
		write_reg_word(0x1fa7a084,0x1010000);
		//b'TXPLL_LPF\r\n'
		write_reg_word(0x1fa7a050,0x1f05000f);
		write_reg_word(0x1fa7a054,0x180b02);
		//b'TXPLL_VCO\r\n'
		write_reg_word(0x1fa7a074,0x3000001);
		write_reg_word(0x1fa7a078,0x4040701);
		//b'TXPLL_PCW\r\n'
		write_reg_word(0x1fa7b798,0x8000000);
		write_reg_word(0x1fa7b794,0x1000000);
		//b'TXPLL_KBand\r\n'
		write_reg_word(0x1fa7a058,0x30004e4);
		write_reg_word(0x1fa7a05c,0x101);
		write_reg_word(0x1fa7a054,0x180b02);
		//b'TXPLL_DIV\r\n'
		write_reg_word(0x1fa7a05c,0x1);
		write_reg_word(0x1fa7a074,0x3000001);
		//b'TXPLL_TCL\r\n'
		write_reg_word(0x1fa7a094,0x1000f);
		write_reg_word(0x1fa7a070,0x4000b03);
		write_reg_word(0x1fa7a074,0x3000001);
		write_reg_word(0x1fa7a06c,0x1000003);
		//b'TXPLL_EN\r\n'
		write_reg_word(0x1fa7b854,0x1010000);
		//b'TXPLL_Out\r\n'
		write_reg_word(0x1fa7b854,0x1010101);
		//b'TX BringUp 2\r\n'
		//b'tx_rate_ctrl 2\r\n'
		write_reg_word(0x1fa7b580,0x1);
		//b'TX_CONFIG\r\n'
		write_reg_word(0x1fa7a0c4,0x1010401);
		write_reg_word(0x1fa7b874,0x1010000);
		write_reg_word(0x1fa7b77c,0x1020101);
		write_reg_word(0x1fa7b784,0x101);
		//b'TX_FIR_Load_Para swing 2, len 4, cn1 0, c0b c, c1 0, en 1\r\n'
		//b'TX_FIR\r\n'
		write_reg_word(0x1fa7b778,0x100010c);
		write_reg_word(0x1fa7b780,0x100);
		//b'TX_RSTB\r\n'
		write_reg_word(0x1fa7b260,0x101);
		//b'RX BringUp 2\r\n'
		//b'RX_INIT\r\n'
		//b'rx_rate_ctrl\r\n'
		write_reg_word(0x1fa7b374,0x0);
		//b'RX_Path_Init\r\n'
		write_reg_word(0x1fa7b184,0x40003ff);
		write_reg_word(0x1fa7a148,0x1010101);
		write_reg_word(0x1fa7a144,0x1000000);
		write_reg_word(0x1fa7a11c,0x2000401);
		write_reg_word(0x1fa7b004,0xc100a01);
		write_reg_word(0x1fa7a13c,0x20000);
		write_reg_word(0x1fa7a120,0x3ff08);
		write_reg_word(0x1fa7b320,0x10101);
		write_reg_word(0x1fa7b48c,0x1000202);
		write_reg_word(0x1fa7a0dc,0x0);
		write_reg_word(0x1fa7b80c,0x1000000);
		write_reg_word(0x1fa7b814,0x1010000);
		write_reg_word(0x1fa7a10c,0x70604);
		//b'RX_FE,force 0, Gain 1, Peaking 1\r\n'
		write_reg_word(0x1fa7b88c,0x0);
		write_reg_word(0x1fa7b768,0x0);
		//b'RX_FE_VOS\r\n'
		write_reg_word(0x1fa7b79c,0x10100);
		//b'RX_IMP 1\r\n'
		write_reg_word(0x1fa7a114,0x1040000);
		//b'RX_FLL_PR_FMeter\r\n'
		write_reg_word(0x1fa7b390,0x100001);
		write_reg_word(0x1fa7b394,0xffff0000);
		write_reg_word(0x1fa7b39c,0x3107);
		//b'RX_REV\r\n'
		write_reg_word(0x1fa7a0d4,0xc8c31030);
		//b'RX_Rdy_TimeOut\r\n'
		write_reg_word(0x1fa7b100,0xa0005);
		//b'RX_CalBoundry_Init\r\n'
		write_reg_word(0x1fa7b08c,0x101);
		write_reg_word(0x1fa7b104,0x2);
		write_reg_word(0x1fa7b090,0x320002);
		write_reg_word(0x1fa7b09c,0x320002);
		write_reg_word(0x1fa7b094,0x320002);
		write_reg_word(0x1fa7b098,0x320002);
		//b'RX_BySerdes\r\n'
		//b'RX_OSR\r\n'
		write_reg_word(0x1fa7b76c,0x1030000);
		write_reg_word(0x1fa7a0dc,0x100);
		//b'CDR_LPF_RATIO\r\n'
		write_reg_word(0x1fa7a0e8,0x2000003);
		//b'CDR_PR\r\n'
		write_reg_word(0x1fa7a0f8,0x4010808);
		write_reg_word(0x1fa7a0fc,0x80606);
		//b'RX_EYE_Mon\r\n'
		write_reg_word(0x1fa7b120,0x103);
		write_reg_word(0x1fa7b088,0x1);
		//b'RX_SYS_En\r\n'
		write_reg_word(0x1fa7b38c,0x1);
		write_reg_word(0x1fa7b000,0x1000000);
		//b'RX_FLL_PR_FMeter\r\n'
		write_reg_word(0x1fa7b33c,0x1010100);
		write_reg_word(0x1fa7b330,0x1);
		//b'RX_CMLEQ_EN\r\n'
		write_reg_word(0x1fa7a118,0x1010100);
		//b'RX_CDR_PR\r\n'
		write_reg_word(0x1fa7a10c,0x70604);
		//b'RX_CDR_xxx_Pwdb\r\n'
		write_reg_word(0x1fa7b824,0x1000100);
		write_reg_word(0x1fa7b81c,0x100);
		write_reg_word(0x1fa7b894,0x100);
		write_reg_word(0x1fa7b84c,0x1000000);
		write_reg_word(0x1fa7b34c,0x0);
		//b'RX_SigDet\r\n'
		write_reg_word(0x1fa7a114,0x1020200);
		write_reg_word(0x1fa7a110,0x3000200);
		//b'RX_SigDet_Pwdb\r\n'
		write_reg_word(0x1fa7b350,0x0);
		//b'PXP_RX_PHYCK\r\n'
		write_reg_word(0x1fa7a0d8,0x10129);
		write_reg_word(0x1fa7a0cc,0x1000000);
		//b'RX_CDR_xxx_Pwdb\r\n'
		write_reg_word(0x1fa7b824,0x1010101);
		write_reg_word(0x1fa7b81c,0x101);
		write_reg_word(0x1fa7b894,0x101);
		write_reg_word(0x1fa7b84c,0x1010000);
		write_reg_word(0x1fa7b34c,0x1010101);
		//b'RX_SigDet_Pwdb\r\n'
		write_reg_word(0x1fa7b350,0x1);
		//b'RX_PR_CAL_SEQ 2\r\n'
		//b'RX_CDR_LFP_L2D mode 1, sel 0\r\n'
		write_reg_word(0x1fa7b818,0x100);
		//b'RX_PRCal 2\r\n'
		write_reg_word(0x1fa7b460,0x20);
		write_reg_word(0x1fa7b150,0xa43aa372);
		write_reg_word(0x1fa7b14c,0x7fff7fff);
		write_reg_word(0x1fa7b158,0x3307);
		write_reg_word(0x1fa7b154,0xa43aa372);
		write_reg_word(0x1fa7a0f4,0x1000000);
		write_reg_word(0x1fa7b820,0x1010100);
		write_reg_word(0x1fa7b794,0x1010000);
		write_reg_word(0x1fa7b824,0x1010101);
		write_reg_word(0x1fa7b824,0x1000101);
		write_reg_word(0x1fa7b824,0x1010101);

		
		for (PrCal_Serach = 1; PrCal_Serach < 8 ; PrCal_Serach++)
  		{

		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac = PrCal_Serach<<8;		
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,(u32)(rg_force_da_pxp_cdr_pr_idac.dat.value));

	 	write_reg_word(0x1fa7b158,0x3300);

		write_reg_word(0x1fa7b158,0x3303);

	 	udelay(5000);

		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);		
		RO_FL_Out = RO_RX_FREQDET.hal.ro_fl_out;
		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		RO_pr_idac = rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac;
	 	printf("pr_idac = 0x%x ,RO_FL_Out = 0x%x\n" ,RO_pr_idac , RO_FL_Out);

		//abs
		if (RO_FL_Out > FL_Out_target) RO_FL_Out_diff = RO_FL_Out - FL_Out_target;
		else if (RO_FL_Out < FL_Out_target) RO_FL_Out_diff = FL_Out_target - RO_FL_Out;
		else RO_FL_Out_diff = 0;
		
	 	if(RO_FL_Out > FL_Out_target)
	 	{        
			RO_FL_Out_diff_tmp = RO_FL_Out_diff;
			cdr_pr_idac_tmp = (PrCal_Serach<<8);
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);		 
	 	}
  		}

  		for (turn_pr_idac_bit_position = 7; turn_pr_idac_bit_position > -1 ; turn_pr_idac_bit_position--)
  		{
		pr_idac = cdr_pr_idac_tmp |(0x1<<turn_pr_idac_bit_position); 
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac = pr_idac;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);

	 	write_reg_word(0x1fa7b158,0x3300);

		write_reg_word(0x1fa7b158,0x3303);

		udelay(5000);	  
	    
		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);	  
		RO_FL_Out = RO_RX_FREQDET.hal.ro_fl_out;

		printf("pr_idac = 0x%x ,RO_FL_Out = 0x%x\n",pr_idac,RO_FL_Out);

		if(RO_FL_Out < FL_Out_target)
		{
	        pr_idac &= ~(0x1<<turn_pr_idac_bit_position);
			cdr_pr_idac_tmp = pr_idac;
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);
		}
		else
		{
			cdr_pr_idac_tmp = pr_idac;
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);
		}   
	  
  		}

		rg_force_da_pxp_cdr_pr_idac.dat.value = cdr_pr_idac_tmp;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);
		printf("sel_cdr_pr_idac = 0x%x\n",cdr_pr_idac_tmp);

		write_reg_word(0x1fa7b158,0x3300);
		write_reg_word(0x1fa7b158,0x3303);

		udelay(5000);
		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);	  
  		RO_state_freqdet = RO_RX_FREQDET.hal.ro_fbck_lock; 

		printf("RO_state_freqdet = 0x%x\n",RO_state_freqdet);

		//Load_Band
		RG_PXP_CDR_PR_INJ_MODE.dat.value = RG_R_PL(0x1fa7a000,PXP_BASE_OFFSET,(u32) _RG_PXP_CDR_PR_INJ_MODE);
		rg_force_da_pxp_cdr_pr_lpf_c_en.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_lpf_c_en);
		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		SS_RX_FLL_b.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_b);
		SS_RX_FLL_1.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_1);
		rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb);
		
		RG_PXP_CDR_PR_INJ_MODE.hal.rg_pxp_cdr_pr_inj_force_off = 0;
		RG_W_PL(0x1fa7a000,PXP_BASE_OFFSET,(u32) _RG_PXP_CDR_PR_INJ_MODE,RG_PXP_CDR_PR_INJ_MODE.dat.value);
		
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_da_pxp_cdr_pr_lpf_c_en = 0;	
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_sel_da_pxp_cdr_pr_lpf_c_en = 0;
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_da_pxp_cdr_pr_lpf_r_en = 1;
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_sel_da_pxp_cdr_pr_lpf_r_en = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_lpf_c_en,rg_force_da_pxp_cdr_pr_lpf_c_en.dat.value);
			
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_sel_da_pxp_cdr_pr_idac = 0;	
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);
		
		SS_RX_FLL_b.hal.rg_load_en = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_b,SS_RX_FLL_b.dat.value);
		SS_RX_FLL_1.hal.rg_ipath_idac = cdr_pr_idac_tmp;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_1,SS_RX_FLL_1.dat.value);
			
		
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_da_pxp_cdr_pr_pwdb = 0;	
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_da_pxp_cdr_pr_pwdb = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);	
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_sel_da_pxp_cdr_pr_pwdb = 0;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);
		
		SW_RST_SET.hal.rg_sw_ref_rst_n = RX_PRCal_REF_RESETB_HI_EN;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SW_RST_SET,SW_RST_SET.dat.value);
		//return RO_state_freqdet;
		//RX_PRCal_end--------------------------------
		//b'RX_CDR_LPF_RSTB mode 1, sel0\r\n'
		write_reg_word(0x1fa7b818,0x1000100);
		udelay(700);
		//b'RX_CDR_LPF_RSTB mode 1, sel1\r\n'
		write_reg_word(0x1fa7b818,0x1010100);
		udelay(100);
		//b'RX_CDR_LFP_L2D mode 1, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x1010101);
		
		//b'RX_CDR_LPF_RSTB mode 0, sel1\r\n'
		write_reg_word(0x1fa7b818,0x10101);
		//b'RX_CDR_LFP_L2D mode 0, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x10001);
		//---------------------------------------------
		//b'RSTB sel8, val 0\r\n'
		write_reg_word(0x1fa7b460,0x20);
		//b'RSTB sel8, val 1\r\n'
		write_reg_word(0x1fa7b460,0xfff);
		//b'RX_CDR_RST\r\n'
        //b'RX_CDR_LFP_L2D mode 1, sel 0\r\n'
        write_reg_word(0x1fa7b818,0x10100);
        //b'RX_CDR_LPF_RSTB mode 1, sel0\r\n'
        write_reg_word(0x1fa7b818,0x1000100);
		udelay(700);
        //b'RX_CDR_LPF_RSTB mode 1, sel1\r\n'
        write_reg_word(0x1fa7b818,0x1010100);
		udelay(100);
        //b'RX_CDR_LFP_L2D mode 1, sel 1\r\n'
        write_reg_word(0x1fa7b818,0x1010101);
        //b'RX_CDR_LPF_RSTB mode 0, sel1\r\n'
        write_reg_word(0x1fa7b818,0x10101);
        //b'RX_CDR_LFP_L2D mode 0, sel 1\r\n'
        write_reg_word(0x1fa7b818,0x10001);
		//b'xSGMII_Solution : 0\r\n'
		//b'Solution0\r\n'
		write_reg_word(0x1fa70034,0x31120029);
		write_reg_word(0x1fa70a24,0x1);
		//b'xsgmii_init\r\n'
		write_reg_word(0x1fa70a00,0x4c9cc000);
		write_reg_word(0x1fa74018,0x24);
		//b'sgmii_init\r\n'
		write_reg_word(0x1fa76018,0x7070707);
		write_reg_word(0x1fa76020,0xff);
		//b'SGMII_Interrupt_init\r\n'
		write_reg_word(0x1fa74014,0x1);
		//b'xsgmii 2,int enable 1\r\n'
		//b'\r\n'
		//b' request_irq() (irq number: 138) OK \r\n'
		write_reg_word(0x1fa70a20,0x1);
		write_reg_word(0x1fa7414c,0x1);
		//b'rg_INTERRUPT_EN_0 1\r\n'
		//b'xsgmii: sync_int exit 1\r\n'
		//b'force mode\r\n'
		//b'xSGMII_AN_API 2, enable : 0\r\n'
		//b'sgmii_an 0\r\n'
		write_reg_word(0x1fa70000,0x140);
		//b'sgmii_rate_api rate 0\r\n'
		//b'SGMII_1G\r\n'
		write_reg_word(0x1fa76000,0xc000c11);
		write_reg_word(0x1fa74018,0x24);
		//b'SGMII_1G exit\r\n'
		printf("SGMII_1G exit\n");
		//b'xSGMII_Solution : 1\r\n'

	}else if((serdes_intf[0][len_eth-2] == SERDES_ETHERLAN) && (serdes_intf[0][len_eth-1] == SERDES_USXGMII_MODE)){
		printf("ETH VER = ETH.2.2.1-R\n");
		uint FL_Out_target = 0x9EDF ;
		if((len_eth>=3) &&(serdes_intf[0][len_eth-3] == 2))   //phy B
		{
			miiStationWrite45(0,4,0xc441,8);
		}
      	write_reg_word(0x1fb00094,0xe0002820);//add
    	//b''
		//b'echo "1 0 1 0 0">/proc/xsgmii\r\n'
		//b'op = 1,xsgmii 0,mod 1,an 0,rate 0,buf 31,num 5,count a\r\n'
		//b'xsgmii_api: serdes 1,xsgmii 0,mod 1,rate 0,an 0\r\n'
		//b'xsgmii_chg 1\r\n'
		write_reg_word(0x1fa7a000,0x10040001);
		//b'JCPLL BringUp 0\r\n'
		//b'JCPLL_LDO\r\n'
		write_reg_word(0x1fa7a048,0x1020ff);
		//b'JCPLL_RSTB\r\n'
		write_reg_word(0x1fa7a01c,0x3000004);
		write_reg_word(0x1fa7a01c,0x3000104);
		//b'JCPLL_EN\r\n'
		write_reg_word(0x1fa7b828,0x1000000);
		//b'JCPLL_SDM\r\n'
		write_reg_word(0x1fa7a01c,0x104);
		write_reg_word(0x1fa7a020,0x30000);
		write_reg_word(0x1fa7a024,0x5010100);
		//b'JCPLL_SSC\r\n'
		write_reg_word(0x1fa7a038,0x0);
		write_reg_word(0x1fa7a034,0x0);
		write_reg_word(0x1fa7a030,0x3018);
		//b'JCPLL_LPF\r\n'
		write_reg_word(0x1fa7a004,0x180000);
		write_reg_word(0x1fa7a008,0x101f0a);
		write_reg_word(0x1fa7a00c,0x2ff0000);
		//b'JCPLL_VCO\r\n'
		write_reg_word(0x1fa7a02c,0x4010100);
		write_reg_word(0x1fa7a030,0x301d);
		//b'JCPLL_PCW\r\n'
		write_reg_word(0x1fa7b800,0x25800000);
		write_reg_word(0x1fa7b79c,0x10000);
		//b'JCPLL_DIV\r\n'
		write_reg_word(0x1fa7a014,0x10000);
		write_reg_word(0x1fa7a02c,0x4010100);
		//b'JCPLL_KBand\r\n'
		write_reg_word(0x1fa7a010,0x1000300);
		write_reg_word(0x1fa7a00c,0x2e40000);
		//b'JCPLL_TCL\r\n'
		write_reg_word(0x1fa7a048,0xf20ff);
		write_reg_word(0x1fa7a024,0x5010100);
		write_reg_word(0x1fa7a028,0x10400);
		//b'JCPLL_EN\r\n'
		write_reg_word(0x1fa7b828,0x1010000);
		//b'JCPLL_Out\r\n'
		write_reg_word(0x1fa7b828,0x1010101);
		//b'TXPLL BringUp\r\n'
		//b'TXPLL_VCOLDO_Out\r\n'
		write_reg_word(0x1fa7a084,0x101031b);
		//b'TXPLL_RSTB\r\n'
		write_reg_word(0x1fa7a064,0x1040001);
		//b'TXPLL_EN\r\n'
		write_reg_word(0x1fa7b854,0x1000000);
		//b'TXPLL_SDM\r\n'
		write_reg_word(0x1fa7a068,0x0);
		write_reg_word(0x1fa7a06c,0x1010003);
		//b'TXPLL_SSC\r\n'
		write_reg_word(0x1fa7a080,0x0);
		write_reg_word(0x1fa7a07c,0x0);
		write_reg_word(0x1fa7a084,0x1010000);
		//b'TXPLL_LPF\r\n'
		write_reg_word(0x1fa7a050,0x1f05000f);
		write_reg_word(0x1fa7a054,0x180b02);
		//b'TXPLL_VCO\r\n'
		write_reg_word(0x1fa7a074,0x1000001);
		write_reg_word(0x1fa7a078,0x4040701);
		//b'TXPLL_PCW\r\n'
		write_reg_word(0x1fa7b798,0x8400000);
		write_reg_word(0x1fa7b794,0x1000000);
		//b'TXPLL_KBand\r\n'
		write_reg_word(0x1fa7a058,0x30004e4);
		write_reg_word(0x1fa7a05c,0x101);
		write_reg_word(0x1fa7a054,0x180b02);
		//b'TXPLL_DIV\r\n'
		write_reg_word(0x1fa7a05c,0x1);
		write_reg_word(0x1fa7a074,0x1000001);
		//b'TXPLL_TCL\r\n'
		write_reg_word(0x1fa7a094,0x1000f);
		write_reg_word(0x1fa7a070,0x4000b03);
		write_reg_word(0x1fa7a074,0x1000001);
		write_reg_word(0x1fa7a06c,0x1010003);
		//b'TXPLL_EN\r\n'
		write_reg_word(0x1fa7b854,0x1010000);
		//b'TXPLL_Out\r\n'
		write_reg_word(0x1fa7b854,0x1010101);
		//b'TX BringUp 0\r\n'
		//b'tx_rate_ctrl 0\r\n'
		write_reg_word(0x1fa7b580,0x2);
		//b'TX_CONFIG\r\n'
		write_reg_word(0x1fa7a0c4,0x1010401);
		write_reg_word(0x1fa7b874,0x1010000);
		write_reg_word(0x1fa7b77c,0x1050101);
		write_reg_word(0x1fa7b784,0x102);
		//b'TX_FIR_Load_Para swing 2, len 4, cn1 1, c0b 1, c1 b, en 1\r\n'
		//b'TX_FIR\r\n'
		write_reg_word(0x1fa7b778,0x1010101);
		write_reg_word(0x1fa7b780,0x10b);
		//b'TX_RSTB\r\n'
		write_reg_word(0x1fa7b260,0x101);
		//b'RX BringUp 0\r\n'
		//b'RX_INIT\r\n'
		//b'rx_rate_ctrl\r\n'
		write_reg_word(0x1fa7b374,0x2);
		//b'RX_Path_Init\r\n'
		write_reg_word(0x1fa7b184,0x40003ff);
		write_reg_word(0x1fa7a148,0x1010101);
		write_reg_word(0x1fa7a144,0x1000000);
		write_reg_word(0x1fa7a11c,0x2000401);
		write_reg_word(0x1fa7b004,0xc100a01);
		write_reg_word(0x1fa7a13c,0x20000);
		write_reg_word(0x1fa7a120,0x3ff08);
		write_reg_word(0x1fa7b320,0x10101);
		write_reg_word(0x1fa7b48c,0x1000202);
		write_reg_word(0x1fa7a0dc,0x0);
		write_reg_word(0x1fa7b80c,0x1000000);
		write_reg_word(0x1fa7b814,0x1010000);
		write_reg_word(0x1fa7a10c,0x70604);
		//b'RX_FE,force 0, Gain 1, Peaking 2\r\n'
		write_reg_word(0x1fa7b88c,0x0);
		write_reg_word(0x1fa7b768,0x0);
		//b'RX_FE_VOS\r\n'
		//write_reg_word(0x1fa7b79c,0x10000);
		//b'RX_IMP 1\r\n'
		write_reg_word(0x1fa7a114,0x1040000);
		//b'RX_FLL_PR_FMeter\r\n'
		write_reg_word(0x1fa7b390,0x100001);
		write_reg_word(0x1fa7b394,0xffff0000);
		write_reg_word(0x1fa7b39c,0x3107);
		//b'RX_REV\r\n'
		write_reg_word(0x1fa7a0d4,0xc8c31030);
		//b'RX_Rdy_TimeOut\r\n'
		write_reg_word(0x1fa7b100,0xa0005);
		//b'RX_CalBoundry_Init\r\n'
		write_reg_word(0x1fa7b08c,0x101);
		write_reg_word(0x1fa7b104,0x2);
		write_reg_word(0x1fa7b090,0x320002);
		write_reg_word(0x1fa7b09c,0x320002);
		write_reg_word(0x1fa7b094,0x320002);
		write_reg_word(0x1fa7b098,0x320002);
		//b'RX_BySerdes\r\n'
		//b'RX_OSR\r\n'
		write_reg_word(0x1fa7b76c,0x1000000);
		write_reg_word(0x1fa7a0dc,0x0);
		//b'CDR_LPF_RATIO\r\n'
		write_reg_word(0x1fa7a0e8,0x2000000);
		//b'CDR_PR\r\n'
		write_reg_word(0x1fa7a0f8,0x4010808);
		write_reg_word(0x1fa7a0fc,0x80606);
		//b'RX_EYE_Mon\r\n'
		write_reg_word(0x1fa7b120,0x103);
		write_reg_word(0x1fa7b088,0x1);
		//b'RX_SYS_En\r\n'
		write_reg_word(0x1fa7b38c,0x1);
		write_reg_word(0x1fa7b000,0x1000000);
		//b'RX_FLL_PR_FMeter\r\n'
		write_reg_word(0x1fa7b33c,0x1010100);
		write_reg_word(0x1fa7b330,0x1);
		//b'RX_CMLEQ_EN\r\n'
		write_reg_word(0x1fa7a118,0x1010100);
		//b'RX_CDR_PR\r\n'
		write_reg_word(0x1fa7a10c,0x70604);
		//b'RX_CDR_xxx_Pwdb\r\n'
		write_reg_word(0x1fa7b824,0x1000100);
		write_reg_word(0x1fa7b81c,0x100);
		write_reg_word(0x1fa7b894,0x100);
		write_reg_word(0x1fa7b84c,0x1000000);
		write_reg_word(0x1fa7b34c,0x0);
		//b'RX_SigDet\r\n'
		write_reg_word(0x1fa7a114,0x1020200);
		write_reg_word(0x1fa7a110,0x3000200);
		//b'RX_SigDet_Pwdb\r\n'
		write_reg_word(0x1fa7b350,0x0);
		//b'PXP_RX_PHYCK\r\n'
		write_reg_word(0x1fa7a0d8,0x10242);
		write_reg_word(0x1fa7a0cc,0x1000000);
		//b'RX_CDR_xxx_Pwdb\r\n'
		write_reg_word(0x1fa7b824,0x1010101);
		write_reg_word(0x1fa7b81c,0x101);
		write_reg_word(0x1fa7b894,0x101);
		write_reg_word(0x1fa7b84c,0x1010000);
		write_reg_word(0x1fa7b34c,0x1010101);
		//b'RX_SigDet_Pwdb\r\n'
		write_reg_word(0x1fa7b350,0x1);
		//b'RX_PR_CAL_SEQ 0\r\n'
		//b'RX_CDR_LFP_L2D mode 1, sel 0\r\n'
		write_reg_word(0x1fa7b818,0x100);
		//b'RX_PRCal 1\r\n'
        write_reg_word(0x1fa7b460,0x20);
        write_reg_word(0x1fa7b150,0x9f439e7b);
        write_reg_word(0x1fa7b14c,0x7fff7fff);
        write_reg_word(0x1fa7b158,0x3307);
        write_reg_word(0x1fa7b154,0x9f439e7b);
        write_reg_word(0x1fa7a0f4,0x1000000);
        write_reg_word(0x1fa7b820,0x1010100);
        write_reg_word(0x1fa7b794,0x1010000);
        write_reg_word(0x1fa7b824,0x1010101);
        write_reg_word(0x1fa7b824,0x1000101);
        write_reg_word(0x1fa7b824,0x1010101);
		
		for (PrCal_Serach = 1; PrCal_Serach < 8 ; PrCal_Serach++)
  		{

		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac = PrCal_Serach<<8;		
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,(u32)(rg_force_da_pxp_cdr_pr_idac.dat.value));

	 	write_reg_word(0x1fa7b158,0x3300);

		write_reg_word(0x1fa7b158,0x3303);

	 	udelay(5000);

		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);		
		RO_FL_Out = RO_RX_FREQDET.hal.ro_fl_out;
		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		RO_pr_idac = rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac;
	 	printf("pr_idac = 0x%x ,RO_FL_Out = 0x%x\n" ,RO_pr_idac , RO_FL_Out);

		//abs
		if (RO_FL_Out > FL_Out_target) RO_FL_Out_diff = RO_FL_Out - FL_Out_target;
		else if (RO_FL_Out < FL_Out_target) RO_FL_Out_diff = FL_Out_target - RO_FL_Out;
		else RO_FL_Out_diff = 0;
		
	 	if(RO_FL_Out > FL_Out_target)
	 	{        
			RO_FL_Out_diff_tmp = RO_FL_Out_diff;
			cdr_pr_idac_tmp = (PrCal_Serach<<8);
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);		 
	 	}
  		}

  		for (turn_pr_idac_bit_position = 7; turn_pr_idac_bit_position > -1 ; turn_pr_idac_bit_position--)
  		{
		pr_idac = cdr_pr_idac_tmp |(0x1<<turn_pr_idac_bit_position); 
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_da_pxp_cdr_pr_idac = pr_idac;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);

	 	write_reg_word(0x1fa7b158,0x3300);

		write_reg_word(0x1fa7b158,0x3303);

		udelay(5000);	  
	    
		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);	  
		RO_FL_Out = RO_RX_FREQDET.hal.ro_fl_out;

		printf("pr_idac = 0x%x ,RO_FL_Out = 0x%x\n",pr_idac,RO_FL_Out);

		if(RO_FL_Out < FL_Out_target)
		{
	        pr_idac &= ~(0x1<<turn_pr_idac_bit_position);
			cdr_pr_idac_tmp = pr_idac;
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);
		}
		else
		{
			cdr_pr_idac_tmp = pr_idac;
			printf("cdr_pr_idac_tmp = 0x%x\n",cdr_pr_idac_tmp);
		}   
	  
  		}

		rg_force_da_pxp_cdr_pr_idac.dat.value = cdr_pr_idac_tmp;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);
		printf("sel_cdr_pr_idac = 0x%x\n",cdr_pr_idac_tmp);

		write_reg_word(0x1fa7b158,0x3300);
		write_reg_word(0x1fa7b158,0x3303);

		udelay(5000);
		RO_RX_FREQDET.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _RO_RX_FREQDET);	  
  		RO_state_freqdet = RO_RX_FREQDET.hal.ro_fbck_lock; 

		printf("RO_state_freqdet = 0x%x\n",RO_state_freqdet);

		//Load_Band
		RG_PXP_CDR_PR_INJ_MODE.dat.value = RG_R_PL(0x1fa7a000,PXP_BASE_OFFSET,(u32) _RG_PXP_CDR_PR_INJ_MODE);
		rg_force_da_pxp_cdr_pr_lpf_c_en.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_lpf_c_en);
		rg_force_da_pxp_cdr_pr_idac.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac);
		SS_RX_FLL_b.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_b);
		SS_RX_FLL_1.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_1);
		rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value = RG_R_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb);
		
		RG_PXP_CDR_PR_INJ_MODE.hal.rg_pxp_cdr_pr_inj_force_off = 0;
		RG_W_PL(0x1fa7a000,PXP_BASE_OFFSET,(u32) _RG_PXP_CDR_PR_INJ_MODE,RG_PXP_CDR_PR_INJ_MODE.dat.value);
		
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_da_pxp_cdr_pr_lpf_c_en = 0;	
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_sel_da_pxp_cdr_pr_lpf_c_en = 0;
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_da_pxp_cdr_pr_lpf_r_en = 1;
		rg_force_da_pxp_cdr_pr_lpf_c_en.hal.rg_force_sel_da_pxp_cdr_pr_lpf_r_en = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_lpf_c_en,rg_force_da_pxp_cdr_pr_lpf_c_en.dat.value);
			
		rg_force_da_pxp_cdr_pr_idac.hal.rg_force_sel_da_pxp_cdr_pr_idac = 0;	
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_idac,rg_force_da_pxp_cdr_pr_idac.dat.value);
		
		SS_RX_FLL_b.hal.rg_load_en = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_b,SS_RX_FLL_b.dat.value);
		SS_RX_FLL_1.hal.rg_ipath_idac = cdr_pr_idac_tmp;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SS_RX_FLL_1,SS_RX_FLL_1.dat.value);
			
		
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_da_pxp_cdr_pr_pwdb = 0;	
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_da_pxp_cdr_pr_pwdb = 1;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);	
		rg_force_da_pxp_cdr_pr_pieye_pwdb.hal.rg_force_sel_da_pxp_cdr_pr_pwdb = 0;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _rg_force_da_pxp_cdr_pr_pieye_pwdb,rg_force_da_pxp_cdr_pr_pieye_pwdb.dat.value);
		
		SW_RST_SET.hal.rg_sw_ref_rst_n = RX_PRCal_REF_RESETB_HI_EN;
		RG_W_PL(0x1fa7b000,PMA_BASE_OFFSET,(u32) _SW_RST_SET,SW_RST_SET.dat.value);
		//return RO_state_freqdet;
		//RX_PRCal_end--------------------------------
		//b'RX_CDR_LPF_RSTB mode 1, sel0\r\n'
		write_reg_word(0x1fa7b818,0x1000100);
		udelay(700);
		//b'RX_CDR_LPF_RSTB mode 1, sel1\r\n'
		write_reg_word(0x1fa7b818,0x1010100);
		udelay(100);
		//b'RX_CDR_LFP_L2D mode 1, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x1010101);
		
		//b'RX_CDR_LPF_RSTB mode 0, sel1\r\n'
		write_reg_word(0x1fa7b818,0x10101);
		//b'RX_CDR_LFP_L2D mode 0, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x10001);
		
		//b'RSTB sel8, val 0\r\n'
		write_reg_word(0x1fa7b460,0x20);
		//b'RSTB sel8, val 1\r\n'
		write_reg_word(0x1fa7b460,0xfff);
		//b'RX_CDR_RST\r\n'
		//b'RX_CDR_LFP_L2D mode 1, sel 0\r\n'
		write_reg_word(0x1fa7b818,0x10100);
		//b'RX_CDR_LPF_RSTB mode 1, sel0\r\n'
		write_reg_word(0x1fa7b818,0x1000100);
		udelay(700);
		//b'RX_CDR_LPF_RSTB mode 1, sel1\r\n'
		write_reg_word(0x1fa7b818,0x1010100);
		udelay(100);
		//b'RX_CDR_LFP_L2D mode 1, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x1010101);
		//b'RX_CDR_LPF_RSTB mode 0, sel1\r\n'
		write_reg_word(0x1fa7b818,0x10101);
		//b'RX_CDR_LFP_L2D mode 0, sel 1\r\n'
		write_reg_word(0x1fa7b818,0x10001);
		//b'xSGMII_Solution : 0\r\n'
		//b'xsgmii_init\r\n'
		//b'usxgmii_init 1\r\n'
		write_reg_word(0x1fa74100,0x10010001);
        //b'PCS E0/E1 solution\r\n'
		write_reg_word(0x1fa75b2c,0x0);
		//b'usxgmii_pcs_int en 0\r\n'
		write_reg_word(0x1fa75bc0,0x0);
		write_reg_word(0x1fa75bc4,0x0);
		write_reg_word(0x1fa75bc8,0x0);
		write_reg_word(0x1fa75bcc,0x0);
		write_reg_word(0x1fa75be0,0x0);
		write_reg_word(0x1fa75bd8,0x0);
		write_reg_word(0x1fa75bdc,0x0);
		write_reg_word(0x1fa75be4,0x0);
		write_reg_word(0x1fa75bc8,0x0);
		write_reg_word(0x1fa75bcc,0x0);
		write_reg_word(0x1fa75be0,0x0);
		//b'xsgmii 0 int enable 0\r\n'
		//b'AN mode\r\n'
		printf("AN mode\n");
		//b'xSGMII_AN_API 0, enable : 1\r\n'
		//b'usxgmii_an\r\n'
		write_reg_word(0x1fa75bf8,0x6330001);
		//b'_rg_usxgmii_an_control_0 6330001\r\n'
		//b'_rg_usxgmii_an_control_1 0\r\n'
		//b'xSGMII_AN_AutoSetting\r\n'
		write_reg_word(0x1fa76000,0xc11);
		
        //set mac reg init
      	write_reg_word(0x1fa09000,0x71082800);
      	printf("set mac reg init\n");
#if 1 //Interrupt for USXGMII		
		//enable USXGMII interrupt(pcs)
		usxgmii_pcs_int_init(1);
		//ARM interrupt settings
		gic_dic_set_icenable(50);
		gic_dic_set_enable(50);
		irq_register(50,usxgmii_isr,1);
#endif	
		printf("USXGMII_10G exit\n");
		return ;
	}
	return ;
#endif
	return ;
}



