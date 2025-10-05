/*
 * (C) Copyright 2000-2009
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


/*
 * Boot support
 */
#include <asm/io.h>
#include <malloc.h>
#include <spi/spi_nand_flash.h>
#include <spi/spi_controller.h>
#include <flashhal.h>
#include <ecnt/image/ecnt_image.h>
#include <trx.h>

#define MTD_DBG 0

#if MTD_DBG == 0
#define printf_dbg(args...) 	do{}while(0)
#else
#define printf_dbg 				 printf
#endif

#define PARTS_SIZE 512
#define STR_LEN	256

#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define ALIGN(x, a)		__ALIGN_KERNEL((x), (a))
#define ROOTFS_PARTITION(a)	((a + 0x10000) & ~(0x10000-1))

#define INT_MAX		((int)(~0U>>1))
#define INT_MIN		(-INT_MAX - 1)
#define UINT_MAX	(~0U)
#define LONG_MAX	((long)(~0UL>>1))
#define LONG_MIN	(-LONG_MAX - 1)
#define ULONG_MAX	(~0UL)

/* special size referring to all the remaining space in a partition */
#define SIZE_REMAINING UINT_MAX
#define SIZE_TO_GET (UINT_MAX-1)
#define OFFSET_CONTINUOUS UINT_MAX
#define OFFSET_BACK_FORWARD	(UINT_MAX-1)
#define OFFSET_NOT_FOUND UINT_MAX 

/* error message prefix */
#define ERRP "mtd: "
#define TCLINUX "tclinux"
#define TCLINUX_SLAVE "tclinux_slave"
#define TCLINUX_REAL_SIZE "tclinux_real_size"
#define MTK_UBOOT_ENV "u-boot-env"
#define MTK_FACTORY "Factory"
#define TCLINUX_SLAVE_REAL_SIZE "tclinux_slave_real_size"
#if defined(TCSUPPORT_OPENWRT) || defined(RDKB_BUILD)
#define RESERVEAREA "art"
#else
#define RESERVEAREA "reservearea"
#endif
#define KERNEL_PART "kernel"
#define ROOTFS_PART "rootfs"
#define KERNEL_SLAVE_PART "kernel_slave"
#define ROOTFS_SLAVE_PART "rootfs_slave"
#define BOOTLOADER_PART "bootloader"
#define ROMFILE_PART "romfile"
#define BOOTLOADER_PART_STR "0[bootloader],"
#define ROMFILE_PART_STR "0[romfile],"
#define BOOTLOADER_DEFAULT_SIZE (512 << 10)
#define ROMFILE_DEFAULT_SIZE (256 << 10)

extern int nand_logic_size;
extern int nand_flash_avalable_size;
extern flashdev_info devinfo;
extern struct mtd_info mtd;
extern unsigned long flash_base;
unsigned long flash_tclinux_start;

struct mtd_partition {
	char *name;		/* identifier string */
	u_int32_t size;		/* partition size */
	u_int32_t offset;		/* offset within the master MTD space */
	u_int32_t mask_flags;	/* master MTD flags to mask out for this partition */
};

/************************************************************************
*                  P R I V A T E   D A T A
*************************************************************************
*/
static struct mtd_partition *ecnt_parts;
static int num_parts = 0;
static int has_remaining_part_flag = 0;
static uint64_t tclinux_part_size = 0;			
static uint64_t tclinux_part_offset = OFFSET_CONTINUOUS;	
static int kernel_part_index = -1;
static int kernel_slave_part_index = -1;
static char parts_size[PARTS_SIZE];

/************************************************************************
*                  F U N C T I O N   D E F I N I T I O N S
*************************************************************************
*/
uint32_t ecnt_get_tclinux_flash_offset(void);

