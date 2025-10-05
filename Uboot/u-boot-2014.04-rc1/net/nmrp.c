/*
 * NMRP support driver
 *
 * Shi Dong <shidong@twsz.com> 2020
 *
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include <environment.h>
#include "nmrp.h"
#include "led.h"
#include <ecnt/gpio/en7523_pin.h>
#include <ecnt/gpio/en7523_evb_mtk_gpio.h>
#include <ecnt/gpio/en7523_evb_mtk_pinmux.h>
#define ebuf_append_u8(eb,b) ((eb)->eb_ptr[(eb)->eb_length++]) = (b) ; 
#define ebuf_append_u16_be(eb,w) ebuf_append_u8(eb,((w) >> 8) & 0xFF) ; \
                              ebuf_append_u8(eb,((w) & 0xFF))

#define ebuf_append_u32_be(eb,w) ebuf_append_u8(eb,((w) >> 24) & 0xFF) ; \
                              ebuf_append_u8(eb,((w) >> 16) & 0xFF) ; \
                              ebuf_append_u8(eb,((w) >> 8) & 0xFF) ; \
                              ebuf_append_u8(eb,((w) & 0xFF))

#define ebuf_append_bytes(eb,b,l) memcpy(&((eb)->eb_ptr[(eb)->eb_length]),(b),(l)); \
                              (eb)->eb_length += (l)


#define NMRP_INTERVAL_TIMEOUT             500UL      /*nmrp timeou 500ms*/
#define NMRP_INTERVAL_LEDTIMEOUT        50UL      /*led timeou 50ms*/
#define NMRP_LISTEN_TIMEOUT                  (NMRP_INTERVAL_TIMEOUT*10)	/*5s*/
#define NMRP_KEEPALIVE_TIMER                (NMRP_INTERVAL_TIMEOUT*60*2)     /*60s*/

int g_nHasNmrp = 0;
int NmrpFwUPOption = 0;
int NmrpSTUPOption = 0;
int NmrpStringTableUpdateCount = 0;
int NmrpStringTableUpdateIndex = 0;
int NmrpState = NMRP_STATE_INIT;
int NmrpFailed = 0;
int upgradeResult = 0;

static unsigned int NmrpStringTableBitmask = 0;

static int s_nNmrpTftpUploadFirmware = 0;
static int s_nNmrpTftpUploadStringTableIndex = 0;
static int s_nNmrpTftpUploadStringTableBitmask = 0;


static unsigned long NmrpOuripSubnetMask;
static unsigned char NetOurTftpIP[4] = { 192, 168, 1, 1 };
static unsigned char NmrpServerEther[6] = { 0, 0, 0, 0, 0, 0 };

static uchar *NetNmrpTxPacket;	/* THE ARP transmit packet */
static uchar NetNmrpPacketBuf[PKTSIZE_ALIGN + PKTALIGN]={0};
static void Nmrp_done(void);
int nmrptftp_firmware = 0;
//static int nmrptftp_st = 0;
static  int req_times  = 0; 

static int keep_aliveTimer = NMRP_KEEPALIVE_TIMER/4;
extern void nmrp_send(void);
#ifdef CONFIG_CMD_TFTPSRV
void TftpStartServer(void);
#endif

/*
Enter nmrp mode：
Power LED solid Red，WPS LED solid Green
Other off
 
Reworking：
Power LED solid Red, WPS LED blinking Green（0.25 sec on and 0.75 sec off）
Other off
 
Upgrade success：
Power LED blinking green（0.5 sec on and 0.5 sec off）
Other off
 
Upgrade Failed：
Power LED blinking red (0.5 sec on and 0.5 sec off)
Other off
*/

static void nmrp_ledInit(void)
{
    an_gpio_set_value(LED_WPS_GREEN, LOW);
    an_gpio_set_value(LED_POWER_GREEN, HIGH);
    an_gpio_set_value(LED_POWER_RED, LOW);
}

