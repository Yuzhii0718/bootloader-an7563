#include <common.h>
#include <config.h>
#include <asm/io.h>


#include <asm/arch/typedefs.h>
#include <asm/arch/en7523.h>
#include <asm/arch/ecnt_timer.h>
#include <asm/tc3162.h>

static volatile U32 timestamp;
static volatile U32 lastinc;

/**************************
 * Timer Module Registers *
 **************************/
#define CR_TIMER_BASE		0x1FBF0100
#define CR_TIMER_CTL		(CR_TIMER_BASE + 0x00)
#define CR_TIMER0_LDV		(CR_TIMER_BASE + 0x04)
#define CR_TIMER0_VLR		(CR_TIMER_BASE + 0x08)
#define CR_TIMER1_LDV		(CR_TIMER_BASE + 0x0C)
#define CR_TIMER1_VLR		(CR_TIMER_BASE + 0x10)
#define CR_TIMER2_LDV		(CR_TIMER_BASE + 0x14)
#define CR_TIMER2_VLR		(CR_TIMER_BASE + 0x18)
#define CR_TIMER3_LDV		(CR_TIMER_BASE + 0x1C)
#define CR_TIMER3_VLR		(CR_TIMER_BASE + 0x20)
#define CR_TIMER4_LDV		(CR_TIMER_BASE + 0x24)
#define CR_TIMER4_VLR		(CR_TIMER_BASE + 0x28)
#define CR_TIMER5_LDV		(CR_TIMER_BASE + 0x2C)
#define CR_TIMER5_VLR		(CR_TIMER_BASE + 0x30)

#define TIMER_ENABLE		1
#define TIMER_DISABLE		0
#define TIMER_TOGGLEMODE	1
#define TIMER_INTERVALMODE	0
#define TIMER_TICKENABLE	1
#define TIMER_TICKDISABLE	0
#define TIMER_WDENABLE		1
#define TIMER_WDDISABLE		0
#define TIMER_HALTENABLE	1
#define TIMER_HALTDISABLE	0

#define TIMERTICKS_1MS		1
#define TIMERTICKS_10MS		10	// set timer ticks as 10 ms
#define TIMERTICKS_100MS	100
#define TIMERTICKS_1S		1000
#define TIMERTICKS_10S		10000

#define FPGA_SYS_HCLK		40							/* 40MHZ / 10^6 */
#if defined(TC3262_FPGA)
#define TIMER_TICK_PER_US	(FPGA_SYS_HCLK / 2)			/* (40MHZ / 2) / 10^6 : the fequency of Timer is half of Bus */
#else
#define TIMER_TICK_PER_US	(SYS_HCLK / 2)
#endif
#define TIMER_TICK_PER_MS	(TIMER_TICK_PER_US * 1000)

#define MAX_TIMESTAMP_MS	(0xFFFFFFFF / TIMER_TICK_PER_MS)

#define timerCtlSet(timer_no, timer_enable, timer_mode,timer_halt)	timer_Configure(timer_no, timer_enable, timer_mode, timer_halt)
#define timerLdvSet(timer_no,val) writel(val, (CR_TIMER0_LDV + (timer_no * 0x08)))

void timer_Configure(uint8 timer_no, uint8 timer_enable, uint8 timer_mode, uint8 timer_halt)
{
	uint32 word;

	word = readl(CR_TIMER_CTL);

	if(timer_enable == 1)
	{
		word |= (1 << timer_no);
	}
	else
	{
		word &= ~(1 << timer_no);
	}

	writel(word, CR_TIMER_CTL);
}

void timerSet(uint8 timer_no, uint32 timerTime,	uint8 enable, uint8 mode, uint8 halt)
{
	uint32 word;

	if(readl(CR_TIMER_CTL) & (1 << timer_no))
    {
    	printf("WARNING: Timer%d is in used\n", timer_no);
    }

	word = timerTime * TIMER_TICK_PER_MS;
	timerLdvSet(timer_no, word);
	timerCtlSet(timer_no, enable, mode, halt);
}

static uint32 get_current_tick(void)
{
	return readl(CR_TIMER1_VLR);
}

static bool timeout_tick(uint32 start_tick, uint32 timeout_tick)
{
	volatile uint32 cur_tick;
	volatile uint32 elapse_tick;
	volatile uint32 timer1_ldv = readl(CR_TIMER1_LDV);

	cur_tick = get_current_tick();

	if (start_tick >= cur_tick)
	{
		elapse_tick = start_tick - cur_tick;
	}
	else
	{
		elapse_tick = (timer1_ldv - cur_tick) + start_tick;
	}

	if (timeout_tick <= elapse_tick)
	{
		return TRUE;
	}

	return FALSE;
}

static void busy_wait_us (uint32 timeout_us)
{
	volatile uint32 timer_now, timer_last;
	volatile uint32 tick_acc;
	volatile uint32 tick_wait = timeout_us * TIMER_TICK_PER_US;
	volatile uint32 timer1_ldv = readl(CR_TIMER1_LDV);

	tick_acc = 0;
	timer_last = readl(CR_TIMER1_VLR);
	do {
		timer_now = readl(CR_TIMER1_VLR);
		if (timer_last >= timer_now)
			tick_acc += timer_last - timer_now;
		else
			tick_acc += timer1_ldv - timer_now + timer_last;
		timer_last = timer_now;
	} while (tick_acc < tick_wait);
}

static void busy_wait_ms (uint32 timeout_ms)
{
	busy_wait_us(timeout_ms * 1000);
}

static U32 getTimeInterval_us(uint32 tick)
{
	volatile uint32 timer1_ldv = readl(CR_TIMER1_LDV);
	volatile uint32 timer_now;
	uint32 us_tick_unit = TIMER_TICK_PER_US;
	uint32 timeIntval= 0;

	timer_now = get_current_tick();
	if (tick >= timer_now)
		timeIntval = tick - timer_now;
	else
		timeIntval = timer1_ldv - timer_now + tick;

	return (timeIntval / us_tick_unit);
}

static U32 getTimeInterval_ms(uint32 tick)
{
	return (getTimeInterval_us(tick) / 1000);
}

/* second * CONFIG_SYS_HZ */
ulong get_timer (ulong base)
{
	static uint32 rollover_count = 0, previous_tick = 0;
	ulong now_ms = 0, now_sys = 0, timer = 0;
	uint32 max_tick = readl(CR_TIMER1_LDV);
	uint32 max_timestamp_ms = max_tick / TIMER_TICK_PER_MS;
	uint32 current_tick = get_current_tick();
	uint32 current_timestamp_ms = current_tick / TIMER_TICK_PER_MS;

	if(base == 0)
	{
		previous_tick = max_tick;
		rollover_count = 0;
	}

	if(current_tick > previous_tick)
	{
		rollover_count++;
	}

	previous_tick = current_tick;

	now_ms = rollover_count * max_timestamp_ms + (max_timestamp_ms - current_timestamp_ms);
	now_sys = now_ms * CONFIG_SYS_HZ / 1000;

	timer = now_sys - base;

	return timer;
}

void __udelay (unsigned long usec)
{
	  busy_wait_us(usec);
}

unsigned long long get_ticks (void)
{
	return (unsigned long long) get_timer (0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
 /* has defined in arch/arm/cpu/armv8/generic_timer.c */
#if 0
ulong get_tbclk (void)
{
	ulong tbclk;
	tbclk = CONFIG_SYS_HZ;
	return tbclk;
}
#endif

void ecnt_timer_init (void)
{
	timerSet(1, MAX_TIMESTAMP_MS , 1, TIMER_TOGGLEMODE, TIMER_HALTDISABLE);
}
