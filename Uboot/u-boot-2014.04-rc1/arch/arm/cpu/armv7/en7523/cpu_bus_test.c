
#include <common.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/arch/spl.h>
#include <asm/arch/typedefs.h>
#include <asm/arch/en7523.h>
#include <asm/arch/ecnt_timer.h>
#include <xyzModem.h>
#include <flashhal.h>

#ifdef TEST_BLOCK_CNT_IN_SRAM
#define RBUS_CORE_BASE  (0x1FA00000)
#define CR_BLOCK_EN     (RBUS_CORE_BASE+0x0ec)
#define CR_BLOCK_MASK   (RBUS_CORE_BASE+0x0f0)
#define CR_BLOCK_CNT0   (RBUS_CORE_BASE+0x0f4)  /* gdma,hsdma, woe,wdma,crypto,... */
#define CR_BLOCK_CNT1   (RBUS_CORE_BASE+0x0f8)  /* ppe */
#define CR_BLOCK_CNT2   (RBUS_CORE_BASE+0x0fc)  /* qdma_lan */
#define CR_BLOCK_CNT3   (RBUS_CORE_BASE+0x100)  /* qdma_wan */
#define CR_BLOCK_CNT4   (RBUS_CORE_BASE+0x104)  /* tdma */
#define CR_BLOCK_CNT5   (RBUS_CORE_BASE+0x108)  /* npu */

#define CR_AXI2RBUS_CFG         (0x1EFBC800)
#define CR_RBUS_PENDING_CNT     (CR_AXI2RBUS_CFG+0x10)

#define RBUS_PENDING_ADDR_SHIFT (24)
#define RBUS_PENDING_ADDR_BITS  (0xff)

#define CR_GDMA_BASE    (0x1FB30000)
#define CR_GDMA_SA0     (CR_GDMA_BASE+0x000)
#define CR_GDMA_DA0     (CR_GDMA_BASE+0x004)
#define CR_GDMA_CT00    (CR_GDMA_BASE+0x008)
#define CR_GDMA_CT10    (CR_GDMA_BASE+0x00C)
#define CR_GDMA_DONE    (CR_GDMA_BASE+0x204)

#define CR_NPU_MIB8 (0x1ec0c160)
#define CR_NPU_MIB9 (0x1ec0c164)
#define CR_NPU_MIB10 (0x1ec0c168)
#define CR_NPU_MIB11 (0x1ec0c16c)
#define CR_NPU_MIB12 (0x1ec0c170)

#define NPU_BLK_CNT_LOCK (0x1)
#define REG_CORE_BOOT_CONFIG 0x1ec08004
#define REG_CORE_BOOT_TRIGGER 0x1ec08000
#define REG_CORE0_BOOT_BASE 0x1ec08020

#define DRAM_TEST_BASE  (0x84000000) /* 4000000: 64MB */
#define NPU_SRAM_BASE	(0x1E800000)
#define NPU_16K_SRAM_BASE	(0x1E900000)
#define DMA_BLK_CASE    (2)
#define CPU_WR_CASE     (3)
#define DELAY_ITEM      (2)

#define CR_ARMCPU_CFG       (0x1FBE2E08)

#define FE_BASE         (0x1fb50000)
#define QDMA_LAN_BASE   (FE_BASE+0x4000)
#define QDMA_REG_OFFSET (0x2000)
#define QDMA_BUF_OFFSET (0x02000000)

#define QDMA_GLB_CFG    (0x004+QDMA_LAN_BASE)
#define FDSCP_BASE_CFG  (0x010+QDMA_LAN_BASE)
#define FBUF_BASE_CFG   (0x014+QDMA_LAN_BASE)
#define INT_STATUS      (0x020+QDMA_LAN_BASE)
#define INT_STATUS2     (0x024+QDMA_LAN_BASE)
#define INT_ENABLE      (0x028+QDMA_LAN_BASE)
#define INT_ENABLE2     (0x02c+QDMA_LAN_BASE)
#define IRQ_BASE        (0x050+QDMA_LAN_BASE)
#define IRQ_CFG         (0x054+QDMA_LAN_BASE)
#define IRQ_CLRLEN      (0x058+QDMA_LAN_BASE)
#define IRQ_STATUS      (0x05c+QDMA_LAN_BASE)
#define IRQ_PTIME       (0x060+QDMA_LAN_BASE)
#define TXQ_CFG_BASE    (0x100+QDMA_LAN_BASE)
#define RXQ_CFG_BASE    (0x200+QDMA_LAN_BASE)
#define DBGCNT_CFG      (0x400+QDMA_LAN_BASE)
#define LMGR_INIT_CFG   (0x1000+QDMA_LAN_BASE)
#define DSCP_AVAI_NUM   (0x10f0+QDMA_LAN_BASE)