static void nmrp_ledReworking(void)
{
    int ret = 0;
    struct udevice *dev = NULL;


    gpio_led_set_period(LED_WPS_GREEN, 250, 750);
    an_gpio_set_value(LED_POWER_GREEN, HIGH);
    an_gpio_set_value(LED_POWER_RED, LOW);
}

static void nmrp_ledEnd(int result)
{
    int ret = 0;
    struct udevice *dev = NULL;

    gpio_led_set_period(LED_WPS_GREEN, 0, 0);
    an_gpio_set_value(LED_WPS_GREEN, HIGH);

    if (result == 0)
    {
        gpio_led_set_period(LED_POWER_GREEN, 500, 500);
        an_gpio_set_value(LED_POWER_RED, HIGH);
    }
    else
    {
        an_gpio_set_value(LED_POWER_GREEN, HIGH);
        gpio_led_set_period(LED_POWER_RED, 500, 500);
    }
}

void Nmrp_LedHander(int state)
{
    if (state > NMRP_STATE_CONFIGING && state < NMRP_STATE_CLOSING)
    {
        nmrp_ledReworking();
    }
    else if (state == NMRP_STATE_CLOSED 
            || state == NMRP_STATE_CLOSING)
    {
        if (upgradeResult)
        {
            nmrp_ledEnd(1);
        }
        else
        {
            nmrp_ledEnd(0); 
        }
    }
}


void Nmrp_setState(int state)
{
    if (state != NmrpState)
    {
        req_times = 0;
    }

    if (!NMRP_STATE_INIT)
    {
        NmrpState = state;
    }
    Nmrp_LedHander(state);
}

int Nmrp_getState(void)
{
    return NmrpState;
}

