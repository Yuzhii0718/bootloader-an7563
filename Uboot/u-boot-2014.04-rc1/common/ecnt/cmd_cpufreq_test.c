#include <asm/io.h>
#include <common.h>
#include <ecnt/arm_v7_pmu.h>
#include "ecnt_cpufreq.h"

#define HZ_1M               1000000
#define FREQ_ARMPLL_MIN     500
#define FREQ_PLL1           540
#if defined(TCSUPPORT_CPU_EN7581)
#define FREQ_XTAL_50M       50
#define FREQ_ARMPLL_MAX     1200
#define FREQ_PLL2           400
#else
#define FREQ_XTAL_20M       20
#define FREQ_XTAL_25M       25
#define FREQ_ARMPLL_MAX     950
#define FREQ_PLL2           500
#endif

#define rdMeml(addr)		*(volatile unsigned long*)(addr)
#define wtMeml(addr,val)    *(volatile unsigned long*)(addr)=(unsigned long)(val)
#define rdMemB(addr)	    *(volatile unsigned char*)(addr)
#define wtMemB(addr,val)    *(volatile unsigned char*)(addr)=(unsigned char)(val)

#define S_512K          (0x80000)
#define S_1K            (0x400)

#define DRAM_TEST_START 0x81800000
#define DRAM_T_SIZE     S_512K
#define SRAM_TEST_START 0x1fa47400
#define SRAM_T_SIZE     S_1K

static unsigned int  armpll_clk_MHz[]={500,  550,  600,  650,  700,  750,  800,  850,  900,  950, 1000, 1050, 1100, 1150, 1200};
#if defined(TCSUPPORT_CPU_EN7581)
static char *clk_src_name[]={"xtal(50MHz)","armpll(500~1200MHz)","pll1(540MHz)","pll2(400MHz)"};
#else
static char *clk_src_name[]={"xtal(20/25MHz)","armpll(500~950MHz)","pll1(540MHz)","pll2(500MHz)"};
#endif

unsigned char testBytePat[] = {0x5a, 0xa5, 0xff, 0x00};
unsigned long testWordPat[] = {0x5a5a5a5a, 0xa5a5a5a5, 0xff00ff00, 0x00ff00ff, 0x0000ffff};

int ecnt_rw_ram_test (unsigned long startAddr, unsigned int size, unsigned long patAdd)
{
    int i, j;
    unsigned long tmpAddr, tmpWord;
    unsigned char tmpByte;
    unsigned char patAddB = (unsigned char)(patAdd&0xff);
    unsigned int wordLen = sizeof(unsigned long);
    unsigned int testWordNum = size/wordLen;
    unsigned int wordPatNum = sizeof(testWordPat)/sizeof(testWordPat[0]);
    unsigned int byteLen = 1;
    unsigned int testByteNum = size;
    unsigned int bytePatNum = sizeof(testBytePat)/sizeof(testBytePat[0]);


    for (j=0; j<wordPatNum; j++) {

        tmpAddr = startAddr;
        tmpWord=testWordPat[j]+patAdd;

        /* CPU writes words to RAM */
        for (i=0; i<testWordNum; i++) {
            wtMeml(tmpAddr,tmpWord);
            tmpAddr += wordLen;
        }


        tmpAddr = startAddr;
        tmpWord=testWordPat[j]+patAdd;

        /* CPU reads words from RAM and compare */
        for (i=0; i<testWordNum; i++) {
            if (rdMeml(tmpAddr) != tmpWord) {
                printf("\nERROR1 rdMeml(0x%lx):0x%lx != tmpWord:0x%lx at word:%d patAdd:0x%x\n",
                                    tmpAddr, rdMeml(tmpAddr), tmpWord, i, patAdd);
                return -1;
            }
            tmpAddr += wordLen;
        }

        tmpAddr = startAddr;
        tmpWord=testWordPat[j]+patAdd+1;

        /* CPU writes a word then reads it back and compare */
        for (i=0; i<testWordNum; i++) {
            wtMeml(tmpAddr,tmpWord);
            if (rdMeml(tmpAddr) != tmpWord) {
                printf("\nERROR2 rdMeml(0x%lx):0x%lx != tmpWord:0x%lx at word:%d patAdd:0x%x\n",
                                    tmpAddr, rdMeml(tmpAddr), tmpWord, i, patAdd);
                return -1;
            }
            tmpAddr += wordLen;
        }
    }


    for (j=0; j<bytePatNum; j++) {

        tmpAddr = startAddr;
        tmpByte=testBytePat[j]+patAddB;

        /* CPU writes bytes to RAM */
        for (i=0; i<testByteNum; i++) {
            wtMemB(tmpAddr,tmpByte);
            tmpAddr += byteLen;
        }

        tmpAddr = startAddr;
        tmpByte=testBytePat[j]+patAddB;

        /* CPU reads bytes from RAM and compare */
        for (i=0; i<testByteNum; i++) {
            if (rdMemB(tmpAddr) != tmpByte) {
                printf("\nERROR3 rdMemB(0x%lx):0x%x != tmpByte:0x%x at byte:%d patAdd:0x%x\n",
                                    tmpAddr, rdMemB(tmpAddr), tmpByte, i, patAdd);
                return -1;
            }
            tmpAddr += byteLen;
        }

        tmpAddr = startAddr;
        tmpByte=testBytePat[j]+patAddB+1;

        /* CPU writes a byte then reads it back and compare */
        for (i=0; i<testByteNum; i++) {
            wtMemB(tmpAddr,tmpByte);
            if (rdMemB(tmpAddr) != tmpByte) {
                printf("\nERROR4 rdMemB(0x%lx):0x%x != tmpByte:0x%x at byte:%d patAdd:0x%x\n",
                                    tmpAddr, rdMemB(tmpAddr), tmpByte, i, patAdd);
                return -1;
            }
            tmpAddr += byteLen;
        }
    }

    return 0;
}

