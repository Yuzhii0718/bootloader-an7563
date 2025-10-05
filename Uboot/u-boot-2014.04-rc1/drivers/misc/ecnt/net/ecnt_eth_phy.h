#ifndef __ECNT_ETH_PHY_H
#define __ECNT_ETH_PHY_H
//sync kernel sys serdes order, ref. arht_serdes_cfg.h
#ifdef TCSUPPORT_CPU_AN7552
#define SERDES_HSGMII_MODE     '1'
#else
#define SERDES_XFI_MODE        '0'
#define SERDES_USXGMII_MODE    '1'
#define SERDES_HSGMII_MODE     '2'
#define SERDES_5GBaseR_MODE    '3' //no setting 20230706
#define SERDES_SGMII_MODE      '4'
#endif


void serdes_phy_init();

#endif

