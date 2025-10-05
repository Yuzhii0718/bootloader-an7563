#include <asm/io.h>
#include <common.h>
#include <image.h>
#include <libfdt.h>
#include <flashhal.h>
#include <bmt.h>
#include <spi/spi_nand_flash.h>

#define SPI_NAND_CHECK_PATTERN_DATA_SIZE	4096
#define SPI_NAND_CHECK_PATTERN_OOB_SIZE		256

unsigned char test_pattern_write_data_buf[SPI_NAND_CHECK_PATTERN_DATA_SIZE];
unsigned char test_pattern_write_oob_buf[SPI_NAND_CHECK_PATTERN_OOB_SIZE];
unsigned char test_pattern_read_data_buf[SPI_NAND_CHECK_PATTERN_DATA_SIZE];
unsigned char test_pattern_read_oob_buf[SPI_NAND_CHECK_PATTERN_OOB_SIZE];

extern int nand_logic_size;
extern int nandflash_erase(unsigned long offset, unsigned long len);
extern int nandflash_write(unsigned long to, unsigned long len, u32 *retlen, unsigned char *buf);
extern int nandflash_read(unsigned long from, unsigned long len, u32 *retlen, unsigned char *buf, SPI_NAND_FLASH_RTN_T *status);
extern SPI_NAND_FLASH_RTN_T spi_nand_erase_block(u32 block_index);
extern void enable_quad(void);

typedef enum {
	_SPI_NAND_TEST_RTN_NO_ERROR,
	_SPI_NAND_TEST_RTN_ERROR_WITH_SOME_REASON,
	_SPI_NAND_TEST_RTN_PARSER_ERROR,
	_SPI_NAND_TEST_RTN_WRITE_MODE_CHECK_ERROR,
	_SPI_NAND_TEST_RTN_WRITE_MODE_NOT_SET,
	_SPI_NAND_TEST_RTN_READ_MODE_CHECK_ERROR,
	_SPI_NAND_TEST_RTN_READ_MODE_NOT_SET,
	_SPI_NAND_TEST_RTN_PATTER_TEST_ERROR,
	_SPI_NAND_TEST_RTN_DEF_NO
} _SPI_NAND_TEST_RTN_T;

unsigned char SPI_NAND_TEST_PATTERN[] = {
	0xA5,
	0x5A,
	0xFF,
	0x00,
	0x55,
	0xAA
};

typedef struct {
	char			*ptr_parameter_name;
	unsigned int 	default_value;
	unsigned int	current_value;
} _spi_nand_rwtest_parameter_value_t;

typedef struct {
	unsigned int  Round;
	unsigned int  ManualRead_Mode;
	unsigned int  ManualWrite_Mode;
	unsigned int  CLK_Rate;
	unsigned int  CLK_Mode;
	unsigned int  Pattern;
	unsigned long StarBlock;
	unsigned long HTMS;
} _spi_nand_rwtest_input_parameter_t;

typedef enum {
	_SPI_NAND_TEST_CLK_MODE0 = 0x01,
	_SPI_NAND_TEST_CLK_MODE1 = 0x02,
	_SPI_NAND_TEST_CLK_MODE2 = 0x04,
} _SPI_NAND_TEST_CLK_MODE_T;


static _spi_nand_rwtest_parameter_value_t _spi_nand_rwtest_info_t[] = {
	{ "Round",				10,			10	},
	{ "ManualRead_Mode",	0x3,		0x3	},
	{ "ManualWrite_Mode",	0x1,		0x1	},
	{ "CLK_Rate",			25,			25	},
	{ "CLK_Mode",			0x3,		0x3	},
	{ "Pattern",			7,			7	},
	{ "StarBlock",			1,			1	},
	{ "HTMS",				0,			0	},
};

static u32 zd_rand = 0;
static u32 rand_word = 0;


