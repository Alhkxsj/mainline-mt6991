/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mainline compatibility shims for the MTK connectivity stack.
 *
 * Force-included into every translation unit of the connectivity tree
 * (conninfra, connfem, common, wlan, bt, gps, fm) so that APIs removed or
 * renamed in recent mainline kernels keep compiling.
 *
 * IMPORTANT: this header must not include any kernel header that pulls in
 * <linux/printk.h>, otherwise the default pr_fmt() would be defined before a
 * translation unit gets a chance to define its own, producing a
 * "'pr_fmt' redefined" warning (fatal under -Werror).  Only <linux/types.h>
 * (size_t) is safe here.
 */
#ifndef _MTK_CONNINFRA_COMPAT_H
#define _MTK_CONNINFRA_COMPAT_H

#include <linux/types.h>

/* strncpy() was removed from mainline headers. */
#ifndef strncpy
static inline char *conninfra_strncpy(char *dst, const char *src, size_t n)
{
	size_t i;

	for (i = 0; i < n && src[i]; i++)
		dst[i] = src[i];
	for (; i < n; i++)
		dst[i] = '\0';
	return dst;
}
#define strncpy conninfra_strncpy
#endif

/* strlcpy() was removed as well. */
#ifndef strlcpy
#define strlcpy(dst, src, n) strscpy((dst), (src), (n))
#endif

/* del_timer()/del_timer_sync() renamed to timer_delete()/timer_delete_sync(). */
#ifndef del_timer
#define del_timer(timer) timer_delete(timer)
#endif
#ifndef del_timer_sync
#define del_timer_sync(timer) timer_delete_sync(timer)
#endif

/* from_timer() was removed in favour of container_of(). */
#ifndef from_timer
#define from_timer(var, callback_timer, timer_fieldname) \
	container_of(callback_timer, typeof(*var), timer_fieldname)
#endif

/* alarm_start_relative() folded into alarm_start_timer(). */
#ifndef alarm_start_relative
#define alarm_start_relative(alarm, expires) \
	alarm_start_timer((alarm), (expires), true)
#endif

#endif /* _MTK_CONNINFRA_COMPAT_H */