static void NmrpTimeout(void)
{
    switch (NmrpState)
    {
        case NMRP_STATE_LISTENING:
        {
            printf("nmrp listen timeout.\n");
            if (g_nHasNmrp)
            {
                printf("nmrp exit fail.\n");
                nmrp_ledEnd(1);
                NmrpSetTimeout(0, NULL);
                net_set_state(NETLOOP_CONTINUE);
                Nmrp_setState(NMRP_STATE_INIT);
            }
            else
            {
                net_set_state(NETLOOP_SUCCESS);
                Nmrp_setState(NMRP_STATE_INIT);
            }
        }
        break;
        case NMRP_STATE_CONFIGING:
        {
            if (req_times < 5)
            {
                nmrp_send();
                NmrpSetTimeout(NMRP_INTERVAL_TIMEOUT, NmrpTimeout);
                net_set_state(NETLOOP_CONTINUE);
                req_times ++ ;
            }
            else
            {
                printf("nmrp configing timeout to listen.\n");
                Nmrp_setState(NMRP_STATE_LISTENING);
                NmrpSetTimeout(NMRP_LISTEN_TIMEOUT, NmrpTimeout);
                net_set_state(NETLOOP_CONTINUE);
            }
        }
        break;
        case NMRP_STATE_TFTPWAITING:
        {
            if (req_times < 5)
            {
                nmrp_send();
                NmrpSetTimeout(NMRP_INTERVAL_TIMEOUT, NmrpTimeout);
                net_set_state(NETLOOP_CONTINUE);
                req_times ++ ;
            }
            else
            {
                printf("nmrp tftpwaiting timeout to configing.\n");
                Nmrp_setState(NMRP_STATE_CONFIGING);
                NmrpSetTimeout(NMRP_LISTEN_TIMEOUT, NmrpTimeout);
                net_set_state(NETLOOP_CONTINUE);
            }
        }
        break;
        case NMRP_STATE_TFTPUPLOADING:
        {
            if (nmrptftp_firmware == 1)
            {
                printf("nmrp tftp upload complete.\n");
                nmrptftp_firmware = 0;
                Nmrp_setState(NMRP_STATE_KEEP_ALIVE);
                nmrp_send();

                Nmrp_setState(NMRP_STATE_FILEUPLOADING);
                net_set_state(NETLOOP_CONTINUE);
                NmrpSetTimeout(NMRP_INTERVAL_TIMEOUT, NmrpTimeout);
            }
            else 
            {
                if (nmrptftp_firmware == 2)
                {
                    printf("nmrp tftp up request again.\n");
                    nmrptftp_firmware = 0;
                    //TftpStartServer();
                    memcpy(NetServerEther, NmrpServerEther, 6);
                    Nmrp_setState(NMRP_STATE_TFTPWAITING);
                    nmrp_send();
                }
                NmrpSetTimeout(NMRP_INTERVAL_TIMEOUT, NmrpTimeout);
                net_set_state(NETLOOP_CONTINUE);
            }
        }
        break;
        case NMRP_STATE_FILEUPLOADING:
        {
            /*write image to flash*/
            upgradeResult = 0;
            printf("write firmware...\n");

            if (run_command(getenv("boot_wr_img"), 0) < 0)
            {
                upgradeResult = 1;
                printf("write flash fail.\n");
            }
            else
            {
                if (strcmp("good", getenv("decrypt_result")) == 0 &&
                        strcmp("good", getenv("filesize_result")) == 0)
                {
                    int ret = 0;
                    printf("upgrade firmeare success.\n");
                    ret = setenv("fenv_restore", "yes");
                    //if ((ret == 0) && (saveenv() == 0))
                    if (ret == 0)
                    {
                        upgradeResult = 0;
                        printf("reset config default succes.\n");
                    }
                    else
                    {
                        upgradeResult = 1;
                        printf("reset config default fail.\n");
                    }
                }
                else
                {
                    upgradeResult = 1;
                    printf("write flash decrypt[%s]  filesize[%s] fail.\n",
                            getenv("decrypt_result"), getenv("filesize_result"));
                }
            }
#if 0
            if (upgradeResult)
            {
                 upgradeResult = 0;
                /*fail to tftpwaiting */
                printf("nmrp write image error tftp up request again.\n");
                nmrptftp_firmware = 0;
                TftpStartServer();
                memcpy(NetServerEther, NmrpServerEther, 6);
                Nmrp_setState(NMRP_STATE_TFTPWAITING);
                nmrp_send();
                net_set_state(NETLOOP_CONTINUE);
                NmrpSetTimeout(NMRP_INTERVAL_TIMEOUT, NmrpTimeout);
            }
            else
#endif
            Nmrp_setState(NMRP_STATE_CLOSING);
            /*send keep alive*/
            net_set_state(NETLOOP_CONTINUE);
            NmrpSetTimeout(NMRP_INTERVAL_TIMEOUT, NmrpTimeout);
        }
        case NMRP_STATE_CLOSING:
        {
            if (req_times < 12)  /*6s*/
            {
                nmrp_send();
                printf("nmrp closing...\n");
                NmrpSetTimeout(NMRP_INTERVAL_TIMEOUT, NmrpTimeout);
                req_times ++;
            }
            else
            {
                Nmrp_done();
            }
        }
        case NMRP_STATE_CLOSED:
        {
            /*led active*/
        }
        break;
    }
}

static void NmrpLedTimeout(void)
{
#ifdef CONFIG_LED_BLINK
    gpio_led_tick(get_timer(0));
#endif
    if (Nmrp_getState() == NMRP_STATE_FILEUPLOADING)
    {
        nmrp_keepalive_send(get_timer(0));
    }

    NmrpLedSetTimeout(NMRP_INTERVAL_LEDTIMEOUT, NmrpLedTimeout);
}

void NmrpInit(void)
{ 
    NetNmrpTxPacket = &NetNmrpPacketBuf[0] + (PKTALIGN - 1);
    NetNmrpTxPacket -= (ulong)NetNmrpTxPacket % PKTALIGN;
    NmrpState = NMRP_STATE_INIT;
    NetLoop(NMRP);
}

