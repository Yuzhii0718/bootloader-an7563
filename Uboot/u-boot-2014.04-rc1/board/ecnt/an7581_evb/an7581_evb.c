#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <asm/tc3162.h>

#include <asm/arch/typedefs.h>
#include <asm/arch/ecnt_timer.h>
#include <asm/arch/en7523.h>

DECLARE_GLOBAL_DATA_PTR;

static char *an7581_chip_name[] =
{
	"AN7581GT",
	"AN7566GT",
	"AN7581PT",
	"AN7581ST",
	"AN7551PT",
	"AN7581CT",
	"AN7581DT",
	"AN7581FG",
	"AN7581FP",
	"AN7581FD",
	"AN7551GT",
	"AN7566PT",
	"AN7581IT",
	"AN7581SIT",
};

static char *en7523_chip_name[] =
{
	"EN7529DU",		"EN7529DT",
	"EN7529CU",		"EN7562DU",
	"EN7562DT",		"EN7562CU",
	"EN7523GU",		"EN7523DU",
	"EN7523GTH",	"EN7562GTH",
	"EN7523SU",		"EN7523GTS",
	"EN7562GTS",	"EN7529IT",
	"EN7529CT",		"EN7562CT",
	"EN7523DT",		"EN7529DTM",
	"EN7562DTM",	"EN7529ITM",
	"EN7529CTM",	"EN7562CTM",
	"EN7523DTM",
};


int dram_init(void)
{
	uint64_t size = 0;

	size = (uint64_t)GET_DRAM_SIZE * (uint64_t)SZ_1M;

	/* Uboot only support 32bit dram size (2GB)*/ 
	if (size > SZ_2G)
		size = SZ_2G;
	gd->ram_size = size;

#if defined(CONFIG_MTK_ATF)
	/*It just sync from MT6735m preloader. They will reserve 1MB for ATF log*/
	gd->ram_size -= SZ_16M;
#endif

	return 0;
}

int board_init(void)
{
	char *name = "Unknow Chip.\n";
	if(isEN7581) {
		name = an7581_chip_name[GET_PACKAGE_ID];
	} else if(isEN7523){
		name = en7523_chip_name[GET_PACKAGE_ID];
	}
	printf("%s\n", name);
	ecnt_timer_init();

	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

    return 0;
}

int board_late_init (void)
{
	gd->env_valid = 1;
	env_relocate();

	return 0;
}


void ft_board_setup(void *blob, bd_t *bd)
{

}

#ifndef CONFIG_SYS_DCACHE_OFF
#ifndef CONFIG_ARM64
/* enable_caches() is define in arch/arm/cpu/armv8/cache_v8.c */
void enable_caches(void)
{
	dcache_enable();
}
#endif
#endif

int board_eth_init(bd_t *bis)
{
	ecnt_eth_initialize(bis);
	return 0;
}

