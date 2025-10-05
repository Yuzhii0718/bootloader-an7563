#include "skbuff.h"
#include "eth.h"
#include "multiupgrade.h"
#include "linux/stddef.h"
#include <libfdt.h>

#include "common.h"
#include <asm/tc3162.h>

#include "ecnt_eth.h"
#include <flashhal.h>
#include <trx.h>
#include <flash_layout/tc_partition.h>

#if defined(TCSUPPORT_CPU_EN7512)||defined(TCSUPPORT_CPU_EN7521)
#ifdef TCSUPPORT_SPI_CONTROLLER_ECC
#include <spi/spi_nand_flash.h>
#else
#include "spi_nand_flash.h"
#endif
#else
#include <nandflash.h>
#endif


#if !defined(TCSUPPORT_CT_WAN_PTM)
#define DBG_PRINT
#endif


enum IMGTYPE
{
	IMG_TCLINUX,
	IMG_TCLINUXALLINONE,
	IMG_TCBOOT,
};

extern struct mtd_info mtd;
#ifdef TCSUPPORT_BB_NAND
extern struct ra_nand_chip ra;
#endif


/*
	pkt format is shown as below:		
	byte 0~1 	pkt type		
	byte 2~3 	pkt sequence
	byte 4~11	pkt tag
	byte 12~15	total length
	byte 16~19	CRC of firmware fragment
	byte 20~23	CRC of whole firmware 
	1024bytes	data
*/
	
#define PART_FDT_MAGIC 				0xD00DFEED

#define LAST_MULTIPKT					0x40
#define NOT_LAST_MULTIPKT				0x20
#define IMG_LEN_OFFSET					20
#define LT_OFFSET						8
#define CRC_OFFSET						28
#define MULTI_UPGRADE_PKT_HEADLEN 	32
#define MULTI_UPGRADE_DATA_LEN 		1024

unsigned long receive_len = 0;
char startMultiUpgrade = 0;
#define FLASH_ERASE_SIZE			0x10000
extern int nand_logic_size;
extern int nand_flash_avalable_size;
extern u32 reservearea_size;

#define ROMFILE_SIZE	ecnt_get_romfile_size()
#define TCBOOT_BASE     0x0
#define TCBOOT_SIZE		ecnt_get_boot_size()

#if !defined(TCSUPPORT_C1_TRUE)
volatile char finishMultiUpgrade = 0;
#endif

#define MULTI_BUF_BASE					0x81800000
#define ENV_OFFSET					0x7C000
#define ENV_SIZE					0x4000
#define TRX_SIZE					sizeof(struct trx_header)


extern unsigned long Jiffies;
#if 0
/**********************************************************************/
/* The following was grabbed and tweaked from the old snippets collection
 * of public domain C code. */

/**********************************************************************\
|* Demonstration program to compute the 32-bit CRC used as the frame  *|
|* check sequence in ADCCP (ANSI X3.66, also known as FIPS PUB 71     *|
|* and FED-STD-1003, the U.S. versions of CCITT's X.25 link-level     *|
|* protocol).  The 32-bit FCS was added via the Federal Register,     *|
|* 1 June 1982, p.23798.  I presume but don't know for certain that   *|
|* this polynomial is or will be included in CCITT V.41, which        *|
|* defines the 16-bit CRC (often called CRC-CCITT) polynomial.  FIPS  *|
|* PUB 78 says that the 32-bit FCS reduces otherwise undetected       *|
|* errors by a factor of 10^-5 over 16-bit FCS.                       *|
\**********************************************************************/

/* Copyright (C) 1986 Gary S. Brown.  You may use this program, or
   code or tables extracted from it, as desired without restriction.*/

/* First, the polynomial itself and its table of feedback terms.  The  */
/* polynomial is                                                       */
/* X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0 */
/* Note that we take it "backwards" and put the highest-order term in  */
/* the lowest-order bit.  The X^32 term is "implied"; the LSB is the   */
/* X^31 term, etc.  The X^0 term (usually shown as "+1") results in    */
/* the MSB being 1.                                                    */

