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
//#include <linux/init.h>
//#include <linux/kernel_stat.h>
#include <linux/types.h>
//#include <linux/interrupt.h>
//#include <linux/random.h>

#include <asm/bitops.h>
//#include <asm/bootinfo.h>
//#include <asm/irq.h>
//#include <asm/mipsregs.h>
#include <asm/system.h>
//#include <linux/circ_buf.h>
#include <asm/tc3162.h>
#include <asm/io.h>
#include <malloc.h>
#include <miiphy.h>
#include <phy.h>

#ifdef __BIG_ENDIAN
#define FE_BYTE_SWAP
#endif
#define CONFIG_RX_2B_OFFSET 1

#include "eth.h"
#include "ecnt_eth.h"
#include "multiupgrade.h"
#include "net.h"
#define isMT752XG	(((read_reg_word(0x1fb000f8)&0x3)==0x3) && isMT751020)

static unsigned char mac_addr[6] = {0x00, 0xaa, 0xbb, 0x01, 0x23, 0x45};

//void mac_interrupt(int irq, void *dev_id, struct pt_regs *regs);
extern void flush_dcache_range(unsigned long start, unsigned long end);
extern void ecnt_ImageUpgrade(int fw_type);

//struct irqaction mac_irq = {mac_interrupt, NULL, 0, "mac", NULL, NULL};

static QDMA_Private_T DummygpQdmaPriv;
QDMA_Private_T *gpQdmaPriv = &DummygpQdmaPriv;
static uint DummydscpInfoAddr[DESC_INFO_SIZE];
static macAdapter_t DummyAdapter;
macAdapter_t *mac_p = &DummyAdapter;
uint8 use_ext_switch;

#ifndef CONFIG_SYS_CACHELINE_SIZE
#define CONFIG_SYS_CACHELINE_SIZE	64
#endif	/* CONFIG_SYS_CACHELINE_SIZE */


#define OP_SAVE							(1)
#define OP_RESTORE						(2)
static macMemPool_t enetMemPool __attribute__ ((__aligned__(CONFIG_SYS_CACHELINE_SIZE)));

static int ecnt_eth_initd = 0;


extern void pause(int ms);
extern int eth_phy_init(macAdapter_t *mac_p);
static int   ecnt_eth_init(struct eth_device* dev, bd_t* bis);
static int   ecnt_eth_send(struct eth_device* dev, void *packet, int length);
static int   ecnt_eth_recv(struct eth_device* dev);
static int ecnt_iomux_reg_op(int op);
static int ecnt_force_mdio0_en(int op);

int tftp_start = 0 ;
ulong fw_upgrade_select = 0;

void dump_skb(sk_buff *skb)
{
	unsigned 	char tmp[80];
	unsigned char *p = skb->data;
	unsigned char *t = tmp;
	int i, n = 0;

	//qdma_bm_dump_dscp();
    printf("skb data is below\n");
	if(skb == NULL)
		printf("skb is null");
	if(skb->data == NULL)
		printf("skb data is null");
	for (i = 0; i < skb->len; i++) {
		t += sprintf(t, "%02x ", *p++ & 0xff);
		if ((i & 0x0f) == 0x0f) {
			printf("%04x: %s\n", n, tmp);
			n += 16;
			t = tmp;
		}
	}
	if (i & 0x0f)
		printf("%04x: %s\n", n, tmp);
	printf("skb->len %d\n", skb->len);
}

#ifdef TCSUPPORT_CPU_EN7581
static int ecnt_force_mdio0_en(int op)
{
	uint32_t gpio = 0;
	uint32_t reg = 0;
	static uint32_t iomux_bit;
	static uint32_t REG_IOMUX_1;
	static uint32_t REG_IOMUX_2;
	static uint32_t REG_FORCE_GPIO;
	static uint32_t REG_FLASH_MODE;
	static uint32_t REG_LED_DIS;
	static uint32_t REG_GPIO_CTRL;
	static uint32_t REG_GPIO_DATA;
	static uint32_t REG_GPIO_OE;

	/* check if mdio0 force en is needed*/
	if ((read_reg_word(0x1FB0005C) == 0x1) || (read_reg_word(0x1FA2036C) == 0x0))
	{
		if (op == OP_SAVE)
		{
			/* preserve the regs status */
			REG_IOMUX_1 = read_reg_word(0x1fa20214);
			REG_IOMUX_2 = read_reg_word(0x1fa20218);
			REG_FORCE_GPIO = read_reg_word(0x1fa20228);
			REG_FLASH_MODE = read_reg_word(0x1fbf0234);
			REG_LED_DIS = read_reg_word(0x1fbf021c);
			REG_GPIO_CTRL = read_reg_word(0x1fbf0200);
			REG_GPIO_DATA = read_reg_word(0x1fbf0204);
			REG_GPIO_OE = read_reg_word(0x1fbf0214);

			if ((REG_IOMUX_1 & (0x1 << 14)) == 0)
			{
				reg = REG_GPIO_CTRL | (0x1<<4);
				write_reg_word(0x1fbf0200, reg);

				reg = REG_GPIO_DATA | (0x1<<2);
				write_reg_word(0x1fbf0204, reg);	

				reg = REG_GPIO_OE | (0x1<<2);
				write_reg_word(0x1fbf0214, reg);

				reg = REG_LED_DIS & ~((0x3)<<4);
				write_reg_word(0x1fbf021c, reg);
				
				reg = REG_FLASH_MODE & ~((0x1)<<2);
				write_reg_word(0x1fbf0234, reg);

				if((REG_FORCE_GPIO & ((0x1) << 1) != 0) || (REG_IOMUX_1 & ((0x1) << 13) != 0) || (REG_IOMUX_2 & ((0x1) << 9) != 0))
				{
					reg = REG_FORCE_GPIO | ((0x1) << 2);
				}
				else
				{
					reg = REG_FORCE_GPIO | ((0x3) << 1);
				}
				write_reg_word(0x1fa20228, reg);

				write_reg_word(0x1fa20214, (REG_IOMUX_1 | ((0x1) << 14)) & ~((0x1) << 13));
				write_reg_word(0x1fa20218, (REG_IOMUX_2 & ~((0x1) << 8) | ((0x1) << 9)));
			}

		}
		else if (op == OP_RESTORE)
		{
			/* recover the gpio/iomux status*/
			if ((REG_IOMUX_1 & (0x1 << 14)) == 0)
			{
				write_reg_word(0x1fbf0200, REG_GPIO_CTRL);
				write_reg_word(0x1fbf0204, REG_GPIO_DATA);
				write_reg_word(0x1fbf0214, REG_GPIO_OE);
				write_reg_word(0x1fbf021c, REG_LED_DIS);
				write_reg_word(0x1fbf0234, REG_FLASH_MODE);
				write_reg_word(0x1fa20228, REG_FORCE_GPIO);
				write_reg_word(0x1fa20214, REG_IOMUX_1);
				write_reg_word(0x1fa20218, REG_IOMUX_2);
				__udelay(100);
			}

		}
	}
	else
	{
		;
	}
	return 0;
}
#endif

void miiStationWrite(uint32 enetPhyAddr, uint32 phyReg, uint32 miiData)
{
	uint32 reg;
	uint32 cnt=10000;

	do {
		reg=read_reg_word (GSW_CFG_PIAC);
		cnt--;
	} while ((reg & PHY_ACS_ST) && (cnt != 0));

	reg = PHY_ACS_ST | (MDIO_ST_START << MDIO_ST_SHIFT) | (MDIO_CMD_WRITE<<MDIO_CMD_SHIFT) | 
		(enetPhyAddr << MDIO_PHY_ADDR_SHIFT) | (phyReg << MDIO_REG_ADDR_SHIFT) | 
		(miiData & MDIO_RW_DATA);
	write_reg_word (GSW_CFG_PIAC, reg);

	cnt = 10000;
	do {
		//pause(1);
		reg=read_reg_word (GSW_CFG_PIAC);
		cnt--;
	} while ((reg & PHY_ACS_ST) && (cnt != 0));
}

uint32 miiStationRead(uint32 enetPhyAddr, uint32 phyReg)
{
	uint32 reg;
	uint32 cnt=10000;

#ifdef TCSUPPORT_CPU_EN7581
	ecnt_force_mdio0_en(OP_SAVE);
#endif

	do {
		reg=read_reg_word (GSW_CFG_PIAC);
		cnt--;
	} while ((reg & PHY_ACS_ST) && (cnt != 0));

	reg = PHY_ACS_ST | (MDIO_ST_START << MDIO_ST_SHIFT) | (MDIO_CMD_READ<<MDIO_CMD_SHIFT) | 
		(enetPhyAddr << MDIO_PHY_ADDR_SHIFT) | (phyReg << MDIO_REG_ADDR_SHIFT);
	write_reg_word (GSW_CFG_PIAC, reg);

	cnt = 10000;
	do {
		//pause(1);
		reg=read_reg_word (GSW_CFG_PIAC);
		cnt--;
	} while ((reg & PHY_ACS_ST) && (cnt != 0));
	reg = reg & MDIO_RW_DATA;

#ifdef TCSUPPORT_CPU_EN7581
		ecnt_force_mdio0_en(OP_RESTORE);
#endif
	return reg;
}

uint16_t miiStationRead45(uint32_t phy_addr, uint32_t dev_addr, uint32_t phy_reg)
{
	uint32_t reg;
	uint32_t cnt = 10000;
	uint16_t cl45_value = 0;
	unsigned long flags;
	
	if(isEN7523)
		ecnt_iomux_reg_op(OP_SAVE);

#ifdef TCSUPPORT_CPU_EN7581
	ecnt_force_mdio0_en(OP_SAVE);
#endif
	cnt = 10000;
	do {
		reg = read_reg_word(GSW_CFG_PIAC);
		cnt--;
	} while((reg & PHY_ACS_ST) && (cnt != 0));

	reg = PHY_ACS_ST | (MDIO_CL45_ST_START << MDIO_ST_SHIFT) |
		(MDIO_CL45_CMD_ADDR << MDIO_CMD_SHIFT) |
		(phy_addr << MDIO_PHY_ADDR_SHIFT) |
		(dev_addr << MDIO_REG_ADDR_SHIFT) |
		(phy_reg & MDIO_RW_DATA);
	write_reg_word(GSW_CFG_PIAC, reg);

	cnt = 10000;
	do {
		reg = read_reg_word(GSW_CFG_PIAC);
		cnt--;
	} while((reg & PHY_ACS_ST) && (cnt != 0));

	reg = PHY_ACS_ST | (MDIO_CL45_ST_START << MDIO_ST_SHIFT) |
		(MDIO_CL45_CMD_POSTREAD_INCADDR << MDIO_CMD_SHIFT) |
		(phy_addr << MDIO_PHY_ADDR_SHIFT) |
		(dev_addr << MDIO_REG_ADDR_SHIFT);
	write_reg_word(GSW_CFG_PIAC, reg);

	cnt = 10000;
	do {
		reg = read_reg_word(GSW_CFG_PIAC);
		cnt--;
	} while((reg & PHY_ACS_ST) && (cnt != 0));

	cl45_value = reg;
	if(isEN7523)
		ecnt_iomux_reg_op(OP_RESTORE);
	
#ifdef TCSUPPORT_CPU_EN7581
	ecnt_force_mdio0_en(OP_RESTORE);
#endif
	return cl45_value;
}

