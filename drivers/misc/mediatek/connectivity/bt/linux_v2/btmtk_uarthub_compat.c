// SPDX-License-Identifier: GPL-2.0
/*
 * Dispatch layer for the legacy 8250_mtk uarthub API used by the downstream
 * BT driver.
 *
 * The mainline 8250_mtk.c does not implement the legacy mtk8250_uart_hub_*()
 * entry points, so provide them here and forward to the callbacks registered
 * by the (mainline) uarthub driver through uarthub_drv_callbacks_register().
 *
 * The MTK Bluetooth UART on MT6991 is routed through the UARTHUB block; vendor
 * always runs it in hub mode (hub-en = 1).  Without the hub the AP UART3 is
 * not bridged to the connsys BT UART and no data flows.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/tty.h>

#include "../../../uarthub/common/uarthub_drv_export.h"

typedef void (*btmtk_uarthub_irq_cb)(unsigned int err_type);
typedef void (*btmtk_uart_wakeup_cb)(void *);

static struct uarthub_drv_cbs uarthub_cbs;
static bool uarthub_cbs_valid;
static bool uarthub_opened;

void uarthub_drv_callbacks_register(struct uarthub_drv_cbs *cb)
{
	if (!cb)
		return;

	uarthub_cbs = *cb;
	uarthub_cbs_valid = true;
}
EXPORT_SYMBOL(uarthub_drv_callbacks_register);

void uarthub_drv_callbacks_unregister(void)
{
	memset(&uarthub_cbs, 0, sizeof(uarthub_cbs));
	uarthub_cbs_valid = false;
	uarthub_opened = false;
}
EXPORT_SYMBOL(uarthub_drv_callbacks_unregister);

static void btmtk_uarthub_ensure_open(void)
{
	if (uarthub_cbs_valid && !uarthub_opened && uarthub_cbs.open) {
		uarthub_cbs.open();
		uarthub_opened = true;
	}
}

int mtk8250_uart_hub_reset_flow_ctrl(void)
{
	btmtk_uarthub_ensure_open();
	if (uarthub_cbs_valid && uarthub_cbs.reset_flow_control)
		return uarthub_cbs.reset_flow_control();
	return 0;
}

int mtk8250_uart_hub_enable_bypass_mode(int bypass)
{
	btmtk_uarthub_ensure_open();
	/*
	 * Keep the hub in bypass mode.  The switch to multi-host after the
	 * firmware download needs UARTHUB baud/in-band flow setup that is not
	 * reproducible from the AP side here, and every transfer after that
	 * switch (dynamic patch download, HCI_RESET) times out.  In bypass the
	 * AP UART and the BT UART are connected raw and the whole download
	 * sequence works.
	 */
	bypass = 1;
	if (uarthub_cbs_valid && uarthub_cbs.bypass_mode_ctrl)
		return uarthub_cbs.bypass_mode_ctrl(bypass);
	return 0;
}

int mtk8250_uart_hub_is_ready(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.dev0_is_uarthub_ready)
		return uarthub_cbs.dev0_is_uarthub_ready(NULL);
	return 0;
}

int mtk8250_uart_hub_clear_request(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.dev0_clear_txrx_request)
		return uarthub_cbs.dev0_clear_txrx_request();
	return 0;
}

int mtk8250_uart_hub_fifo_ctrl(int ctrl)
{
	btmtk_uarthub_ensure_open();
	if (uarthub_cbs_valid && uarthub_cbs.md_adsp_fifo_ctrl)
		return uarthub_cbs.md_adsp_fifo_ctrl(ctrl);
	return 0;
}

int mtk8250_uart_hub_dump_with_tag(const char *tag)
{
	if (uarthub_cbs_valid && uarthub_cbs.dump_debug_info_with_tag)
		return uarthub_cbs.dump_debug_info_with_tag(tag);
	return 0;
}

int mtk8250_uart_hub_reset(void)
{
	btmtk_uarthub_ensure_open();
	if (uarthub_cbs_valid && uarthub_cbs.sw_reset)
		return uarthub_cbs.sw_reset();
	return 0;
}

int mtk8250_uart_hub_register_cb(btmtk_uarthub_irq_cb irq_callback)
{
	if (uarthub_cbs_valid && uarthub_cbs.irq_register_cb)
		return uarthub_cbs.irq_register_cb(irq_callback);
	return 0;
}

int mtk8250_uart_hub_assert_bit_ctrl(int ctrl)
{
	if (uarthub_cbs_valid && uarthub_cbs.assert_state_ctrl)
		return uarthub_cbs.assert_state_ctrl(ctrl);
	return 0;
}

int mtk8250_uart_hub_dev0_set_tx_request(struct tty_struct *tty)
{
	btmtk_uarthub_ensure_open();
	if (uarthub_cbs_valid && uarthub_cbs.dev0_set_tx_request)
		return uarthub_cbs.dev0_set_tx_request();
	return 0;
}