unsigned long long ecnt_memparse(const char *ptr, char **retptr, unsigned int blocksize)
{
	char *endptr;	/* local pointer to end of parsed string */

	unsigned long long ret = simple_strtoull(ptr, &endptr, 0);

	switch (*endptr) {
	case 'G':
	case 'g':
		ret <<= 10;
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		endptr++;
		break;
	case 'B':
	case 'b':
		ret *=blocksize;
		endptr++;
		break;
	default:
		break;
	}

	if (retptr)
		*retptr = endptr;

	return ret;
}

static void ecnt_newpart_set_other_parts(struct mtd_partition *part, char *name, int index, unsigned int blocksize)
{
	if(!part || !name){
		printf("ecnt_newpart_set_other_parts fail, input NULL\n");
		return;
	}
	if(!strcmp(name, TCLINUX)){
		part[0].name = KERNEL_PART;
		part[0].size = SIZE_TO_GET;
		part[0].offset = OFFSET_CONTINUOUS;
		part[1].name = ROOTFS_PART;
		part[1].size = SIZE_TO_GET;
		part[1].offset = OFFSET_CONTINUOUS;
		kernel_part_index = index;
	}
	else if(!strcmp(name, TCLINUX_SLAVE)){
		part[0].name = KERNEL_SLAVE_PART;
		part[0].size = SIZE_TO_GET;
		part[0].offset = OFFSET_CONTINUOUS;
		part[1].name = ROOTFS_SLAVE_PART;
		part[1].size = SIZE_TO_GET;
		part[1].offset = OFFSET_CONTINUOUS;
		kernel_slave_part_index = index;
	}
	else if(!strcmp(name, RESERVEAREA)){
		blocksize = 0x40000;
		part->name = RESERVEAREA;
		part->offset = OFFSET_BACK_FORWARD;
		part->size = (blocksize*TCSUPPORT_RESERVEAREA_BLOCK);
	}
}

/*
 * Parse one partition definition for an MTD. Since there can be many
 * comma separated partition definitions, this function calls itself
 * recursively until no more partition definitions are found. Nice side
 * effect: the memory to keep the mtd_partition structs and the names
 * is allocated upon the last definition being found. At that point the
 * syntax has been verified ok.
 */
