
#include <common.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/arch/spl.h>
#include <asm/arch/typedefs.h>
#include <asm/arch/en7523.h>
#include <asm/arch/ecnt_timer.h>
#include <xyzModem.h>
#include <flashhal.h>

DECLARE_GLOBAL_DATA_PTR;
#define ATF_BOOT_ARG_ADDR               (CONFIG_SYS_SDRAM_BASE + SZ_8K)
#define BL31_BASE			            (ATF_BOOT_ARG_ADDR + SZ_4K)
#define BL33_BASE			            (CONFIG_SYS_TEXT_BASE)
#define RVBADDRESS_CPU0					(0x1EFBE038)

#define XMODEM_BUFFER_SIZE				(SZ_128)

u32 bl31_base_addr = BL31_BASE;
u32 rst_vector_base_addr = RVBADDRESS_CPU0;

extern int ecnt_uart_init (void);
#ifdef TEST_BLOCK_CNT_IN_SRAM
extern void dma_block_cnt_test(void);
#endif
//#define TEST_RBUS_TIMEOUT
#define RBUS_TIMEOUT_BASE	(0x1fa00000)
#define TIMEOUT_STS0		(RBUS_TIMEOUT_BASE + 0xd0)
#define TIMEOUT_STS1		(RBUS_TIMEOUT_BASE + 0xd4)

#ifdef TEST_RBUS_TIMEOUT
#if 1 //FPGA CLK
#define DRAM_CONTROLLER_CLK 0x4000000
#else //ASIC CLK
#define DRAM_CONTROLLER_CLK 0x19000000
#endif
#define DRAM_TEST_ADDR		(0x84000000)
#define FORCE_TIMEOUT		(RBUS_TIMEOUT_BASE + 0xbc)
#define TIMEOUT_CFG0		(RBUS_TIMEOUT_BASE + 0xd8)
#define TIMEOUT_CFG1		(RBUS_TIMEOUT_BASE + 0xdc)
#define TIMEOUT_CFG2		(RBUS_TIMEOUT_BASE + 0xe0)
#endif
static u32 trustzone_get_atf_boot_param_addr(void)
{
    return ATF_BOOT_ARG_ADDR;
}

void trustzone_jump(u32 addr, u32 arg1, u32 arg2)
{
	unsigned int reg;
	unsigned itlinesnr, i;

    printf("Jump to ATF, then 0x%x\n", bl31_base_addr);

    jumparch64(addr, arg1, arg2, trustzone_get_atf_boot_param_addr());
}

#if defined(CONFIG_TPL_BUILD)
int getcxmodem(void) {
	if (tstc())
		return (getc());
	return -1;
}
#endif

/* If rbus timeout happended, print the messages for debug*/
void rbus_timeout_check(void)
{

	if ((VPint(TIMEOUT_STS0) & 0x1) == 1)
	{
		printf("\r\nERROR !!! Rbus timeout happended before !!!\r\n");
		printf("\r\nThe Rbus timeout status is as below\r\n");
		printf("\r\nstatus(TIMEOUT_STS0) = %x ; errAddr(TIMEOUT_STS1) = %x \r\n", VPint(TIMEOUT_STS0), VPint(TIMEOUT_STS1));
		/* clear the status*/
		VPint(TIMEOUT_STS0) = 0;
		VPint(TIMEOUT_STS1) = 0;		
	}
}