#define INTRQ_BASE      (0x01000000+DRAM_TEST_BASE)
#define RDSCP_BASE      (0x01020000+DRAM_TEST_BASE)
#define TDSCP_BASE      (0x01080000+DRAM_TEST_BASE)
#define FDSCP_BASE      (0x010c0000+DRAM_TEST_BASE)
#define RXPKT_BASE      (0x01100000+DRAM_TEST_BASE)
#define TXPKT_BASE      (0x01500000+DRAM_TEST_BASE)
#define FWBUF_BASE      (0x02000000+DRAM_TEST_BASE)

#define rxDSCP_NUM      (2048)
#define rxDSCP_HOOK_NUM (32)
#define RX_PKT_MAX_SIZE (2048)

#define LITTLE_ENDIAN   (1)
//#define QDMA_DEBUG      (1)

#define XMODEM_BUFFER_SIZE	(SZ_128)

enum blockCntTestCase
{
    bctc_gdma=0,
    bctc_ppe,
    bctc_qdma_lan,
    bctc_qdma_wan,
    bctc_tdma,
    bctc_npu,
    bctc_gdma_qdmalan,
    bctc_max_case
};

char *dmaNames[7] = {"gdma", "ppe", "qdma_lan", "qdma_wan", "tdma", "npu", "gdma_qdmalan"};
unsigned int g_reg_off=0, g_buf_off=0;
int isQdmaLanInit=0;
#if defined(CONFIG_TPL_BUILD)
extern int getcxmodem(void);
#endif


int qdma_loopback_init(int isWan)
{
    unsigned int dscp_idx;
    unsigned int dscp0=0, dscp1, dscp2, dscp3;
    unsigned int rdscp_addr;
    unsigned int reg;

    if (isWan) {
        g_reg_off=QDMA_REG_OFFSET;
        g_buf_off=QDMA_BUF_OFFSET;
    }
    else { /* qdma_lan */
        g_reg_off=0;
        g_buf_off=0;
        if (isQdmaLanInit==1)
            return 0;
    }

    /* 0x1FB00958[0] == 0 for QDMA registers, == 1 for FE_SRAM */
    if ((regRead32(0x1FB00958)&0x1)!=0) {
        printf("ERROR: regRead32(0x1FB00958):0x%x bit0 is not 0\n", regRead32(0x1FB00958));
        return -1;
    }

    /* qdma_init */

    regWrite32(TXQ_CFG_BASE+0x0+g_reg_off, TDSCP_BASE+g_buf_off);
    regWrite32(TXQ_CFG_BASE+0x4+g_reg_off, 1);
    regWrite32(TXQ_CFG_BASE+0x8+g_reg_off, 0); /* CTX */
    regWrite32(TXQ_CFG_BASE+0xC+g_reg_off, 0); /* DTX */

    regWrite32(RXQ_CFG_BASE+0x0+g_reg_off, RDSCP_BASE+g_buf_off);
    regWrite32(RXQ_CFG_BASE+0x4+g_reg_off, rxDSCP_NUM);
    regWrite32(RXQ_CFG_BASE+0x8+g_reg_off, 0); /* CRX */
    regWrite32(RXQ_CFG_BASE+0xC+g_reg_off, 0); /* DRX */
    regWrite32(RXQ_CFG_BASE+0x10+g_reg_off, 0x410);

    regWrite32(FDSCP_BASE_CFG+g_reg_off, FDSCP_BASE+g_buf_off);
    regWrite32(FBUF_BASE_CFG+g_reg_off, FWBUF_BASE+g_buf_off);
    regWrite32(INT_ENABLE+g_reg_off, 0);
    regWrite32(INT_ENABLE2+g_reg_off, 0);
    regWrite32(IRQ_BASE+g_reg_off, INTRQ_BASE+g_buf_off);
    regWrite32(IRQ_CFG+g_reg_off, 0x00100400);
    regWrite32(IRQ_PTIME+g_reg_off, 0x00000022);
    regWrite32(LMGR_INIT_CFG+g_reg_off, 0x91801000);
    udelay(10000);

    regWrite32(QDMA_GLB_CFG+g_reg_off, 0x840100c5); /* QDMA_INIT */

    #ifdef QDMA_DEBUG
    /* enable debug counters */
    regWrite32(DBGCNT_CFG+g_reg_off, 0xf0000000); /* set dbg_ctx_done */
    regWrite32(DBGCNT_CFG+0x8+g_reg_off, 0xf1000000); /* set dbg_ftx_done */
    regWrite32(DBGCNT_CFG+0x10+g_reg_off, 0xf2000000); /* set dbg_crx_done */
    #endif

    reg = DSCP_AVAI_NUM+g_reg_off;
    if (regRead32(reg)!=0xff8) {
        printf("ERROR: qdma_init failed due to regRead32(0x%x):0x%x !=0xff8\n", reg, regRead32(reg));
        return -1;
    }

    /* qdma rx_dscp hook */

    for (dscp_idx=0, dscp1=RX_PKT_MAX_SIZE, dscp3=0; dscp_idx<=(rxDSCP_HOOK_NUM+1); dscp_idx++) { /* workaround: rx needs to hook one more dscp */

        rdscp_addr = RDSCP_BASE + (dscp_idx<<5) +g_buf_off;
        dscp2 = RXPKT_BASE + (dscp_idx*RX_PKT_MAX_SIZE) +g_buf_off;
        
        regWrite32(rdscp_addr+0x0, dscp0); /* write dscp0 */
        regWrite32(rdscp_addr+0x4, dscp1); /* write dscp1 (max_rx_receive_len. For loopback, it's about 100 bytes) */
        regWrite32(rdscp_addr+0x8, dscp2); /* write dscp2 */
        regWrite32(rdscp_addr+0xc, dscp3); /* write dscp3 */
        asm volatile ("dsb sy");
        regWrite32(RXQ_CFG_BASE+0x8+g_reg_off, dscp_idx+1); /* update CRX */
    }

    if (isWan==0)
        isQdmaLanInit=1;

    return 0;
}