static unsigned int estimate_cpu_frequency(void)
{
	unsigned int count;
	unsigned long flags;
	unsigned int start;

	local_irq_save(flags);

	/* Start r4k counter. */
	start = read_c0_count();

	/* delay 100 ms */
    /* mdelay() is from lib/time.c. mdelay() in time.c calls __udelay() which is implemented 
     * by ecnt's timer1 in arch/arm/cpu/armv7/en7523/ecnt_timer.c */
	mdelay(100);

	count = read_c0_count() - start;

	/* restore interrupts */
	local_irq_restore(flags);

	count*=10;          /* now delay 1 sec */
	count += 5000;      /* round */
	count -= count%10000;

	return count;
}

static int cpufreq_clksrc_compare (void)
{
    unsigned int cpufreq, freq_MHz;
    unsigned int armpll, divider, estimated_freq;
    enum e_clk_src clk_src;

    armpll = curr_armpll_clk_get();
    divider = curr_clk_divider_get();
    clk_src = curr_clk_src_enum_get();

    if ((armpll==0)||(divider==0)) {
        printf("ERROR: armpll:%d==0  or  divider:%d==0\n", armpll, divider);
        return -1;
    }

    cpufreq = estimate_cpu_frequency();
    freq_MHz = cpufreq/HZ_1M;

    if (clk_src==clk_src_armpll) {
        if (freq_MHz==(armpll/divider))
            return 0;
    }
    else if (clk_src==clk_src_xtal) {
        #if defined(TCSUPPORT_CPU_EN7581)
        if (freq_MHz==(FREQ_XTAL_50M/divider))
        #else
        if (((isXtalClk25M()==1)&&(freq_MHz==FREQ_XTAL_25M/divider)) ||
            ((isXtalClk25M()==0)&&(freq_MHz==FREQ_XTAL_20M/divider)))
        #endif
            return 0;
    }
    else if (clk_src==clk_src_pll1) {
        estimated_freq = FREQ_PLL1/divider;
        if ((freq_MHz==estimated_freq)||(freq_MHz==(estimated_freq+1)))
            return 0;
    }
    else if (clk_src==clk_src_pll2) {
        if (freq_MHz==FREQ_PLL2/divider)
            return 0;
    }
    else {
        printf("ERROR: wrong clk_src:%d, should be 0,1,2,3\n", clk_src);
        return -1;
    }

    printf("ERROR: %s failed with cpufreq:%d, freq_MHz:%d, clk_src:%d, armpll:%d, divider:%d\n",
            __func__, cpufreq, freq_MHz, clk_src, armpll, divider);
    return -1;
}