#ifdef TEST_RBUS_TIMEOUT
void rbus_timeout_test(char test_index)
{
		uint32_t read_data =0 ;
		uint32_t reg = 0;
		uint32_t addr = 0;
		uint32_t time_config = 0;
		int i = 0;
		int test =0 ;
		uint32_t time_start = 0;
		uint32_t time_end = 0;

		printf ("test index = %c\n", test_index);
		/* In case of EN7523 timer reg would not be reset when reboot, set the regs to default before tests*/
		VPint(TIMEOUT_STS0) = 0;
		VPint(TIMEOUT_STS1) = 0;

		if (test_index == '1')
		{
			printf("Rbus timeout disable Test\r\n");
			printf("0x1fa000d0 = %x\r\n", VPint(TIMEOUT_STS0));
			printf("Force bus timeout when rbus timeout is disabled!\n");
			VPint(FORCE_TIMEOUT) = 0;
			printf("Trying to read data from DRAM\n");
			/* System will hang here */
			read_data = VPint(DRAM_TEST_ADDR);
			/* this should not be excuted*/
			printf("Error!\n");
			
		}
		else if (test_index == '2')
		{
			printf("Rbus timeout trigger by read command Test\r\n");
			VPint(TIMEOUT_STS0) = 0x80000000;
			printf("0x1fa000d0 = %x\n", VPint(TIMEOUT_STS0));
			printf("Force bus timeout\n");
			VPint(FORCE_TIMEOUT) = 0;
			printf("Read data from DRAM 0x84000000\n");
			read_data = VPint(DRAM_TEST_ADDR);
	
			printf("CPU BUS Test: action: Read\n");
			printf("read_data = %x; rbus timeout status = %x ; errAddr = %x \n",read_data, VPint(TIMEOUT_STS0), VPint(TIMEOUT_STS1));
		}
		else if (test_index == '3' )
		{
			printf("Rbus timeout trigger by write command Test\r\n");
			VPint(TIMEOUT_STS0) = 0x80000000;
			VPint(TIMEOUT_CFG0) = 0x400;
			printf("0x1fa000d0 = %x\n", VPint(TIMEOUT_STS0));
			printf("Force bus timeout\n");
			VPint(FORCE_TIMEOUT) = 0;
			printf("Write data to DRAM 0x84000000\n");
			VPint(DRAM_TEST_ADDR) = 0x12345678; //write data to dram
			/* wait write cmd into bus*/
			i = 0;
			time_start = get_ticks();
			while(VPint(TIMEOUT_STS1)!= DRAM_TEST_ADDR)
			{
				i++;
			}
			time_end = get_ticks();
			printf("CPU BUS Test: action: Write\n");
			printf("loop %d times; wait %dms; rbus timeout status = %x ; errAddr = %x ; TIMEOUT_CFG0 = %x \n",i, time_end - time_start,VPint(TIMEOUT_STS0), VPint(TIMEOUT_STS1),VPint(TIMEOUT_CFG0));

		}
		else if ( test_index == '4' )
		{
			printf("Rbus timeout cmd cnt Read Test\r\n");
			time_config = 1;
			VPint(0x1EFBC800) = 0; //set to non bufferable
			for (time_config = 1 ; time_config < 6; time_config ++)
			{
				VPint(TIMEOUT_CFG0) = DRAM_CONTROLLER_CLK * time_config;
				VPint(TIMEOUT_STS0) = 0x80000000;
				addr = DRAM_TEST_ADDR + (0x100 * time_config);
				
				printf("0x1fa000d8 = %x , read_addr = %x, timer config = %d sec(s)\n",VPint(TIMEOUT_CFG0), addr, time_config);
				VPint(FORCE_TIMEOUT) = 0;
				printf("force timeout\n");
				time_start = get_ticks();
				addr = VPint(addr);
				time_end = get_ticks();
				printf("CPU BUS Test: \n");
				printf("rbus timeout status = %x ; errAddr = %x ; wait time = %dms\n", VPint(TIMEOUT_STS0), VPint(TIMEOUT_STS1), time_end - time_start);
			}
		}
		else if (test_index == '5')
		{
			printf("Rbus timeout cmd cnt Write Test\r\n");

			VPint(0x1EFBC800) = 0; //set to non bufferable
			for (time_config = 1 ; time_config < 6; time_config ++)
			{
				VPint(TIMEOUT_CFG0) = DRAM_CONTROLLER_CLK * time_config;
				VPint(TIMEOUT_STS0) = 0x80000000;
				addr = DRAM_TEST_ADDR + (0x100 * time_config);			
			
				printf("0x1fa000d8 = %x , write_addr = %x, timer config = %d sec(s)\n",VPint(TIMEOUT_CFG0), addr, time_config);
				VPint(FORCE_TIMEOUT) = 0;
				printf("force timeout\n");
				time_start = get_ticks();
				VPint(addr) = 0x12345678;
				i = 0;
				/* wait write cmd into bus*/
				while(VPint(TIMEOUT_STS1)!= addr)
				{
					i++;
				}
				time_end = get_ticks();
				printf("CPU BUS Test: \n");
				printf("loop %d times; rbus timeout status = %x ; errAddr = %x ; wait time = %dms\n", i,VPint(TIMEOUT_STS0), VPint(TIMEOUT_STS1), time_end - time_start);
				
			}


		}
		/* test finished*/
		while(1);

}
#endif