void qdma_tx_dscp_hook(unsigned int dscp_idx, unsigned int src_addr, unsigned int test_len)
{
    unsigned int dscp0=0, dscp1, dscp2, dscp3;
    unsigned int tdscp_addr;
    unsigned int dma_chnl=0, dma_queue=0;
    unsigned int tmsg;
    unsigned int tpkt_buf, rpkt_buf;
    int res, i;

    tdscp_addr = TDSCP_BASE + (dscp_idx<<5) +g_buf_off;
    tmsg = (dma_chnl<<3) + dma_queue;

    dscp1 = test_len;
    dscp2 = src_addr;
    dscp3 = dscp_idx + 1;

    regWrite32(tdscp_addr+0x0, dscp0); /* write dscp0 */
    regWrite32(tdscp_addr+0x4, dscp1); /* write dscp1 */
    regWrite32(tdscp_addr+0x8, dscp2); /* write dscp2 */
    regWrite32(tdscp_addr+0xc, dscp3); /* write dscp3 */
    regWrite32(tdscp_addr+0x10, tmsg); /* write tmsg */
    asm volatile ("dsb sy");

}

void trigger_qdma(unsigned int dscp_idx)
{
    regWrite32(TXQ_CFG_BASE+0x8+g_reg_off, dscp_idx + 1); /* update CTX */
    #ifdef QDMA_DEBUG
    printf("dscp_idx:%d\n", dscp_idx);
    /* read debug counters */
    printf("\tqdma fetches  0x%x desp\n", regRead32(DBGCNT_CFG+0x4+g_reg_off)); /* read dbg_ctx_done */
    printf("\tqdma fetches  0x%x pkt\n", regRead32(DBGCNT_CFG+0xC+g_reg_off)); /* read dbg_ftx_done */
    printf("\tqdma receives 0x%x pkt\n", regRead32(DBGCNT_CFG+0x14+g_reg_off)); /* read dbg_crx_done */
    printf("\tregRead32(0x%x):0x%x\n", RXQ_CFG_BASE+0xC+g_reg_off, regRead32(RXQ_CFG_BASE+0xC+g_reg_off));
    #endif
    return;
}

