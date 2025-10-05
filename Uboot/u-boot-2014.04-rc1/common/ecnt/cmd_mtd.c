#include <asm/io.h>
#include <common.h>
#include <image.h>
#include <libfdt.h>
#include <fdt.h>

extern int do_get_mtd_info(void);

static int mtd_command(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{	
	return do_get_mtd_info();
}

U_BOOT_CMD(
		mtd,   1,      0,      mtd_command,
		"mtd - mtd  command\n",
		"mtd  usage:\n"
		"	mtd\n"
);

