/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>
#if defined(CONFIG_ECNT_TFTP_AUTO_UPGRADE)
#include <flashhal.h>
#endif
#include <aes.h>
#include <u-boot/crc.h>

/* RichOS memory partitions */
#define CONFIG_SYS_LOAD_ADDR				0x81800000

/* Linux DRAM definition */
#define CONFIG_LOADADDR			            CONFIG_SYS_LOAD_ADDR

#define ENCRPTED_IMG_TAG  "encrpted_img"
#define AES_KEY_256  "he9-4+M!)d6=m~we1,q2a3d1n&2*Z^%8$"
#define AES_NOR_IV  "J%1iQl8$=lm-;8AE@"

extern void aes_expand_key(u8 *key, u32 key_size, u8 *expkey);

extern void aes_cbc_encrypt_blocks(u32 key_size, u8 *key_exp, u8 *iv, u8 *src, u8 *dst,
			    u32 num_aes_blocks);
typedef  struct
{
    char model[32];        /*model name*/
    char region[32];       /*region*/
    char version[64];      /*version*/
    char dateTime[64];     /*date*/
    unsigned int productHwModel;  /*product hardware model*/
    char modelIndex;       /*model index - default 0:don't change model in nmrp upgrade - others: change model by index in nmrp upgrade*/
    char hwIdNum;          /*hw id list num*/
    char modelNum;         /*model list num*/
    char reserved0[13];    /*reserved*/
    char modelHwInfo[200]; /*save hw id list and model list*/
    char reserved[100];    /*reserved space, if add struct member, please adjust this reserved size to keep the head total size is 512 bytes*/
} __attribute__((__packed__)) image_head_t;

typedef  struct
{
    char checkSum[4];      /*checkSum*/
} __attribute__((__packed__)) image_tail_t;

static int netboot_common(enum proto_t, cmd_tbl_t *, int, char * const []);

#ifndef CONFIG_ECNT
static int do_bootp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return netboot_common(BOOTP, cmdtp, argc, argv);
}

U_BOOT_CMD(
	bootp,	3,	1,	do_bootp,
	"boot image via network using BOOTP/TFTP protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);
#endif

int do_tftpb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;

	bootstage_mark_name(BOOTSTAGE_KERNELREAD_START, "tftp_start");
	ret = netboot_common(TFTPGET, cmdtp, argc, argv);
	bootstage_mark_name(BOOTSTAGE_KERNELREAD_STOP, "tftp_done");
	return ret;
}

U_BOOT_CMD(
	tftpboot,	3,	1,	do_tftpb,
	"boot image via network using TFTP protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);

#ifdef CONFIG_CMD_TFTPPUT
int do_tftpput(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;

	ret = netboot_common(TFTPPUT, cmdtp, argc, argv);
	return ret;
}

U_BOOT_CMD(
	tftpput,	4,	1,	do_tftpput,
	"TFTP put command, for uploading files to a server",
	"Address Size [[hostIPaddr:]filename]"
);
#endif


#ifdef CONFIG_CMD_TFTPSRV
#ifndef CONFIG_USE_IRQ
static int do_tftpsrv(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	return netboot_common(TFTPSRV, cmdtp, argc, argv);
}

U_BOOT_CMD(
	tftpsrv,	2,	1,	do_tftpsrv,
	"act as a TFTP server and boot the first received file",
	"[loadAddress]\n"
	"Listen for an incoming TFTP transfer, receive a file and boot it.\n"
	"The transfer is aborted if a transfer has not been started after\n"
	"about 50 seconds or if Ctrl-C is pressed."
);
#endif
#endif

#ifdef CONFIG_CMD_RARP
int do_rarpb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return netboot_common(RARP, cmdtp, argc, argv);
}

U_BOOT_CMD(
	rarpboot,	3,	1,	do_rarpb,
	"boot image via network using RARP/TFTP protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);
#endif

#if defined(CONFIG_CMD_DHCP)
static int do_dhcp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return netboot_common(DHCP, cmdtp, argc, argv);
}

