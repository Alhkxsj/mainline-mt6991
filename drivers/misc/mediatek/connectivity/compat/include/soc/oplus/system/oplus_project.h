/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Minimal OPLUS project compatibility header for the mainline connectivity
 * port.  The OPLUS project/CDT services are Android-specific and not present
 * in mainline; connectivity code only consumes a few getters that select
 * board variants.  On mainline the variant is described entirely by device
 * tree, so these return neutral values.
 */
#ifndef _MTK_CONNINFRA_OPLUS_PROJECT_H
#define _MTK_CONNINFRA_OPLUS_PROJECT_H

#include <linux/types.h>

static inline unsigned int get_PCB_Version(void) { return 0; }
static inline unsigned int get_project(void) { return 0; }
static inline unsigned int get_prj(void) { return 0; }
static inline unsigned int is_project(int project) { return 0; }
static inline unsigned int get_eng_version(void) { return 0; }
static inline unsigned int get_cdt_version(void) { return 0; }
static inline unsigned int get_Oplus_Boot_Mode(void) { return 0; }

#endif /* _MTK_CONNINFRA_OPLUS_PROJECT_H */