int miiStationWrite45(uint32_t phy_addr, uint32_t dev_addr, uint32_t phy_reg, uint32_t phy_data)
{
	uint32_t reg;
	uint32_t cnt = 10000;
	unsigned long flags;

	if(isEN7523)
		ecnt_iomux_reg_op(OP_SAVE);
	cnt = 10000;
	do {
		reg = read_reg_word(GSW_CFG_PIAC);
		cnt--;
	} while((reg & PHY_ACS_ST) && (cnt != 0));

	reg = PHY_ACS_ST | (MDIO_CL45_ST_START << MDIO_ST_SHIFT) |
		(MDIO_CL45_CMD_ADDR << MDIO_CMD_SHIFT) |
		(phy_addr << MDIO_PHY_ADDR_SHIFT) |
		(dev_addr << MDIO_REG_ADDR_SHIFT) |
		(phy_reg & MDIO_RW_DATA);
	write_reg_word(GSW_CFG_PIAC, reg);

	cnt = 10000;
	do {
		reg = read_reg_word(GSW_CFG_PIAC);
		cnt--;
	} while((reg & PHY_ACS_ST) && (cnt != 0));

	reg = PHY_ACS_ST | (MDIO_CL45_ST_START << MDIO_ST_SHIFT) |
		(MDIO_CL45_CMD_WRITE << MDIO_CMD_SHIFT) |
		(phy_addr << MDIO_PHY_ADDR_SHIFT) |
		(dev_addr << MDIO_REG_ADDR_SHIFT) |
		(phy_data & MDIO_RW_DATA);
	write_reg_word(GSW_CFG_PIAC, reg);

	cnt = 10000;
	do {
		reg = read_reg_word(GSW_CFG_PIAC);
		cnt--;
	} while((reg & PHY_ACS_ST) && (cnt != 0));

	if(isEN7523)
		ecnt_iomux_reg_op(OP_RESTORE);
	return 0;
}

uint32 gswPbusRead(uint32 pbus_addr)
{
	uint32 pbus_data;

	uint32 phyaddr;
	uint32 reg;
	uint32 value;

	phyaddr = 31;
	// 1. write high-bit page address
	reg = 31;
	value = (pbus_addr >> 6);
  	miiStationWrite(phyaddr, reg, value);

	// 2. read low DWord
	reg = (pbus_addr>>2) & 0x000f;
	value = miiStationRead(phyaddr, reg);
	pbus_data = value;

	// 3. read high DWord
	reg = 16;
	value = miiStationRead(phyaddr, reg);

	pbus_data = (pbus_data) | (value<<16);

	return pbus_data;
} 

//7520G/7525G will use this func
int gswPbusWrite(uint32 pbus_addr, uint32 pbus_data)
{
	uint32 phyaddr;
	uint32 reg;
	uint32 value;

	phyaddr = 31;

	// 1. write high-bit page address
	reg = 31;
	value = (pbus_addr >> 6);
	miiStationWrite(phyaddr, reg, value);

	// 2. write low DWord
	reg = (pbus_addr>>2) & 0x000f;
	value = pbus_data & 0xffff;
	miiStationWrite(phyaddr, reg, value);

	// 3. write high DWord
	reg = 16;
	value = (pbus_data>>16) & 0xffff;
	miiStationWrite(phyaddr, reg, value);

  	return 0;
} 

//PHY2 read/write
uint32 gswPmiRead(uint32 phy_addr, uint32 phy_reg)
{
	uint32 pbus_addr;
	uint32 pbus_data;
	uint32 phy_data;
	uint32 phy_acs_st;

	pbus_addr = 0x701c;
	// b31	- phy access 1:start&busy, 0:complete&idle
	// b29:25 - mdio phy reg addr
	// b24:20 - mdio phy addr
	// b19:18 - 2'b01: write, 2'b10: read
	// b17:16 - start field, always 2'b01
	// b15:0	- data

	phy_addr = phy_addr & 0x1f;
	phy_reg  = phy_reg & 0x1f;

	// 1. write phy_addr & phy_reg
	pbus_data = 0x80090000; // read
	pbus_data = pbus_data | (phy_addr<<20);
	pbus_data = pbus_data | (phy_reg<<25);

	gswPbusWrite(pbus_addr,pbus_data);

	// 2. check phy_acs_st
	phy_acs_st = 1;
	while (phy_acs_st) {
		pbus_data = gswPbusRead(pbus_addr);
		phy_acs_st = (pbus_data>>31) & 0x1;
	}

	// 3. return data
	phy_data = pbus_data & 0xffff;
	return phy_data;
} 


uint32 gswPmiWrite(uint32 phy_addr, uint32 phy_reg, uint32 phy_data)
{
	uint32 pbus_addr;
	uint32 pbus_data;
//	uint32 phy_acs_st;

	pbus_addr = 0x701c;
	// b31    - phy access 1:start&busy, 0:complete&idle
	// b29:25 - mdio phy reg addr
	// b24:20 - mdio phy addr
	// b19:18 - 2'b01: write, 2'b10: read
	// b17:16 - start field, always 2'b01
	// b15:0  - data

	phy_addr = phy_addr & 0x1f;
	phy_reg  = phy_reg & 0x1f;
	phy_data = phy_data & 0xffff;

	// 1. write phy_addr & phy_reg & phy_data
	pbus_data = 0x80050000; // write
	pbus_data = pbus_data | (phy_addr<<20);
	pbus_data = pbus_data | (phy_reg<<25);
	pbus_data = pbus_data | (phy_data);

	gswPbusWrite(pbus_addr,pbus_data);

	return 0;
}

void setPortIsolate(void)
{
	int i;

	for(i = 1; i < 5; i++)
		write_reg_word(GSW_PCR(i), 0x400000);

	return;
}

void macResetSwMAC(void)
{
	uint32 reg;
#if defined(TCSUPPORT_CPU_EN7580)
	//SPI_NAND_FLASH_RTN_T status;
#endif

	/* reset ethernet phy, ethernet switch, frame engine */
	reg = read_reg_word(CR_RSTCTRL2);
	reg |= (QDMA1_RST | ESW_RST | FE_RST);
	write_reg_word(CR_RSTCTRL2, reg);

	mdelay(1);

	/* de-assert reset ethernet phy, ethernet switch, frame engine */
	reg = read_reg_word(CR_RSTCTRL2);
	reg &= ~(QDMA1_RST | ESW_RST | FE_RST);
	write_reg_word(CR_RSTCTRL2, reg);

	/* add delay time to prevent switch reg I/O hang */
    mdelay(1);

	/* EN7580: read flash info to register, for prom init */
#if defined(TCSUPPORT_CPU_EN7580)
	//VPint(QDMA_WAN_DBG_MEM_XS_DATA_HI) = READ_FLASH_BYTE(FLASH_QDMA_INIT, &status);
#endif

}

void resetSwMAC3262(void)
{
	unsigned iomux = 0, iomux_ori = 0;
	write_reg_word(GSW_PMCR(6), read_reg_word(GSW_PMCR(6)) & ~(FORCE_LNK_PN));	
	/* disable & store Ether PHY LED */
	iomux_ori = read_reg_word(IOMUX_CONTROL1);
	iomux = iomux_ori & ~(0xFF << 3);
	write_reg_word(IOMUX_CONTROL1, iomux);
	
	macResetSwMAC();
	setPortIsolate();
    
    if(use_ext_switch){
        gswPbusWrite(0xA4,0x08160816);
        gswPbusWrite(0x1C,0x68166816);
        gswPbusWrite(0x20,0x08160816);
        gswPbusWrite(0x24,0x08160816);
    }else{
        write_reg_word(GSW_BASE + 0xA4,0x08160816);
        write_reg_word(GSW_BASE + 0x1C,0x68166816);
        write_reg_word(GSW_BASE + 0x20,0x08160816);
        write_reg_word(GSW_BASE + 0x24,0x08160816);
    }

	macPhyReset();
	/* restore Ether PHY LED */
	write_reg_word(IOMUX_CONTROL1, iomux_ori);
}

void setXsiRateLimit (int disableRateLimit)
{
	if (disableRateLimit == 1)
	{
		write_reg_word(TOTAL_RX_RC_CFG, 0x7d);
		write_reg_word(TOTAL_RC_DATA_H, 0x0);
		write_reg_word(TOTAL_RC_DATA_L, 0x0);
		write_reg_word(TOTAL_RX_CFG, 0x0);
	}
	else
	{		
		write_reg_word(TOTAL_RX_RC_CFG, 0x8001007d);
		write_reg_word(TOTAL_RC_DATA_H, 0x0);
		
		write_reg_word(TOTAL_RC_DATA_L, 0x4);
		write_reg_word(TOTAL_RX_CFG, 0x80000001);
		
		write_reg_word(TOTAL_RC_DATA_L, 0x1388);
		write_reg_word(TOTAL_RX_CFG, 0x90000001);
		
		write_reg_word(TOTAL_RC_DATA_L, 0xd);
		write_reg_word(TOTAL_RX_CFG, 0xa0000001);
	}

	return;
}

void setEtherRateLimit (int disableRateLimit)
{
	if(!isFPGA)
	{
		if (disableRateLimit == 1)
		{
			write_reg_word(GSW_ERLCR(6),0x4E28488);//set p6 egress traffic limit (cir=40mbps,cbs=8kbyte)
		}
		else
		{
			write_reg_word(GSW_ERLCR(6),0x9C8488);//set p6 egress traffic limit (cir=5mbps,cbs=8kbyte)
		}
		setXsiRateLimit(disableRateLimit);
	}

	return;
}

void macGetMacAddr(macAdapter_t *mac_p, unsigned char *macAddr)
{
	int i;

	for (i = 0; i < 6; i++)
		mac_p->macAddr[i] = macAddr[i];
}


void macPhyRestartAN(void)
{
    uint8 i;
    uint16 value;
    for ( i = 0; i < 32; i++ ) {
        value = miiStationRead(i, PHY_CONTROL_REG);
        value |= PHY_RESTART_AN;
	    miiStationWrite(i, PHY_CONTROL_REG, value);
    }
	return;
}

int tc_mii_ext_station_fill_addr_ext(uint32 portAddr, uint32 devAddr, uint32 regAddr)
{
	uint32 pbus_addr;
	uint32 pbus_data;
	uint32 phy_acs_st;
	uint32 max_wait_cnt = 10000;

	pbus_addr = 0x701C;

	// 1. write phy_addr & phy_reg & phy_data
	// b31    - phy access 1:start&busy, 0:complete&idle
	// b29:25 - mdio phy reg addr
	// b24:20 - mdio phy addr
	// b19:18 - (Clause45) 2'b00:address, 2'b01:write, 2'b11:read, 2'b10:post_read_inc_addr
	// b17:16 - start field, 2'b00:Clause45, 2'b01:Clause22
	// b15:0  - data
	pbus_data = 0x80000000; // write
	pbus_data = pbus_data | ((portAddr & 0x1F) << 20);
	pbus_data = pbus_data | ((devAddr & 0x1F) << 25);
	pbus_data = pbus_data | (regAddr & 0xFFFF);
	gswPbusWrite(pbus_addr, pbus_data);

	// 2. check phy_acs_st
	phy_acs_st = 1;
	while (phy_acs_st) {
		pbus_data = gswPbusRead(pbus_addr);
		phy_acs_st = (pbus_data >> 31) & 0x1;
		if (--max_wait_cnt == 0)
		{
			//prom_printf("\n\n\n!!! %s hang : wait busy bit timeout for %lX/%lX/%lX !!!\n\n\n", __FUNCTION__, portAddr, devAddr, regAddr);
			break;
		}
	}

	// 3. complete
	return (0);
}/*end tc_mii_ext_station_fill_addr_ext*/

void tc_mii_ext_station_write_ext(uint32 portAddr, uint32 devAddr, uint32 regAddr, uint32 miiData)
{
	uint32 pbus_addr;
	uint32 pbus_data;
	uint32 phy_acs_st;
	uint32 max_wait_cnt = 10000;
	portAddr &= 0x1F;
	devAddr &= 0x1F;
	regAddr &= 0xFFFF;

	tc_mii_ext_station_fill_addr_ext(portAddr, devAddr, regAddr);

	pbus_addr = 0x701C;

	// 1. write phy_addr & phy_reg & phy_data
	// b31    - phy access 1:start&busy, 0:complete&idle
	// b29:25 - mdio phy reg addr
	// b24:20 - mdio phy addr
	// b19:18 - (Clause45) 2'b00:address, 2'b01:write, 2'b11:read, 2'b10:post_read_inc_addr
	// b17:16 - start field, 2'b00:Clause45, 2'b01:Clause22
	// b15:0  - data
	pbus_data = 0x80040000; // write
	pbus_data = pbus_data | (portAddr << 20);
	pbus_data = pbus_data | (devAddr << 25);
	pbus_data = pbus_data | (miiData & 0xFFFF);
	gswPbusWrite(pbus_addr, pbus_data);

	// 2. check phy_acs_st
	phy_acs_st = 1;
	while (phy_acs_st) {
		pbus_data = gswPbusRead(pbus_addr);
		phy_acs_st = (pbus_data >> 31) & 0x1;
		if (--max_wait_cnt == 0)
		{
			//prom_printf("\n\n\n!!! %s hang : wait busy bit timeout for %lX/%lX/%lX !!!\n\n\n", __FUNCTION__, portAddr, devAddr, regAddr);
			break;
		}
	}

	// 3. complete

}/*end tc_mii_ext_station_write_ext*/