void NmrpStart(void)
{
    int ret = 0;
    struct udevice *dev = NULL;

    Nmrp_setState(NMRP_STATE_LISTENING);
    NetNmrpTxPacket = &NetNmrpPacketBuf[0] + (PKTALIGN - 1);
    NetNmrpTxPacket -= (ulong)NetNmrpTxPacket % PKTALIGN;

    an_gpio_set_value(LED_POWER_GREEN, LOW);
    an_gpio_set_value(LED_POWER_RED, LOW);

    /*open advertise timer 5s*/    
    NmrpSetTimeout(NMRP_LISTEN_TIMEOUT, NmrpTimeout);
    NmrpLedTimeout();
}

static void Nmrp_done(void)
{
    if (NmrpState == NMRP_STATE_CLOSING)
    {
        Nmrp_setState(NMRP_STATE_CLOSED);
        nmrp_send();

        memset(NetServerEther, 0, 6);
        NmrpSetTimeout(0, NULL);
        net_set_state(NETLOOP_CONTINUE);
        printf("nmrp closed.\n");
        saveenv();
        Nmrp_LedHander(NMRP_STATE_CLOSED);
    }
}

void Nmrp_cleanup(void)
{
    int ret = 0;
    struct udevice *dev = NULL;

    NmrpState = NMRP_STATE_INIT;
    memset(NetServerEther, 0, 6);
    NmrpSetTimeout(0, NULL);
    NmrpLedSetTimeout(0, NULL);
    net_set_state(NETLOOP_SUCCESS);
    /* reinit power led */
    printf("nmrp Nmrp_cleanup.\n");

    an_gpio_set_value(LED_WPS_GREEN, HIGH);
    an_gpio_set_value(LED_POWER_GREEN, LOW);
    an_gpio_set_value(LED_POWER_RED, LOW);
}

int nmrp_need_tftp_upload_firmware(void)
{
    return s_nNmrpTftpUploadFirmware;
}

void nmrp_set_tftp_upload_firmware_status(int nStatus)
{
    s_nNmrpTftpUploadFirmware = nStatus;
}

int nmrp_need_tftp_upload_string_table(void)
{
    return s_nNmrpTftpUploadStringTableIndex;
}

void nmrp_set_tftp_upload_string_table_status(int nStatus, int nIndex)
{
    if (nStatus == 0)
    {
        s_nNmrpTftpUploadStringTableIndex = 0;
    }
    else
    {
        s_nNmrpTftpUploadStringTableIndex = nIndex;
    }
    s_nNmrpTftpUploadStringTableBitmask |= (nStatus << (nIndex - 1));
}

static nmrp_parsed_opt_t *nmrp_parse(unsigned char * pkt, unsigned short optType)
{
    nmrp_parsed_msg_t *msg = (nmrp_parsed_msg_t *) pkt;
    nmrp_parsed_opt_t *opt, *optEnd;
    optEnd = &msg->options[msg->numOptions];
    for (opt = msg->options; opt != optEnd; opt++)
        if (opt->type == ntohs(optType))
            break;
    return msg->numOptions == 0 ? NULL : (opt == optEnd ? NULL : opt);
}

/*
  Function to parse the NMRP options inside packet.
  If all options are parsed correctly, it returns 0.
 */