/* Note that the usual hardware shift register implementation, which   */
/* is what we're using (we're merely optimizing it by doing eight-bit  */
/* chunks at a time) shifts bits into the lowest-order term.  In our   */
/* implementation, that means shifting towards the right.  Why do we   */
/* do it this way?  Because the calculated CRC must be transmitted in  */
/* order from highest-order term to lowest-order term.  UARTs transmit */
/* characters in order from LSB to MSB.  By storing the CRC this way,  */
/* we hand it to the UART in the order low-byte to high-byte; the UART */
/* sends each low-bit to hight-bit; and the result is transmission bit */
/* by bit from highest- to lowest-order term without requiring any bit */
/* shuffling on our part.  Reception works similarly.                  */

/* The feedback terms table consists of 256, 32-bit entries.  Notes:   */
/*                                                                     */
/*  1. The table can be generated at runtime if desired; code to do so */
/*     is shown later.  It might not be obvious, but the feedback      */
/*     terms simply represent the results of eight shift/xor opera-    */
/*     tions for all combinations of data and CRC register values.     */
/*                                                                     */
/*  2. The CRC accumulation logic is the same for all CRC polynomials, */
/*     be they sixteen or thirty-two bits wide.  You simply choose the */
/*     appropriate table.  Alternatively, because the table can be     */
/*     generated at runtime, you can start by generating the table for */
/*     the polynomial in question and use exactly the same "updcrc",   */
/*     if your application needn't simultaneously handle two CRC       */
/*     polynomials.  (Note, however, that XMODEM is strange.)          */
/*                                                                     */
/*  3. For 16-bit CRCs, the table entries need be only 16 bits wide;   */
/*     of course, 32-bit entries work OK if the high 16 bits are zero. */
/*                                                                     */
/*  4. The values must be right-shifted by eight bits by the "updcrc"  */
/*     logic; the shift must be unsigned (bring in zeroes).  On some   */
/*     hardware you could probably optimize the shift in assembler by  */
/*     using byte-swap instructions.                                   */
static const uint32_t crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

#define UPDC32(octet,crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

uint32_t crc32buf(char *buf, size_t len, int flags)
{
#if defined(TCSUPPORT_CPU_EN7512)||defined(TCSUPPORT_CPU_EN7521)
	SPI_NAND_FLASH_RTN_T status;
#endif

      uint32_t crc;
	  /* frank added 20101208 */
	  char ch;
	#ifdef TCSUPPORT_NEW_SPIFLASH
	#define READBUFSIZE 4096
	  unsigned long retlen=0;
	  unsigned char membuf[READBUFSIZE];
	  uint32 i=0, index=0;
	  uint32 curLen= len;
	  uint32 from = buf;
	#endif

      crc = 0xFFFFFFFF;

	  /* for read data from flash */
	  if (flags == 0) {
	#ifdef TCSUPPORT_NEW_SPIFLASH
		from &= 0x03FFFFFF;
		curLen = len % READBUFSIZE;
		index = 0;
		while(len) {			
			flash_read((unsigned long)(from+index), curLen, &retlen, membuf);
			for (i=0; i<curLen; i++)
			{
				crc = UPDC32(membuf[i], crc);
			}
			len -= curLen;
			index += curLen;
			curLen = READBUFSIZE;
		}
	#else
    	  for ( ; len; --len, ++buf)
	      {
				ch = READ_FLASH_CACHE(buf, &status);
				crc = UPDC32(ch, crc);
//				crc = UPDC32(*buf, crc);
      	  }
	#endif
	  /* for read data from sdram */  
	  }	else {
    	  for ( ; len; --len, ++buf)
	      {
				crc = UPDC32(*buf, crc);
      	  }
	  }
      return (uint32_t)crc;
}