U_BOOT_CMD(
	dhcp,	3,	1,	do_dhcp,
	"boot image via network using DHCP/TFTP protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);
#endif

#if defined(CONFIG_CMD_NFS)
static int do_nfs(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return netboot_common(NFS, cmdtp, argc, argv);
}

U_BOOT_CMD(
	nfs,	3,	1,	do_nfs,
	"boot image via network using NFS protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);
#endif

static void netboot_update_env(void)
{
	char tmp[22];

	if (NetOurGatewayIP) {
		ip_to_string(NetOurGatewayIP, tmp);
		setenv("gatewayip", tmp);
	}

	if (NetOurSubnetMask) {
		ip_to_string(NetOurSubnetMask, tmp);
		setenv("netmask", tmp);
	}

	if (NetOurHostName[0])
		setenv("hostname", NetOurHostName);

	if (NetOurRootPath[0])
		setenv("rootpath", NetOurRootPath);

	if (NetOurIP) {
		ip_to_string(NetOurIP, tmp);
		setenv("ipaddr", tmp);
	}
#if !defined(CONFIG_BOOTP_SERVERIP)
	/*
	 * Only attempt to change serverip if net/bootp.c:BootpCopyNetParams()
	 * could have set it
	 */
	if (NetServerIP) {
		ip_to_string(NetServerIP, tmp);
		setenv("serverip", tmp);
	}
#endif
	if (NetOurDNSIP) {
		ip_to_string(NetOurDNSIP, tmp);
		setenv("dnsip", tmp);
	}
#if defined(CONFIG_BOOTP_DNS2)
	if (NetOurDNS2IP) {
		ip_to_string(NetOurDNS2IP, tmp);
		setenv("dnsip2", tmp);
	}
#endif
	if (NetOurNISDomain[0])
		setenv("domain", NetOurNISDomain);

#if defined(CONFIG_CMD_SNTP) \
    && defined(CONFIG_BOOTP_TIMEOFFSET)
	if (NetTimeOffset) {
		sprintf(tmp, "%d", NetTimeOffset);
		setenv("timeoffset", tmp);
	}
#endif
#if defined(CONFIG_CMD_SNTP) \
    && defined(CONFIG_BOOTP_NTPSERVER)
	if (NetNtpServerIP) {
		ip_to_string(NetNtpServerIP, tmp);
		setenv("ntpserverip", tmp);
	}
#endif
}

static int netboot_common(enum proto_t proto, cmd_tbl_t *cmdtp, int argc,
		char * const argv[])
{
	char *s;
	char *end;
	int   rcode = 0;
	int   size;
	ulong addr;
#if defined(CONFIG_ECNT_TFTP_AUTO_UPGRADE)
	unsigned long			retlen = 0;
	unsigned long			img_size = 0;

	if (proto == TFTPSRV)
	{
		setenv("filename", "none");
	}
#endif

	/* pre-set load_addr */
	if ((s = getenv("loadaddr")) != NULL) {
		load_addr = simple_strtoul(s, NULL, 16);
	}
	/* set the default filesize to 0 before we to the real download */
	setenv_hex("filesize", 0);
	
	switch (argc) {
	case 1:
		break;

	case 2:	/*
		 * Only one arg - accept two forms:
		 * Just load address, or just boot file name. The latter
		 * form must be written in a format which can not be
		 * mis-interpreted as a valid number.
		 */
		addr = simple_strtoul(argv[1], &end, 16);
		if (end == (argv[1] + strlen(argv[1])))
			load_addr = addr;
		else
			copy_filename(BootFile, argv[1], sizeof(BootFile));
		break;

	case 3:	load_addr = simple_strtoul(argv[1], NULL, 16);
		copy_filename(BootFile, argv[2], sizeof(BootFile));

		break;

#ifdef CONFIG_CMD_TFTPPUT
	case 4:
		if (strict_strtoul(argv[1], 16, &save_addr) < 0 ||
			strict_strtoul(argv[2], 16, &save_size) < 0) {
			printf("Invalid address/size\n");
			return cmd_usage(cmdtp);
		}
		copy_filename(BootFile, argv[3], sizeof(BootFile));
		break;
#endif
	default:
		bootstage_error(BOOTSTAGE_ID_NET_START);
		return CMD_RET_USAGE;
	}
	bootstage_mark(BOOTSTAGE_ID_NET_START);

	if ((size = NetLoop(proto)) < 0) {
		bootstage_error(BOOTSTAGE_ID_NET_NETLOOP_OK);
		return 1;
	}
	bootstage_mark(BOOTSTAGE_ID_NET_NETLOOP_OK);

	/* NetLoop ok, update environment */
	netboot_update_env();

	/* done if no file was loaded (no errors though) */
	if (size == 0) {
		bootstage_error(BOOTSTAGE_ID_NET_LOADED);
		return 0;
	}
	setenv_hex("filesize", size);
	printf("get filesize 0x%x\n",size);

	/* flush cache */
	flush_cache(load_addr, size);
#if defined(CONFIG_ECNT_TFTP_AUTO_UPGRADE)
	if (proto == TFTPSRV)
	{
		img_size = size;
		s = getenv("filename");

		if (!strcmp(s, getenv("uboot_filename")))
			addr = CONFIG_TCBOOT_OFFSET;
		else if (!strcmp(s, getenv("kernel_filename")))
			addr = ecnt_get_tclinux_flash_offset();
		else
			addr = 0xFFFFFFFF;

		if (addr != 0xFFFFFFFF)
		{
			printf("erase: addr=0x%lx, len=0x%lx\n", addr, img_size);
			flash_erase(addr, img_size);
			printf("write: src=0x%lx, len=0x%lx, dst=0x%lx\n", load_addr, img_size, addr);
			flash_write(addr, img_size, &retlen, (unsigned char *) load_addr);
		}
	}
#endif
	bootstage_mark(BOOTSTAGE_ID_NET_LOADED);

	rcode = bootm_maybe_autostart(cmdtp, argv[0]);

	if (rcode < 0)
		bootstage_error(BOOTSTAGE_ID_NET_DONE_ERR);
	else
		bootstage_mark(BOOTSTAGE_ID_NET_DONE);
	return rcode;
}

#if defined(CONFIG_CMD_PING)
static int do_ping(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return -1;

	NetPingIP = string_to_ip(argv[1]);
	if (NetPingIP == 0)
		return CMD_RET_USAGE;

	if (NetLoop(PING) < 0) {
		printf("ping failed; host %s is not alive\n", argv[1]);
		return 1;
	}

	printf("host %s is alive\n", argv[1]);

	return 0;
}

U_BOOT_CMD(
	ping,	2,	1,	do_ping,
	"send ICMP ECHO_REQUEST to network host",
	"pingAddress"
);
#endif

#if defined(FW_UPGRADE_BY_WEBUI)
static int do_http_upgrade(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return -1;

	NetPingIP = string_to_ip(argv[1]);
	if (NetPingIP == 0)
		return CMD_RET_USAGE;

	if (NetLoop(PING) < 0) {
		printf("ping failed; host %s is not alive\n", argv[1]);
		if (NetLoop(PING) < 0) {
			printf("ping failed again; host %s is not alive!!!\n", argv[1]);
		}else{
			printf("host %s is alive\n", argv[1]);
		}
	}else {

		printf("host %s is alive\n", argv[1]);
	}

	///////////////////////////////////////////////////////
	char *s;
	if ((s = getenv("loadaddr")) != NULL) {
		load_addr = simple_strtoul(s, NULL, 16);
	}
	printf("load_addr = %x\n", load_addr);
	uip_main();
	return 0;
}

U_BOOT_CMD(
	http_upgrade,	2,	1,	do_http_upgrade,
	"http_upgrade",
	"pingAddress"
);
#endif

#if defined(CONFIG_CMD_CDP)

static void cdp_update_env(void)
{
	char tmp[16];

	if (CDPApplianceVLAN != htons(-1)) {
		printf("CDP offered appliance VLAN %d\n", ntohs(CDPApplianceVLAN));
		VLAN_to_string(CDPApplianceVLAN, tmp);
		setenv("vlan", tmp);
		NetOurVLAN = CDPApplianceVLAN;
	}

	if (CDPNativeVLAN != htons(-1)) {
		printf("CDP offered native VLAN %d\n", ntohs(CDPNativeVLAN));
		VLAN_to_string(CDPNativeVLAN, tmp);
		setenv("nvlan", tmp);
		NetOurNativeVLAN = CDPNativeVLAN;
	}

}

int do_cdp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int r;

	r = NetLoop(CDP);
	if (r < 0) {
		printf("cdp failed; perhaps not a CISCO switch?\n");
		return 1;
	}

	cdp_update_env();

	return 0;
}

U_BOOT_CMD(
	cdp,	1,	1,	do_cdp,
	"Perform CDP network configuration",
	"\n"
);
#endif

#if defined(CONFIG_CMD_SNTP)
int do_sntp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *toff;

	if (argc < 2) {
		NetNtpServerIP = getenv_IPaddr("ntpserverip");
		if (NetNtpServerIP == 0) {
			printf("ntpserverip not set\n");
			return (1);
		}
	} else {
		NetNtpServerIP = string_to_ip(argv[1]);
		if (NetNtpServerIP == 0) {
			printf("Bad NTP server IP address\n");
			return (1);
		}
	}

	toff = getenv("timeoffset");
	if (toff == NULL)
		NetTimeOffset = 0;
	else
		NetTimeOffset = simple_strtol(toff, NULL, 10);

	if (NetLoop(SNTP) < 0) {
		printf("SNTP failed: host %pI4 not responding\n",
			&NetNtpServerIP);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	sntp,	2,	1,	do_sntp,
	"synchronize RTC via network",
	"[NTP server IP]\n"
);
#endif

#if defined(CONFIG_CMD_DNS)
int do_dns(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc == 1)
		return CMD_RET_USAGE;

	/*
	 * We should check for a valid hostname:
	 * - Each label must be between 1 and 63 characters long
	 * - the entire hostname has a maximum of 255 characters
	 * - only the ASCII letters 'a' through 'z' (case-insensitive),
	 *   the digits '0' through '9', and the hyphen
	 * - cannot begin or end with a hyphen
	 * - no other symbols, punctuation characters, or blank spaces are
	 *   permitted
	 * but hey - this is a minimalist implmentation, so only check length
	 * and let the name server deal with things.
	 */
	if (strlen(argv[1]) >= 255) {
		printf("dns error: hostname too long\n");
		return 1;
	}

	NetDNSResolve = argv[1];

	if (argc == 3)
		NetDNSenvvar = argv[2];
	else
		NetDNSenvvar = NULL;

	if (NetLoop(DNS) < 0) {
		printf("dns lookup of %s failed, check setup\n", argv[1]);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	dns,	3,	1,	do_dns,
	"lookup the IP of a hostname",
	"hostname [envvar]"
);

#endif	/* CONFIG_CMD_DNS */

#if defined(CONFIG_CMD_LINK_LOCAL)
static int do_link_local(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	char tmp[22];

	if (NetLoop(LINKLOCAL) < 0)
		return 1;

	NetOurGatewayIP = 0;
	ip_to_string(NetOurGatewayIP, tmp);
	setenv("gatewayip", tmp);

	ip_to_string(NetOurSubnetMask, tmp);
	setenv("netmask", tmp);

	ip_to_string(NetOurIP, tmp);
	setenv("ipaddr", tmp);
	setenv("llipaddr", tmp); /* store this for next time */

	return 0;
}

U_BOOT_CMD(
	linklocal,	1,	1,	do_link_local,
	"acquire a network IP address using the link-local protocol",
	""
);

#endif  /* CONFIG_CMD_LINK_LOCAL */

#if defined(CONFIG_CMD_NMRP)
static int do_nmrp(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
    if (argc == 1)
        return CMD_RET_USAGE;

    if (NetLoop(NMRP) < 0) {
        printf("nmrp fail\n");
        return 1;
    }

    return 0;
}

U_BOOT_CMD(
    nmrp, 3, 1, do_nmrp,
    "netgear nmrp tools",
    "nmrp [args]"
);

static int decrypt_image(void)
{
    ulong size = 0;
    ulong file_size = 0;
    unsigned char *src_addr = NULL;
    unsigned char *dst_addr = NULL;
    size_t data_load_addr;
    image_head_t *image_head = NULL;
    char *image_tag = NULL;
    char *fenv_hwid = NULL;
    char *fenv_model = NULL;
    ulong image_size = 0;
    ulong encrypted_size = 0;
    ulong block_size = 0;
    ulong checksum = 0;
    ulong curr_checksum = 0;
    unsigned char key_exp[AES256_EXPAND_KEY_LENGTH] = {0};
    unsigned int aes_blocks = 0;

    setenv("decrypt_result", "bad");
    setenv("filesize_result", "bad");

    size = sizeof(image_head_t) + strlen(ENCRPTED_IMG_TAG) + 4 * 2;

    file_size = getenv_hex("filesize", 0);
    if (file_size < size)
    {
        printf("Image head not found!\n");
        return 1;
    }

    /* Set load address */
#if defined(CONFIG_SYS_LOAD_ADDR)
    data_load_addr = CONFIG_SYS_LOAD_ADDR;
#elif defined(CONFIG_LOADADDR)
    data_load_addr = CONFIG_LOADADDR;
#endif

    src_addr = (unsigned char *)data_load_addr;
    dst_addr = (unsigned char *)data_load_addr;

    image_head = (image_head_t *)src_addr;
    src_addr += sizeof(image_head_t);

    image_tag = (char *)src_addr;
    if (strncmp(image_tag, ENCRPTED_IMG_TAG, strlen(ENCRPTED_IMG_TAG)))
    {
        printf("Encrpted tag not found!\n");
        return 1;
    }
    src_addr += strlen(ENCRPTED_IMG_TAG);

    printf("Image is encrypted\n");
    printf("model: %s\n", image_head->model);
    printf("region: %s\n", image_head->region);
    printf("version: %s\n", image_head->version);
    printf("dateTime: %s\n", image_head->dateTime);
    printf("productHwModel: %d\n", image_head->productHwModel);
    printf("modelIndex: %d\n", image_head->modelIndex);
    printf("hwIdNum: %d\n", image_head->hwIdNum);
    printf("modelNum: %d\n", image_head->modelNum);
    printf("modelHwInfo: %s\n", image_head->modelHwInfo);

    if (image_head->modelIndex != 0 && image_head->modelIndex <= image_head->modelNum)
    {
        char loop = 0;
        char delims[] = ";";
        char *result = NULL;
        char achModelHwInfo[512] = { 0 };

        strcpy(achModelHwInfo, image_head->modelHwInfo);
        result = strtok(achModelHwInfo, delims);
        //skip hw id
        while(result != NULL && loop < image_head->hwIdNum)
        {
            printf("hwid is \"%s\"\n", result);
            loop++;
            result = strtok(NULL, delims);
        }
        //model list
        loop = 0;
        while(result != NULL && loop < image_head->modelNum)
        {
            loop++;
            printf("model[%d] is \"%s\"\n", loop, result);

            if (loop == image_head->modelIndex)
            {
                setenv("fenv_model", result);
                break;
            }
            result = strtok(NULL, delims);
        }
    }

    fenv_hwid = getenv("fenv_hw_id");
    printf("fenv_hwid: %s\n", fenv_hwid);
    if (!fenv_hwid || !strstr(image_head->modelHwInfo, fenv_hwid))
    {
        printf("Image hw id not match!\n");
        return 1;
    }

    fenv_model = getenv("fenv_model");
    printf("fenv_model: %s\n", fenv_model);
    if (!fenv_model || !strstr(image_head->modelHwInfo, fenv_model))
    {
        printf("Image model not match!\n");
        return 1;
    }

    image_size = ntohl(*(uint *)src_addr);
    src_addr += sizeof(uint);
    printf("size: 0x%lx\n", image_size);

    encrypted_size = DIV_ROUND_UP(image_size, AES_BLOCK_LENGTH) * AES_BLOCK_LENGTH;

    block_size = ntohl(*(uint *)src_addr);
    src_addr += sizeof(uint);
    printf("block size: 0x%lx\n", block_size);

    if (block_size % AES_BLOCK_LENGTH)
    {
        printf("Image block size not times of AES_BLOCK_LENGTH!\n");
        return 1;
    }

    if (file_size < (size + encrypted_size + sizeof(image_tail_t)))
    {
        printf("Image incomplete!\n");
        return 1;
    }

    checksum = ntohl(*(uint *)(src_addr + encrypted_size));
    printf("checksum: 0x%lx\n", checksum);

    curr_checksum = crc32_no_comp(0, (uint *)data_load_addr, size + encrypted_size);
    printf("curr_checksum: 0x%lx\n", curr_checksum);
    if (curr_checksum != checksum)
    {
        printf("Image checksum error!\n");
        return 1;
    }

    printf("Decrypt image...\n");

    aes_expand_key((u8 *)AES_KEY_256, AES256_KEY_LENGTH, key_exp);
    for (size = 0; size < encrypted_size; size += block_size)
    {
        if (size + block_size > encrypted_size)
        {
            block_size = encrypted_size - size;
        }

        aes_blocks = DIV_ROUND_UP(block_size, AES_BLOCK_LENGTH);
        aes_cbc_decrypt_blocks(AES256_KEY_LENGTH, key_exp, (u8 *)AES_NOR_IV, (u8 *)src_addr, (u8 *)dst_addr, aes_blocks);

        src_addr += block_size;
        dst_addr += block_size;
    }
    printf("Decrypt finish\n");

    setenv_hex("filesize", image_size);

    setenv("filesize_result", "good");

    setenv("decrypt_result", "good");

    return 0;
}

static int do_write_img(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    size_t data_load_addr;
    ulong data_size;
    ulong addr = 0xFFFFFFFF;;
    unsigned long	retlen = 0;
    unsigned int    bootflag = 0;

    bootflag = readBootFlagFromFlash();

    if (bootflag == 1)
    {
        addr = ecnt_get_tclinux_flash_offset();
        bootflag = 0;
    }
    else
    {
        addr = ecnt_get_tclinux_slave_flash_offset();
        bootflag = 1;
    }

    if (addr == 0xFFFFFFFF)
    {
        printf("ERROR: Unknown file name !\n");
        return -1;
    }

    printf("*** Upgrading  ***\n");

/* Set load address */
#if defined(CONFIG_SYS_LOAD_ADDR)
    data_load_addr = CONFIG_SYS_LOAD_ADDR;
#elif defined(CONFIG_LOADADDR)
    data_load_addr = CONFIG_LOADADDR;
#endif

    if (0 != decrypt_image())
    {
        printf("decrypt image fail!");
        return -1;
    }

    data_size = getenv_hex("filesize", 0);
    printf("filesize  = %d\n", data_size);
    printf("\n");

    printf("erase: addr=0x%lx, len=0x%lx\n", addr, data_size);
    flash_erase(addr, data_size);
    printf("write: src=0x%lx, len=0x%lx, dst=0x%lx\n", data_load_addr, data_size, addr);
    flash_write(addr, data_size, &retlen, (unsigned char *) data_load_addr);
    printf("upgrade finished !\n");
    writeBootFlagtoFlash(bootflag);
    printf("bootflag swap %u !\n", bootflag);
}

U_BOOT_CMD(writeimg, 2, 0, do_write_img,
    "write firmware",
    "write firmware\n"
);
#endif	/* CONFIG_CMD_NMRP */