static int nmrp_parse_options(unsigned char *pkt, nmrp_parsed_msg_t *nmrp_parsed)
{
    nmrp_t *nmrphdr= (nmrp_t*) pkt;
    nmrp_opt_t *nmrp_opt;
    int remain_len, opt_index = 0;

    nmrp_parsed->reserved = nmrphdr->reserved;
    nmrp_parsed->code     = nmrphdr->code;
    nmrp_parsed->id       = nmrphdr->id;
    nmrp_parsed->length   = nmrphdr->length;

    remain_len = ntohs(nmrphdr->length) - NMRP_HDR_LEN;
    nmrp_opt = &nmrphdr->opt;
    while (remain_len > 0)
    {
        memcpy(&nmrp_parsed->options[opt_index], nmrp_opt, ntohs(nmrp_opt->len));
        remain_len -= ntohs(nmrp_opt->len);
        nmrp_opt = (nmrp_opt_t *)(((unsigned char *)nmrp_opt) + ntohs(nmrp_opt->len));
        opt_index++;

        if (opt_index >= NMRP_MAX_OPT_PER_MSG)
        {
            break;
        }
    }
    nmrp_parsed->numOptions=opt_index;
    return remain_len;
}

void nmrp_string_table_bitmask_check(void)
{
    int update_bit;

    /* find string tables need to be update, begin with smallest bit */
    for (update_bit = 0; update_bit < STRING_TABLE_BITMASK_LEN; update_bit++)
    {
        if ((NmrpStringTableBitmask & (1 << update_bit)) != 0) {
            //if bit 0 is set, update ST 1, ... etc
            //NmrpStringTableUpdateList[NmrpStringTableUpdateCount] = update_bit + 1;
            NmrpStringTableUpdateCount++;
        }
    }
}

void nmrp_send_config_req(void)
{
    pkt_info_t pkt_info;
    pkt_info_t *pkt = NULL;

    pkt = &pkt_info;
    pkt->eb_length = 0;
    pkt->eb_ptr = NetNmrpTxPacket;
    pkt->eb_length = NetSetEther(pkt->eb_ptr, NmrpServerEther, PROT_NMRP);

    ebuf_append_u16_be(pkt, 0);                 //reserved
    ebuf_append_u8(pkt, NMRP_CODE_CONF_REQ);    //code
    ebuf_append_u8(pkt, 0);                     //id
    ebuf_append_u16_be(pkt, 6);                 //len
    NetSendPacket(NetNmrpTxPacket, pkt->eb_length);
    Nmrp_setState(NMRP_STATE_CONFIGING);
}

void nmrp_send_close_req(void)
{
    pkt_info_t pkt_info;
    pkt_info_t *pkt = NULL;

    pkt = &pkt_info;
    pkt->eb_length = 0;
    pkt->eb_ptr = NetNmrpTxPacket;
    pkt->eb_length = NetSetEther(pkt->eb_ptr, NmrpServerEther, PROT_NMRP);

    ebuf_append_u16_be(pkt, 0);                 //reserved
    ebuf_append_u8(pkt, NMRP_CODE_CLOSE_REQ);    //code
    ebuf_append_u8(pkt, 0);                     //id
    ebuf_append_u16_be(pkt, 6);                 //len
    NetSendPacket(NetNmrpTxPacket, pkt->eb_length);
}

void nmrp_send_keep_alive_req(void)
{
    pkt_info_t pkt_info;
    pkt_info_t *pkt = NULL;

    pkt = &pkt_info;
    pkt->eb_length = 0;
    pkt->eb_ptr = NetNmrpTxPacket;
    pkt->eb_length = NetSetEther(pkt->eb_ptr, NmrpServerEther, PROT_NMRP);

    ebuf_append_u16_be(pkt, 0);                 //reserved
    ebuf_append_u8(pkt, NMRP_CODE_KEEP_ALIVE_REQ);    //code
    ebuf_append_u8(pkt, 0);                     //id
    ebuf_append_u16_be(pkt, 6);                 //len
    NetSendPacket(NetNmrpTxPacket, pkt->eb_length);    
}

void nmrp_keepalive_send(ulong timestamp)
{
    static ulong keeptimeStart = 0;

    if (keeptimeStart == 0)
    {
        keeptimeStart = timestamp;
    }

    if ( (timestamp - keeptimeStart) >= keep_aliveTimer * CONFIG_SYS_HZ / 1000 )
    {
        nmrp_send_keep_alive_req();
        keeptimeStart = timestamp;
    }
}