static int freq_config_stress (enum e_clk_src clk_src, enum e_cpu_freq cpu_freq, enum e_div_sel div_sel, int round)
{
    if (clk_src==clk_src_armpll) {
        if (en7523_armpll_set(cpu_freq))
            return -1;
    }
    else {
        if (clk_src_switch(clk_src))
            return -1;
    }

    if (clk_divider_sel_set(div_sel))
        return -1;

    if (cpufreq_clksrc_compare())
        return -1;

    if (ecnt_rw_ram_test(DRAM_TEST_START, DRAM_T_SIZE, round))
        return -1;

    if (ecnt_rw_ram_test(SRAM_TEST_START, SRAM_T_SIZE, round))
        return -1;

    return 0;
}

static int cpufreq_stress (void)
{
    enum e_clk_src clk_src;
    enum e_div_sel div_sel;
    enum e_cpu_freq cpu_freq;
    int round = 0;
    int cpufreq, clksrc, divsel;

    #if defined(TCSUPPORT_CPU_EN7581)
    printf("%s start\n", __func__);
    #else   
    printf("%s start (isXtalClk25M:%d)\n", __func__, isXtalClk25M());
    #endif

    for (clksrc=0; clksrc<clk_src_last; clksrc++) {
        for (cpufreq=0; cpufreq<cpu_freq_last; cpufreq++) {
            for (divsel=0; divsel<div_sel_last; divsel++) {
                if ((clksrc==clk_src_xtal) && (divsel>0))
                    continue;
                if (freq_config_stress(clksrc, cpufreq, divsel, round)==-1) {
                    goto stress_basic_fail_exit;
                }
                round++;
            }
        }
    }

    printf("\nbasic stress OK\n");
    round = 0;

    while (1)
    {
        if (read_c0_count()&0x1)
            clk_src=clk_src_armpll;
        else
            clk_src = read_c0_count() % clk_src_last;

        cpu_freq=0;
        if (clk_src==clk_src_armpll)
            cpu_freq = read_c0_count() % cpu_freq_last;

        if (clk_src==clk_src_xtal)
            div_sel = div_sel_1;
        else
            div_sel = read_c0_count() % div_sel_last;

        if (freq_config_stress(clk_src, cpu_freq, div_sel, round)==-1) {
            goto stress_fail_exit;
        }

        if ((round&0xf)==0)
            printf("%s OK for %d rounds\n", __func__, round+1);

        round++;

        if (round==3000) {
            printf("%s Pass!\n", __func__);
            return 1;
        }
    }

    return 0;

stress_basic_fail_exit:
    printf("%s basic failed at round:%d (clksrc:%d, armpll_clk:%d, divsel:%d)\n",
            __func__, round, clksrc, armpll_clk_MHz[cpufreq], divsel);
    return -1;

stress_fail_exit:
    printf("%s failed at round:%d (clk_src:%d, armpll_clk:%d, div_sel:%d)\n",
            __func__, round, clk_src, armpll_clk_MHz[cpu_freq], div_sel);
    return -1;
}

void arm_pmu_init(void)
{
    enablePMU() ;               /* Enable the PMU */

    /* reset counters */
    resetCCNT();                /* Reset the CCNT (cycle counter) */
    //resetPMN();                 /* Reset the configurable counters */

    /* Configure CCNT to count events in all PLs
     * NOTE: For the CCNT the event code is ignored
     * NOTE: Only needed on processors that implement PMUv2 */
    pmnConfig(V7_PMU_CCNT, 0, V7_PMU_EVENT_FILTER_ALL_PL);
    enableCCNT();               /* Enable CCNT */

    #if 0 /* enable pmu for instructions */
    /* Configure counter 0 to count event code 0x03 (for instructions) */
    pmnConfig(V7_PMU_COUNTER0, 0x03, V7_PMU_EVENT_FILTER_ALL_PL);
    enablePMN(0);               /* Enable counter */
    #endif
    #if 0 /* print inst and cycle counters */
    printk("Counter_0 = %d,  CCNT = %d\n", readPMN(0), read_c0_count());
    mdelay(1);
    printk("Counter_0 = %d,  CCNT = %d\n", readPMN(0), read_c0_count());
    #endif

    return;
}

