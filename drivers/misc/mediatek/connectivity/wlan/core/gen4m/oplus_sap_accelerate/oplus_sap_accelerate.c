/******************************************************************************
** Copyright (C), 2019-2029, Oplus Mobile Comm Corp., Ltd
** File: oplus_sap_accelerate.c
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

#include "precomp.h"
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/version.h>
#include "oplus_sap_accelerate.h"

#define OPLUS_HIGH_PRIOR_PKT_TID1 6
#define OPLUS_HIGH_PRIOR_PKT_TID2 7
#define OPLUS_HIGH_PRIOR_SKB_MARK 1
#define OPLUS_PRIOR_IP_ADDR_LIST_MAX_LENGTH 32
#define OPLUS_SAP_TX_ACCELERATE_FLAG 3
#define OPLUS_SAP_ACCELERATE_THRESHOLD 10

static struct hlist_head prior_ip_addr_list_head;
static spinlock_t prior_ip_addr_list_lock;
static bool g_func_enable;
static bool g_inited;

struct prior_ip_addr_node
{
    __be32 server_ip_addr;
    __be32 sta_ip_addr;
    int high_prior_count;
    struct hlist_node node;
};

void oplusSapAccelerateModuleInit(void)
{
    spin_lock_init(&prior_ip_addr_list_lock);
    INIT_HLIST_HEAD(&prior_ip_addr_list_head);
    g_inited = TRUE;
}

void oplusSapAccelerateModuleDeInit(void)
{
    struct prior_ip_addr_node *entry;
    struct hlist_node *tmp;
    spin_lock_bh(&prior_ip_addr_list_lock);
    hlist_for_each_entry_safe(entry, tmp, &prior_ip_addr_list_head, node)
    {
        if (entry) {
            hlist_del(&entry->node);
            kfree(entry);
        }
    }
    spin_unlock_bh(&prior_ip_addr_list_lock);
    g_inited = FALSE;
}

static bool oplusCheckifIpAddrPrivate(__be32 ipaddr)
{
    uint8_t a = (ipaddr) & 0xFF;
    uint8_t b = (ipaddr >> 8) & 0xFF;

    if (a == 10) {
        return TRUE;
    }
    if (a == 172 && (b >= 16 && b <= 31)) {
        return TRUE;
    }
    if (a == 192 && b == 168) {
        return TRUE;
    }

    return FALSE;
}

static void addNewLinklistNode(__be32 server_ip_addr, __be32 sta_ip_addr)
{
    struct prior_ip_addr_node *new_node = kmalloc(sizeof(struct prior_ip_addr_node), GFP_ATOMIC);
    if (!new_node) {
        DBGLOG(RX, WARN, "[oplusSapAccelerate]assign new node failed\n");
        return;
    }
    new_node->server_ip_addr = server_ip_addr;
    new_node->sta_ip_addr = sta_ip_addr;
    new_node->high_prior_count = 1;
    hlist_add_head(&new_node->node, &prior_ip_addr_list_head);
}

void oplusEnableSapAccelerateModule(void)
{
    if (!g_inited) {
        oplusSapAccelerateModuleInit();
    }
    g_func_enable = 1;
}

void oplusDisableSapAccelerateModule(void)
{
    if (g_inited) {
        oplusSapAccelerateModuleDeInit();
    }
    g_func_enable = 0;
}

bool oplusGetSapAcceFuncStatus(void)
{
    return g_func_enable;
}

void oplusNicRxMarkPriorPkt(struct SW_RFB *prRetSwRfb)
{
    if (!g_func_enable) {
        return;
    }
    struct sk_buff *prSkb = prRetSwRfb->pvPacket;
    if (!prSkb) {
        return;
    }
    uint32_t tid = prRetSwRfb->ucTid;

    if (tid == OPLUS_HIGH_PRIOR_PKT_TID1 || tid == OPLUS_HIGH_PRIOR_PKT_TID2) {
#ifdef CONFIG_ANDROID_KABI_RESERVE
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0))
        prSkb->android_kabi_reserved2 |= OPLUS_HIGH_PRIOR_SKB_MARK;
#else
        prSkb->__kabi_reserved2 |= OPLUS_HIGH_PRIOR_SKB_MARK;

#endif
#endif
    }
}

void oplusKalRxSavePriorPktInfo(struct sk_buff *prSkb)
{
    if (!g_func_enable) {
        return;
    }
    if (prSkb->protocol != htons(ETH_P_IP)) {
#ifdef CONFIG_ANDROID_KABI_RESERVE
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0))
        prSkb->android_kabi_reserved2 &= ~OPLUS_HIGH_PRIOR_SKB_MARK;
#else
        prSkb->__kabi_reserved2 &= ~OPLUS_HIGH_PRIOR_SKB_MARK;
#endif
#endif
        return;
    }

    bool is_high_prior = FALSE;
#ifdef CONFIG_ANDROID_KABI_RESERVE
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0))
    if (prSkb->android_kabi_reserved2 & OPLUS_HIGH_PRIOR_SKB_MARK) {
        is_high_prior = TRUE;
        prSkb->android_kabi_reserved2 &= ~OPLUS_HIGH_PRIOR_SKB_MARK;
    }
#else
    if (prSkb->__kabi_reserved2 & OPLUS_HIGH_PRIOR_SKB_MARK) {
        is_high_prior = TRUE;
        prSkb->__kabi_reserved2 &= ~OPLUS_HIGH_PRIOR_SKB_MARK;
    }
#endif
#endif

    struct ethhdr *eth = (struct ethhdr *) skb_mac_header(prSkb);
    if (!eth || skb_mac_header_was_set(prSkb) == 0) {
        DBGLOG(RX, WARN, "[oplusSapAccelerate]skb's mac header doesn't exist\n");
        return;
    }

    if (skb_headlen(prSkb) < ETH_HLEN + sizeof(struct iphdr)) {
        DBGLOG(RX, WARN, "[oplusSapAccelerate]skb's length is too short\n");
        return;
    }

    struct iphdr *iph = (struct iphdr *)((char *) eth + ETH_HLEN);
    __be32 saddr = iph->saddr;
    __be32 daddr = iph->daddr;

    if (oplusCheckifIpAddrPrivate(daddr)) {
        return;
    }

    struct prior_ip_addr_node *entry, *last = NULL;
    bool is_found_in_list = FALSE;
    unsigned char prior_ip_addr_list_cur_length = 0;
    spin_lock_bh(&prior_ip_addr_list_lock);
    hlist_for_each_entry(entry, &prior_ip_addr_list_head, node) {
        prior_ip_addr_list_cur_length++;
        last = entry;
        if (entry->server_ip_addr == daddr && entry->sta_ip_addr == saddr) {
            is_found_in_list = TRUE;
            break;
        }
    }

    if (!is_high_prior && !is_found_in_list) {
        goto unlock;
    }
    else if (is_high_prior && is_found_in_list) {
        if (entry->high_prior_count < OPLUS_SAP_ACCELERATE_THRESHOLD) {
            entry->high_prior_count++;
        }
        goto unlock;
    }
    else if (is_high_prior && !is_found_in_list) {
        if (prior_ip_addr_list_cur_length >= OPLUS_PRIOR_IP_ADDR_LIST_MAX_LENGTH) {
            if (last) {
                hlist_del(&last->node);
                kfree(last);
            }
        }
        addNewLinklistNode(daddr, saddr);
        DBGLOG(RX, INFO, "[oplusSapAccelerate]new addr pair: server[%pI4] sta[%pI4]\n", &daddr, &saddr);
        goto unlock;
    }
    else {
        if (--entry->high_prior_count <= 0) {
            hlist_del(&entry->node);
            kfree(entry);
        }
        goto unlock;
    }

unlock:
    spin_unlock_bh(&prior_ip_addr_list_lock);
    return;
}

void oplusKalTxAcceleratePriorPkt(struct sk_buff *prSkb)
{
    if (!g_func_enable) {
        return;
    }
    if (prSkb->protocol != htons(ETH_P_IP)) {
        return;
    }

    struct ethhdr *eth = (struct ethhdr *) skb_mac_header(prSkb);
    if (!eth) {
        return;
    }

    struct iphdr *iph = (struct iphdr *)((char *) eth + ETH_HLEN);
    __be32 saddr = iph->saddr;
    __be32 daddr = iph->daddr;
    struct prior_ip_addr_node *entry = NULL;

    spin_lock_bh(&prior_ip_addr_list_lock);
    hlist_for_each_entry(entry, &prior_ip_addr_list_head, node) {
        if (!entry) {
            DBGLOG(RX, WARN, "[oplusSapAccelerate]entry is null\n", &daddr, &saddr);
            continue;
        }
        if (entry->server_ip_addr == saddr && entry->sta_ip_addr == daddr) {
            if (entry->high_prior_count >= OPLUS_SAP_ACCELERATE_THRESHOLD) {
#ifdef CONFIG_ANDROID_KABI_RESERVE
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0))
                prSkb->android_kabi_reserved2 |= OPLUS_SAP_TX_ACCELERATE_FLAG;
#else
                prSkb->__kabi_reserved2 |= OPLUS_SAP_TX_ACCELERATE_FLAG;
#endif
#endif
            }
            break;
        }
    }
    spin_unlock_bh(&prior_ip_addr_list_lock);
    return;
}
