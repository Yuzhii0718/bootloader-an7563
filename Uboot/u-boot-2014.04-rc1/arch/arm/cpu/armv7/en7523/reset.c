#include <common.h>
#include <config.h>
#include <asm/io.h>

#include <asm/arch/en7523.h>
#include <asm/arch/typedefs.h>


void reset_cpu(ulong addr)
{
	writel(0x80000000, 0x1FB00040);
	while(1);
}

