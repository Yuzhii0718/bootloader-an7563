/*
 * (C) Copyright 2000-2009
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/interrupts.h>
#include <asm/arch/arm-gic-v3.h>
#include <asm/arch/arm-gic.h>


/* Initialize to non-zero so that we avoid clearing of bss */
void (*interrupt_vectors[MAX_INT_NUM])(void) = {{INVERSE_NULL} };
extern void gic_dic_set_enable (unsigned int intrID);
extern void gic_dic_set_config (unsigned int intrID,
	unsigned char level0edge1);

extern void gic_dist_init(void);
extern void gic_cpu_init(void);

struct gic_chip_data {
	unsigned long		*dist_base;
	unsigned long		redist_regions;
	struct rdists		rdists;
	unsigned long long		redist_stride;
	unsigned long			nr_redist_regions;
	unsigned int		irq_nr;
};
struct gic_chip_data gic_data;

static void null_func(void)
{
	printf("Dummy ISR function \n");
	
}


void do_irq (struct pt_regs *pt_regs)
{
	/* possible spurious interrupt */
	u32 irqnr;

	do {
		irqnr = gic_read_iar();

		if (likely(irqnr > 15 && irqnr < MAX_INT_NUM)) {

			gic_write_eoir(irqnr);	
			/* if no handler registered, it will skip the handler call */
			if (null_func == (interrupt_vectors[irqnr])) {
				printf("does not register ISR for IRQ %d\n",irqnr);
				(interrupt_vectors[irqnr])();
				gic_write_eoir(irqnr);
				gic_write_dir(irqnr);

			}
			(interrupt_vectors[irqnr])();
			gic_dic_clear_active(irqnr);
			continue;
		}
		else
		{
			if (!((irqnr >1020)&& (irqnr < 1024)))
			{
				printf("invalid IRQ: %d, deactived it\n\r",irqnr);
				gic_write_eoir(irqnr);
				gic_write_dir(irqnr);
			}
			continue;
		}
	} while (irqnr != ICC_IAR1_EL1_SPURIOUS);
}

static void init_handler_irq(void)
{
	int i =0 ;
	for (i = 0 ; i < MAX_INT_NUM;i++)
	{
		interrupt_vectors[i] = null_func;
	}

}
int arch_interrupt_init (void)
{
	void *dist_base;
	u64 redist_stride;
	u32 nr_redist_regions;
	u32 typer;
	u32 reg;
	int gic_irqs;
	int ret = 0;
	
	dist_base = GICD_BASE;
	reg = readl(dist_base + GICD_PIDR2) & GIC_PIDR2_ARCH_MASK;
	if (reg != GIC_PIDR2_ARCH_GICv3 && reg != GIC_PIDR2_ARCH_GICv4) {
		printf("no distributor detected, giving up\n"	);
		return -1;
	}
	nr_redist_regions = 1;
	redist_stride = 0;

	gic_data.dist_base = GICD_BASE;
	gic_data.redist_regions = GICR_CTLR;
	gic_data.nr_redist_regions = nr_redist_regions;
	gic_data.redist_stride = redist_stride;
	

	/*
	 * Find out how many interrupts are supported.
	 * The GIC only supports up to 1020 interrupt sources (SGI+PPI+SPI)
	 */
	typer = readl(GICD_BASE + GICD_TYPER);
	gic_data.rdists.id_bits = GICD_TYPER_ID_BITS(typer);
	gic_irqs = GICD_TYPER_IRQS(typer);
	if (gic_irqs > 1020)
		gic_irqs = 1020;
	gic_data.irq_nr = gic_irqs;

	init_handler_irq();
	gic_dist_init();
	gic_cpu_init();

	
	return ret;
}



int irq_register (unsigned int irq_num, void (*fxn)(void),
	unsigned char level0edge1)
{
	interrupt_vectors[irq_num] = fxn;
		
	gic_dic_set_config (irq_num, level0edge1);
	/* always goto CPU0 for now */
	gic_dic_set_enable (irq_num);
	return 0;
}