#endif
int multiupgrade_process(sk_buff *skb, char *mac)
{

#if !defined(TCSUPPORT_C1_TRUE)
	int i = 0;
	uint8 multi_led;
#endif
#if !defined(TCSUPPORT_C1_OBM)
	if(mac[0] & 0x1)
#endif
	{
		if (0){
			;
		}
		else if(memcmp((skb->data + MULTIPKT_TAG_OFFSET), MULTI_UPGRADE_PKT_TAG, (sizeof(MULTI_UPGRADE_PKT_TAG)-1)) == 0){
			if (skb->len < 1048) {
#ifdef DBG_PRINT
			printf("skb->len < 1048 \r\n");
#endif
			return 0;
			}					
			if(startMultiUpgrade == 0){
				printf("\nStartMultiUpgrade\n");
				setEtherRateLimit (1);
				startMultiUpgrade = 1;
				LED_OEN(internet_gpio); 
				LED_OEN(dsl_gpio);
#if !defined(TCSUPPORT_C1_TRUE)
				for(i=0; i<MAXMULTILEDNUM; i++){
					multi_led = multi_upgrade_gpio[i];
					if(multi_led != 0)
#if defined(TCSUPPORT_CT_WAN_PTM)
					{
						if(multi_led > 31){
							if(multi_led > 47){
								regWrite32(CR_GPIO_CTRL3,regRead32(CR_GPIO_CTRL3)|(1<<((multi_led-48)*2)));
							}else{
								regWrite32(CR_GPIO_CTRL2,regRead32(CR_GPIO_CTRL2)|(1<<((multi_led-32)*2)));
							}
							regWrite32(CR_GPIO_ODRAIN1,regRead32(CR_GPIO_ODRAIN1)|(1<<(multi_led-32))); 
							}else{
								if(multi_led > 15){
									regWrite32(CR_GPIO_CTRL1,regRead32(CR_GPIO_CTRL1)|(1<<((multi_led-16)*2)));
								}else{
									regWrite32(CR_GPIO_CTRL,regRead32(CR_GPIO_CTRL)|(1<<(multi_led*2)));
								}
								regWrite32(CR_GPIO_ODRAIN,regRead32(CR_GPIO_ODRAIN)|(1<<multi_led));
							}
						}
#else
						LED_OEN(multi_led);
#endif
				}		
#endif
				LED_OFF(internet_gpio); 		
			}
			MultiUpgradeHandle(skb);
			return 0;
			}				
		}	
		if(startMultiUpgrade == 1){
			return 0;
		}
	return 1;
}

unsigned long get_rf_offset(void)
{
	return ecnt_get_romfile_mtd_offset();
}

unsigned long get_reservearea_offset(void)
{
	unsigned long reservearea_offset = 0;
	unsigned int is_EMMC = 0;

#ifdef TCSUPPORT_EMMC
	is_EMMC = (unsigned int)isEMMC;
#endif
	
#ifdef TCSUPPORT_NEW_SPIFLASH
	if (IS_SPIFLASH) {
		reservearea_offset = mtd.size - (TCSUPPORT_RESERVEAREA_BLOCK * RESERVEAREA_ERASE_SIZE);
	} else 
#endif
	if (IS_NANDFLASH || is_EMMC) {
#if defined(TCSUPPORT_BB_NAND) || defined(TCSUPPORT_EMMC)
		reservearea_offset = nand_flash_avalable_size - (TCSUPPORT_RESERVEAREA_BLOCK * RESERVEAREA_ERASE_SIZE);
#endif
	}

	return reservearea_offset;
}
#if !defined(TCSUPPORT_OPENWRT) && !defined(RDKB_BUILD)
unsigned long get_backuprf_offset(void)
{
	return (get_reservearea_offset() + BACKUPROMFILE_RA_OFFSET);
}
#endif

void multiupgrade_update_image(unsigned long from, unsigned long to, unsigned long datalen)
{
	unsigned long retlen;

#ifdef DBG_PRINT
	printf("multiupgrade_update_image: from:0x%lx, to:0x%lx, length:0x%lx\n", from, to, datalen);
#endif

	flash_erase(to, datalen);
	printf("\n");
	flash_write(to, datalen, &retlen, (const unsigned char *)(from));
	printf("\n");
}