void show_array(u8 *data_buf, int len, char *buf_name)
{
	unsigned int idx;

	printf("%s:\n", buf_name);

	for(idx = 0; idx < len; idx++)
	{
		if((idx % 8 == 0) && (idx != 0))
		{
			printf(" ");
		}

		if(idx % 16 == 0)
		{
			printf("\nidx:0x%04x, addr:0x%08p : ", idx, (data_buf + idx));
		}
		printf(" %02x", data_buf[idx]);
	}
	printf("\n\n");
}

static u8 random_byte(void)
{
	zd_rand = zd_rand * 1664525 + 1013904223;
	return (u8) (zd_rand >> 24);
}

static u32 random_word(void)
{
	rand_word += (VPint(CR_TIMER1_VLR)) * (random_byte());
	return rand_word;
}

static void SPI_NAND_RWTest_Helper(void)
{
	printf("\nusage: spinand_rwtest[Round] [ManualRead_Mode] [ManualWrite_Mode] [CLK_Rate]\n");
	printf("                     [Pattern] [StartAddr]\n");
	printf("[Round]:            0~65535 means test round.   DEFAULT VALUE is 10\n");
	printf("[ManualRead_Mode]:  To setup the ManualRead_Mode Test Enable or not\n");
	printf("                    0~7 is b'000~111, from bit0 to bit2\n");
	printf("                    set bit0 to enable Manual_singleRead\n");
	printf("                    set bit1 to enable Manual_DualRead\n");
	printf("                    set bit2 to enable Manual_QualRead\n");
	printf("                    Default is 0x1\n");
	printf("[ManualWrite_Mode]: To setup the ManualWrite_Mode Test Enable or not\n");
	printf("                    0~3 is b'00~11, from bit0 to bit1\n");
	printf("                    set bit0 to enable Manual_singleWrite\n");
	printf("                    set bit1 to enable Manual_QualWrite\n");	
	printf("                    Default is 0x1\n");
	printf("[CLK Rate]:         0 means auto switch clock test\n");
	if(isEN7526c || isEN751627|| isEN7580) {
		printf("                    1 ~ 31 means (200/CLK Rate)MHZ\n");
	} else {
		printf("                    1 ~ 31 means (250/CLK Rate)MHZ\n");
	}
	printf("                    DEFAULT VALUE is 0\n");
	printf("[CLK Mode]:         When [CLK Rate] is equal to 0,\n");	
	printf("                    set bit0 to enable test clock 25MHz\n");
	printf("                    set bit1 to enable test clock 50MHz\n");
	printf("                    set bit2 to enable test clock 100MHz\n");
	printf("[TestPattern]:      To setup test pattern\n");
	printf("                    0: A5, 1: 5A, 2: FF, 3: 00, 4: 55, 5: AA\n");
	printf("                    6: Random, 7: All\n");
	printf("                    DEFAULT VALUE is 0\n");
	printf("[StartAddr]:        To setup the test start block\n");
	printf("                    (Test Till to the end of Flash)\n");
}

int spinand_rw_test_input_parser(int argc, char * const argv[], _spi_nand_rwtest_input_parameter_t *in_para)
{
	unsigned int  					idx;
	struct SPI_NAND_FLASH_INFO_T	flash_info_t;

	if(argc < 2 ){
		return -1;
	}

	SPI_NAND_Flash_Get_Flash_Info(&flash_info_t);

	for( idx=2 ; idx<=argc ; idx++) {
		if(argv[idx-1]) {
			_spi_nand_rwtest_info_t[idx-2].current_value = simple_strtoul(argv[idx-1], NULL, 10);
		}
	}

	if(_spi_nand_rwtest_info_t[7].current_value == 1) {
		printf("The Test Parameter Setting\n");
		printf("Parameter_Name     Default_Value     Current_Value\n");
		printf("---------------------------------------------\n");
		for( idx=0 ; idx< (sizeof(_spi_nand_rwtest_info_t)/sizeof(_spi_nand_rwtest_parameter_value_t)) ; idx++)
		{
			printf("%-18s, 0x%-18x, 0x%-18x\n", _spi_nand_rwtest_info_t[idx].ptr_parameter_name, _spi_nand_rwtest_info_t[idx].default_value, _spi_nand_rwtest_info_t[idx].current_value);
		}
	}
	in_para->Round 				= _spi_nand_rwtest_info_t[0].current_value;
	in_para->ManualRead_Mode	= _spi_nand_rwtest_info_t[1].current_value;
	in_para->ManualWrite_Mode	= _spi_nand_rwtest_info_t[2].current_value;
	in_para->CLK_Rate			= _spi_nand_rwtest_info_t[3].current_value;
	in_para->CLK_Mode			= _spi_nand_rwtest_info_t[4].current_value;
	in_para->Pattern			= _spi_nand_rwtest_info_t[5].current_value;
	in_para->StarBlock			= _spi_nand_rwtest_info_t[6].current_value;
	in_para->HTMS				= _spi_nand_rwtest_info_t[7].current_value;

	return 0;
}