static int cpufreq_test_command(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned long clk_src, armpll, divider=1;

    if (argc < 2)
        return CMD_RET_USAGE;

    arm_pmu_init();
    printf("current cpu freq: %d MHz\n", estimate_cpu_frequency()/HZ_1M);

    if (!strncmp(argv[1], "source", 6))
    {
        if (argc < 3)
            return CMD_RET_USAGE;

        clk_src = simple_strtoul(argv[2], NULL, 10);
        if (clk_src>3) {
            printf("ERROR: wrong source:%d, should be 0,1,2,3\n", clk_src);
            return 0;
        }
        printf("cpufreq source: %s\n", clk_src_name[clk_src]);

        if (argv[3]) {
            divider = simple_strtoul(argv[3], NULL, 10);
            if (!((divider==1)||(divider==2)||(divider==4)||(divider==6))) {
                printf("ERROR: wrong divider:%d, should be 1,2,4,6\n", divider);
                return 0;
            }
        }
        printf("divider: %d\n", divider);

        if (clk_src_switch(clk_src))
            return 0;

        /* set divider after clk_switch is done */
        if (clk_divider_sel_set(divider_sel_enum_get(divider)))
            return 0;

        if (cpufreq_clksrc_compare())
            return 0;
    }
    else if (!strncmp(argv[1], "freq", 4))
    {
        if (argc < 3)
            return CMD_RET_USAGE;

        armpll = simple_strtoul(argv[2], NULL, 10);
        if ((armpll<FREQ_ARMPLL_MIN)||(armpll>FREQ_ARMPLL_MAX)||((armpll%50)!=0)) {
            #if defined(TCSUPPORT_CPU_EN7581)
            printf("ERROR: wrong armpll:%d, should be 500,550,600,650,700,750,800,850,900,950,1000,1050,1100,1150,1200\n", armpll);
            #else
            printf("ERROR: wrong armpll:%d, should be 500,550,600,650,700,750,800,850,900,950\n", armpll);
            #endif
            return 0;
        }
        printf("armpll: %d MHz\n", armpll);

        if (argv[3]) {
            divider = simple_strtoul(argv[3], NULL, 10);
            if (!((divider==1)||(divider==2)||(divider==4)||(divider==6))) {
                printf("ERROR: wrong divider:%d, should be 1,2,4,6\n", divider);
                return 0;
            }
        }
        printf("divider: %d\n", divider);

        /* adjust armpll */
        if (en7523_armpll_set(cpu_freq_enum_get(armpll)))
            return 0;

        /* set divider after armpll adjustment is done */
        if (clk_divider_sel_set(divider_sel_enum_get(divider)))
            return 0;

        if (cpufreq_clksrc_compare())
            return 0;
    }
    else if (!strncmp(argv[1], "stress", 6))
    {
        if (cpufreq_stress())
            return 0;
    }
	else
		return CMD_RET_USAGE;

    printf("adjusted cpu freq: %d MHz\n", estimate_cpu_frequency()/HZ_1M);

	return 0;
}


U_BOOT_CMD(
		cpufreq,   5,      0,      cpufreq_test_command,
		"cpufreq - cpufreq command\n",
		"cpufreq usage:\n"
		#if defined(TCSUPPORT_CPU_EN7581)
        "	cpufreq source [src] (src:0,1,2,3 for xtal(50M),armpll(500~1200M),pll1(540M),pll2(400M))\n"
		"	cpufreq freq [frequency] (frequency:500,550,600,650,700,750,800,850,900,950,1000,1050,1100,1150,1200(MHz) for armpll)\n"
        #else
		"	cpufreq source [src] (src:0,1,2,3 for xtal(20/25M),armpll(500~950M),pll1(540M),pll2(500M))\n"
		"	cpufreq freq [frequency] (frequency:500,550,600,650,700,750,800,850,900,950(MHz) for armpll)\n"
		#endif
        "	cpufreq source [src] [divider] (divider:1,2,4,6)\n"
		"	cpufreq freq [frequency] [divider] (divider:1,2,4,6) for armpll\n"
		"	cpufreq stress\n"
);