static struct mtd_partition * ecnt_newpart(char *s, char **retptr,int *num_parts,
                                      int this_part,unsigned char **extra_mem_ptr,
                                      int extra_mem_size, unsigned int blocksize)
{
	struct mtd_partition *parts;
	unsigned long size = SIZE_TO_GET;
	unsigned long offset = OFFSET_CONTINUOUS;
	char *name;
	int name_len;
	unsigned char *extra_mem;
	char delim;
	unsigned int mask_flags;

	/* fetch the partition size */
	if (*s == '-'){	
		if(has_remaining_part_flag == 0){
			/* assign all remaining space to this partition */
			size = SIZE_REMAINING;
			has_remaining_part_flag = 1;
			s++;
		}
		else{
			printf(ERRP "no fill-up partitions allowed after already having a fill-up partition\n");
			return NULL;
		}
	}
	
	else{
		size = ecnt_memparse(s, &s, blocksize);
		if(size == 0)
			size = SIZE_TO_GET;
		if (size < blocksize){
			printf(ERRP "partition size too small (%lx)\n", size);
			return NULL;
		}
	}

	/* fetch partition name and flags */
	mask_flags = 0; /* this is going to be a regular partition */
	delim = 0;
	
        /* now look for name */
	if (*s == '['){
		delim = ']';
	}

	if (delim){
		char *p;

	    	name = ++s;
		p = strchr(name, delim);
		if (!p){
			printf(ERRP "no closing %c found in partition name\n", delim);
			return NULL;
		}
		name_len = p - name;
		s = p + 1;
	}
	else{
	    	name = NULL;
		name_len = 13; /* Partition_000 */
	}

	/* record name length for memory allocation later */
	extra_mem_size += name_len + 1;

	/* offset type is append */
        if (strncmp(s, "a", 1) == 0){
		offset = OFFSET_CONTINUOUS;
		s += 1;
        }

        /* offset type is back forward*/
        if (strncmp(s, "end", 3) == 0){
		offset = OFFSET_BACK_FORWARD;
		s += 3;
        }
	
	/* test if more partitions are following */
	if (*s == ','){
		if(!strncmp(name, TCLINUX, name_len)){
			this_part += 2;
			extra_mem_size +=strlen(KERNEL_PART)+strlen(ROOTFS_PART)+2;
		}
		else if(!strncmp(name, TCLINUX_SLAVE, name_len)){
			this_part += 2;
			extra_mem_size +=strlen(KERNEL_SLAVE_PART)+strlen(ROOTFS_SLAVE_PART)+2;
		}
		/* more partitions follow, parse them */
		parts = ecnt_newpart(s + 1, &s, num_parts, this_part + 1,
				&extra_mem, extra_mem_size, blocksize);
		if (!parts)
			return NULL;
	}
	else{	/* this is the last partition: allocate space for all */
		int alloc_size;
		if(!strncmp(name, TCLINUX, name_len)){
			this_part += 2;
			extra_mem_size +=strlen(KERNEL_PART)+strlen(ROOTFS_PART)+2;
		}
		else if(!strncmp(name, TCLINUX_SLAVE, name_len)){
			this_part += 2;
			extra_mem_size +=strlen(KERNEL_SLAVE_PART)+strlen(ROOTFS_SLAVE_PART)+2;
		}
		
		*num_parts = this_part + 2; /*add reservearea partition*/
		extra_mem_size += strlen(RESERVEAREA)+1;
		alloc_size = *num_parts * sizeof(struct mtd_partition) +
			     extra_mem_size;
		parts = parts_size;
	
		if(alloc_size > PARTS_SIZE){
			printf(ERRP "out of memory\n");
			return NULL;
		}
		extra_mem = (unsigned char *)(parts + *num_parts);
	}
	/* enter this partition (offset will be calculated later if it is zero at this point) */
	parts[this_part].size = size;
	parts[this_part].offset = offset;
	parts[this_part].mask_flags = mask_flags;
	if (name){
		strncpy(extra_mem, name, name_len);
	}
	else{
		sprintf(extra_mem, "Partition_%03d", this_part);
	}
	parts[this_part].name = extra_mem;
	extra_mem += name_len + 1;
	
	if(!strcmp(parts[this_part].name, TCLINUX)){
		ecnt_newpart_set_other_parts(&parts[this_part-2],TCLINUX, this_part-2, blocksize);
	}
	else if(!strcmp(parts[this_part].name, TCLINUX_SLAVE)){
		ecnt_newpart_set_other_parts(&parts[this_part-2],TCLINUX_SLAVE, this_part-2, blocksize);
	}
	if(this_part == (*num_parts -2)){
		ecnt_newpart_set_other_parts(&parts[*num_parts -1], RESERVEAREA, *num_parts -1, blocksize);
	}

	printf_dbg("partition %d: name <%s>, offset %x, size %x, mask flags %x\n",this_part, parts[this_part].name,
	     parts[this_part].offset, parts[this_part].size, parts[this_part].mask_flags);

	/* return (updated) pointer to extra_mem memory */
	if (extra_mem_ptr)
	  *extra_mem_ptr = extra_mem;

	/* return (updated) pointer command line string */
	*retptr = s;

	/* return partition table */
	return parts;
}

