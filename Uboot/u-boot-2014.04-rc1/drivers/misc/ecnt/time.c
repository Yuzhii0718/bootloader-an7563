#include <asm/tc3162.h>
#include <asm/io.h>
#include <asm/arch/interrupts.h>

volatile unsigned long Jiffies;
#define timerLdvSet(timer_no,val) *(volatile uint32 *)(CR_TIMER0_LDV+timer_no*0x08) = (val)
#define timerVlrGet(timer_no,val) (val)=*(volatile uint32 *)(CR_TIMER0_VLR+timer_no*0x08)
#define timerCtlSet(timer_no, timer_enable, timer_mode,timer_halt)	timer_Configure(timer_no, timer_enable, timer_mode, timer_halt)
#define timerWdSet(tick_enable, watchdog_enable) timer_WatchDogConfigure(tick_enable,watchdog_enable)

extern void timer_Configure(uint8 timer_no, uint8 timer_enable, uint8 timer_mode, uint8 timer_halt);
extern void timerSet(uint8 timer_no, uint32 timerTime,	uint8 enable, uint8 mode, uint8 halt);

void timer_interrupt(void *args)
{             
	uint32 word;

	word = regRead32(CR_TIMER_CTL);
	word &= 0xffc0ffff;
	word |= 0x00040000;
	regWrite32(CR_TIMER_CTL, word);
	Jiffies++;	

#ifdef CONFIG_ECNT_MULTIUPGRADE
	multiupgrade_blink();
#endif

}

uint32 getTime(void)
{
	return (Jiffies * 10);
}
   
void time_init(void)
{
	timerSet(2, TIMERTICKS_10MS, ENABLE, TIMER_TOGGLEMODE, TIMER_HALTDISABLE);
	irq_register(TIMER2_INT, timer_interrupt, 0);
}             