void multiupgrade_update_tcboot(unsigned long from, unsigned long to, unsigned long datalen)
{
#ifdef DBG_PRINT
	printf("Write tcboot.bin\n");
#endif

	multiupgrade_update_image(from, to, datalen);
}

void multiupgrade_update_tclinux(unsigned long from, unsigned long to, unsigned long datalen)
{
#ifdef DBG_PRINT
	printf("Write tclinux.bin\n");
#endif

	multiupgrade_update_image(from, to, datalen);
}

#if !defined(TCSUPPORT_OPENWRT) && !defined(RDKB_BUILD)
void multiupgrade_erase_romfile(int isEraseRomfile)
{
	unsigned long romfile_offset = 0;
	unsigned long backuprf_offset = 0;

	romfile_offset = get_rf_offset();
	backuprf_offset = get_backuprf_offset();

	if(isEraseRomfile) {
#ifdef DBG_PRINT
		printf("Erase romfile.cfg from 0x%lx\n", romfile_offset);
#endif
		flash_erase(romfile_offset, ROMFILE_SIZE);
		printf("\n");
	}

#ifdef DBG_PRINT
	printf("Erase backup romfile.cfg from 0x%lx\n", backuprf_offset);
#endif	
	flash_erase(backuprf_offset, ROMFILE_SIZE);//erase backup romfile for reset with default setting
	printf("\n");

}
#endif

void MultiWriteImage(char *ptr, unsigned long datalen, int isAllinone)
{
	unsigned long tclinux_offset = 0;
	unsigned long tclinux_len = 0;
	
	/*
		turn off ADSL LED and turn on Internet LED
	*/
	LED_ON(dsl_gpio);
	LED_ON(internet_gpio);

	switch(isAllinone) {
		case IMG_TCBOOT:
#ifdef DBG_PRINT
			printf("multi-upgrade tcboot.bin file\n");
			printf("Write to flash from 0x%X to 0x%X with 0x%X bytes\n", MULTI_BUF_BASE, TCBOOT_BASE, TCBOOT_SIZE);
#endif
			/* update tcboot.bin */
			multiupgrade_update_tcboot(MULTI_BUF_BASE, TCBOOT_BASE, TCBOOT_SIZE);
			break;

		case IMG_TCLINUX:
			tclinux_offset = ecnt_get_tclinux_mtd_offset();
			tclinux_len = datalen;
			
#ifdef DBG_PRINT
			printf("multi-upgrade tclinux.bin file\n");
			printf("Write to flash from 0x%X to 0x%X with 0x%X bytes\n", MULTI_BUF_BASE, tclinux_offset, datalen);
#endif

			/* erase romfile and backup romfile */
#if !defined(TCSUPPORT_OPENWRT) && !defined(RDKB_BUILD)
			multiupgrade_erase_romfile(1);
#endif
			/* update tclinux.bin */
			multiupgrade_update_tclinux(MULTI_BUF_BASE , tclinux_offset, tclinux_len);

			break;

		case IMG_TCLINUXALLINONE:
			tclinux_offset = get_rf_offset() + ROMFILE_SIZE;
			tclinux_len = datalen - tclinux_offset;
	
#ifdef DBG_PRINT
			printf("multi-upgrade tclinux_allinone file\n");
			printf("Write to flash from 0x%X to 0x%X with 0x%X bytes\n", MULTI_BUF_BASE, TCBOOT_BASE, datalen);
#endif
			/* update tclinux_allinone */
			multiupgrade_update_tcboot(MULTI_BUF_BASE, TCBOOT_BASE, datalen);
			
			/* erase backup romfile */
#if !defined(TCSUPPORT_OPENWRT) && !defined(RDKB_BUILD)
			multiupgrade_erase_romfile(0);
#endif
			break;

		default:
			printf("Unknown image.\n");
			break;
	}
	
	/*
		turn on ADSL and Internet LED
	*/	
	printf("upgrade finished !\n");
	#ifdef DBG_PRINT
	LED_ON(dsl_gpio);
	LED_OFF(internet_gpio);	
	#endif
}