static void ecnt_check_mtdpart_str(char *dst, char *src)
{
	int have_bootloader_part = 0;
	int have_romfile_part = 0;
	char *bootloader_p = NULL;
	char *bootloader_end_p = NULL;
	
	if(bootloader_p = strstr(src, BOOTLOADER_PART)){
		have_bootloader_part = 1;
	}
	if(strstr(src, ROMFILE_PART)){
		have_romfile_part = 1;
	}
#if defined(TCSUPPORT_OPENWRT) || defined(RDKB_BUILD)
			have_romfile_part = 1;
#endif

	if(have_romfile_part && have_bootloader_part){
		strcpy(dst, src);
	}
	else if(!have_romfile_part && !have_bootloader_part){
		strcpy(dst, BOOTLOADER_PART_STR);
		strcat(dst, ROMFILE_PART_STR);
		strcat(dst, src);
	}
	else if(have_romfile_part && !have_bootloader_part){
		strcpy(dst, BOOTLOADER_PART_STR);
		strcat(dst, src);
	}
	else if(!have_romfile_part && have_bootloader_part){
		bootloader_end_p = strchr(bootloader_p, ',');
		if(!bootloader_end_p)
			printf("cmdline, bootloader partition error!\n");
		memcpy(dst, src, bootloader_end_p - src+1);
		strcat(dst, ROMFILE_PART_STR);
		strcat(dst, bootloader_end_p+1);
	}
}

/******************************************************************************
 Function:		ecnt_mtdpart_setup
 Description:	Parse the command line.
 Input:		
 Return:	    	
******************************************************************************/
static int ecnt_mtdpart_setup(char *s, unsigned int blocksize)
{
	char cmdline[STR_LEN];
	unsigned char *extra_mem;
	char *p;

	if(s == NULL){
		printf("ecnt_mtdpart_setup(), mtd partition cmdline is NULL\n");
		return 0;
	}

	if(strlen(s) + strlen(BOOTLOADER_PART_STR)+strlen(ROMFILE_PART_STR) > STR_LEN){
		printf("ecnt_mtdpart_setup(), string length outof size\n");
		return 0;
	}
	
	ecnt_check_mtdpart_str(cmdline, s);
	
	p = cmdline;
	printf_dbg("\nparsing <%s>\n", p);

	/*
	 * parse one mtd. have it reserve memory for the
	 * struct cmdline_mtd_partition and the mtd-id string.
	 */
	ecnt_parts = ecnt_newpart(p,		/* cmdline */
			&s,		/* out: updated cmdline ptr */
			&num_parts,	/* out: number of parts */
			0,		/* first partition */
			&extra_mem, /* out: extra mem */
			 0, blocksize);
	if(!ecnt_parts)
	{
		/*
		 * An error occurred. We're either:
		 * a) out of memory, or
		 * b) in the middle of the partition spec
		 * Either way, this mtd is hosed and we're
		 * unlikely to succeed in parsing any more
		 */
		 return 0;
	 }
	
	return 1;
}

static uint32_t ecnt_get_bootloader_romfile_size(char *name, unsigned int blocksize)
{
	uint32_t bootloader_size = BOOTLOADER_DEFAULT_SIZE;
	uint32_t romfile_size = ROMFILE_DEFAULT_SIZE;
#if defined(TCSUPPORT_BLX_MULTICOPY) || defined(TCSUPPORT_ARM_MULTIBOOT)
		bootloader_size = 0x80000;
#if defined(TCSUPPORT_ARM_MULTIBOOT)
		bootloader_size += 0x00100000;
#endif
#if defined(TCSUPPORT_BLX_MULTICOPY)
		bootloader_size += 0x00180000;
#endif

#elif defined(TCSUPPORT_OPENWRT)
	bootloader_size = 0x00080000;
#endif

	blocksize = 0x40000;

	if(!strcmp(name, BOOTLOADER_PART)){
		if(bootloader_size < blocksize)
			bootloader_size = blocksize;
		printf_dbg("ecnt_get_bootloader_romfile_size(), bootloader = %x, blocksize = %x\n",bootloader_size, blocksize);
		return bootloader_size;
	}
	else if(!strcmp(name, ROMFILE_PART)){
		if(romfile_size < blocksize)
			romfile_size = blocksize;
		printf_dbg("ecnt_get_bootloader_romfile_size(), romfile size = %x\n",romfile_size);
		return romfile_size;
	}
	return SIZE_TO_GET;
	
}