void macCfgExtSwitch(void)
{	
	uint32 reg;
	
    //disable ext switch flow control
	reg = gswPbusRead (0x1fe0);
	reg &= (~(1<<31));
	gswPbusWrite (0x1fe0, reg);

	//Enable TRGMII
	gswPbusWrite(0x7808, 0);
	gswPbusWrite(0x7804, 0x01017e8f);
	gswPbusWrite(0x7808, 1);
	write_reg_word(GSW_BASE+ 0x7808,0);
	write_reg_word(GSW_BASE+ 0x7804,0x01017e8f);
	write_reg_word(GSW_BASE+ 0x7808,1);
	
	//Setup Clock 200M
	mtEMiiRegWrite(0,0x1f,0x404,0x1000);
	mtEMiiRegWrite(0,0x1f,0x409,0x57);
	mtEMiiRegWrite(0,0x1f,0x40a,0x57);
	mtEMiiRegWrite(12,0x1f,0x404,0x1000);
	mtEMiiRegWrite(12,0x1f,0x409,0x57);
	mtEMiiRegWrite(12,0x1f,0x40a,0x57);

	mtEMiiRegWrite(0,0x1f,0x403,0x1800);
	pause(5); //rise delay time
	mtEMiiRegWrite(0,0x1f,0x403,0x1c00);
	mtEMiiRegWrite(0,0x1f,0x401,0xc020);
	mtEMiiRegWrite(0,0x1f,0x406,0xa030);
    mtEMiiRegWrite(0,0x1f,0x406,0xa038);
	mtEMiiRegWrite(0,0x1f,0x410,0x3);
	mtEMiiRegWrite(12,0x1f,0x403,0x1800);
	pause(5); // rise delay time
	mtEMiiRegWrite(12,0x1f,0x403,0x1c00);
	mtEMiiRegWrite(12,0x1f,0x401,0xc020);
	mtEMiiRegWrite(12,0x1f,0x406,0xa030);
    mtEMiiRegWrite(12,0x1f,0x406,0xa038);
	mtEMiiRegWrite(12,0x1f,0x410,0x3);
	pause(50);

	/* TXEN Disable and Link Down */
	reg = (IPG_CFG_64BITS<<IPG_CFG_PN_SHIFT) | MAC_MODE_PN | FORCE_MODE_PN | 
		MAC_RX_EN_PN | BKOFF_EN_PN | BACKPR_EN_PN | 
		(PN_SPEED_1000M<<FORCE_SPD_PN_SHIFT) | FORCE_DPX_PN ;

	write_reg_word(GSW_PMCR(5), reg);
	gswPbusWrite(0x3600, reg);
	pause(5); // rise delay time

	//Reset TRGMII Tx/Rx
	reg = gswPbusRead (0x7a40);
	gswPbusWrite(0x7a40, (reg | (1<<28)));
	pause(5); // rise delay time
	gswPbusWrite(0x7a40, (reg & (~(1<<28))));

	reg =  read_reg_word(GSW_BASE + 0x7a40);
	write_reg_word(GSW_BASE + 0x7a40, (reg | (1<<28)));
	pause(5); //rise delay time
	write_reg_word(GSW_BASE + 0x7a40, (reg & (~(1<<28))));	
	
	reg = gswPbusRead (0x7a00);
	gswPbusWrite(0x7a00, (reg | (1<<31)));

	reg =  read_reg_word(GSW_BASE + 0x7a00);
	write_reg_word(GSW_BASE + 0x7a00, (reg | (1<<31)));
	
	//Deduce MT7530 P6 Tx Driving Strength	
	gswPbusWrite(0x7a54, 0xAA);
	gswPbusWrite(0x7a5c, 0xAA);
	gswPbusWrite(0x7a64, 0xAA);
	gswPbusWrite(0x7a6c, 0xAA);
	gswPbusWrite(0x7a74, 0xAA);
	gswPbusWrite(0x7a7c, 0x77);

	write_reg_word(GSW_BASE + 0x7a54, 0xAA);
	write_reg_word(GSW_BASE + 0x7a5c, 0xAA);
	write_reg_word(GSW_BASE + 0x7a64, 0xAA);
	write_reg_word(GSW_BASE + 0x7a6c, 0xAA);
	write_reg_word(GSW_BASE + 0x7a74, 0xAA);
	write_reg_word(GSW_BASE + 0x7a7c, 0x77);

	//Enable port
 	gswPbusWrite(0x7830, 1);
  	write_reg_word(GSW_BASE+ 0x7830,0x1);
	pause(5); //rise delay time
	
	reg = gswPbusRead (0x7a00);
	gswPbusWrite(0x7a00, (reg & (~(1<<31))));
	reg =  read_reg_word(GSW_BASE + 0x7a00);
	write_reg_word(GSW_BASE + 0x7a00, (reg & (~(1<<31))));

	/* TXEB Disable and Link up */
	reg = (IPG_CFG_64BITS<<IPG_CFG_PN_SHIFT) | MAC_MODE_PN | FORCE_MODE_PN | 
		 MAC_RX_EN_PN | BKOFF_EN_PN | BACKPR_EN_PN | 
		(PN_SPEED_1000M<<FORCE_SPD_PN_SHIFT) | FORCE_DPX_PN | FORCE_LNK_PN;

	write_reg_word(GSW_PMCR(5), reg);
	gswPbusWrite(0x3600, reg);

    /*
  	//Disable internal FE Phy
	reg = read_reg_word(GSW_BASE + 0x7840);
	write_reg_word(GSW_BASE + 0x7840, (reg | 0xf));	
        */
        
	//Disable Port 5/Port 6 SA Learning
	write_reg_word(GSW_PSC(5), 0xfff10);
	write_reg_word(GSW_PSC(6), 0xfff10);

	/* set port 5 as 1000Mbps, FC on */
	reg = (IPG_CFG_64BITS<<IPG_CFG_PN_SHIFT) | MAC_MODE_PN | FORCE_MODE_PN | 
		MAC_TX_EN_PN | MAC_RX_EN_PN | BKOFF_EN_PN | BACKPR_EN_PN | 
		(PN_SPEED_1000M<<FORCE_SPD_PN_SHIFT) | FORCE_DPX_PN | FORCE_LNK_PN;
	
	write_reg_word(GSW_PMCR(5), reg);
	gswPbusWrite(0x3600, reg);

	return;
}




void macSetGSW(macAdapter_t *mac_p)
{
	uint32 reg;
    int phy_add_start=0,phy_add_end=0,phy_add;
	/* set port 6 as 1Gbps, FC on */
	reg = (IPG_CFG_SHORT<<IPG_CFG_PN_SHIFT) | MAC_MODE_PN | FORCE_MODE_PN | 
		MAC_TX_EN_PN | MAC_RX_EN_PN | BKOFF_EN_PN | BACKPR_EN_PN | 
		ENABLE_RX_FC_PN | ENABLE_TX_FC_PN | (PN_SPEED_1000M<<FORCE_SPD_PN_SHIFT) | 
		FORCE_DPX_PN | FORCE_LNK_PN;
	write_reg_word(GSW_PMCR(6), reg);

	/* set cpu port as port 6 */
	reg = (0x40<<MFC_BC_FFP_SHIFT) | (0x40<<MFC_UNM_FFP_SHIFT) | (0x40<<MFC_UNU_FFP_SHIFT) |
			MFC_CPU_EN	| (6<<MFC_CPU_PORT_SHIFT);
    if(use_ext_switch)
        gswPbusWrite(0x10, reg);
    else
        write_reg_word(GSW_MFC, reg);

	/* check if FPGA */
	if (isFPGA) {
		/*decrease mdc/mdio clock*/
		reg = read_reg_word(GSW_CFG_PPSC);
		reg &= ~((1<<6) | (1<<7));
		write_reg_word(GSW_CFG_PPSC, reg);

		/* auto polling enable, 2 PHY ports, start PHY addr=1 and end PHY addr=2 */
		reg = read_reg_word(GSW_CFG_PPSC);
        reg &= ~(0x7F << 24);	/* for FPGA external PHY, always use auto polling*/
        reg &= ~(PHY_END_ADDR | PHY_ST_ADDR);
        if ((miiStationRead(1, 2) == 0x4d) && (miiStationRead(1, 3) == 0xd072)){
            if(read_reg_word(GSW_PMSR(0)) == 0xdeadbeef){
                if(isEN7580){
                    reg |= 0x1f << 24;
                    phy_add_start = 0;
                    phy_add_end = 4;
                }else{               
                    reg |= 0x7 << 24;	/* for FPGA external PHY, always use auto polling*/
                    phy_add_start =0;
                    phy_add_end =2;
                }
            }else{
                if(isEN7580){
                    reg |= 0xf << 24;
                    phy_add_start = 1;
                    phy_add_end = 4;        
                }else{
                    reg |= 0x3 << 24;	/* for FPGA external PHY, always use auto polling*/
                    phy_add_start =1;
                    phy_add_end =2;
                }
            }
        }else if ((miiStationRead(2, 2) == 0x4d) && (miiStationRead(2, 3) == 0xd072)){
            if (read_reg_word(GSW_PMSR(0)) == 0xdeadbeef){
                reg |= 0x7 << 24;	/* for FPGA external PHY, always use auto polling*/
                phy_add_start =1;
                phy_add_end =3;
            }else{
                reg |= 0x3 << 24;	/* for FPGA external PHY, always use auto polling*/
                phy_add_start =2;
                phy_add_end =3;
            }
        }else if((miiStationRead(4, 2) == 0xf) && (miiStationRead(4, 3) == 0xc6c2)){
            // MT7510 FPGA daughter board extra 4 port
            /* auto polling enable, 2 PHY ports, start PHY addr=2 and end PHY addr=7 */
			reg = read_reg_word(GSW_CFG_PPSC);
			reg |= ((1<<24) | (1<<25)| (1<<26)| (1<<27)| (1<<28)| (1<<29));
			reg &= ~(PHY_END_ADDR | PHY_ST_ADDR);
            phy_add_start =2;
            phy_add_end =7;
		} 
		reg |= (phy_add_end << PHY_END_ADDR_SHIFT) | (phy_add_start << PHY_ST_ADDR_SHIFT);
		write_reg_word(GSW_CFG_PPSC, reg);
        // disable phy 1000M capability, fpga not support gmii
        for(phy_add = phy_add_start;phy_add<=phy_add_end;phy_add++){
            reg = miiStationRead(phy_add, 0x9);
            reg &= ~((1<<9) | 1<<8);
            miiStationWrite(phy_add, 0x9, reg);
        }
        macPhyRestartAN();
	}
	else {
		/* Sideband signal error for Port 3, which need the auto polling */
		write_reg_word(GSW_BASE+0x7018, 0x7f7f8c08);
	}
}

void led_gpio_enable(void)
{
    uint32 value=0;
    value = read_reg_word(IOMUX_CONTROL1);
    value |= LAN_LED;
    write_reg_word(IOMUX_CONTROL1,value);
}


