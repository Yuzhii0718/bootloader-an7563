#include <asm/tc3162.h>
#include <asm/io.h>
#include <common.h>

static int bootflag_command(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	if (argc < 2)
	    return CMD_RET_USAGE;

	if (!strncmp(argv[1], "read", 4)) {
		printf("\nbootflag==%d\n", readBootFlagFromFlash());
	} else if (!strncmp(argv[1], "swap", 4)) {	
		return swap_bootflag();
	} else {
		return CMD_RET_USAGE;
	}
    
    return 0;
}

U_BOOT_CMD(
		bootflag,   2,      0,      bootflag_command,
		"bootflag read/swap command\n",
		"usage:\n"
		"	bootflag read\n"
		"	bootflag swap\n"
);