static void ecnt_set_kernel_rootfs_part(void)
{
	int i;	
	uint32_t tclinux_real_size = 0;
	uint32_t tclinux_slave_real_size = 0;
	unsigned long buf = CONFIG_SYS_LOAD_ADDR;
	unsigned long addr = ecnt_get_tclinux_flash_offset();
	unsigned long first_imgAddr = 0, second_imgAddr = 0;
	unsigned long len = 0;
	struct tclinux_imginfo first_imginfo, second_imginfo;
	SPI_NAND_FLASH_RTN_T status;

	if(kernel_part_index < 0)
		return;
	
	if(tclinux_part_offset == OFFSET_CONTINUOUS){
		printf("ecnt_set_kernel_rootfs_part(), tclinux partition offset error\n");
	}
	
	if((tclinux_part_size == 0) || (tclinux_part_size == SIZE_REMAINING)){
		printf("ecnt_set_kernel_rootfs_part(), tclinux partition size error\n");		
	}

	addr = ecnt_get_tclinux_flash_offset() + sizeof(struct trx_header);
	len = CONFIG_BOOTCOMMAND_READ_LEN;
	first_imgAddr = addr;

	/* Get first kernel & rootfs offset & size, and load kernel to DRAM */
	memset((void*) &first_imginfo, 0, sizeof(struct tclinux_imginfo));
	if (get_tclinux_imginfo((const unsigned long)(first_imgAddr), &first_imginfo, NOT_BOOT_IMG, (unsigned char *)buf)) {
		printf("Parse main image fail.\n");
        return -1;
	}

	tclinux_real_size = ecnt_parts[kernel_part_index+2].size;
	ecnt_parts[kernel_part_index].size = first_imginfo.kernel_off + first_imginfo.kernel_size + sizeof(struct trx_header);
	ecnt_parts[kernel_part_index+1].size = ROOTFS_PARTITION(first_imginfo.rootfs_size);
	ecnt_parts[kernel_part_index+1].offset = ecnt_parts[kernel_part_index].offset + first_imginfo.rootfs_off + sizeof(struct trx_header);
	
	if(kernel_slave_part_index > 0){
		second_imgAddr = first_imgAddr + tclinux_real_size;
		printf_dbg("second_imgAddr:0x%x = first_imgAddr:0x%x + tclinux_real_size:0x%x\n", second_imgAddr, first_imgAddr, tclinux_real_size);
		/* Get second kernel & rootfs offset & size */
		memset((void*) &second_imginfo, 0, sizeof(struct tclinux_imginfo));
		if (get_tclinux_imginfo((const unsigned long)(second_imgAddr), &second_imginfo, NOT_BOOT_IMG, (unsigned char *)buf)) {
	        /* No second image */
			printf("Parse second image fail.\n");
		}
		
		ecnt_parts[kernel_slave_part_index].size = second_imginfo.kernel_off + second_imginfo.kernel_size + sizeof(struct trx_header);
		tclinux_slave_real_size = ecnt_parts[kernel_slave_part_index+2].size;
		ecnt_parts[kernel_slave_part_index+1].size = ROOTFS_PARTITION(second_imginfo.rootfs_size);
		ecnt_parts[kernel_slave_part_index+1].offset = ecnt_parts[kernel_slave_part_index].offset + second_imginfo.rootfs_off + sizeof(struct trx_header);
		printf_dbg("tclinux_slave real size = %x\n", tclinux_slave_real_size);
	}
	/*check whether tclinux/tclinux_slave partition enough*/
	for(i=0; i <num_parts; i++){
		if(!strcmp(ecnt_parts[i].name,TCLINUX)){
			
			if(ecnt_parts[i].size < tclinux_real_size){
				printf("tclinux partition size = 0x%x, real size = 0x%x\n", ecnt_parts[i].size, tclinux_real_size);
				printf("tclinux partition size < its real size\n");
			}
		}
		else if(!strcmp(ecnt_parts[i].name,TCLINUX_SLAVE)){
			if(ecnt_parts[i].size < tclinux_slave_real_size){
				printf("tclinux_slave partition size = 0x%x, real size = 0x%x\n", ecnt_parts[i].size,tclinux_slave_real_size);
				printf("tclinux_slave partition size < its real size\n");
			}
		}
	}
}

