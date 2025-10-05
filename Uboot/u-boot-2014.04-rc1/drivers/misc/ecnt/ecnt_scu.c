#include <asm/io.h>

/* Register base address */
#define IO_PHYS				(0x10000000)
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7552)
#define IOMUX_1		(IO_PHYS + 0xFA20214)
#define GPIO_CTR	(IO_PHYS + 0xFBF0200)
#else
#define IOMUX_1		(IO_PHYS + 0xFA20210)
#define GPIO_CTR	(IO_PHYS + 0xFBF0200)
#endif

void LAN_LED0_enable(void)
{
	unsigned int val;

	/* enable HW switch LED0 */
	val = readl(IOMUX_1);
	val |= (0x1 << 3) | (0x1 << 5) | (0x1 << 7) | (0x1 << 9);
	writel(val, IOMUX_1);
}

void LAN_LED_disable(void)
{
	unsigned int val;

	/* disable HW switch */
	val = readl(IOMUX_1);
	val &= ~(0xff << 3);
	writel(val, IOMUX_1);

	/* GPIO 8-11 output */
	val = readl(GPIO_CTR);
	val &= ~(0xff << 16);
	val |= (0x55 << 16);
	writel(val, GPIO_CTR);
}
