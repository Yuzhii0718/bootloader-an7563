#include <asm/tc3162.h>
#include <asm/io.h>
#include <common.h>
#include <asm/tc3162.h>

static long efuse_command(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long r0 = 0, r1 = 0, r2 = 0, r3 = 0;
#ifdef TCSUPPORT_UBOOT_64BIT
	struct arm_smccc_res res;
#endif

	if (argc < 4)
		return CMD_RET_USAGE;

	r0 = 0x82000001;
	r1 = *((unsigned int *) argv[1]);
	r2 = simple_strtoul(argv[2], NULL, 16);
	r3 = simple_strtoul(argv[3], NULL, 16);

#ifdef TCSUPPORT_UBOOT_64BIT
	__arm_smccc_smc(r0, r1, r2, r3, 0, 0, 0 ,0, &res,0);
	return 0;
#else
	do_smc(r0, r1, r2, r3);
	return 0;
#endif
}

U_BOOT_CMD(
		efuse,   4,      0,      efuse_command,
		"efuse - efuse command\n",
		"efuse usage:\n"
		"	efuse CMD ARG1 ARG2\n"
);


static long sr_test_command(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long r0 = 0, r1 = 0, r2 = 0, r3 = 0;
#ifdef TCSUPPORT_UBOOT_64BIT
	struct arm_smccc_res res;
#endif

	if (argc < 4)
		return CMD_RET_USAGE;

	r0 = 0x82000002;
	r1 = *((unsigned int *) argv[1]);
	r2 = simple_strtoul(argv[2], NULL, 16);
	r3 = simple_strtoul(argv[3], NULL, 16);

#ifdef TCSUPPORT_UBOOT_64BIT
	__arm_smccc_smc(r0, r1, r2, r3, 0, 0, 0 ,0, &res,0);
	return (long)res.a0;
#else
	do_smc(r0, r1, r2, r3);
	return 0;
#endif
}

U_BOOT_CMD(
		sr_test,   4,      0,      sr_test_command,
		"sr_test - test command\n",
		"sr_test usage:\n"
		"	sr_test CMD ARG1 ARG2\n"
);