/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Minimal connectivity clkbuf control interface for the MT6991 mainline port.
 *
 * The downstream clkbuf driver controls the SoC clock buffers (26M / BBCK2
 * PCIe reference clock) through the MT6363/MT6373 PMIC over SPMI.  Mainline
 * has no MT6991 PMIC/SPMI support yet, so the two controls used by the PCIe
 * host driver are provided as inert stubs; the bootloader leaves BBCK2/26M in
 * a usable state on this device.
 */
#ifndef _MTK_CLKBUF_CTRL_H
#define _MTK_CLKBUF_CTRL_H

#include <linux/types.h>

static inline int clkbuf_xo_ctrl(char *cmd, int xo_id, u32 input)
{
	return 0;
}

static inline int clkbuf_srclken_ctrl(char *cmd, int sub_id)
{
	return 0;
}

#endif /* _MTK_CLKBUF_CTRL_H */