static int spinand_write_speed_check(_spi_nand_rwtest_input_parameter_t *in_para, SPI_NAND_FLASH_WRITE_SPEED_MODE_T speed_mode )
{
	_SPI_NAND_TEST_RTN_T	rtn_status = _SPI_NAND_TEST_RTN_NO_ERROR;

	if( ((in_para->ManualWrite_Mode) &  (1 << speed_mode)) )
	{
		if( speed_mode == SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE )
		{
		}
		else if( speed_mode == SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD )
		{
		}
		else
		{
			rtn_status = _SPI_NAND_TEST_RTN_WRITE_MODE_CHECK_ERROR;
		}
	}
	else
	{
		rtn_status = _SPI_NAND_TEST_RTN_WRITE_MODE_NOT_SET;
	}

	return (rtn_status);
}

static int spinand_read_speed_check(_spi_nand_rwtest_input_parameter_t *in_para, SPI_NAND_FLASH_READ_SPEED_MODE_T speed_mode )
{
	_SPI_NAND_TEST_RTN_T	rtn_status = _SPI_NAND_TEST_RTN_NO_ERROR;

	if( ((in_para->ManualRead_Mode) &  (1 << speed_mode)) )
	{
		if( speed_mode == SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE )
		{
		}
		else if( speed_mode == SPI_NAND_FLASH_READ_SPEED_MODE_DUAL )
		{
		}
		else if( speed_mode == SPI_NAND_FLASH_READ_SPEED_MODE_QUAD )
		{
		}
		else
		{
			printf("\nSPI_NAND_RW_TEST: NO SUCH READ SPEED:%d\n", speed_mode);
			rtn_status = _SPI_NAND_TEST_RTN_READ_MODE_CHECK_ERROR;
		}
	}
	else
	{
		rtn_status = _SPI_NAND_TEST_RTN_READ_MODE_NOT_SET;
	}

	return (rtn_status);
}