void macSetMACCR(macAdapter_t *map_p)
{
	uint32 reg;

	reg = (12<<GDM_JMB_LEN_SHIFT)  |
		(GDM_P_CPU<<GDM_UFRC_P_SHIFT) | (GDM_P_CPU<<GDM_BFRC_P_SHIFT) | 
		(GDM_P_CPU<<GDM_MFRC_P_SHIFT) | (GDM_P_CPU<<GDM_OFRC_P_SHIFT);
	write_reg_word(GDMA1_FWD_CFG, reg);

#if defined(TCSUPPORT_CPU_AN7552)
	reg = (13<<GDM_JMB_LEN_SHIFT)  |
		(5<<GDM_UFRC_P_SHIFT) | (5<<GDM_BFRC_P_SHIFT) |
		(5<<GDM_MFRC_P_SHIFT) | (5<<GDM_OFRC_P_SHIFT);
	reg |= GDM_STRPCRC;
	write_reg_word(GDMA2_FWD_CFG, reg);
#endif

#if defined(TCSUPPORT_CPU_EN7581)
    reg = GDM_PAD_EN  | GDM_STRPCRC |
		(GDM_P_CPU<<GDM_UFRC_P_SHIFT) | (GDM_P_CPU<<GDM_BFRC_P_SHIFT) | 
		(GDM_P_CPU<<GDM_MFRC_P_SHIFT) | (GDM_P_CPU<<GDM_OFRC_P_SHIFT);
	write_reg_word(GDMA3_FWD_CFG, reg);

    reg = GDM_PAD_EN  | GDM_STRPCRC | GDM_INSV_EN |
		(GDM_P_CPU<<GDM_UFRC_P_SHIFT) | (GDM_P_CPU<<GDM_BFRC_P_SHIFT) | 
		(GDM_P_CPU<<GDM_MFRC_P_SHIFT) | (GDM_P_CPU<<GDM_OFRC_P_SHIFT);
	write_reg_word(GDMA4_FWD_CFG, reg);
#endif

	/* check if FPGA */
	if (isFPGA) {
		/* set 1us clock for FPGA */
		reg = read_reg_word(CR_CLK_CFG);
		reg &= ~(0x3f000000);

		reg |= (0x31<<24);

		write_reg_word(CR_CLK_CFG, reg);
	}
    //led_gpio_enable();
}

void macSetMacReg(macAdapter_t *mac_p)
{
	write_reg_word(GDMA1_MAC_ADRL, mac_p->macAddr[2]<<24 | mac_p->macAddr[3]<<16 | \
                               mac_p->macAddr[4]<<8  | mac_p->macAddr[5]<<0);
	write_reg_word(GDMA1_MAC_ADRH, mac_p->macAddr[0]<<8  | mac_p->macAddr[1]<<0);

	/* fill in switch's MAC address */
	write_reg_word(GSW_SMACCR0, mac_p->macAddr[2]<<24 | mac_p->macAddr[3]<<16 | \
                               mac_p->macAddr[4]<<8  | mac_p->macAddr[5]<<0);
	write_reg_word(GSW_SMACCR1, mac_p->macAddr[0]<<8  | mac_p->macAddr[1]<<0);
}

int macDrvRegInit(macAdapter_t *mac_p)
{
    // ----- setup interrupt mask ---
    //  ---- Setup HASH table------------------------------------------
    //  -----Setup Interrupt Timer-------------------------------------
    //  -----Setup AUTO polling timer----------------------------------
    //  ---- Setup DMA burst and arbitration---------------------------
    //  -----Setup DMA Descriptor Base Address Assign------------------    
    //  -----Setup MACCR-----------------------------------------------
    macSetMACCR(mac_p);
    
    // --- setup MAC address ---
    macSetMacReg(mac_p);      

   	macSetGSW(mac_p);

    return 0;
}



/******************************************************************************
******************************************************************************/
static int qdma_bm_push_tx_dscp(struct QDMA_DscpInfo_S *diPtr, int ringIdx) 
{
	if(diPtr->next != NULL) {
		QDMA_ERR("The TX DSCP is not return from tx used pool\n") ;
		return -1 ;
	}

	diPtr->skb = NULL ;
	if(!gpQdmaPriv->txHeadPtr) {
		gpQdmaPriv->txHeadPtr = diPtr ;
		gpQdmaPriv->txTailPtr = diPtr ;
	} else {
		gpQdmaPriv->txTailPtr->next = diPtr ;
		gpQdmaPriv->txTailPtr = gpQdmaPriv->txTailPtr->next ;
	}
	
	return 0 ;
}


/******************************************************************************
******************************************************************************/
static struct QDMA_DscpInfo_S *qdma_bm_pop_tx_dscp(void)
{
	struct QDMA_DscpInfo_S *diPtr ;
	ulong flags ;
	
	diPtr = gpQdmaPriv->txHeadPtr ;
	if(gpQdmaPriv->txHeadPtr == gpQdmaPriv->txTailPtr) {
		gpQdmaPriv->txHeadPtr = NULL ;
		gpQdmaPriv->txTailPtr = NULL ;
	} else {
		gpQdmaPriv->txHeadPtr = gpQdmaPriv->txHeadPtr->next ;
	}

	if(diPtr) {
		diPtr->next = NULL ;
	}
	
	return diPtr ;
}

/******************************************************************************
 Packet Receive
******************************************************************************/
/******************************************************************************
******************************************************************************/
static void qdma_bm_add_rx_dscp(struct QDMA_DscpInfo_S *diPtr) 
{
	if(!gpQdmaPriv->rxStartPtr) {
		gpQdmaPriv->rxStartPtr = diPtr ;
		diPtr->next = gpQdmaPriv->rxStartPtr ;
	} else {
		diPtr->next = gpQdmaPriv->rxStartPtr->next ;
		gpQdmaPriv->rxStartPtr->next = diPtr ;
		gpQdmaPriv->rxStartPtr = diPtr ;
	}
}


static struct QDMA_DscpInfo_S *qdma_bm_get_unused_rx_dscp(void)
{
	struct QDMA_DscpInfo_S *diPtr = NULL ;

	
	if(gpQdmaPriv->rxStartPtr) {
		if(!gpQdmaPriv->rxEndPtr) {
			diPtr = gpQdmaPriv->rxStartPtr ;
			gpQdmaPriv->rxEndPtr = diPtr ;
		} else if(gpQdmaPriv->rxEndPtr->next != gpQdmaPriv->rxStartPtr) {
			diPtr = gpQdmaPriv->rxEndPtr->next ;
			gpQdmaPriv->rxEndPtr = diPtr ; 
		}
	} 

	return diPtr ;
}


int qdma_bm_transmit_done(int amount) 
{
	QDMA_DMA_DSCP_T txDscp ;
	int ret = 0 ;
	struct QDMA_DscpInfo_S *diPtr ;
	uint base = gpQdmaPriv->csrBaseAddr ;
	uint entryLen, headIdx, irqValue=0, irqDepth=CONFIG_IRQ_DEPTH ;
	uint *irqPtr ;
	int i=0, j=0, idx=0, ringIdx=0 ;
	uint RETRY=3 ;
	uint irqStatus ;
	void *msgPtr;

	irqStatus = qdmaGetIrqStatus(base) ;
	headIdx = (irqStatus & IRQ_STATUS_HEAD_IDX_MASK) >> IRQ_STATUS_HEAD_IDX_SHIFT ;
	entryLen = (irqStatus & IRQ_STATUS_ENTRY_LEN_MASK) >> IRQ_STATUS_ENTRY_LEN_SHIFT ;
	if(entryLen == 0) {
		QDMA_MSG(DBG_WARN, "qdma_bm_transmit_done-111111\n") ;
		goto out2 ;
	}

	entryLen = (amount && amount<entryLen) ? amount : entryLen ;
	for(i=0 ; i<entryLen ; i++) {
		irqPtr = (uint *)gpQdmaPriv->irqQueueAddr + ((headIdx+i)%irqDepth) ;
		
		RETRY = 3 ;
		while(RETRY--) {
			irqValue = *irqPtr ;
			if(irqValue == CONFIG_IRQ_DEF_VALUE) {
				QDMA_ERR("There is no data available in IRQ queue. irq value:%.8x, irq ptr:%.8x TIMEs:%d\n", (uint)irqValue, (uint)irqPtr, RETRY) ;
				if(RETRY <= 0) {
					gpQdmaPriv->counters.IrqQueueAsynchronous++ ;
					ret = -1 ;
					goto out1 ;
				}
			} else {
				*irqPtr = CONFIG_IRQ_DEF_VALUE ;
				break ;
			}
		}
		
		idx = (irqValue & IRQ_CFG_IDX_MASK) ;
		ringIdx = (irqValue & IRQ_CFG_RINGIDX_MASK) >> IRQ_CFG_RINGIDX_SHIFT;
		if(idx<0 || idx>=gpQdmaPriv->txDscpNum) {
			QDMA_ERR("The TX DSCP index %d is invalid.\n", idx) ;
			gpQdmaPriv->counters.txIrqQueueIdxErrs++ ;
			ret = -1;
			continue ;
		}
			
		diPtr = (struct QDMA_DscpInfo_S *)gpQdmaPriv->txBaseAddr + idx ;
		if(diPtr->dscpIdx!=idx || diPtr->next!=NULL) {
			QDMA_ERR("The content of the TX DSCP_INFO(%.8x) is incorrect. ENTRY_LEN:%d, HEAD_IDX:%d, IRQ_VALUE:%.8x.\n", (uint)diPtr, entryLen, headIdx, irqValue) ;
			gpQdmaPriv->counters.txDscpIncorrect++ ;
			ret = -1;
			continue ;
		}
		
		msgPtr = (void *)txDscp.msg ;
		if(msgPtr)
			memset(msgPtr, 0, QDMA_TX_DSCP_MSG_LENS);
		free_skb(diPtr->skb);
		
		qdma_bm_push_tx_dscp(diPtr, 0) ;

	}
	QDMA_MSG(DBG_WARN, "qdma_bm_transmit_done-000000\n") ;

out1:
	for(j=0 ; j<(i>>7) ; j++) {
		qdmaSetIrqClearLen(base, 0x80) ;
	}
	qdmaSetIrqClearLen(base, (i&0x7F)) ;

out2:
	return ret ;
}

int qdma_bm_transmit_packet(sk_buff *skb, int ringIdx, uint msg0, uint msg1)
{
	struct QDMA_DscpInfo_S *pNewDscpInfo ;
	QDMA_DMA_DSCP_T *pTxDscp ;
	uint base = gpQdmaPriv->csrBaseAddr ;
	int ret = 0 ;
	unsigned long   addr, len;

	if(skb==NULL){
		QDMA_ERR("The input arguments are wrong, skb is NULL\n"); 
        return -1;
	}

	if(skb->len<=0 || skb->len>CONFIG_MAX_PKT_LENS) 
	{
		QDMA_ERR("The input arguments are wrong, skb:%.8x, skbLen:%d.\n", (uint)skb, skb->len) ; 
        return -1;
	}

	/* recycle TX DSCP when send packets in tx polling mode */
	
	if(qdmaGetIrqEntryLen(base) >= QDMA_TX_THRESHOLD) { 
		qdma_bm_transmit_done(0) ;
	}

	/* Get unused TX DSCP from TX unused DSCP link list */	
	pNewDscpInfo = qdma_bm_pop_tx_dscp() ;
	if(pNewDscpInfo == NULL) {
		gpQdmaPriv->counters.noTxDscps++ ;
		QDMA_ERR("pNewDscpInfo is NULL\n") ; 
		return -1 ;
	}
	pTxDscp = gpQdmaPriv->txUsingPtr->dscpPtr ;
	pTxDscp->msg[0] = msg0;
	pTxDscp->msg[1] = msg1;
	pTxDscp->next_idx = pNewDscpInfo->dscpIdx ;
	pTxDscp->pkt_addr = K1_TO_PHY(skb->data) ;
	pTxDscp->ctrl.pkt_len = skb->len /* pktLen */; 
#if defined(TCSUPPORT_CPU_EN7580) || defined(TCSUPPORT_CPU_EN7527) || defined(TCSUPPORT_CPU_EN7516)
	pTxDscp->ctrl.nls = 0 ;
#if defined(TCSUPPORT_CPU_EN7580)
	pTxDscp->ctrl.drop_pkt = 0 ;
#endif
#endif
	pTxDscp->ctrl.done = 0 ;

	QDMA_MSG(DBG_MSG, "Hook TX DSCP to TXDMA. TX_CPU_IDX:%d, TX_NULL_IDX:%d\n", gpQdmaPriv->txUsingPtr->dscpIdx, pNewDscpInfo->dscpIdx) ;
	QDMA_MSG(DBG_MSG, "TXDSCP: DONE:%d, PKT:%.8x, PKTLEN:%d, NEXT_IDX:%d, loopcnt:%d\n", 
																(uint)pTxDscp->ctrl.done, 
																(uint)pTxDscp->pkt_addr,
																(uint)pTxDscp->ctrl.pkt_len,
																(uint)pTxDscp->next_idx,
																(uint)pTxDscp->msg[1]>>24) ;


	gpQdmaPriv->txUsingPtr->skb = skb ;


	//dump_skb(skb);
	gpQdmaPriv->txUsingPtr = pNewDscpInfo ;


	addr = (unsigned long)(pTxDscp->pkt_addr) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = ((unsigned long)(pTxDscp->ctrl.pkt_len) + 2 * CONFIG_SYS_CACHELINE_SIZE ) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len);

	
	addr = (unsigned long )&mac_p->macMemPool_p->descrPool[0] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = (CONFIG_TX0_DSCP_NUM*CONFIG_TX0_DSCP_SIZE  + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len );

	//need for irq recycle
	addr = (unsigned long )&mac_p->macMemPool_p->descrPool[DESC_TOTAL_SIZE] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = ((CONFIG_IRQ_DEPTH<<2)+ 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len );


	qdmaSetTxCpuIdx(base, ringIdx, pNewDscpInfo->dscpIdx) ;
	gpQdmaPriv->counters.txCounts++ ;


	QDMA_MSG(DBG_MSG, "RingIdx:%d, GLG:%.8x, IRQStatus:%.8x, CSR info: RX_CPU_IDX:%d, RX_DMA_IDX:%d, TX_CPU_IDX:%d, TX_DMA_IDX:%d\n", 
																ringIdx, 
																qdmaGetGlbCfg(base), 
																qdmaGetIrqStatus(base), 
																qdmaGetRxCpuIdx(base, ringIdx), 
																qdmaGetRxDmaIdx(base, ringIdx), 
																qdmaGetTxCpuIdx(base, ringIdx), 
																qdmaGetTxDmaIdx(base, ringIdx)) ;
    return ret ;
}


