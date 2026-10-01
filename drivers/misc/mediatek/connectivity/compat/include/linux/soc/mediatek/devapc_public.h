/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Minimal devapc_public.h for the mainline connectivity port.
 *
 * The vendor DEVAPC (device access permission control) framework is not part
 * of mainline; connectivity only registers a debug-dump callback on access
 * violations.  The callback registration is stubbed in
 * connadp/common/mtk_compat.c.
 */
#ifndef __DEVAPC_PUBLIC_H__
#define __DEVAPC_PUBLIC_H__

#include <linux/types.h>
#include <linux/list.h>

enum infra_subsys_id {
	INFRA_SUBSYS_MD = 0,
	INFRA_SUBSYS_CONN,
	INFRA_SUBSYS_PCIE,
	INFRA_SUBSYS_ADSP,
	INFRA_SUBSYS_GCE,
	INFRA_SUBSYS_APMCU,
	INFRA_SUBSYS_GZ,
	DEVAPC_SUBSYS_CLKMGR,
	DEVAPC_SUBSYS_CLKM,
	DEVAPC_SUBSYS_TEST,
	DEVAPC_SUBSYS_HFRP,
	DEVAPC_SUBSYS_RESERVED,
};

enum devapc_type {
	DEVAPC_TYPE_INFRA = 0,
	DEVAPC_TYPE_INFRA1,
	DEVAPC_TYPE_PERI_PAR,
	DEVAPC_TYPE_VLP,
	DEVAPC_TYPE_ADSP,
	DEVAPC_TYPE_MMINFRA,
	DEVAPC_TYPE_MMUP,
	DEVAPC_TYPE_GPU,
	DEVAPC_TYPE_GPU1,
	DEVAPC_TYPE_MAX,
};

enum devapc_cb_status {
	DEVAPC_OK = 0,
	DEVAPC_NOT_KE,
};

struct devapc_vio_callbacks {
	struct list_head list;
	enum infra_subsys_id id;
	void (*debug_dump)(void);
	enum devapc_cb_status (*debug_dump_adv)(uint32_t vio_addr);
};

void register_devapc_vio_callback(struct devapc_vio_callbacks *viocb);

#endif  /* __DEVAPC_PUBLIC_H__ */