static int spinand_rw_test_with_pattern(unsigned char test_pattern, _spi_nand_rwtest_input_parameter_t *in_para)
{

	SPI_NAND_FLASH_WRITE_SPEED_MODE_T	write_speed;
	SPI_NAND_FLASH_READ_SPEED_MODE_T	read_speed;
	struct SPI_NAND_FLASH_INFO_T		flash_info_t;
	u32									test_addr;
	u32									write_return_len=0;
	u32									read_return_len=0;
	SPI_NAND_FLASH_RTN_T				flash_rtn_status;
	u32									rand_page, page_per_block;
	u32									block, total_block;
	int									i;
	SPI_NAND_FLASH_RTN_T 				status;

	printf("\nSPI_NAND_RW_TEST: pattern=0x%x\n", test_pattern);

	/* 1. Get Flash Information */
	SPI_NAND_Flash_Get_Flash_Info(&flash_info_t);

	if(sizeof(test_pattern_write_data_buf) < flash_info_t.page_size) {
		printf("flash page size:0x%x is more than array size:0x%x\n\n", flash_info_t.page_size, sizeof(test_pattern_write_data_buf) );
		return _SPI_NAND_TEST_RTN_PATTER_TEST_ERROR;
	}

	page_per_block = flash_info_t.erase_size / flash_info_t.page_size;

	/* 2. Preparre Test pattern buffer */
	if(test_pattern != 0x01) {
		memset( test_pattern_write_data_buf, test_pattern, sizeof(test_pattern_write_data_buf));
		memset( test_pattern_write_oob_buf, test_pattern, sizeof(test_pattern_write_oob_buf));
	} else {
		for(i = 0; i < sizeof(test_pattern_write_data_buf); i++) {
			test_pattern_write_data_buf[i] = random_byte() & 0xff;
		}
		for(i = 0; i < sizeof(test_pattern_write_oob_buf); i++) {
			test_pattern_write_oob_buf[i] = random_byte() & 0xff;
		}
	}
	test_pattern_write_oob_buf[0] = 0xFF;
	test_pattern_write_oob_buf[1] = 0xFF;

	if(in_para->HTMS == 0) {
		total_block = (nand_logic_size / flash_info_t.erase_size);
	} else {
		total_block = (flash_info_t.device_size/ flash_info_t.erase_size);
	}

	for(write_speed = SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE; write_speed < SPI_NAND_FLASH_WRITE_SPEED_MODE_DEF_NO; write_speed++ ) {
		if( spinand_write_speed_check(in_para, write_speed) != _SPI_NAND_TEST_RTN_NO_ERROR) {
			continue;
		} else {
			/* Set Flash Information */
			flash_info_t.write_mode = write_speed;
			SPI_NAND_Flash_Set_Flash_Info(&flash_info_t);

			for(block = in_para->StarBlock; block < total_block; block++) {
				rand_page = (random_word() % page_per_block);
				test_addr = ((block * page_per_block + rand_page) * flash_info_t.page_size);

				printf(".");

				/* 1. Erase current test block */
				if(in_para->HTMS == 0) {
					flash_rtn_status = nandflash_erase((block * flash_info_t.erase_size), flash_info_t.erase_size);
				} else {
					if (en7512_nand_erase(test_addr)) {
						/* skip bad block */
						if (en7512_nand_check_block_bad(block * flash_info_t.erase_size, BAD_BLOCK_RAW))
				        {
				            printf("erase fail, Skip bad block: 0x%x \n", block);
				            continue;
				        }
			            flash_rtn_status = SPI_NAND_FLASH_RTN_ERASE_FAIL;
			        } else {
						flash_rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
			        }
				}
				if( flash_rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
					/* 2. Write Test Pattern */
					if(in_para->HTMS == 0) {
						flash_rtn_status = nandflash_write(test_addr, flash_info_t.page_size, &write_return_len, test_pattern_write_data_buf);
					} else {
						if (en7512_nand_exec_write_page(test_addr / flash_info_t.page_size, test_pattern_write_data_buf, test_pattern_write_oob_buf)) {
							/* skip bad block */
							if (en7512_nand_check_block_bad(block * flash_info_t.erase_size, BAD_BLOCK_RAW))
					        {
					            printf("write fail, Skip bad block: 0x%x \n", block);
					            continue;
					        }
				            flash_rtn_status = SPI_NAND_FLASH_RTN_PROGRAM_FAIL;
				        } else {
							flash_rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
				        }
					}
					if(  flash_rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR ) {
						/* 3. Read Check (For the read mode if user enable) */
						for( read_speed =SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE ; read_speed<SPI_NAND_FLASH_READ_SPEED_MODE_DEF_NO ; read_speed++ ) {
							SPI_NAND_Flash_Clear_Read_Cache_Data();

							if( spinand_read_speed_check(in_para, read_speed) !=  _SPI_NAND_TEST_RTN_NO_ERROR) {
								continue;
							} else {
								/* Set Flash Information */
								flash_info_t.read_mode = read_speed;
								SPI_NAND_Flash_Set_Flash_Info(&flash_info_t);

								memset(test_pattern_read_data_buf, 0, sizeof(test_pattern_read_data_buf));
								memset(test_pattern_read_oob_buf, 0, sizeof(test_pattern_read_oob_buf));
								if(in_para->HTMS == 0) {
									flash_rtn_status = nandflash_read(test_addr, flash_info_t.page_size, &read_return_len, test_pattern_read_data_buf, &status);
								} else {
									if (en7512_nand_exec_read_page(test_addr / flash_info_t.page_size, test_pattern_read_data_buf, test_pattern_read_oob_buf) != 0) {
										/* skip bad block */
										if (en7512_nand_check_block_bad(block * flash_info_t.erase_size, BAD_BLOCK_RAW))
								        {
											read_speed = SPI_NAND_FLASH_READ_SPEED_MODE_DEF_NO;
								            printf("read fail, Skip bad block: 0x%x \n", block);
								            continue;
								        }
							            flash_rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
							        } else {
										flash_rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
							        }
								}
								if( flash_rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR ) {
									/* check data */
									if( memcmp(test_pattern_write_data_buf, test_pattern_read_data_buf , (flash_info_t.page_size) ) !=0 ) {
										printf("spinand_rw_test_with_pattern: test pattern compare data error at:\n");
										printf("test_addr=0x%x, page:0x%04x, block:0x%04x, CPOL:%lx, rw speed:r%d,w%d\n", test_addr, rand_page, block, VPint(0xbfa10098), read_speed, write_speed);

										show_array(test_pattern_write_data_buf, sizeof(test_pattern_write_data_buf), "test_pattern_write_data_buf");
										show_array(test_pattern_read_data_buf, sizeof(test_pattern_read_data_buf), "test_pattern_read_data_buf");
										mdelay(20);

										return _SPI_NAND_TEST_RTN_PATTER_TEST_ERROR;
									}
									/* check oob */
									if(in_para->HTMS == 1) {
										if( memcmp(test_pattern_write_oob_buf, test_pattern_read_oob_buf , (flash_info_t.oob_free_layout->oobsize) ) !=0 ) {
											printf("spinand_rw_test_with_pattern: test pattern compare oob error at:\n");
											printf("test_addr=0x%x, page:0x%04x, block:0x%04x, CPOL:%lx, rw speed:r%d,w%d\n", test_addr, rand_page, block, VPint(0xbfa10098), read_speed, write_speed);

											show_array(test_pattern_write_oob_buf, sizeof(test_pattern_write_oob_buf), "test_pattern_write_oob_buf");
											show_array(test_pattern_read_oob_buf, sizeof(test_pattern_read_oob_buf), "test_pattern_read_oob_buf");
											mdelay(20);

											return _SPI_NAND_TEST_RTN_PATTER_TEST_ERROR;
										}
									}
								} else {
									printf("spinand_rw_test_with_pattern: read fail at addr =0x%x\n\n", test_addr);
									return _SPI_NAND_TEST_RTN_PATTER_TEST_ERROR;
								}
							}
						}
					} else {
						printf("spinand_rw_test_with_pattern: write pattern fail at addr =0x%x, with test pattern0x%x\n\n", test_addr, test_pattern );
						return _SPI_NAND_TEST_RTN_PATTER_TEST_ERROR;
					}
				} else {
					printf("spinand_rw_test_with_pattern: erase fail at addr =0x%x\n\n", test_addr );
					return _SPI_NAND_TEST_RTN_PATTER_TEST_ERROR;
				}
			}
		}
	}

	return (_SPI_NAND_TEST_RTN_NO_ERROR);
}

