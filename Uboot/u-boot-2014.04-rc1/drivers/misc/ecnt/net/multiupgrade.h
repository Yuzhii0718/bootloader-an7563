#ifndef __MULTIUPGRADE_H
#define __MULTIUPGRADE_H

#define UDP  	0x11
#define ICMP    0x01
#define TCP	0x06
#define MAXMULTILEDNUM 16
#define MULTI_UPGRADE_PKT_TAG		"TCBulkFW"
#define MULTIPKT_TAG_OFFSET 			12

#include <asm/tc3162.h>


 
extern uint8 internet_gpio;
extern uint8 dsl_gpio;

extern uint8 multi_upgrade_gpio[MAXMULTILEDNUM];
extern char startMultiUpgrade;

/*
 *	This structure defines an ip header.
 */
struct iphdr {
#ifndef TCSUPPORT_LITTLE_ENDIAN
    unsigned char   version:4,
                        ihl:4;
#else
    unsigned char       ihl:4,
                    version:4;
#endif
	unsigned char	tos;
	unsigned short	tot_len;
	unsigned short	id;
	unsigned short	frag_off;
	unsigned char	ttl;
	unsigned char	protocol;
	unsigned short	check;
	unsigned int	saddr;
	unsigned int 	daddr;
	/*The options start here. */
}__attribute__ ((packed));

#endif /* __MULTIUPGRADE_H */