unsigned long getimagelen()
{
#ifdef TCSUPPORT_NEW_SPIFLASH
	if (IS_SPIFLASH){ 
		return (mtd.size-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*TCSUPPORT_RESERVEAREA_BLOCK);
	}else 
#endif
#ifdef TCSUPPORT_EMMC
	if(isEMMC) {
		return (get_emmc_user_capacity()-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*TCSUPPORT_RESERVEAREA_BLOCK);
	}
#endif

	if (IS_NANDFLASH) {
#ifdef TCSUPPORT_BB_NAND	
//		if (regRead32(CR_AHB_HWCONF) & (0x00080000)) 
//			return ((1<<ra.flash->chip_shift) - 7*(1<<ra.flash->erase_shift)) ;
//		else
			return ((1<<ra.flash->chip_shift)-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*TCSUPPORT_RESERVEAREA_BLOCK);
#endif	
	}	
	return 0; //COV, CID 186048
}


void
MultiUpgradeHandle(sk_buff *skb)
{
	unsigned long LenAndType = __swab32(ntohl(*(unsigned long *)(skb->data + LT_OFFSET)));
	unsigned long pktseqMask = 0xffff;
	unsigned long pktseq = ((LenAndType >> 16) & pktseqMask);
	unsigned long imagelen = __swab32(ntohl(*(unsigned long *)(skb->data + IMG_LEN_OFFSET)));
	unsigned char last_packet = 0;
	static unsigned long Jiffiestmp;
	static char tag = 0;
	static int oldSeq=-1;
	static int startSeq = -1;
	static int maxSeq = 0, recvSeq = 0;
	int cal_check_sum = 0;
	int raw_check_sum = 0;
	unsigned char *crc_ptr = NULL;
	int isallinone = 0;

	unsigned long retlen = 0;
	static char badimg = 0;

    #ifdef TCSUPPORT_SECURE_BOOT
    unsigned long keyLen;
    #endif
	
	if(finishMultiUpgrade == 1){
		return; //upgrade success
	}
/* may modify for general sun.zhu_20140503 */	
	if (startSeq == -1)
	{
		startSeq = pktseq;
		oldSeq = pktseq - 1; 		
		receive_len = 0;
		maxSeq = imagelen / MULTI_UPGRADE_DATA_LEN;
		if (imagelen % MULTI_UPGRADE_DATA_LEN)
			maxSeq++;
		Jiffiestmp = Jiffies;
	}

	/*
		check seq
	*/
	if (pktseq!=((oldSeq+1) & pktseqMask))
	{
		if ((pktseq != 1) || (oldSeq != (maxSeq & pktseqMask))) // on only one condition it would be fine: pktseq == 1 && oldSeq == macSeq 
		{
			goto error;
		}
	}

	oldSeq = pktseq;        // always assign current packet seq to old seq, or the (oldSeq != maxSeq) would be false after the loop meets the end
	recvSeq++;
	
	if((startSeq != -1) && (maxSeq == recvSeq))
	{
		last_packet = 1;
	}
	else
	{
		last_packet = 0;
	}
	memcpy(MULTI_BUF_BASE + ((((recvSeq-1) + (startSeq - 1)) % maxSeq)*MULTI_UPGRADE_DATA_LEN), skb->data + MULTI_UPGRADE_PKT_HEADLEN, MULTI_UPGRADE_DATA_LEN);

	receive_len += MULTI_UPGRADE_DATA_LEN;


	if((Jiffies - Jiffiestmp) > 20){
		Jiffiestmp = Jiffies;
		if(tag == 0){
			
#ifdef DBG_PRINT
			LED_OFF(internet_gpio);
#endif
			tag = 1;
		}else{
#ifdef DBG_PRINT
			LED_ON(internet_gpio);
#endif
			tag = 0;
		}
	}

	if(last_packet == 1){

		if(receive_len >= imagelen){
			/*
				decide the type of image
			*/

			if(PART_FDT_MAGIC == (unsigned long) fdt_magic(MULTI_BUF_BASE+TRX_SIZE+get_fip_offset())){
				if (fit_all_image_verify((const void *) (MULTI_BUF_BASE+TRX_SIZE+get_fip_offset()))== -1){
					#ifdef DBG_PRINT
					printf("hash check error!!! \r\n");

					/*
						turn off ADSL and Internet LED
					*/
					LED_OFF(dsl_gpio);
					LED_ON(internet_gpio);
					#endif
					goto error;
				}
				isallinone = IMG_TCLINUX;

#if defined(TCSUPPORT_DUAL_WLAN_MT7615E) || defined(TCSUPPORT_WLAN_MT7615_11N) || defined(TCSUPPORT_WLAN_MT7615D) || defined(TCSUPPORT_DUAL_WLAN_MT7613E)|| defined(TCSUPPORT_WLAN_MT7915D)  || defined(TCSUPPORT_WLAN_MT7916D) || defined(TCSUPPORT_WLAN_MULTI_CHIP)
				if (imagelen > getimagelen()){
					badimg = 1;
				}
#else				
#ifdef TCSUPPORT_NEW_SPIFLASH					
				if (IS_SPIFLASH) {
					/*Check the image size*/
					#if (TCSUPPORT_RESERVEAREA_BLOCK==1)
					if(imagelen > (mtd.size-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*1)) {
					#elif (TCSUPPORT_RESERVEAREA_BLOCK==2)
					if(imagelen > (mtd.size-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*2)) {
					#elif (TCSUPPORT_RESERVEAREA_BLOCK==3)
					if(imagelen > (mtd.size-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*3)) {
					#elif (TCSUPPORT_RESERVEAREA_BLOCK==4)
					if(imagelen > (mtd.size-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*4)) {
					#else
					if(imagelen > (mtd.size-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*7)) {
					#endif
						badimg = 1;
					}
				} else 
#endif

#ifdef TCSUPPORT_EMMC
				if(isEMMC) {
					if (imagelen > getimagelen()){
						badimg = 1;
					}
				} else
#endif

				if (IS_NANDFLASH) {
#ifdef TCSUPPORT_BB_NAND				
					if (regRead32(CR_AHB_HWCONF) & (0x00080000)) {
						if(imagelen > ((1<<ra.flash->chip_shift) - 7*(1<<ra.flash->erase_shift))) {
							badimg = 1;
						}
					} else {
						/*Check the image size*/
						#if (TCSUPPORT_RESERVEAREA_BLOCK==1)
						if(imagelen > ((1<<ra.flash->chip_shift)-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*1)) {
						#elif (TCSUPPORT_RESERVEAREA_BLOCK==2)
						if(imagelen > ((1<<ra.flash->chip_shift)-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*2)) {
						#elif (TCSUPPORT_RESERVEAREA_BLOCK==3)
						if(imagelen > ((1<<ra.flash->chip_shift)-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*3)) {
						#elif (TCSUPPORT_RESERVEAREA_BLOCK==4)
						if(imagelen > ((1<<ra.flash->chip_shift)-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*4)) {
						#elif (TCSUPPORT_RESERVEAREA_BLOCK==5)
						if(imagelen > ((1<<ra.flash->chip_shift)-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*5)) {
						#else
						if(imagelen > ((1<<ra.flash->chip_shift)-TCBOOT_SIZE-ROMFILE_SIZE-FLASH_ERASE_SIZE*7)) {
						#endif
							badimg = 1;
						}	
					}
#endif					
							}
#endif				
				if (badimg){
					#ifdef DBG_PRINT
#ifdef TCSUPPORT_NEW_SPIFLASH	
					printf("Image too big for partition: %s\n", mtd);
#endif
					LED_OFF(dsl_gpio);
					LED_ON(internet_gpio);
					#endif
					goto error;
				}
			}
            #if defined(TCSUPPORT_SECURE_BOOT)
            else if(check_key((void*)MULTI_BUF_BASE, &keyLen) == 0) {
                printf("MultiUpgrade rsa_key\n");
                #if defined(TCSUPPORT_SECURE_BOOT_FLASH_OTP) || defined(TCSUPPORT_SECURE_BOOT_7526)

                if (write_public_key_to_otp((void*)MULTI_BUF_BASE)==0) {
                #else

                if (write_public_key_to_efuse((void*)MULTI_BUF_BASE, 1)) {
                #endif
                    finishMultiUpgrade = 1;
                    return;
                }
                else {
                    goto error;
                	}
                	}
            #endif
			else{
				
				/*
					check if image is tclinux_allinone
				*/
/* may modify for general sun.zhu_20140503 */
				if(PART_FDT_MAGIC == (unsigned long) fdt_magic(MULTI_BUF_BASE+TCBOOT_SIZE+ROMFILE_SIZE+TRX_SIZE+get_fip_offset())){
					#ifdef DBG_PRINT
					printf(" it is tclinux_allineone!!! \r\n");	
					#endif
				}
				else{
					#ifdef DBG_PRINT
					printf(" it is not tclinux_allineone!!! \r\n");	
					#endif
					goto error;
				}
				if (fit_all_image_verify((const void *) (MULTI_BUF_BASE+TCBOOT_SIZE+ROMFILE_SIZE+TRX_SIZE+get_fip_offset()))== -1){
					#ifdef DBG_PRINT
					printf("hash check error!!! \r\n");

					/*
						turn off ADSL and Internet LED
					*/
					LED_OFF(dsl_gpio);
					LED_ON(internet_gpio);
					#endif
					goto error;
				}
				isallinone = IMG_TCLINUXALLINONE;
				cal_check_sum = crc32_no_comp(DEFAULT_CRC,(char*)MULTI_BUF_BASE, (imagelen-4));
				
				crc_ptr = (unsigned char *)(MULTI_BUF_BASE+imagelen-4);
				#ifdef TCSUPPORT_LITTLE_ENDIAN
				raw_check_sum |= *(crc_ptr+0)<<24;
				raw_check_sum |= *(crc_ptr+1)<<16;
				raw_check_sum |= *(crc_ptr+2)<<8;
				raw_check_sum |= *(crc_ptr+3);
				#else
				raw_check_sum |= *crc_ptr;
				raw_check_sum |= *(crc_ptr+1)<<8;
				raw_check_sum |= *(crc_ptr+2)<<16;
				raw_check_sum |= *(crc_ptr+3)<<24;
				#endif
				
				if (cal_check_sum != raw_check_sum){
					#ifdef DBG_PRINT
					printf("tclinux_allinone checksum error!!! \r\n");

					/*
						turn off ADSL and Internet LED
					*/
					
					LED_OFF(dsl_gpio);
					LED_ON(internet_gpio);
					#endif
					
					goto error;
				}
				
				/*
					backup env
				*/
				printf("restore env\n");
				flash_read(TCBOOT_BASE+ENV_OFFSET, ENV_SIZE, &retlen, MULTI_BUF_BASE+ENV_OFFSET);
			}
			#ifdef DBG_PRINT
			printf("receive ok\n");
			#endif
			MultiWriteImage(MULTI_BUF_BASE, imagelen,isallinone);
			finishMultiUpgrade = 1;
			setEtherRateLimit (0);
		}else{
			#ifdef DBG_PRINT
			printf("receive_len < imagelen\n");
			#endif
			receive_len = 0;
			goto error;
		}
	} else {
		printf(".");
	}
/* may modify for general sun.zhu_20140503 */
	return;
error:
	startSeq = -1;
	maxSeq = 0;
	recvSeq = 0;
	badimg = 0;
	return;
}
