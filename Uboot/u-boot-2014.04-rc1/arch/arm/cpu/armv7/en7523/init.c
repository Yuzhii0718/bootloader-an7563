#include <common.h>

#define FPGA_SYS_HCLK 40

int arch_cpu_init(void)
{
	icache_enable();

	return 0;
}