void nmrp_send_tftp_upload_packet(char *pchFileName)
{
    pkt_info_t pkt_info;
    pkt_info_t *pkt = NULL;
    int nOptionFilenameLen = 0;

    pkt = &pkt_info;
    pkt->eb_length = 0;
    pkt->eb_ptr = NetNmrpTxPacket;
    pkt->eb_length = NetSetEther(pkt->eb_ptr, NmrpServerEther, PROT_NMRP);

    nOptionFilenameLen = NMRP_OPT_HEADER_LEN + strlen(pchFileName);
    //printf("\nReq file[%s][%d]\n", pchFileName, strlen(pchFileName));
    ebuf_append_u16_be(pkt, 0);
    ebuf_append_u8(pkt, NMRP_CODE_TFTP_UL_REQ);
    ebuf_append_u8(pkt, 0);
    /* Append the total length to packet
    * NMRP_HEADER_LEN for the length of "Reserved", "Code", "Identifier", "Length"
    * STRING_TABLE_FILENAME_OPT_LEN for the length of "Options". */
    ebuf_append_u16_be(pkt, NMRP_HEADER_LEN + nOptionFilenameLen);
    /* Append NMRP option type FILE-NAME */
    ebuf_append_u16_be(pkt, NMRP_OPT_FILE_NAME);
    /* Append the total length of NMRP option FILE-NAME */
    ebuf_append_u16_be(pkt, nOptionFilenameLen);
    /* Append the string table filename to FILE-NAME option value */
    /* Append the firmware filename to FILE-NAME option value */
    //sprintf(NmrpFirmwareFilename, "%s", pchFileName);
    //printf("\nReq file[%s][%d]\n", pchFileName, strlen(pchFileName));
    ebuf_append_bytes(pkt, pchFileName, strlen(pchFileName));

    NetSendPacket(NetNmrpTxPacket, pkt->eb_length);

}

void nmrp_send_tftp_upload_req(void)
{
    char achFileName[32] = { 0 };

    if (nmrp_need_tftp_upload_firmware())    //firmware upload
    {
        strcpy(achFileName, FIRMWARE_FILENAME);
    }
    else if (nmrp_need_tftp_upload_string_table()) //string table upload
    {
        sprintf(achFileName, "%s%02d", STRING_TABLE_FILENAME_PREFIX, s_nNmrpTftpUploadStringTableIndex);
    }
    else
    {
        return;
    }
    nmrp_send_tftp_upload_packet(achFileName);
}


void nmrp_send(void)
{
    switch (NmrpState)
    {
    case NMRP_STATE_LISTENING:
        nmrp_send_config_req();
        break;
    case NMRP_STATE_CONFIGING:
        nmrp_send_config_req();
        break;
    case NMRP_STATE_TFTPWAITING:
        nmrp_send_tftp_upload_req();
        break;
    case NMRP_STATE_TFTPUPLOADING:
        printf("TFTP upload done.\n");
        break;
    case NMRP_STATE_KEEP_ALIVE:
    case NMRP_STATE_FILEUPLOADING:
        nmrp_send_keep_alive_req();
        break;
    case NMRP_STATE_CLOSING:
        nmrp_send_close_req();
        break;
    case NMRP_STATE_CLOSED:
        //nmrp_led_success();
        //net_set_state(NETLOOP_SUCCESS);
        break;
    default:
        break;
    }
}