int qdma_blkCntTest_config (int d, unsigned int *dscp_idx_p, unsigned int dscp_idx_backup, int masks)
{
    int res=0;

    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)||(d==bctc_gdma_qdmalan)) {
    
        /* qdma_lan needs to handle 2 test cases, so make sure it hooks enough rx dscps */
        if (rxDSCP_HOOK_NUM < (((DMA_BLK_CASE*masks*CPU_WR_CASE)<<1)+2)) {
            printf("ERROR: %s need to hook more qdma rx dscps\n", dmaNames[d]);
            return -1;
        }

        *dscp_idx_p=0;
        if ((dscp_idx_backup!=0) && ((d==bctc_qdma_lan)||(d==bctc_gdma_qdmalan))) {
            /* if using qdma_lan for the 2nd time, uses next dscp_idx_p */
            *dscp_idx_p = dscp_idx_backup;
        }
        
        if (d==bctc_qdma_wan)
            res = qdma_loopback_init(1);
        else { /* bctc_qdma_lan and bctc_gdma_qdmalan */
            res = qdma_loopback_init(0);
        }
    }
    
    return res;
}

void enable_block_dma_mechanism(u32 enable)
{
    u32 reg = CR_BLOCK_EN;
    u32 val = regRead32(reg);

    if(enable)
        val |= 0x1;
    else
        val &= (~0x1);

    regWrite32(reg, val);
}

void set_block_dma_mask(u32 mask)
{
    u32 reg = CR_BLOCK_MASK;
    u32 old_mask = regRead32(reg);
    u32 val= 0xffffffff;

    val &= (~mask);
    regWrite32(reg, val);

    printf("old_blk_mask:0x%x, new_blk_mask:0x%x\n", old_mask, regRead32(reg));
}

u32 get_block_dma_counter(int cnt_no)
{
    if ((cnt_no>=bctc_gdma_qdmalan) || (cnt_no<0)) {
        printf("cnt_no:%d is wrong! Correct value: 0~5\n", cnt_no);
        return 0;
    }

    return regRead32(CR_BLOCK_CNT0+(cnt_no<<2));
}

void enable_rbus_pending(int enable)
{
    u32 reg = CR_AXI2RBUS_CFG;
    u32 val = regRead32(reg);

    if (enable)
        val |= (1<<1);
    else
        val &= (~(1<<1));
    
    regWrite32(reg, val);
    return;
}

void set_rbus_pending_addr(u32 addr)
{
    u32 val, val2;
    u32 reg = CR_AXI2RBUS_CFG;
    
    val = regRead32(reg);
    val2 = val;
    
    val &= (~(RBUS_PENDING_ADDR_BITS<<2));
    val |= (((addr>>RBUS_PENDING_ADDR_SHIFT)&RBUS_PENDING_ADDR_BITS)<<2);

    regWrite32(reg, val);
    
    printf("old rbus_pending_addr:0x%lx  new rbus_pending_addr:0x%lx\n",
                ((val2>>2)&RBUS_PENDING_ADDR_BITS)<<RBUS_PENDING_ADDR_SHIFT, 
                ((regRead32(reg)>>2)&RBUS_PENDING_ADDR_BITS)<<RBUS_PENDING_ADDR_SHIFT);
    return;
}

/* set pending time for matched address, 0 means forever */
void set_rbus_pending_cnt(u32 cnt_val)
{        
    regWrite32(CR_RBUS_PENDING_CNT, cnt_val);   
    return;
}

void set_gdma_config(u32 channel, u32 sa, u32 da, u32 ct0, u32 ct1)
{
    regWrite32(CR_GDMA_SA0+(channel<<4), sa);
    regWrite32(CR_GDMA_DA0+(channel<<4), da);
    regWrite32(CR_GDMA_CT10+(channel<<4), ct1);
    if (ct0&(1<<1)) /* enable bit */
        asm volatile ("dsb sy");
    regWrite32(CR_GDMA_CT00+(channel<<4), ct0);
    
    #if 0
    printf("CR_GDMA_SA0(0x%x):0x%x\n", CR_GDMA_SA0, regRead32(CR_GDMA_SA0));
    printf("CR_GDMA_DA0(0x%x):0x%x\n", CR_GDMA_DA0, regRead32(CR_GDMA_DA0));
    printf("CR_GDMA_CT10(0x%x):0x%x\n", CR_GDMA_CT10, regRead32(CR_GDMA_CT10));
    printf("CR_GDMA_CT00(0x%x):0x%x\n", CR_GDMA_CT00, regRead32(CR_GDMA_CT00));
    #endif
    
    return;
}