int mtk8250_uart_hub_dev0_set_rx_request(void)
{
	btmtk_uarthub_ensure_open();
	if (uarthub_cbs_valid && uarthub_cbs.dev0_set_rx_request)
		return uarthub_cbs.dev0_set_rx_request();
	return -1;
}

int mtk8250_uart_hub_dev0_clear_tx_request(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.dev0_clear_tx_request)
		return uarthub_cbs.dev0_clear_tx_request();
	return -1;
}

int mtk8250_uart_hub_dev0_clear_rx_request(struct tty_struct *tty)
{
	if (uarthub_cbs_valid && uarthub_cbs.dev0_clear_rx_request)
		return uarthub_cbs.dev0_clear_rx_request();
	return 0;
}

int mtk8250_uart_hub_get_host_wakeup_status(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.get_host_wakeup_status)
		return uarthub_cbs.get_host_wakeup_status();
	return 0;
}

int mtk8250_uart_hub_get_bt_sleep_flow_hw_mech_en(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.get_bt_sleep_flow_hw_mech_en)
		return uarthub_cbs.get_bt_sleep_flow_hw_mech_en();
	return 0;
}

int mtk8250_uart_hub_set_bt_sleep_flow_hw_mech_en(int enable)
{
	if (uarthub_cbs_valid && uarthub_cbs.set_bt_sleep_flow_hw_mech_en)
		return uarthub_cbs.set_bt_sleep_flow_hw_mech_en(enable);
	return 0;
}

int mtk8250_uart_hub_get_host_awake_sta(int dev_index)
{
	if (uarthub_cbs_valid && uarthub_cbs.get_host_awake_sta)
		return uarthub_cbs.get_host_awake_sta(dev_index);
	return 0;
}

int mtk8250_uart_hub_set_host_awake_sta(int dev_index)
{
	if (uarthub_cbs_valid && uarthub_cbs.set_host_awake_sta)
		return uarthub_cbs.set_host_awake_sta(dev_index);
	return 0;
}

int mtk8250_uart_hub_clear_host_awake_sta(int dev_index)
{
	if (uarthub_cbs_valid && uarthub_cbs.clear_host_awake_sta)
		return uarthub_cbs.clear_host_awake_sta(dev_index);
	return 0;
}

int mtk8250_uart_hub_get_host_bt_awake_sta(int dev_index)
{
	if (uarthub_cbs_valid && uarthub_cbs.get_host_bt_awake_sta)
		return uarthub_cbs.get_host_bt_awake_sta(dev_index);
	return 0;
}

int mtk8250_uart_hub_get_cmm_bt_awake_sta(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.get_cmm_bt_awake_sta)
		return uarthub_cbs.get_cmm_bt_awake_sta();
	return 0;
}

int mtk8250_uart_hub_get_bt_awake_sta(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.get_bt_awake_sta)
		return uarthub_cbs.get_bt_awake_sta();
	return 0;
}

int mtk8250_uart_hub_bt_on_count_inc(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.bt_on_count_inc)
		return uarthub_cbs.bt_on_count_inc();
	return 0;
}

int mtk8250_uart_hub_inband_is_support(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.inband_is_support)
		return uarthub_cbs.inband_is_support();
	return 0;
}

int mtk8250_uart_hub_inband_enable_ctrl(int enable)
{
	if (uarthub_cbs_valid && uarthub_cbs.inband_enable_ctrl)
		return uarthub_cbs.inband_enable_ctrl(enable);
	return 0;
}

int mtk8250_wakeup_callback_register(btmtk_uart_wakeup_cb wakeup_cb,
				     void *wakeup_param)
{
	return 0;
}

/*
 * Helpers used by the 8250_mtk driver.  In multi-host (non-bypass) mode the
 * AP UART and the BT side must run at the same rate and the UARTHUB has to be
 * told about it; otherwise its internal baud stays at the 12M default and the
 * first command after the firmware download (e.g. HCI_RESET) times out.
 */
int mtk8250_uarthub_is_bypass(void)
{
	if (uarthub_cbs_valid && uarthub_cbs.is_bypass_mode)
		return uarthub_cbs.is_bypass_mode();
	return 1;	/* no hub / not registered: behaves as raw link */
}

int mtk8250_uarthub_config_baud(unsigned int baud)
{
	/* The hub must be powered/open before its registers are touched. */
	if (!uarthub_cbs_valid || !uarthub_opened)
		return -1;

	if (uarthub_cbs.config_internal_baud_rate)
		uarthub_cbs.config_internal_baud_rate(0, baud);
	if (uarthub_cbs.config_external_baud_rate)
		uarthub_cbs.config_external_baud_rate(baud);

	return 0;
}
