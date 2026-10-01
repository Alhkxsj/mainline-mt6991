/******************************************************************************
** Copyright (C), 2019-2029, Oplus Mobile Comm Corp., Ltd
** File: oplus_sap_accelerate.h
** Description: add for accelerating real-time data packet in SAP mode
** Version: 1.0
** Date : 2025/09/20
** CONNECTIVITY.WIFI.BASIC.SOFTAP.10084878
** TAG: OPLUS_FEATURE_WIFI_SAP_ACCELERATE
** ------------------------------- Revision History: ----------------------------
** <author>     <data>   <version>
** ------------------------------------------------------------------------------
** Deng Jia   2025/09/20    1.0
*******************************************************************************/

#ifndef _OPLUS_SAP_ACCELERATE_H
#define _OPLUS_SAP_ACCELERATE_H

void oplusSapAccelerateModuleInit(void);
void oplusSapAccelerateModuleDeInit(void);
void oplusEnableSapAccelerateModule(void);
void oplusDisableSapAccelerateModule(void);
bool oplusGetSapAcceFuncStatus(void);
void oplusNicRxMarkPriorPkt(struct SW_RFB*);
void oplusKalRxSavePriorPktInfo(struct sk_buff*);
void oplusKalTxAcceleratePriorPkt(struct sk_buff*);

#endif /* _OPLUS_SAP_ACCELERATE_H */