void trigger_gdma(u32 src, u32 dst, u32 len, u32 ch, u32 burst)
{
    int coherent_en = 0;

    //printf("GDMA moves data from src:0x%x to dst:0x%x (len:0x%x)\n", src, dst, len);

    set_gdma_config(ch, src, dst, ((len&0xffff)<<16)|(burst<<3)|(1<<1)|(1<<0), (coherent_en<<2));
   
    return;
}

void trigger_npu(u32 src, u32 dst, u32 len)
{
	while ((regRead32(CR_NPU_MIB8)!=2))
	{
		printf("wait npu ready ! CR_NPU_MIB8 is %d\n",regRead32(CR_NPU_MIB8));
		mdelay(10);
	}
	regWrite32(CR_NPU_MIB9, src);
	regWrite32(CR_NPU_MIB10, dst);
	regWrite32(CR_NPU_MIB11, len);
	regWrite32(CR_NPU_MIB8, 3);
	mdelay(100);
    return;
}

void wait_gdma_done(u32 channel)
{
    /* wait until GDMA is done */
    while((regRead32(CR_GDMA_DONE) & (1<<channel))== 0) ;
    /* clear done bit */
    regWrite32(CR_GDMA_DONE, (1<<channel)); 
    return;
}

void wait_npu_done (void)
{
	/* wait until NPU is done*/
	while ((regRead32(CR_NPU_MIB8)!=2))
	{
		mdelay(10);
	}
	return;
}

void wakeup_npu_core0(void)
{
	regWrite32(REG_CORE0_BOOT_BASE, NPU_SRAM_BASE);
	regWrite32(REG_CORE_BOOT_CONFIG,1);
	regWrite32(REG_CORE_BOOT_TRIGGER,1);
	
	/* wait NPU up*/
	mdelay(100);
	
	/* set npu test case*/
	regWrite32(CR_NPU_MIB8,45);

}

void show_block_dbg_regs(void)
{
    unsigned int offset;

    printf("%s:\n", __func__);
    for (offset=0x110; offset<=0x134; ) {
        printf("\t0x%x: 0x%x\n", RBUS_CORE_BASE+offset, regRead32(RBUS_CORE_BASE+offset));
        offset+=4;
    }
    
    return;
}

/* phase==1: before gdmq data moving,  phase==2: after gdmq data moving*/
int check_dma_block_cnt(int d, int e, int i, int phase)
{
    u32 cur_block_cnt=0, pre_block_cnt=0, block_cnt_diff;
    u32 cur_block_cnt2=0, pre_block_cnt2=0, block_cnt_diff2;
    int count_down=3;
    int dma;

    if (d==bctc_gdma_qdmalan)
        dma = bctc_gdma;
    else
        dma = d;

    if ((phase==1) && (e==1) && (i<2)) {
        
        /* make sure that dma_block_cnt is counting */
        cur_block_cnt = get_block_dma_counter(dma);
        if (d==bctc_gdma_qdmalan)
            cur_block_cnt2 = get_block_dma_counter(bctc_qdma_lan);
            
        while(1) {
            udelay(1);
            pre_block_cnt = cur_block_cnt;
            cur_block_cnt = get_block_dma_counter(dma);
            block_cnt_diff = cur_block_cnt-pre_block_cnt;
            block_cnt_diff2 = 1; /* for easier comparison later */
            if (d==bctc_gdma_qdmalan) {
                pre_block_cnt2 = cur_block_cnt2;
                cur_block_cnt2 = get_block_dma_counter(bctc_qdma_lan);
                block_cnt_diff2 = cur_block_cnt2-pre_block_cnt2;
            }
            if ((block_cnt_diff>0) && (block_cnt_diff2>0))
                break;
            else {
                if (count_down>0) {
                    count_down--;
                }
                else {
                    printf("ERROR1: %s_block_cnt is not counting\n", dmaNames[d]);
                    printf("  --pre_block_cnt:0x%x  cur_block_cnt:0x%x\n", pre_block_cnt, cur_block_cnt);
                    if (d==bctc_gdma_qdmalan)
                        printf("  --pre_block_cnt2:0x%x  cur_block_cnt2:0x%x\n", pre_block_cnt2, cur_block_cnt2);
                    return -1;
                }
            }
        }

        printf("%s block cnt:0x%x in 1 us\n", dmaNames[dma], block_cnt_diff);
        if (d==bctc_gdma_qdmalan)
            printf("qdma_lan block cnt:0x%x in 1 us\n", block_cnt_diff2);
    }
    else { /* make sure that dma_block_cnt won't count */

        pre_block_cnt = get_block_dma_counter(dma);
        if (d==bctc_gdma_qdmalan) {
            pre_block_cnt2 = get_block_dma_counter(bctc_qdma_lan);
        }
        udelay(100);
        cur_block_cnt = get_block_dma_counter(dma);
        if (d==bctc_gdma_qdmalan) {
            cur_block_cnt2 = get_block_dma_counter(bctc_qdma_lan);
        }
        
        if ((pre_block_cnt != cur_block_cnt) || (pre_block_cnt2 != cur_block_cnt2)) {
            printf("\nERROR2: [%s] pre_block_cnt:0x%x != cur_block_cnt:0x%x\n", 
                    dmaNames[dma], pre_block_cnt, cur_block_cnt);
            if (d==bctc_gdma_qdmalan)
                printf("\tqdma_lan pre_block_cnt2:0x%x != cur_block_cnt2:0x%x\n", pre_block_cnt2, cur_block_cnt2);
            return -1;
        }
        
        //printf("dma block cnt is not counting which is correct\n");
    }


    return 0;
}