int tc3162_eth_send(sk_buff *skb)
{

	uint32 length = skb->len;
	ethTxMsg_t ethTxMsg;
	int error;

	if (skb->data == NULL) {
		printf("Tx a empty mbuf\n"); 	
		return 1;
	}

	if (length < 60) {
		length = 60;
	}


  // start filling tx message
  /* GDMA1 */
    memset(&ethTxMsg, 0, sizeof(ethTxMsg_t));
    ethTxMsg.raw.fport = GDM_P_GDMA1;
    ethTxMsg.raw.channel = 0;
    ethTxMsg.raw.queue =0;
#ifdef TCSUPPORT_CPU_EN7580
    ethTxMsg.raw.nboq = 0;
    ethTxMsg.raw.mtr_g = 0x7f; /*not use any meter ratelimit*/
#endif

	error = qdma_bm_transmit_packet(skb, 0,ethTxMsg.msg[0],ethTxMsg.msg[1]) ;
    
    if(error){
		free_skb(skb);
		//prom_printf("qdma tranmit fail\n");
		return 0;
     }


	return 0;
}
#if 0
int32 macSearchPhyAddr(macAdapter_t *mac_p)
{
	uint32 miiReg = 0;
	uint32 phyAddr;

	mac_p->enetPhyAddr = 0;
	for ( mac_p->enetPhyAddr = 0; mac_p->enetPhyAddr < 32; mac_p->enetPhyAddr++ ) {
		miiReg = miiStationRead(mac_p->enetPhyAddr, PHY_ID_REG1);
		if (miiReg == 0)
			miiReg = miiStationRead(mac_p->enetPhyAddr, PHY_ID_REG2);
		if ((miiReg != 0) && (miiReg != 0xffff))
			return 0;
	}

	mac_p->enetPhyAddr = 0;

	return -1;
}
#endif

#if defined(TCSUPPORT_CPU_EN7580)
int qdmaEnableInt(uint base, uint bit, QDMA_InterruptIdx_T intIdx)
{
	ulong flags=0 ;
	uint t=0 ;

	if( (intIdx < QDMA_INT1_ENABLE1) || (intIdx > QDMA_INT4_ENABLE2) )
	{
		printf("qdmaEnableInt: INT Index Error.\n");
		return -EINVAL;
	}

	if( (intIdx%2) == 1 )
	{
		t = IO_GREG(QDMA_CSR_INT_ENABLE1(base,((intIdx+1)>>1))) ;
		IO_SREG(QDMA_CSR_INT_ENABLE1(base,((intIdx+1)>>1)), (t|bit));
	}
	else
	{
		t = IO_GREG(QDMA_CSR_INT_ENABLE2(base,(intIdx>>1))) ;
		IO_SREG(QDMA_CSR_INT_ENABLE2(base,(intIdx>>1)), (t|bit));
	}

	return 0 ;
}

/******************************************************************************
******************************************************************************/
int qdmaDisableInt(uint base, uint bit, QDMA_InterruptIdx_T intIdx)
{
	ulong flags=0 ;
	uint t=0 ;

	if( (intIdx < QDMA_INT1_ENABLE1) || (intIdx > QDMA_INT4_ENABLE2) )
	{
		printf("qdmaDisableInt: INT Index Error.\n");
		return -EINVAL;
	}

	if( (intIdx%2) == 1 )
	{
		t = IO_GREG(QDMA_CSR_INT_ENABLE1(base,((intIdx+1)>>1))) ;
		IO_SREG(QDMA_CSR_INT_ENABLE1(base,((intIdx+1)>>1)), (t&(~bit)));
	}
	else
	{
		t = IO_GREG(QDMA_CSR_INT_ENABLE2(base,(intIdx>>1))) ;
		IO_SREG(QDMA_CSR_INT_ENABLE2(base,(intIdx>>1)), (t&(~bit)));
	}

	return 0 ;
}

/******************************************************************************
******************************************************************************/
int qdmaSetIntMask(uint base, uint value, QDMA_InterruptIdx_T intIdx)
{
	ulong flags=0 ;

	if( (intIdx < QDMA_INT1_ENABLE1) || (intIdx > QDMA_INT4_ENABLE2) )
	{
		printf("qdmaSetIntMask: INT Index Error\n");
		return -EINVAL;
	}

	if( (intIdx%2) == 1 )
	{
		IO_SREG(QDMA_CSR_INT_ENABLE1(base,((intIdx+1)>>1)), value) ;
	}
	else
	{
		IO_SREG(QDMA_CSR_INT_ENABLE2(base,(intIdx>>1)), value) ;
	}

	return 0 ;
}

/******************************************************************************
******************************************************************************/
int qdmaGetIntMask(uint base, QDMA_InterruptIdx_T intIdx)
{
	ulong flags=0, value=0 ;

	if( (intIdx < QDMA_INT1_ENABLE1) || (intIdx > QDMA_INT4_ENABLE2) )
	{
		printf("qdmaGetIntMask: INT Index Error\n");
		return -EINVAL;
	}

	if( (intIdx%2) == 1 )
	{
		value = IO_GREG(QDMA_CSR_INT_ENABLE1(base, ((intIdx+1)>>1))) ;
	}
	else
	{
		value = IO_GREG(QDMA_CSR_INT_ENABLE2(base, (intIdx>>1))) ;
	}

	return value ;
}
#else
int qdmaEnableInt(uint base, uint bit)
{
	uint RETRY=3, t=0 ;
		
	while(RETRY--) {
		t = IO_GREG(QDMA_CSR_INT_ENABLE(base)) ;
		if(t == 0) {
			QDMA_MSG(DBG_WARN, "########WARNING: Enable INT: Get INT_ENABLE:%.8x, RETRY:%d\n", t, 3-RETRY) ;
		} else {
			IO_SREG(QDMA_CSR_INT_ENABLE(base), (t|bit)) ;
			break ;
		}
	}

	
	return 0 ;
}

/******************************************************************************
******************************************************************************/
int qdmaDisableInt(uint base, uint bit)
{
	uint RETRY=3, t=0 ;
	
	while(RETRY--) {
		t = IO_GREG(QDMA_CSR_INT_ENABLE(base)) ;
		if(t == 0) {
			QDMA_MSG(DBG_WARN, "########WARNING: Disable INT: Get INT_ENABLE:%.8x, RETRY:%d\n", t, 3-RETRY) ;
		} else {
			IO_SREG(QDMA_CSR_INT_ENABLE(base), (t&(~bit))) ;
			break ;
		}
	}
		
	
	return 0 ;
}

/******************************************************************************
******************************************************************************/
int qdmaSetIntMask(uint base, uint value)
{	
	IO_SREG(QDMA_CSR_INT_ENABLE(base), value) ;
	
	return 0 ;
}

/******************************************************************************
******************************************************************************/
int qdmaGetIntMask(uint base)
{
	ulong value=0 ;
	
	value = IO_GREG(QDMA_CSR_INT_ENABLE(base)) ;
	
	return value ;
}
#endif

int qdma_bm_dump_dscp( void ){

	if(ecnt_eth_initd == 0)
		return 0;
	int idx = 0; 
	struct QDMA_DscpInfo_S *diPtr=NULL ;
	idx = 1 ;
	diPtr = gpQdmaPriv->rxStartPtr ;
	printf("\nRx%d DSCP Ring: RxStartIdx:%d, RxEndIdx:%d\n", 0, gpQdmaPriv->rxStartPtr->dscpIdx, gpQdmaPriv->rxEndPtr->dscpIdx) ;
	do {
		if(diPtr) {
			printf("%d: DSCP Idx:%d, DSCP Ptr:%.8x, Done Bit:%d, PktLen:%d, PktAddr:%.8x(%.8x), Next Idx:%d\n", 
																idx, diPtr->dscpIdx, 
																(uint)diPtr->dscpPtr, 
																diPtr->dscpPtr->ctrl.done,
																diPtr->dscpPtr->ctrl.pkt_len, 
																(uint)diPtr->skb, 
																(uint)diPtr->dscpPtr->pkt_addr,
																diPtr->dscpPtr->next_idx) ;
			diPtr = diPtr->next ;
			idx++ ;
		}
	} while(diPtr!=NULL && diPtr!=gpQdmaPriv->rxStartPtr) ;

	return 0;
}


/******************************************************************************
 Packet Transmit
******************************************************************************/


int qdma_bm_hook_receive_buffer(sk_buff *skb, int ringIdx)
{
	struct QDMA_DscpInfo_S *pNewDscpInfo ;
	QDMA_DMA_DSCP_T *pRxDscp ;
	dma_addr_t dmaPktAddr ;
	uint base = gpQdmaPriv->csrBaseAddr ;
	int ret = 0 ;
	unsigned long	addr, len;

	pNewDscpInfo = qdma_bm_get_unused_rx_dscp() ;
	if(pNewDscpInfo == NULL) {
		QDMA_ERR("There is not any free RX DSCP.\n") ; 
		gpQdmaPriv->counters.noRxDscps++ ;
		return -1 ;
	}
	
#ifdef CONFIG_RX_2B_OFFSET
	QDMA_MSG(DBG_MSG, "Adjust the skb->tail location for net IP alignment\n") ;
	if(((uint)skb->data & 7) != 0) {
		//prom_printf("address not align 8\n");
	}
	skb_reserve(skb, 2) ;
	dmaPktAddr = K1_TO_PHY((void *)((uint)skb->data - 2));
#else
	dmaPktAddr = K1_TO_PHY((void *)((uint)skb->data));
#endif /* CONFIG_RX_2B_OFFSET */
	
	
	pRxDscp = gpQdmaPriv->rxUsingPtr->dscpPtr ;
	pRxDscp->msg[0] = 0;
	pRxDscp->msg[1] = 0;
	pRxDscp->msg[2] = 0;
	pRxDscp->msg[3] = 0;
	pRxDscp->pkt_addr = dmaPktAddr ;
	pRxDscp->next_idx = pNewDscpInfo->dscpIdx ;
	pRxDscp->ctrl.pkt_len = 1518 ;
	pRxDscp->ctrl.done = 0 ;
	

	QDMA_MSG(DBG_MSG, "Hook RX DSCP to RXDMA. RX_CPU_IDX:%.8x, RX_NULL_IDX:%.8x\n", gpQdmaPriv->rxUsingPtr->dscpIdx, pNewDscpInfo->dscpIdx) ;
	QDMA_MSG(DBG_MSG, "RXDSCP(%x): DONE:%d, PKT:%.8x, PKTLEN:%d, NEXT_IDX:%d\n", 
													pRxDscp,
													(uint)pRxDscp->ctrl.done, 
													(uint)pRxDscp->pkt_addr,
													(uint)pRxDscp->ctrl.pkt_len,
													(uint)pRxDscp->next_idx) ;
													
	gpQdmaPriv->rxUsingPtr->skb = skb ;
	gpQdmaPriv->rxUsingPtr = pNewDscpInfo ;
	gpQdmaPriv->counters.rxCounts++ ; 


	addr = (unsigned long )&mac_p->macMemPool_p->descrPool[CONFIG_TX0_DSCP_NUM*CONFIG_TX0_DSCP_SIZE] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = (CONFIG_RX0_DSCP_NUM*CONFIG_RX0_DSCP_SIZE + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len );

	addr = (unsigned long )pRxDscp->pkt_addr & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = (1518 + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len);

	/* Setting DMA Rx Descriptor Register */
	qdmaSetRxCpuIdx(base, ringIdx, pNewDscpInfo->dscpIdx) ;
	
	return ret ;
}