/******************************************************************************
 Function:		__ecnt_parse_cmdline_partitions
 Description:	It's used to init tc3162_parts by cmdline partiotion string
 Input:		
 Return:	    	
******************************************************************************/
static int __ecnt_parse_cmdline_partitions(unsigned int blocksize, unsigned long long size)
{
	unsigned long offset;
	int i;
	int size_remaining_index = -1;
	int first_back_forward_index = -1;
	unsigned long size_remaining_offset_start = 0; //COV, CID 140153 
	unsigned long back_forward_total_size = 0;
	int is_tclinux_remaining_flag = 0;
	
	/* parse command line */
	if(ecnt_mtdpart_setup(TCSUPPORT_PARTITIONS_CMDLINE_STR, blocksize) == 0){
		printf("ecnt_mtdpart_setup fail\n");
		return 0;
	}
	printf_dbg("__ecnt_parse_cmdline_partitions(), num_parts = %d\n", num_parts);
	for(i = 0, offset = 0; i < num_parts; i++)
	{
		if(ecnt_parts[i].offset == OFFSET_BACK_FORWARD){
			first_back_forward_index = i;
			break;
		}
		if(ecnt_parts[i].size == SIZE_TO_GET){
			ecnt_parts[i].offset = offset;
			if(!strcmp(ecnt_parts[i].name,BOOTLOADER_PART) || !strcmp(ecnt_parts[i].name, ROMFILE_PART))
				ecnt_parts[i].size = ecnt_get_bootloader_romfile_size(ecnt_parts[i].name, blocksize);
			else	
				continue;
		}
		
		ecnt_parts[i].offset = offset;
					
		if(ecnt_parts[i].size == SIZE_REMAINING){
			size_remaining_index = i;
			size_remaining_offset_start = ecnt_parts[i].offset;
			if((i+1) == num_parts){
				ecnt_parts[i].size = size - offset;
			}
			else{
				if(!strcmp(ecnt_parts[i].name, TCLINUX)){
					is_tclinux_remaining_flag = 1;
				}
				first_back_forward_index = i+1;
				break;
			}
		}
		if (offset + ecnt_parts[i].size > size)
		{
			printf(ERRP" ecnt_parts[%d].size:0x%x partition exceeds flash size:0x%llx, truncating\n", i, ecnt_parts[i].size, size);
			return 0;
		}
		offset +=ecnt_parts[i].size;
		/*offset align*/
		offset = ALIGN(offset, blocksize);
		printf_dbg("%d %s offset = %x, size = %x\n", i, ecnt_parts[i].name, ecnt_parts[i].offset, ecnt_parts[i].size);
		if(!strcmp(ecnt_parts[i].name, TCLINUX)){
			tclinux_part_offset = ecnt_parts[i].offset;
			tclinux_part_size = ecnt_parts[i].size;
		}
	}
	/*offset type is back forward*/
	for(i = (num_parts -1), offset = size; i >=first_back_forward_index && first_back_forward_index != -1; i--){
		ecnt_parts[i].size = ALIGN(ecnt_parts[i].size, blocksize);
		back_forward_total_size += ecnt_parts[i].size;
		ecnt_parts[i].offset = offset-ecnt_parts[i].size;
		printf_dbg("offset = %x, i= %d, part offset = %x, size = %x\n", offset, i, ecnt_parts[i].offset, ecnt_parts[i].size);
		offset -= ecnt_parts[i].size;
		
		if (offset + back_forward_total_size > size)
		{
			printf(ERRP"partitioning exceeds flash size, partition index = %d\n",i);
			return 0;
		}
		printf_dbg("end %d %s offset = %x, size = %x\n", i, ecnt_parts[i].name, ecnt_parts[i].offset, ecnt_parts[i].size);
	}
	/*calculate remaining size*/
	if(size_remaining_index != -1){
		ecnt_parts[size_remaining_index].size = offset - size_remaining_offset_start;
		if(ecnt_parts[size_remaining_index].size < 0)
			printf(ERRP"partition size < 0, index = %d \n", size_remaining_index);
		if(is_tclinux_remaining_flag){
			tclinux_part_offset = ecnt_parts[size_remaining_index].offset;
			tclinux_part_size = ecnt_parts[size_remaining_index].size;
		}
		printf_dbg("%d %s offset = %x, size = %x\n", size_remaining_index, ecnt_parts[size_remaining_index].name, ecnt_parts[size_remaining_index].offset, ecnt_parts[size_remaining_index].size);
	}
	
	ecnt_set_kernel_rootfs_part();

	return num_parts;
}