int read_compare_data(int d, int e, int i, u32 cmp_addr, u32 cmp_val, int flag)
{
    unsigned dst_data;

    if (d==bctc_gdma_qdmalan) {
        if (flag==0)
            d=bctc_gdma;
        else
           d=bctc_qdma_lan; 
    }

    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)) { /* qdma will move data to rxBuf+2 */
        #ifdef LITTLE_ENDIAN
        dst_data = ((regRead32(cmp_addr+4)&0xffff)<<16) | ((regRead32(cmp_addr)>>16)&0xffff);
        #else
        dst_data = ((regRead32(cmp_addr)&0xffff)<<16) | ((regRead32(cmp_addr+4)>>16)&0xffff);
        #endif
    }
    else
        dst_data = regRead32(cmp_addr);

    if ((e==1) && (i<2)) { /* when block is enabled, dma-read should happen "after" cpu-write, so comparison should succeed */
        if (dst_data != cmp_val) {
            printf("ERROR3: regRead32(0x%x):0x%x != cmp_val:0x%x\n", cmp_addr, dst_data, cmp_val);
            if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)) {
                printf("\tNote: regRead32(0x%x):0x%x  regRead32(0x%x):0x%x\n", 
                        cmp_addr, regRead32(cmp_addr), cmp_addr+4, regRead32(cmp_addr+4));
            }
            return -1;
        }
    }
    if ((e==0) && (i<2)) { /* when block is disabled, dma-read should happen "before" cpu-write, so comparison should fail */
        if (dst_data == cmp_val) {
            printf("ERROR4: regRead32(0x%x):0x%x == cmp_val:0x%x\n", cmp_addr, dst_data, cmp_val);
            if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)) {
                printf("\tNote: regRead32(0x%x):0x%x  regRead32(0x%x):0x%x\n", 
                        cmp_addr, regRead32(cmp_addr), cmp_addr+4, regRead32(cmp_addr+4));
            }
            return -1;
        }
    }
    return 0;
}

void transXmodem(unsigned int dst_addr)
{
	int size = 0;
	int err;
	int res;
	connection_info_t info;
	char ymodemBuf[XMODEM_BUFFER_SIZE];
	ulong store_addr = ~0;
	ulong addr = 0;
	
	info.mode = xyzModem_xmodem;
	res = xyzModem_stream_open(&info, &err);
	if (!res)
	{
	
		while ((res = xyzModem_stream_read(ymodemBuf, XMODEM_BUFFER_SIZE, &err)) > 0)
		{
			store_addr = addr + dst_addr;
			size += res;
			addr += res;
	
			memcpy((char *)(store_addr), ymodemBuf, res);
		}
	}
	else
	{
		printf("%s\n", xyzModem_error(err));
	}
	
	xyzModem_stream_close(&err);
    #if defined(CONFIG_TPL_BUILD)
	xyzModem_stream_terminate(false, &getcxmodem);
    #endif
}

