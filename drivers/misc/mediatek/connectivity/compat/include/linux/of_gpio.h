/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mainline shim for the removed <linux/of_gpio.h> legacy integer-GPIO API.
 *
 * The connectivity PMIC code only uses the legacy API to read (for debug
 * logging) the MT6376 PMIC_EN/FAULTB pins.  The legacy integer GPIO API has
 * been removed from mainline, so provide inert fallbacks that make the code
 * take the "GPIO not available" path.  The pins are driven through pinctrl
 * states elsewhere, so functionality is unaffected.
 */
#ifndef _MTK_CONNINFRA_OF_GPIO_H
#define _MTK_CONNINFRA_OF_GPIO_H

#include <linux/of.h>
#include <linux/types.h>
/* mainline still provides gpio_is_valid()/gpio_get_value() through the
 * legacy gpiolib shim, so only of_get_named_gpio() needs a fallback.  Pull
 * the real declarations in first to avoid clashing with the legacy inline
 * definitions. */
#include <linux/gpio.h>

static inline int conninfra_of_get_named_gpio(struct device_node *np,
					      const char *propname, int index)
{
	return -1;
}
#define of_get_named_gpio conninfra_of_get_named_gpio

#endif /* _MTK_CONNINFRA_OF_GPIO_H */