int nmrp_pkt_advertisement_handler(nmrp_parsed_msg_t *ptNmrpParsedMsg)
{
    nmrp_parsed_opt_t *ptOption;

    if (NmrpState == NMRP_STATE_LISTENING)
    {
        /*
        Check if we get the MAGIC-NO option and the content is match with the MAGICNO.
        */
        if ((ptOption = nmrp_parse((unsigned char *)(ptNmrpParsedMsg), NMRP_OPT_MAGIC_NO)) != NULL)
        {
            if (MAGICNO == ptOption->value.magicno)
            {
                g_nHasNmrp = 2;
                printf("nmrp configing\n");
                nmrp_ledInit();
                nmrp_send();
            }
        }
    }

    return 0;
}

int nmrp_pkt_config_ack_handler(nmrp_parsed_msg_t *ptNmrpParsedMsg)
{
    nmrp_parsed_opt_t *ptOption;

    if (NmrpState != NMRP_STATE_CONFIGING)
    {
        return 0;
    }

    /*
    If there is no DEV-IP option inside the packet, it must be
    something wrong in the packet, so just ignore this packet
    without any action taken.
    */
    ptOption = nmrp_parse((unsigned char *)(ptNmrpParsedMsg), NMRP_OPT_DEV_IP);
    if (ptOption == NULL)
    {
        return -1;
    }
    memcpy(NetOurTftpIP, ptOption->value.ip.addr, IP_ADDR_LEN);
    printf("option ip[%d.%0d.%d.%d] subnet:%d.%d.%d.%d\n", 
            NetOurTftpIP[0],NetOurTftpIP[1],
            NetOurTftpIP[2],NetOurTftpIP[3],
               ptOption->value.ip.mask[0], ptOption->value.ip.mask[1],
               ptOption->value.ip.mask[2], ptOption->value.ip.mask[3]);

    //NetOurIP = (*(uint *) (NetOurTftpIP));
	memcpy(&NetOurIP, ptOption->value.ip.addr, IP_ADDR_LEN);
    /* Do we need the subnet mask? */
    memcpy(&NmrpOuripSubnetMask, ptOption->value.ip.mask, IP_ADDR_LEN);
    //NetOurSubnetMask = NmrpOuripSubnetMask;
	memcpy(&NetOurSubnetMask, ptOption->value.ip.mask, IP_ADDR_LEN);

    /*
    FW-UP option is optional for CONF-ACK and it has no effect no
    matter what is the content of this option, so we just skip the
    process of this option for now, and will add it back when
    this option is defined as mandatory.
    The process for FW-UP would be similar as the action taken for
    DEV-IP and MAGIC-NO.
    */
    /*When NMRP Client get CONF-ACK with DEV-REGION option*/
    ptOption = nmrp_parse((unsigned char *)(ptNmrpParsedMsg), NMRP_OPT_DEV_REGION);
    if (ptOption != NULL)
    {
        unsigned short wRegionNumber;
        /* Save DEV-REGION value to board */
        /*
        0x0001    NA
        0x0002    WW EU
        0x0003    GR
        0x0004    PR
        0x0005    RU
        0x0006    BZ
        0x0007    IN
        0x0008    KO
        0x0009    JP
        0x000a    AU
        0x000b    CA
        0x000c    US
        */
        char *Region[0x0d] =
         {"NA","NA","WW","GR","PR","RU",
           "BZ","IN","KO","JP","AU",
           "CA","US"};
        
        wRegionNumber = ntohs(ptOption->value.region);
        printf("get region option, value:0x%04x\n", wRegionNumber);
        
        if (wRegionNumber > 0)
        {
            int ret = 0;

            if ((strcmp(Region[wRegionNumber], "KO") == 0) || (strcmp(Region[wRegionNumber], "BZ") == 0) || (strcmp(Region[wRegionNumber], "GR") == 0))
            {
                 printf("protest is not support these countrycode: KO, BZ, GR\n");
            }
            else
            {
                ret = setenv("fenv_region", Region[wRegionNumber]);
            }
            ret |= setenv("fenv_restore", "yes");
            //if ((ret == 0) && (saveenv() == 0))
            if (ret == 0)
            {
                printf("set region %s success.\n", Region[wRegionNumber]);
            }
            else
            {
                printf("set region %s fail.\n", Region[wRegionNumber]);
            }
        }
        else
        {
            printf("set region fail.\n");
        }
        
    }

    /*Check if NMRP Client get CONF-ACK with FW-UP option*/
    nmrp_set_tftp_upload_firmware_status(0);
    ptOption = nmrp_parse((unsigned char *)(ptNmrpParsedMsg), NMRP_OPT_FW_UP);
    if (ptOption != NULL)
    {
        printf("Recv FW-UP option\n");
        nmrp_set_tftp_upload_firmware_status(1);
        NmrpFwUPOption = 1;
    }
    else
    {
        printf("No FW-UP option\n");
        NmrpFwUPOption = 0;
    }

    /*When NMRP Client get CONF-ACK with ST-UP option*/
    ptOption = nmrp_parse((unsigned char *)(ptNmrpParsedMsg), NMRP_OPT_ST_UP);
    if (ptOption != NULL)
    {
        printf("Recv ST-UP option\n");
        NmrpSTUPOption = 1;
        /* Reset string tables' update related variables. */
        NmrpStringTableUpdateCount = 0;
        NmrpStringTableUpdateIndex = 0;
        //memset(NmrpStringTableUpdateList, 0, sizeof(NmrpStringTableUpdateList));

        /* Save from network byte-order to host byte-order. */
        NmrpStringTableBitmask = ntohl(ptOption->value.string_table_bitmask);

        nmrp_string_table_bitmask_check();
        printf("Total %d String Table need updating\n", NmrpStringTableUpdateCount);
    }
    else
    {
        printf("No ST-UP option\n");
        NmrpSTUPOption = 0;
    }

    if (NmrpFwUPOption == 0 && NmrpSTUPOption == 0)
    {
        Nmrp_setState(NMRP_STATE_CLOSING);
        nmrp_send();
    }
    else
    {
        Nmrp_setState(NMRP_STATE_TFTPWAITING);
        nmrp_set_tftp_upload_firmware_status(1);
        nmrp_send();
        //TftpStartServer();
        memcpy(NetServerEther, NmrpServerEther, 6);
    }

    return 0;
}