int qdma_prepare_rx_buffer(void)
{
	sk_buff *skb = NULL ;

	skb = alloc_skb(2000);
	if(skb == NULL) {
		return -1;
	}

	if(qdma_bm_hook_receive_buffer(skb, 0) != 0) {
		free_skb(skb) ;
		return -1;
	}
	
	return 0 ;
	

}

int qdma_has_free_rxdscp(void)
{
	return (gpQdmaPriv->rxEndPtr->next != gpQdmaPriv->rxStartPtr) ;
}

#ifdef CONFIG_USE_IRQ

int ip_rcv_packet(sk_buff *skb)
{
	struct iphdr *ip_hdr = (struct iphdr *)(skb->data);
	struct ip_udp_hdr *ip_udp_hdr = (struct ip_udp_hdr *)(skb->data);
	int templen = skb->len;
	char *mac = (char *)(ip_hdr) - sizeof(struct ethhdr);

	/* record ip header pointer */
	skb->ip_hdr = ip_hdr;

	if (ip_hdr->protocol == UDP){
		skb->len = ntohs(ip_hdr->tot_len);
		skb_pull(skb, sizeof(struct iphdr));

#ifdef CONFIG_ECNT_MULTIUPGRADE
		if (!multiupgrade_process(skb, mac))
			return 0;
#endif
		if ((ntohs(ip_udp_hdr->udp_dst) == 0x45 )&& (tftp_start ==0))
		{
			setEtherRateLimit (1);
			printf("Start Tftp handler\n");
			TftpStartServer();
			tftp_start = 1;
		}
		skb_push(skb, sizeof(struct iphdr));
		skb->len = templen;

	}
	
nonHandle:

	return 1;

}


#endif

