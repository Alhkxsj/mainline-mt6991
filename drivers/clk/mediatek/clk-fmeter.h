/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __CLK_FMETER_H
#define __CLK_FMETER_H

enum FMETER_TYPE {
	FT_NULL,
	ABIST,
	CKGEN,
	ABIST_2,
	ABIST_CK2,
	CKGEN_CK2,
	SUBSYS,
	VLPCK,
};

/*
 * Mainline build shim: the fmeter clock provider is not ported, so frequency
 * queries used only for debug logging return 0.
 */
static inline unsigned int mt_get_fmeter_freq(unsigned int id,
					      enum FMETER_TYPE type)
{
	return 0;
}

#endif