void board_init_f(ulong dummy)
{
#if defined(CONFIG_TPL_BUILD)
	__attribute__((noreturn)) void (*uboot)(void);
	unsigned long retlen = 0;
	unsigned char c = 'a';
	unsigned char r = 'a';

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* Set global data pointer. */
	gd = &gdata;
	arch_cpu_init();
	ecnt_timer_init();
	preloader_console_init();
  //YMC add for ASIC DDR calibration
	dramc_main();
	dram_init();

	flash_init(0);
	rbus_timeout_check();

	flash_read(CONFIG_ATF_OFFSET, CONFIG_ATF_MAX_SIZE, &retlen, (unsigned char *) BL31_BASE);
	flash_read(CONFIG_UBOOT_OFFSET, CONFIG_UBOOT_MAX_SIZE, &retlen, (unsigned char *) BL33_BASE);

	retlen = get_timer(0);
	do {
		if (tstc())
		{
			c = getc();
			break;
		}
		udelay(100);
	} while (get_timer(retlen) < 1000);
	
	#ifdef TEST_RBUS_TIMEOUT
	/* get test index*/
	do {
		if (tstc())
		{
			r = getc();
			break;
		}
		udelay(100);
	} while (get_timer(retlen) < 1000);
	#endif
	
	if (c == 'u')
	{
		uboot = (void *)BL33_BASE;
		(*uboot)();
	}
	else if (c == 'x')
	{
		int size = 0;
		int err;
		int res;
		connection_info_t info;
		char ymodemBuf[XMODEM_BUFFER_SIZE];
		ulong store_addr = ~0;
		ulong addr = 0;

		info.mode = xyzModem_xmodem;
		res = xyzModem_stream_open(&info, &err);
		if (!res)
		{

			while ((res = xyzModem_stream_read(ymodemBuf, XMODEM_BUFFER_SIZE, &err)) > 0)
			{
				store_addr = addr + CONFIG_SYS_LOAD_ADDR;
				size += res;
				addr += res;

				memcpy((char *)(store_addr), ymodemBuf, res);
			}
		}
		else
		{
			printf("%s\n", xyzModem_error(err));
		}

		xyzModem_stream_close(&err);
		xyzModem_stream_terminate(false, &getcxmodem);

		flush_cache(CONFIG_SYS_LOAD_ADDR, size);

		flash_erase(CONFIG_TCBOOT_OFFSET, size);
		flash_write(CONFIG_TCBOOT_OFFSET, size, &retlen, (unsigned char *) CONFIG_SYS_LOAD_ADDR);
		printf("Success\n");
		reset_cpu(0);
	}
    #ifdef TEST_BLOCK_CNT_IN_SRAM
    else if (c == 't')
	{
		dma_block_cnt_test();
	}
    #endif
	#ifdef TEST_RBUS_TIMEOUT
	else if (c == 'r')
	{
		rbus_timeout_test(r);
	}
	#endif
	else
	{
		trustzone_jump(BL33_BASE, 0, 0);
	}

	while(1);
#endif
}

u32 spl_boot_device(void)
{
	int mode = -1;

	if (mode == -1)
	{
		puts("Unsupported boot mode selected\n");
		hang();
	}

	return mode;
}