static uint32_t ecnt_get_mtd_offset(char *name)
{
	int i;

	if(!ecnt_parts){
		printf("mtd parts is NULL\n");
		return OFFSET_NOT_FOUND;
	}
		
	for(i = 0; i < num_parts; i++){
		if(!strcmp(ecnt_parts[i].name, name)) {
			return ecnt_parts[i].offset;
		}
	}
	printf("not find %s partition\n", name);
	return OFFSET_NOT_FOUND;
}

static uint32_t ecnt_get_mtd_size(char *name)
{
	int i;

	if(!ecnt_parts){
		printf("mtd parts is NULL\n");
		return SIZE_TO_GET;
	}
		
	for(i = 0; i < num_parts; i++){
		if(!strcmp(ecnt_parts[i].name, name))
			return ecnt_parts[i].size;
	}
	printf("not find %s partition\n", name);
	return SIZE_TO_GET;
}

inline static uint32_t ecnt_get_mtd_part_size(char *name)
{
	uint32_t size;

	size = ecnt_get_mtd_size(name);
	if(size == SIZE_TO_GET)
		return 0;
	else
		return size;
}

uint32_t ecnt_get_romfile_mtd_offset(void)
{
	uint32_t offset;

	offset = ecnt_get_mtd_offset(ROMFILE_PART);
	if(offset == OFFSET_NOT_FOUND)
		return 0;
	else 
		return offset;
}

uint32_t ecnt_get_tclinux_mtd_offset(void)
{
	uint32_t offset;

	offset = ecnt_get_mtd_offset(TCLINUX);
	if(offset == OFFSET_NOT_FOUND)
		return 0;
	else 
		return offset;
}

uint32_t ecnt_get_tclinux_slave_mtd_offset(void)
{
	uint32_t offset;

	offset = ecnt_get_mtd_offset(TCLINUX_SLAVE);
	if(offset == OFFSET_NOT_FOUND)
		return 0;
	else 
		return offset;
}

uint32_t ecnt_get_tclinux_flash_offset(void) {
	uint32_t offset;

	offset = ecnt_get_mtd_offset(TCLINUX);
	if(offset == OFFSET_NOT_FOUND)
		return 0;
	else 
		return (offset+flash_base);
}

uint32_t ecnt_get_tclinux_slave_flash_offset(void){
	uint32_t offset;

	offset = ecnt_get_mtd_offset(TCLINUX_SLAVE);
	if(offset == OFFSET_NOT_FOUND)
		return 0;
	else 
		return (offset+flash_base);
}

uint32_t ecnt_get_reservearea_flash_offset(void){
	uint32_t offset;

	offset = ecnt_get_mtd_offset(RESERVEAREA);
	if(offset == OFFSET_NOT_FOUND)
		return 0;
	else 
		return (offset+flash_base);
}

uint32_t ecnt_get_tclinux_size(void)
{
	uint32_t size;

	size = ecnt_get_mtd_size(TCLINUX);
	if(size == SIZE_TO_GET)
		return 0;
	else 
		return size;
}