void prepare_npu_block_test(void)
{
	transXmodem(NPU_SRAM_BASE);
	udelay(100);
	transXmodem(NPU_16K_SRAM_BASE);
	udelay(100);
}
void dma_block_cnt_test(void)
{
    int i, m, e, d;
    u32 test_len, test_offset;
    u32 block_mask[] = {0x3f, 0x7f};
    int masks = sizeof(block_mask)/sizeof(block_mask[0]);
    u32 wtVal = 0x12345678;
    u32 wtVal2 = 0x23456789;
    u32 dma_src_addr, dma_dst_addr, cpu_wt_addr;
    u32 dma_src_addr2, dma_dst_addr2, cpu_wt_addr2;
    unsigned int dscp_idx, dscp_idx_backup=0;
    
	/*put npu bin to sram*/
	prepare_npu_block_test();
    
    /* set pending (high-8bits) address */
    set_rbus_pending_addr(DRAM_TEST_BASE);
    /* set pending time for matched address, 0 means forever */
    set_rbus_pending_cnt(0);

    /*
     * d==(0,1,2,3,4,5,6) for (gdma,ppe,qdma_lan,qdma_wan,tdma,npu,gdma+qdmaLan) respectively
     * e==1 for enabling, e==0 for disabling block mechanism.
     * m==0 for 64B, m==1 for 128B block range.
     * i==0 or 1 for block counting case, i==2 for block not counting case.
     *
     * in case (e,m,i)==(1,0,0), cpu writes to addr:0x00 and dma reads addr:0x0~0x3f,  so block should heppen. 
     * in case (e,m,i)==(1,0,1), cpu writes to addr:0x20 and dma reads addr:0x0~0x3f, so block should heppen. 
     * in case (e,m,i)==(1,0,2), cpu writes to addr:0x40 and dma reads addr:0x0~0x3f, so block should not heppen. 
     */
    for (d=0; d<bctc_max_case; d++) { 
        
        if ((d==bctc_ppe)||(d==bctc_tdma)) continue; /* not supported yet */
        
        if (qdma_blkCntTest_config (d, &dscp_idx, dscp_idx_backup, masks))
            return;

        if (d == bctc_npu) { /* wake up NPU core0 */
			wakeup_npu_core0();
        }
            
        for (e=(DMA_BLK_CASE-1); e>=0; e--) {
            for (m=0; m<masks; m++) {
                for (i=0; i<CPU_WR_CASE; i++) {
                    
                    printf("%s at (e=%d,m=%d,i=%d)\n", dmaNames[d],e,m,i);
            
                    test_len = block_mask[m]+1;
                    test_offset = (test_len>>1);
                    cpu_wt_addr = DRAM_TEST_BASE+(test_offset*i);
                    dma_src_addr = DRAM_TEST_BASE;
                    dma_dst_addr = DRAM_TEST_BASE+test_len;
                    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)) {
                        /* replace dma_dst_addr */
                        dma_dst_addr = RXPKT_BASE + (dscp_idx*RX_PKT_MAX_SIZE) +g_buf_off;
                    }
                    else if (d==bctc_gdma_qdmalan) {
                        /* xxx_xxx_addr is for gdma, xxx_xxx_addr2 is for qdmalan */
                        cpu_wt_addr2 = DRAM_TEST_BASE+(test_offset*i)+(test_offset<<2);
                        dma_src_addr2 = DRAM_TEST_BASE+(test_offset<<2);
                        dma_dst_addr2 = RXPKT_BASE + (dscp_idx*RX_PKT_MAX_SIZE) +g_buf_off;
                    }
                    else {}
                    

                    /* qdma will read 16 more bytes from source area, so cpu should write to the 3rd test_len area 
                     * in case (i==2) that qdma blcok counters don't want to be triggered */
                    if ((i==2) && ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)||(d==bctc_gdma_qdmalan))) {
                        if (d==bctc_gdma_qdmalan)
                            cpu_wt_addr2 += test_len;
                        else
                            cpu_wt_addr += test_len;
                    }
                    
                    /* reset dma src & dst areas */
                    memset(dma_src_addr, 0, test_len);
                    memset(dma_dst_addr, 0, test_len);
                    if (d==bctc_gdma_qdmalan) {
                        memset(dma_src_addr2, 0, test_len);
                        memset(dma_dst_addr2, 0, test_len);
                    }
                    asm volatile ("dsb sy");

                    enable_block_dma_mechanism(DISABLE);
                    
                    if (e) {
                        set_block_dma_mask(block_mask[m]);
                        enable_block_dma_mechanism(ENABLE);
                    }
                    else {
                        printf("block disable case for m=%d,i=%d\n", m,i);
                    }

                    printf("test_len:0x%x  cpu_wt_addr:0x%x  dma_dst_addr:0x%x\n", test_len, cpu_wt_addr, dma_dst_addr);
                    if (d==bctc_gdma_qdmalan)
                        printf("\t\tcpu_wt_addr2:0x%x  dma_dst_addr2:0x%x\n", cpu_wt_addr2, dma_dst_addr2);

                    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan))
                        qdma_tx_dscp_hook(dscp_idx, dma_src_addr, test_len);
                    else if (d==bctc_gdma_qdmalan)
                        qdma_tx_dscp_hook(dscp_idx, dma_src_addr2, test_len);
                    else {}

                    /* enable pending to make sure that cpu-write can stay in Wbuff when dma-read happens */ 
                    enable_rbus_pending(ENABLE);
                    asm volatile ("dsb sy");
                    
                    regWrite32(cpu_wt_addr, wtVal);
                    if (d==bctc_gdma_qdmalan)
                        regWrite32(cpu_wt_addr2, wtVal2);

                    /* trigger DMA */
                    
                    switch (d) {
                        case bctc_gdma:
                        case bctc_gdma_qdmalan:
                            /* gdma-coherent is disabled here, otherwise "i==2" case will fail, because 
                             * if gdma-coherent is enabled, after gdma finishes writing to dst area, 
                             * it will uncached read back last byte of dst area which matches cpu_wt_addr's
                             * 64-byte-masked value. */
                            trigger_gdma(dma_src_addr, dma_dst_addr, test_len, 0, 0);
                            if (d!=bctc_gdma_qdmalan) /* bctc_gdma_qdmalan go head */
                                break;

                        case bctc_qdma_lan:
                        case bctc_qdma_wan:
                            trigger_qdma(dscp_idx);
                            break;
                            
                        case bctc_npu:
							trigger_npu(dma_src_addr, dma_dst_addr, test_len);
							break;

                        default:
                            printf("unknown dma_type:%d\n", d);
                            return;
                    }

                    //show_block_dbg_regs();

                    if (check_dma_block_cnt(d, e, i, 1))
                        goto dma_block_cnt_test_end;

                    /* disable rbus_cmd_pending to let the cpu-wt-cmd out, otherwise, 
                     * dma-read will be blocked by cpu-wt-cmd forever. */
                    enable_rbus_pending(DISABLE);

                    /* wait until DMA done */

                    switch (d) {
                        case bctc_gdma:
                        case bctc_gdma_qdmalan:
                            wait_gdma_done(0);
                            if (d!=bctc_gdma_qdmalan) /* bctc_gdma_qdmalan go head */
                                break;

                        case bctc_qdma_lan:
                        case bctc_qdma_wan:
                            udelay(10000);
                            break;

                        case bctc_npu:
							wait_npu_done();
							break;

                        default:
                            return;
                    }

                    if (check_dma_block_cnt(d, e, i, 2))
                        goto dma_block_cnt_test_end;

                    if (read_compare_data(d, e, i, dma_dst_addr+(test_offset*i), wtVal, 0))
                        goto dma_block_cnt_test_end;

                    if (d==bctc_gdma_qdmalan) {
                        if (read_compare_data(d, e, i, dma_dst_addr2+(test_offset*i), wtVal2, 1))
                            goto dma_block_cnt_test_end;
                    }

                    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)||(d==bctc_gdma_qdmalan))
                        dscp_idx++;

                    printf("\n");
            
                } /* i loop */
            } /* m loop */
        } /* e loop */

        if ((dscp_idx_backup==0) && ((d==bctc_qdma_lan)||(d==bctc_gdma_qdmalan))) /* save dscp_idx for using qdma_lan again */
            dscp_idx_backup = dscp_idx;
        
        printf("%s block_cnt_test done\n\n", dmaNames[d]);
    
    } /* d loop */

    printf("\nALL dma_block_cnt_tests PASS !!\n");
    return;

dma_block_cnt_test_end:
    printf("\nERROR for %s at (e=%d,m=%d,i=%d)\n", dmaNames[d], e , m, i);
    
    return;
}
#endif