int do_spinand_rw_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32									test_round;
	u32									idx;
	unsigned char						test_pattern;
	struct SPI_NAND_FLASH_INFO_T		flash_info_t;
	_spi_nand_rwtest_input_parameter_t	in_para;

	/* 1. Parser User Input and setup Parameter */
	if( spinand_rw_test_input_parser(argc, argv, &in_para) != 0 ) {
		printf("Input Error, the input format is: \n");
		SPI_NAND_RWTest_Helper();
	} else {
		/* 2.1 Get Current Flash Information */
		SPI_NAND_Flash_Get_Flash_Info(&flash_info_t);
		printf("Flash Info: \nChip Name=%s \nchip size=0x%x \nerase size=0x%x\n\n", flash_info_t.ptr_name, flash_info_t.device_size, flash_info_t.erase_size);

		if(((in_para.ManualWrite_Mode) & (1 << SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD)) ||
			((in_para.ManualRead_Mode) & (1 << SPI_NAND_FLASH_READ_SPEED_MODE_QUAD))) {
				enable_quad();
		}

		/* 2.3 Test each round */
		for(test_round = 0 ; test_round < (in_para.Round) ; test_round++) {
			printf("================================================================\n");
			printf("SPI_NAND_RW_TEST: ROUND %d start\n", test_round + 1);

			/* FPGA SPI clock is fix */
			if(!isFPGA) {
				/* Set CPOL */
				VPint(0xBFA10098) = (test_round % 2);

				if( (in_para.CLK_Rate) == 0 ) { /* Aoto test for multiple spi clock frequency */
					switch(test_round % 6) {
						case 0:
							if(in_para.CLK_Mode& _SPI_NAND_TEST_CLK_MODE0) {
								/* set clock 25MHz */
								spi_set_clock_speed(25);
								/* adjust the delay parameter */
								VPint(0xbfa1009c) = 0x9;
								VPint(0xbfa10008) = 0x0;
							} else {
								printf("CLK Mode has no mode0\n\n");
								continue;
							}
							break;

						case 1:
							if(in_para.CLK_Mode & _SPI_NAND_TEST_CLK_MODE0) {
								/* set clock 25MHz */
								spi_set_clock_speed(25);
								/*  adjust the delay parameter */
								VPint(0xbfa1009c) = 0x9;
								VPint(0xbfa10008) = 0x0;
							} else {
								printf("CLK Mode has no mode0\n\n");
								continue;
							}
							break;

						case 2:
							if(in_para.CLK_Mode & _SPI_NAND_TEST_CLK_MODE1) {
								/* set clock 50MHz */
								spi_set_clock_speed(50);
								/* adjust the delay parameter */
								VPint(0xbfa1009c) = 0x9;
								VPint(0xbfa10008) = 0x0;
							} else {
								printf("CLK Mode has no mode1\n\n");
								continue;
							}
							break;

						case 3:
							if(in_para.CLK_Mode & _SPI_NAND_TEST_CLK_MODE1) {
								/* set clock 50MHz */
								spi_set_clock_speed(50);
								/*adjust the delay parameter */
								VPint(0xbfa1009c) = 0x9;
								VPint(0xbfa10008) = 0x0;
							} else {
								printf("CLK Mode has no mode1\n\n");
								continue;
							}
							break;

						case 4:
							if(in_para.CLK_Mode & _SPI_NAND_TEST_CLK_MODE2) {
								/* set clock 100MHz */
								spi_set_clock_speed(100);
								/* adjust the delay parameter */
								/* winbond */
								if(flash_info_t.mfr_id == 0xEF) {
									VPint(0xbfa1009c) = 0xa;
									VPint(0xbfa10008) = 0x0;
								} else {
									VPint(0xbfa1009c) = 0xb;
									VPint(0xbfa10008) = 0x0;
								}
							} else {
								printf("CLK Mode has no mode2\n\n");
								continue;
							}
							break;

						case 5:
							if(in_para.CLK_Mode & _SPI_NAND_TEST_CLK_MODE2) {
								/* set clock 100MHz */
								spi_set_clock_speed(100);
								/* adjust the delay parameter */
								/* winbond */
								if(flash_info_t.mfr_id == 0xEF) {
									VPint(0xbfa1009c) = 0xa;
									VPint(0xbfa10008) = 0x0;
								} else {
									VPint(0xbfa1009c) = 0xb;
									VPint(0xbfa10008) = 0x0;
								}
							} else {
								printf("CLK Mode has no mode2\n\n");
								continue;
							}
							break;
					}
				} else { 	/* if( (in_para.CLK_Rate) == 0 ) */
					if(in_para.HTMS == 1) {
						spi_set_clock_speed((in_para.CLK_Rate));
					}
				}
			} else {
				/* Set CPOL */
				if(isEN751627|| isEN7580) {
					VPint(0xBFA10098) = (test_round % 2);
				}
			}

			if(in_para.HTMS == 1) {
				printf("CPOL register =0x%lx\n", VPint(0xBFA10098));
				printf("0xbfa1009c register =0x%lx\n", VPint(0xbfa1009c));
				printf("0xbfa10008 register =0x%lx\n", VPint(0xbfa10008));
			}

			printf("\n\n");

			for(idx = 0 ; idx < 7 ; idx++) {
				if((idx < 6) && ((in_para.Pattern == idx) || in_para.Pattern == 7)) { 		/* Normal Test Pattern */
					test_pattern = SPI_NAND_TEST_PATTERN[idx];

					if( spinand_rw_test_with_pattern(test_pattern, &in_para) != _SPI_NAND_TEST_RTN_NO_ERROR) {
						printf("\ndo_spinand_rw_test: fail at round %d, with random pattern=0x%x\n\n", test_round + 1, test_pattern);
						printf("\n==================Test Fail ==================\n");
						return 0;
					}
				} else if((idx == 6) && ( in_para.Pattern == 6 || in_para.Pattern == 7)) {			/* Random Test Pattern */
					test_pattern = 0x01;

					if( spinand_rw_test_with_pattern(test_pattern, &in_para) != _SPI_NAND_TEST_RTN_NO_ERROR) {
						printf("\ndo_spinand_rw_test: fail at round %d, with random pattern=0x%x\n\n", test_round + 1, test_pattern);
						printf("\n==================Test Fail ==================\n");
						return 0;
					}
				}
				/* wait for printf */
				mdelay(1);
			}

			printf("\nSPI NAND RW_TEST: ROUND %d finish\n", test_round + 1);
			printf("================================================================\n");
			/* wait for printf */
			mdelay(50);
		}
	}


	printf("\n==================Test success ==================\n");

	return 0;
}