void NmrpReceive(struct ethernet_hdr *et, struct ip_udp_hdr *ip, int len)
{ 
    unsigned char byNmrpCode;
    nmrp_t *nmrphdr = NULL;
    nmrp_parsed_msg_t tNmrpParsedMsg;

    if(NULL == ip)
    {
        return ;
    }
     
    nmrphdr = (nmrp_t *)ip;
    byNmrpCode = nmrphdr->code;

    /*
    Check if Reserved field is zero. Per the specification, the reserved
    must be all zero in a valid NMRP packet.
    */
    if (nmrphdr->reserved != 0)
    {
        return ;
    }

    memcpy(NmrpServerEther, et->et_src, 6);
    memset(&tNmrpParsedMsg, 0, sizeof(nmrp_parsed_msg_t));

    /*
    Parse the options inside the packet and save it into nmrp_parsed for
    future reference.
    */
    if (nmrp_parse_options((unsigned char *)ip, &tNmrpParsedMsg) != 0)
    {
        /* Some wrong inside the packet, just discard it */
        return ;
    }

    //printf("NmrpHandler NmrpState %d,  NmrpCode: %d\n",NmrpState, byNmrpCode);
    switch (byNmrpCode)
    {
        case NMRP_CODE_ADVERTISE:	/*listening state * */
            nmrp_pkt_advertisement_handler(&tNmrpParsedMsg);
        break;

        case NMRP_CODE_CONF_ACK:
            nmrp_pkt_config_ack_handler(&tNmrpParsedMsg);
        break;

        case NMRP_CODE_KEEP_ALIVE_ACK:
        break;

        case NMRP_CODE_CLOSE_ACK:
            Nmrp_done();
        break;

        default:
        break;
    }
}