int qdma_bm_receive_packets(uint maxPkts, int ringIdx) 
{
	QDMA_DMA_DSCP_T rxDscp ;
	struct QDMA_DscpInfo_S dscpInfo ;
	uint cnt = maxPkts ;
	uint pktCount = 0 ;
	int retValue = 0 , g;
	sk_buff *newSkb = NULL ;
	sk_buff *rxSkb = NULL ;
	uint base = gpQdmaPriv->csrBaseAddr ;
	struct ethhdr *eth_hdr = NULL; 
	unsigned long	addr, len;

	
#if 1
	do {

		addr = (unsigned long )&mac_p->macMemPool_p->descrPool[CONFIG_TX0_DSCP_NUM*CONFIG_TX0_DSCP_SIZE] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
		len = (CONFIG_RX0_DSCP_NUM*CONFIG_RX0_DSCP_SIZE + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
		invalidate_dcache_range(addr, addr + len );
		
		if(!gpQdmaPriv->rxStartPtr || gpQdmaPriv->rxStartPtr==gpQdmaPriv->rxEndPtr || !gpQdmaPriv->rxStartPtr->dscpPtr->ctrl.done)
		{
			goto ret ;
		}

		//qdma_bm_dump_dscp();
		memcpy(&rxDscp, gpQdmaPriv->rxStartPtr->dscpPtr, sizeof(QDMA_DMA_DSCP_T)) ;
		memcpy(&dscpInfo, gpQdmaPriv->rxStartPtr, sizeof(struct QDMA_DscpInfo_S)) ;
		
		gpQdmaPriv->rxStartPtr = gpQdmaPriv->rxStartPtr->next ;
		
		pktCount++ ;
		/* check DSCP cotent */
		if(!rxDscp.pkt_addr || !rxDscp.ctrl.pkt_len)
		{
			QDMA_ERR("The content of the RX DSCP is incorrect.\n") ;
			gpQdmaPriv->counters.rxDscpIncorrect++ ; 
			break ;
		}
		
		newSkb = alloc_skb(2000);
		if(newSkb == NULL) {
			newSkb = dscpInfo.skb;
			QDMA_ERR("\nalloc fail\n");
			goto next;
		}
		dscpInfo.skb->len = rxDscp.ctrl.pkt_len;

		addr = (unsigned long)dscpInfo.skb->data & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
		len = (rxDscp.ctrl.pkt_len + 2 * CONFIG_SYS_CACHELINE_SIZE ) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
		invalidate_dcache_range(addr, addr + len);
		//dump_skb(dscpInfo.skb);
		//eth_rcv(dscpInfo.skb);
#ifdef CONFIG_USE_IRQ
		eth_hdr = (struct ethhdr *)(dscpInfo.skb->data);
		if (ntohs(eth_hdr->h_proto) == ETH_P_IP)
		{
			skb_pull(dscpInfo.skb, sizeof(struct ethhdr));
			if (ip_rcv_packet(dscpInfo.skb))
			{
				skb_push(dscpInfo.skb, sizeof(struct ethhdr));
				NetReceive(dscpInfo.skb->data, dscpInfo.skb->len);
			}
		}
		else
		{
			NetReceive(dscpInfo.skb->data, dscpInfo.skb->len);
		}

		if (tftp_start == 1)
		{
			switch (net_state) {
			
			case NETLOOP_RESTART:

				goto done;
			
			case NETLOOP_SUCCESS:

				if (NetBootFileXferSize > 0) {
					printf("Bytes transferred = %ld (%lx hex)\n",
						NetBootFileXferSize,
						NetBootFileXferSize);
					setenv_hex("filesize", NetBootFileXferSize);
					setenv_hex("fileaddr", load_addr);
				}
				debug_cond(DEBUG_INT_STATE, "--- NetLoop Success!\n");
				ecnt_ImageUpgrade(0);

				goto done;
			
			case NETLOOP_FAIL:

				/* Invalidate the last protocol */
				eth_set_last_protocol(BOOTP);
				debug_cond(DEBUG_INT_STATE, "--- NetLoop Fail!\n");
				goto done;
			
			case NETLOOP_CONTINUE:
				break;
			}

		}
		
#else
		NetReceive(dscpInfo.skb->data, dscpInfo.skb->len);
#endif
		free_skb(dscpInfo.skb);
next:
		if (qdma_has_free_rxdscp())
		{
			qdma_bm_hook_receive_buffer(newSkb, 0);
		} else {
			QDMA_ERR("\nRX Error: no available QDMA RX descritor\n");
			gpQdmaPriv->counters.noRxDscps++ ;
			free_skb(newSkb);
		}
		

	} while((!maxPkts) || (--cnt)) ;

	return pktCount ;

ret:

	return pktCount ;
#else
	int i = 0;
	for(i = 0; i < gpQdmaPriv->rxDscpNum; i ++){
		memcpy(&rxDscp, gpQdmaPriv->rxStartPtr->dscpPtr, sizeof(QDMA_DMA_DSCP_T)) ;
		memcpy(&dscpInfo, gpQdmaPriv->rxStartPtr, sizeof(struct QDMA_DscpInfo_S)) ;
		gpQdmaPriv->rxStartPtr = gpQdmaPriv->rxStartPtr->next;
		if(gpQdmaPriv->rxStartPtr == NULL)
		{
			printf("gpQdmaPriv->rxStartPtr is null");

		return 0;
		}

		if(dscpInfo.skb == NULL)
			printf("skb is null");
		else{
			dscpInfo.skb->len = rxDscp.ctrl.pkt_len;
			dump_skb(dscpInfo.skb);

		}
		printf("idx is %d dscp idx is %daddr is\n", i, dscpInfo.dscpIdx);


	}
	return 0;

#endif
done:
	tftp_start = 0;
	NetBootFileXferSize = 0;
	net_set_state(NETLOOP_CONTINUE);
	net_set_udp_handler(NULL);
	net_set_arp_handler(NULL);
	NetSetTimeout(0, NULL);

	return 0;

}

static int qdma_bm_dscp_init(void)
{
	struct QDMA_DscpInfo_S *diPtr ;
	QDMA_DMA_DSCP_T *pHwDscp ;
	dma_addr_t dscpDmaAddr, irqDmaAddr, hwFwdDmaAddr ;
	dma_addr_t hwFwdBuffAddr ;
	uint virAddr=0;
	uint dscpBaseAddr, hwTotalDscpSize, hwTotalMsgSize, hwTotalPktSize ;
	uint i, base;
	int ringIdx=0 ;
	uint txDscpNum ;
	uint rxDscpNum ;
	uint hwDscpNum ;
	uint irqDepth ;
	uint hwFwdPktLen ;
	int flag=0, cnt;
#if defined(TCSUPPORT_CPU_EN7580)
	uint lmgrInitCfg = 0 ;
#endif
	unsigned long   addr, len;

	base = gpQdmaPriv->csrBaseAddr ;
	txDscpNum = gpQdmaPriv->txDscpNum ;
	rxDscpNum = gpQdmaPriv->rxDscpNum ;
	hwDscpNum = gpQdmaPriv->hwFwdDscpNum ;
	irqDepth = gpQdmaPriv->irqDepth ;
	hwFwdPktLen = gpQdmaPriv->hwPktSize ;	

	
	/******************************************
	* Allocate descriptor DMA memory          *
	*******************************************/
	dscpBaseAddr = K0_TO_K1((uint)&mac_p->macMemPool_p->descrPool[0]) ;
	dscpDmaAddr = K1_TO_PHY(dscpBaseAddr);


	//Set the TX_DSCP_BASE and RX_DSCP_BASE address
	addr = (unsigned long )&mac_p->macMemPool_p->descrPool[0] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = (CONFIG_TX0_DSCP_NUM*CONFIG_TX0_DSCP_SIZE + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len );
	
	qdmaSetTxDscpBase(base, RING_IDX_0, dscpDmaAddr) ;

	addr = (unsigned long )&mac_p->macMemPool_p->descrPool[CONFIG_TX0_DSCP_NUM*CONFIG_TX0_DSCP_SIZE] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	len = (CONFIG_RX0_DSCP_NUM*CONFIG_RX0_DSCP_SIZE + 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	flush_dcache_range(addr, addr + len );
	qdmaSetRxDscpBase(base, RING_IDX_0, (dscpDmaAddr + sizeof(QDMA_DMA_DSCP_T)*(txDscpNum))) ;	

	qdmaSetRxRingSize(base, RING_IDX_0, rxDscpNum);
	qdmaSetRxRingThrh(base, RING_IDX_0, 0);

	/******************************************
	* Allocate memory for IRQ queue           *
	******************************************/
	if(irqDepth) {

		gpQdmaPriv->irqQueueAddr = K0_TO_K1((dscpBaseAddr + sizeof(QDMA_DMA_DSCP_T)*(txDscpNum + rxDscpNum))) ;
		irqDmaAddr = K1_TO_PHY(gpQdmaPriv->irqQueueAddr);

		memset((void *)gpQdmaPriv->irqQueueAddr, CONFIG_IRQ_DEF_VALUE, irqDepth<<2) ;

		addr = (unsigned long )&mac_p->macMemPool_p->descrPool[DESC_TOTAL_SIZE] & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
		len = ((CONFIG_IRQ_DEPTH<<2)+ 2 * CONFIG_SYS_CACHELINE_SIZE) & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
		flush_dcache_range(addr, addr + len );
		
		/* Setting the IRQ queue information to QDMA register */
		qdmaSetIrqBase(base, irqDmaAddr) ;
		qdmaSetIrqDepth(base, irqDepth) ;
	}
	/***************************************************
	* Allocate memory for TX/RX DSCP Information node  *
	****************************************************/
	gpQdmaPriv->dscpInfoAddr = &DummydscpInfoAddr ;

	memset(gpQdmaPriv->dscpInfoAddr, 0, DESC_INFO_SIZE);

	gpQdmaPriv->txBaseAddr = gpQdmaPriv->dscpInfoAddr;
	gpQdmaPriv->rxBaseAddr = gpQdmaPriv->dscpInfoAddr + sizeof(struct QDMA_DscpInfo_S)*txDscpNum;

	//Create unused tx descriptor link list and using rx descriptor ring
	for(i=0 ; i<(txDscpNum + rxDscpNum) ; i++)
	{
		diPtr = (struct QDMA_DscpInfo_S *)gpQdmaPriv->dscpInfoAddr + i ;
		diPtr->dscpPtr = (QDMA_DMA_DSCP_T *)dscpBaseAddr + i ;
		
		if(i < txDscpNum) {
			diPtr->dscpIdx = i ;
			diPtr->next = NULL ;
			qdma_bm_push_tx_dscp(diPtr, 0) ;
		} else  {
			diPtr->dscpIdx = i - txDscpNum ;
			diPtr->next = NULL ;
			qdma_bm_add_rx_dscp(diPtr) ;
		}
	}	

	/***************************************************
	* Initialization first DSCP for Tx0 DMA              *
	****************************************************/
	diPtr = qdma_bm_pop_tx_dscp() ;
	if(!diPtr) {
		QDMA_ERR("There is not any free TX DSCP.\n") ; 
		return -ENOSR ;
	}
	gpQdmaPriv->txUsingPtr = diPtr ;
	qdmaSetTxCpuIdx(base, RING_IDX_0, diPtr->dscpIdx) ;
	qdmaSetTxDmaIdx(base, RING_IDX_0, diPtr->dscpIdx) ;
	
	/***************************************************
	* Initialization first DSCP for Rx0 DMA              *
	****************************************************/
	diPtr = qdma_bm_get_unused_rx_dscp() ;
	if(diPtr == NULL) {
		QDMA_ERR("There is not any free RX DSCP.\n") ;
		return -ENOSR ;
	} 
	gpQdmaPriv->rxUsingPtr = diPtr ;
	qdmaSetRxCpuIdx(base, RING_IDX_0, diPtr->dscpIdx) ;
	qdmaSetRxDmaIdx(base, RING_IDX_0, diPtr->dscpIdx) ;

	/***************************************************
	* Initialization packets for Rx DMA              *
	****************************************************/

	do {
		if(qdma_prepare_rx_buffer() != 0)
		{
			break ;
		}
	} while(qdma_has_free_rxdscp()) ;	

	/***************************************************
	* Initialization DSCP for hardware forwarding      *
	****************************************************/
	if(hwDscpNum) {

		hwTotalDscpSize = sizeof(QDMA_HWFWD_DMA_DSCP_T) * hwDscpNum ;

		hwTotalPktSize = hwFwdPktLen * hwDscpNum ;

		gpQdmaPriv->hwFwdPayloadSize = hwFwdPktLen;

		gpQdmaPriv->hwFwdBaseAddr = K0_TO_K1(gpQdmaPriv->irqQueueAddr + (CONFIG_IRQ_DEPTH<<2) );
		hwFwdDmaAddr = K1_TO_PHY(gpQdmaPriv->hwFwdBaseAddr);

		gpQdmaPriv->hwFwdBuffAddr = K0_TO_K1(gpQdmaPriv->hwFwdBaseAddr + hwTotalDscpSize) ;
		hwFwdBuffAddr = K1_TO_PHY(gpQdmaPriv->hwFwdBuffAddr);

		qdmaSetHwDscpBase(base, hwFwdDmaAddr) ;
		qdmaSetHwBuffBase(base, hwFwdBuffAddr) ;

		qdmaSetHwPayloadSize(base, HWFWD_PAYLOAD_SIZE_2K);
		qdmaSetHwLowThrshld(base, HWFWD_LOW_THRESHOLD);

#if defined(TCSUPPORT_CPU_EN7580) 
		lmgrInitCfg = qdmaGetHwInitCfg(base) ;
		lmgrInitCfg = (lmgrInitCfg | LMGR_INIT_START | hwDscpNum) ; /*set DSCP in DRAM*/
		qdmaSetHwInitCfg(base, lmgrInitCfg) ;
#else
		qdmaSetHwDscpNum(base, hwDscpNum);
		qdmaSetHWInitStart(base);
#endif

		flag = 0 ;

		cnt = 1000;
		while((cnt--) > 0) {		
			if(qdmaGetHWInitStart(base) == 0) {
				flag=1;
				break;
			}
			
		}

		if(flag == 0) {
			QDMA_ERR("hw_fwd init fail!\n") ;
			return -1;
		}
	}
	
	return 0 ;
}


/******************************************************************************
******************************************************************************/
int qdma_dev_init(void) 
{
	uint base = gpQdmaPriv->csrBaseAddr ;
	uint i=0, glbCfg=0, intEnable=0 ;
	uint int_Enable1=0, int_Enable2=0 ;
	
#ifdef TCSUPPORT_CPU_EN7580
	qdmaClearIntStatus1(base, 0xFFFFFFFF) ;
	qdmaClearIntStatus2(base, 0xFFFFFFFF) ;
#else
	qdmaClearIntStatus(base, 0xFFFFFFFF) ;	
#endif
	/********************************************
	* enable/disable the qdma interrupt         *
	*********************************************/
#ifdef TCSUPPORT_CPU_EN7580
	/*only enable rx-0 INT*/
	int_Enable1 = 0 ;
	int_Enable2 = INT_MASK_RX_DONE ;
	qdmaSetIntMask(base, int_Enable1, QDMA_INT1_ENABLE1) ;
	qdmaSetIntMask(base, int_Enable2, QDMA_INT1_ENABLE2) ;
#else
	intEnable = ( INT_STATUS_RX0_COHERENT | INT_STATUS_TX0_COHERENT 
			| INT_STATUS_HWFWD_DSCP_LOW | INT_STATUS_IRQ_FULL | INT_STATUS_HWFWD_DSCP_EMPTY| INT_STATUS_NO_RX1_CPU_DSCP 
			| INT_STATUS_NO_RX0_CPU_DSCP | INT_STATUS_RX0_DONE ) ;
	qdmaSetIntMask(base, intEnable) ;
#endif
	
	/********************************************
	* Setting the global register               *
	*********************************************/
#ifndef TCSUPPORT_LITTLE_ENDIAN
	glbCfg |= (GLB_CFG_DSCP_BYTE_SWAP | GLB_CFG_PAYLOAD_BYTE_SWAP | GLB_CFG_MSG_WORD_SWAP | ((VAL_BST_32_DWARD<<GLB_CFG_BST_SE_SHIFT)&GLB_CFG_BST_SE_MASK)) ;
#else
	glbCfg |= (GLB_CFG_PAYLOAD_BYTE_SWAP | GLB_CFG_MSG_WORD_SWAP | ((VAL_BST_32_DWARD<<GLB_CFG_BST_SE_SHIFT)&GLB_CFG_BST_SE_MASK)) ;
#endif

	glbCfg |= (PREFER_TX1_FWD_TX0<<GLB_CFG_DMA_PREFERENCE_SHIFT)&GLB_CFG_DMA_PREFERENCE_MASK;

	if(gpQdmaPriv->irqDepth) {
		glbCfg |= GLB_CFG_IRQ_EN ;
	}
	
#ifdef CONFIG_RX_2B_OFFSET
	glbCfg |= GLB_CFG_RX_2B_OFFSET ;
#endif /* CONFIG_RX_2B_OFFSET */

#ifdef CONFIG_TX_WB_DONE
	glbCfg |= GLB_CFG_TX_WB_DONE ;
#endif /* GLB_CFG_TX_WB_DONE */
	
	qdmaSetGlbCfg(base, glbCfg) ;
#ifdef TCSUPPORT_CPU_EN7580
	for(i=0; i<RX_RING_NUM; i++) {
		write_reg_word(QDMA_CSR_RX_DELAY_INT_CFG(base, i), 0);
	}
#else
	write_reg_word(QDMA_CSR_RX_DELAY_INT_CFG(base), 0);
#endif
	return 0 ;
}

int  qdma_init(void){
	int ret;
	
	/* Initial device private data */

	memset(gpQdmaPriv, 0, sizeof(QDMA_Private_T));
	
#ifdef CONFIG_DEBUG
	gpQdmaPriv->dbgLevel = DBG_ERR ;
#endif /* CONFIG_DEBUG */
	/* Initial for design and verification function */
	gpQdmaPriv->txDscpNum = CONFIG_TX0_DSCP_NUM;
	gpQdmaPriv->rxDscpNum = CONFIG_RX0_DSCP_NUM;
	gpQdmaPriv->hwFwdDscpNum = CONFIG_HWFWD_DSCP_NUM;
	gpQdmaPriv->irqDepth = CONFIG_IRQ_DEPTH;
	gpQdmaPriv->hwPktSize = CONFIG_MAX_PKT_LENS;
	//gpQdmaPriv->csrBaseAddr = QDMA_REG_BASE ;
	if(fw_upgrade_select)
		gpQdmaPriv->csrBaseAddr = CONFIG_QDMA2_BASE_ADDR;
	else
		gpQdmaPriv->csrBaseAddr = CONFIG_QDMA_BASE_ADDR;
	if((ret = qdma_bm_dscp_init()) != 0) 
	{
		QDMA_ERR("QDMA DSCP initialization failed.\n") ;
		return ret ;
	}
	/***************************************************
	* QDMA device initialization                       *
	****************************************************/
	if((ret = qdma_dev_init()) != 0) {
		QDMA_ERR("QDMA hardware device initialization failed.\n") ;
		return ret ;
	}
	qdmaEnableTxDma(gpQdmaPriv->csrBaseAddr) ;
	qdmaEnableRxDma(gpQdmaPriv->csrBaseAddr) ;

	return 0 ;
} 

extern int tftprole;
#if defined(TCSUPPORT_CPU_EN7580)
void mac_interrupt(void *args)

{
	if (tftprole)
		return;
	eth_rx();
	return;
}
#else
void mac_interrupt(int irq, void *dev_id, struct pt_regs *regs)

{

	uint intStatus, intMask ;
	uint base = gpQdmaPriv->csrBaseAddr ;
	uint32 reg;
	
#ifdef TCSUPPORT_MIPS_1004K
    set_GIC_SH_RMASK(MAC_INT);
#else
	VPint(CR_INTC_IMR) &= ~(1<<MAC_INT);
#endif
	intMask = qdmaGetIntMask(base) ;
	intStatus = qdmaGetIntStatus(base) & intMask ;
	QDMA_MSG(DBG_WARN, "QDMA interrupt service routine is return, Status:%.8x, Mask:%.8x\n", qdmaGetIntStatus(base), qdmaGetIntMask(base)) ;
	
	if(intStatus & (INT_STATUS_RX0_DONE | INT_STATUS_NO_RX0_CPU_DSCP)) {
		qdmaClearIntStatus(base, INT_STATUS_RX0_DONE | INT_STATUS_NO_RX0_CPU_DSCP) ;
    	#if defined(DRAM_PROTECT_TEST)
    	nmi_mem_protect_test();
    	#endif
		qdma_bm_receive_packets(16,0);
	}


	if(intStatus & INT_STATUS_QDMA_FAULT) {
		QDMA_MSG(DBG_ERR, "err qdma INT_STATUS=%08lx\n", intStatus);
	}
#ifdef TCSUPPORT_MIPS_1004K
    set_GIC_SH_SMASK(MAC_INT);
#else
	VPint(CR_INTC_IMR) |= (1<<MAC_INT);
#endif
	return;
}
#endif

void detect_switch()
{
    uint32 switch_chip_id = 0;

    switch_chip_id = read_reg_word(GSW_CFG_CREV);
    if((switch_chip_id & 0xFFFF0000) == 0x75300000){
        use_ext_switch = 0;
    }
    switch_chip_id = gswPbusRead(EXT_GSW_CFG_CREV);
    if((switch_chip_id & 0xFFFF0000) == 0x75300000){
        use_ext_switch = 1;
    }
}

int tc3162_eth_init(unsigned char *mac_addr)
{
	return 0;
}

int tc3162_eth_exit(void)
{
	return 0;
}


int ecnt_init_handler(void){
	struct eth_device* dev = NULL;
	bd_t* bis = NULL;


	ecnt_eth_init(dev, bis);
	ecnt_eth_recv(dev);
	

	return 0;
}

struct serdes_fwd_info fwd_info[SERDES_INTF_MAX] = 
{
    {GDM_P_GDMA4,13,0},
    {GDM_P_GDMA4,12,1},
    {GDM_P_GDMA3,10,4},
    {GDM_P_GDMA3,11,5},
    {0,0,0},        
};

static int ecnt_eth_check_serdes(unsigned int* p_fport,unsigned int* p_chn,unsigned int* p_nbq)
{
    #ifdef TCSUPPORT_CPU_EN7581		
	/* use upgrade_serdes_sel to choose upgrade serdes */
	
    unsigned char* serdes_intf[SERDES_INTF_MAX];
    int i = 0;
	ulong upgrade_serdes = 0;
	int len_eth = 0;
    
    serdes_intf[SERDES_ETH] = getenv("serdes_ethernet");
    serdes_intf[SERDES_USB] = getenv("serdes_usb1");
    serdes_intf[SERDES_PCIE1] = getenv("serdes_wifi1");
    serdes_intf[SERDES_PCIE2] = getenv("serdes_wifi2");

	upgrade_serdes = getenv_ulong("fw_port", NULL, FW_PORT_DEFAULT);
	len_eth = strlen(serdes_intf[SERDES_ETH]);

	//use switch to upgrade fw
	if((upgrade_serdes == 0) || (upgrade_serdes >= SERDES_INTF_MAX+1))
		return 0;

	if(len_eth < 2)
	{
		printf("ecnt_eth_check_serdes error eth_len=%d\n",len_eth);
		return 0;
	}

    for(i = 0;i < SERDES_INTF_MAX;i++)
    {
        if((serdes_intf[i][len_eth-2] == SERDES_ETHERLAN) && (upgrade_serdes == (i+1)))
        {
            *p_fport = fwd_info[i].fport;
            *p_chn = fwd_info[i].chn;
			*p_nbq = fwd_info[i].nbq;
            return 1;
        }
    }
    #endif

    return 0;
}

static void ecnt_eth_check_serdes_pon(void)
{
	fw_upgrade_select = getenv_ulong("fw_upgrade_select", NULL, FW_UPGRADE);
	return ;
}
static int ecnt_eth_send(struct eth_device* dev, void *packet, int length)
{
	sk_buff *skb_temp;
    unsigned int fport = 0;
    unsigned int chn = 0;
	unsigned int nbq = 0;
    
	if (packet == NULL)
	{
		return 1;
	}

	skb_temp = alloc_skb(length);
	skb_temp->data = packet;
	skb_temp->len = length;


		uint32 len = skb_temp->len;
		ethTxMsg_t ethTxMsg;
		int error;
	
		if (len < 60) {
			len = 60;
		}
	
	  // start filling tx message
	  /* GDMA1 */
		memset(&ethTxMsg, 0, sizeof(ethTxMsg_t));
        
		ethTxMsg.raw.queue =0;
#ifdef TCSUPPORT_CPU_EN7580
		ethTxMsg.raw.nboq = 0;
		ethTxMsg.raw.mtr_g = 0x7f; /*not use any meter ratelimit*/
#endif
	if(ecnt_eth_check_serdes(&fport, &chn, &nbq))
        {
		    ethTxMsg.raw.fport = fport;
		    ethTxMsg.raw.channel = chn;
		ethTxMsg.raw.nboq = nbq;
        }
        else
		{
			if(fw_upgrade_select)
				ethTxMsg.raw.fport = GDM_P_GDMA2;
			else
		    	ethTxMsg.raw.fport = GDM_P_GDMA1;
		    ethTxMsg.raw.channel = 0;
        }

	error = qdma_bm_transmit_packet(skb_temp, 0,ethTxMsg.msg[0],ethTxMsg.msg[1]) ;

		if(error){
			free_skb(skb_temp);
			//prom_printf("qdma tranmit fail\n");
			return 0;
		 }

	return 0;
}


static int ecnt_eth_recv(struct eth_device* dev)
{
		
		uint intStatus1=0, intStatus2=0, ringIdx=0 ;
		uint base = gpQdmaPriv->csrBaseAddr ;
		uint32 reg;

#ifdef CONFIG_USE_IRQ
#ifdef TCSUPPORT_MIPS_1004K
		set_GIC_SH_RMASK(MAC_INT);
#elif TCSUPPORT_CPU_EN7523
		if(fw_upgrade_select)
			gic_dic_set_icenable(MAC_INT_WAN);
		else
			gic_dic_set_icenable(MAC_INT);
#else
		VPint(CR_INTC_IMR) &= ~(1<<MAC_INT);
#endif
#endif
		intStatus1 = qdmaGetIntStatus1(base) & qdmaGetIntMask(base, QDMA_INT1_ENABLE1) ;
		intStatus2 = qdmaGetIntStatus2(base) & qdmaGetIntMask(base, QDMA_INT1_ENABLE2) ;

		if((intStatus2 & INT_STATUS_RX_DONE)) {
			qdmaClearIntStatus2(base, (intStatus2 & INT_STATUS_RX_DONE)) ;

	    #if defined(DRAM_PROTECT_TEST)
			nmi_mem_protect_test();
	    #endif
			qdma_bm_receive_packets(16,0);
		}
		if(intStatus1 & INT_STATUS_QDMA_FAULT) {
			QDMA_MSG(DBG_MSG, "err qdma INT_STATUS=%08lx\n", intStatus1);
		}
#ifdef CONFIG_USE_IRQ
#ifdef TCSUPPORT_MIPS_1004K
		set_GIC_SH_SMASK(MAC_INT);
#elif TCSUPPORT_CPU_EN7523
		if(fw_upgrade_select)
			gic_dic_set_enable(MAC_INT_WAN);
		else
			gic_dic_set_enable(MAC_INT);
#else
		VPint(CR_INTC_IMR) |= (1<<MAC_INT);
#endif
#endif
		return 0 ;

}

void ecnt_eth_halt(struct eth_device* dev)
{

}

static int ecnt_eth_init(struct eth_device* dev, bd_t* bis)
{
	unsigned iomux = 0, iomux_ori = 0;
	struct mii_dev *ecnt_mii_dev = NULL;
	struct phy_device *ecnt_phy_device = NULL;
	
	if(ecnt_eth_initd == 0 ){
		int i;
		uint8 k;
		int err;
		uint32 reg_val = 0;
		uint32	efuse_bit31_0_val, efuse_bit63_32_val;

		ecnt_eth_check_serdes_pon();   /* determine whether to init qdma_lan or qdma_wan*/

		/* disable & store Ether PHY LED */
		iomux_ori = read_reg_word(IOMUX_CONTROL1);
		iomux = iomux_ori & ~(0xFF << 3);
		write_reg_word(IOMUX_CONTROL1, iomux);
		
		macResetSwMAC(); 
		setPortIsolate();
		setEtherRateLimit (0);
		detect_switch();

		if(use_ext_switch){
			gswPbusWrite(0x7000, 0x3); /* reset external switch*/
			macCfgExtSwitch();
		}

		VPint(SHARE_FEMEM_SEL) = 0;

		mac_p->macMemPool_p = &enetMemPool;
		
		if(!isFPGA)
			eth_phy_init(mac_p);


		/* restore Ether PHY LED iomux */
		write_reg_word(IOMUX_CONTROL1, iomux_ori);
				
		pause(1000);
		skb_init();
		qdma_init(); //seems OK
		macGetMacAddr(mac_p, mac_addr);
		macDrvRegInit(mac_p);
		pause(100);

		for(i=0;i<7;i++){
			reg_val = VPint(GSW_MAC_BASE+i*0x100);
			reg_val &=0xffffffcf;/*Disable force TX/RX FC*/
			VPint(GSW_MAC_BASE+i*0x100)=reg_val;
		}	

		if(use_ext_switch){
			gswPbusWrite(0xA4,0x08160816);
			gswPbusWrite(0x1C,0x68166816);
			gswPbusWrite(0x20,0x08160816);
			gswPbusWrite(0x24,0x08160816);
		}else{
			write_reg_word(GSW_BASE + 0xA4,0x08160816);
			write_reg_word(GSW_BASE + 0x1C,0x68166816);
			write_reg_word(GSW_BASE + 0x20,0x08160816);
			write_reg_word(GSW_BASE + 0x24,0x08160816);
		}


		write_reg_word(GSW_MFC, 0xffffffe0);
		write_reg_word(GSW_APC, 0x08100810);

		//set I/O driving strength
		gswPbusWrite(0x7810,0x00010022);
		//printf("\r\nreg 0x7810 value is %08x\r\n",gswPbusRead(0x7810));
#ifdef CONFIG_USE_IRQ
		if(fw_upgrade_select)
			irq_register(MAC_INT_WAN, mac_interrupt, 0);
		else
			irq_register(MAC_INT, mac_interrupt, 0);
#endif

		if(isFPGA){
#if defined(TCSUPPORT_CPU_EN7523)
			write_reg_word(PSE_OQ_RSV, 0x300); /* EN7581 FPGA PSE buffer only 128K */
#endif
		}
		//init serdes phy
		if(!isFPGA)
			serdes_phy_init();

		ecnt_mii_dev = miiphy_get_dev_by_name(dev->name);
		if(ecnt_mii_dev)
			ecnt_phy_device = phy_connect(ecnt_mii_dev, AN8811H_PHYADDR, dev, 0) ;
		if(ecnt_phy_device)
		{
			phy_config(ecnt_phy_device);
		}
	}


	ecnt_eth_initd = 1;

	return (1);
}
static int ecnt_iomux_reg_op(int op)
{
	static uint32_t reg01;
	static uint32_t reg02;
	static uint32_t reg03;

	if (op == OP_SAVE) {
		reg01 = read_reg_word(0x1FA20210);
		reg02 = read_reg_word(0x1FA20214);
		reg03 = read_reg_word(0x1FA20218);

		/* update register */
		write_reg_word(0x1FA20210, reg01 & (~((1<<1)|(1<<2))));
		write_reg_word(0x1FA20214, reg02 & (~((1<<20)|(1<<0))));
		VPint(0x1FA20218) |= (1<<13);
		VPint(0x1FA20218) &= (~(1<<3));
	} else if (op == OP_RESTORE) {
		write_reg_word(0x1FA20210, reg01);
		write_reg_word(0x1FA20214, reg02);
		write_reg_word(0x1FA20218, reg03);
	} else {
		printf("Error! %s().%d\n", __func__, __LINE__);
	}

	return 0;
}

static int ecnt_miiphyread(
	const char *devname,
	uchar addr,
	uchar reg,
	ushort *val
)
{
	if(isEN7523)
		ecnt_iomux_reg_op(OP_SAVE);

	*val = miiStationRead(addr, reg);

	if(isEN7523)
		ecnt_iomux_reg_op(OP_RESTORE);
	return 0;
}

static int ecnt_miiphywrite(
	const char *devname,
	uchar addr,
	uchar reg,
	ushort val
)
{
	if(isEN7523)
		ecnt_iomux_reg_op(OP_SAVE);

	miiStationWrite(addr, reg, val);

	if(isEN7523)
		ecnt_iomux_reg_op(OP_RESTORE);

	return 0;
}


int ecnt_eth_initialize(bd_t *bis)
{
	struct	eth_device* 	dev;
	int	i;

	if (!(dev = (struct eth_device *) malloc (sizeof *dev))) {
		printf("Failed to allocate memory\n");
		return 0;
	}
	
	memset(dev, 0, sizeof(*dev));

	sprintf(dev->name, "ecnt_eth");

	dev->iobase = 0;
	dev->init   = ecnt_eth_init;
	dev->halt   = ecnt_eth_halt;
	dev->send   = ecnt_eth_send;
	dev->recv   = ecnt_eth_recv;

	eth_register(dev);

	ecnt_eth_initd = 0;

#ifdef CONFIG_CMD_MII
	miiphy_register(dev->name, ecnt_miiphyread, ecnt_miiphywrite);
#endif

	return 1;
}