static int do_spinand_erase_all(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct SPI_NAND_FLASH_INFO_T	flash_info_t;
	u32								block_index;
	int								isSkipBadBlock = 1;

	if(argv[1]) {
		isSkipBadBlock = simple_strtoul(argv[1], NULL, 10);
	}

	SPI_NAND_Flash_Get_Flash_Info(&flash_info_t);

	for(block_index = 0; block_index < (flash_info_t.device_size / flash_info_t.erase_size); block_index++) {
		if(isSkipBadBlock) {
			if(en7512_nand_check_block_bad(block_index * flash_info_t.erase_size, 0) == 1) {
				continue;
			}
		}
		spi_nand_erase_block(block_index);
	}

	printf("erase all done\n");

	return 0;
}

U_BOOT_CMD(
		spinand_rwtest,		16,		0,		do_spinand_rw_test,
		"spinand_rwtest - flash test command\n",
		"spinand_rwtest usage:\n"
		"	spinand_rwtest [Round] [ManualRead_Mode] [ManualWrite_Mode] [CLK_Rate] [Pattern] [StartAddr]"
);

U_BOOT_CMD(
		spinand_eraseall,	17,		0,		do_spinand_erase_all,
		"spinand_eraseall - flash erase command\n",
		"spinand_eraseall usage:\n"
		"	spinand_erase_all [skip bad]"
);