uint32_t ecnt_get_tclinux_slave_size(void)
{
	uint32_t size;

	size = ecnt_get_mtd_size(TCLINUX_SLAVE);
	if(size == SIZE_TO_GET)
		return 0;
	else 
		return size;
}

uint32_t ecnt_get_kernel_size(unsigned int erasesize)
{
	uint32_t size;

	size = ecnt_get_mtd_size(KERNEL_PART);
	if(size == SIZE_TO_GET)
		return 0;
	else{
		return size;
	}
}

uint32_t ecnt_get_kernel_slave_size(unsigned int erasesize)
{
	uint32_t size;

	size = ecnt_get_mtd_size(KERNEL_SLAVE_PART);
	if(size == SIZE_TO_GET)
		return 0;
	else{ 
		return size;
	}
}

uint32_t ecnt_get_reservearea_size(void)
{
	uint32_t size;

	size = ecnt_get_mtd_size(RESERVEAREA);
	if(size == SIZE_TO_GET)
		return 0;
	else 
		return size;
}

uint32_t ecnt_get_boot_size(void)
{
	uint32_t size;
	
	size = ecnt_get_mtd_size(BOOTLOADER_PART);
	if(size == SIZE_TO_GET)
		return 0;
	else 
		return size;
}

uint32_t ecnt_get_romfile_size(void)
{
	uint32_t size;
	
	size = ecnt_get_mtd_size(ROMFILE_PART);
	if(size == SIZE_TO_GET)
		return 0;
	else 
		return size;
}

int ecnt_parse_cmdline_partitions(void)
{
	int ret;

#ifdef TCSUPPORT_EMMC
	if(isEMMC) {
		ret = __ecnt_parse_cmdline_partitions(MMC_BLOCK_SIZE, get_emmc_user_capacity());
	} else 
#endif
	if (IS_NANDFLASH) {
		ret = __ecnt_parse_cmdline_partitions(devinfo.blocksize*1024, nand_flash_avalable_size);
	} else {
		ret = __ecnt_parse_cmdline_partitions(mtd.erasesize, mtd.size);
	}

	flash_tclinux_start = ecnt_get_tclinux_flash_offset();
	
	return ret;
}

int do_get_mtd_info(void)
{
	int i;

	if(!ecnt_parts){
		printf("mtd parts is NULL\n");
		return 0;
	}
	
	for(i = 0; i < num_parts; i++){
		printf( "0x%08x-0x%08x : \"%s\"\n", ecnt_parts[i].offset, (ecnt_parts[i].offset + ecnt_parts[i].size), ecnt_parts[i].name);
	}

	return 0;
}

unsigned int do_get_mtd_rootfsdata_info(ulong *addr, ulong *size)
{
	unsigned int ret = 0;
	unsigned int i;

	if(!ecnt_parts){
		printf("mtd parts is NULL\n");
		return ret;
	}

	for(i = 0; i < num_parts; i++) {
		if (!strcmp(ecnt_parts[i].name, "rootfs_data")) {
			if (ecnt_parts[i].offset > 0 && ecnt_parts[i].size > 0) {
				*addr = ecnt_parts[i].offset;
				*size = ecnt_parts[i].size;
				ret = 1;
				break;
			}
		}
	}

	return ret;
}
uint32_t ecnt_get_ubootenv_size(void)
{
	ecnt_get_mtd_part_size(MTK_UBOOT_ENV);
}

uint32_t ecnt_get_factory_size(void)
{
	ecnt_get_mtd_part_size(MTK_FACTORY);
}

uint32_t ecnt_get_ubootenv_mtd_offset(void)
{
	uint32_t offset;

	offset = ecnt_get_mtd_offset(MTK_UBOOT_ENV);
	if(offset == OFFSET_NOT_FOUND)
		return 0;
	else
		return offset;
}

uint32_t ecnt_get_factory_mtd_offset(void)
{
	uint32_t offset;

	offset = ecnt_get_mtd_offset(MTK_FACTORY);
	if(offset == OFFSET_NOT_FOUND)
		return 0;
	else
		return offset;
}

