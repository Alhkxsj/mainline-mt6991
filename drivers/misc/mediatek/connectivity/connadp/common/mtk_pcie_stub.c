// SPDX-License-Identifier: GPL-2.0
/*
 * Minimal PCIe helper stubs for the mainline connectivity port.
 *
 * The vendor Wi-Fi driver expects a set of MediaTek PCIe host-controller
 * helpers (mtk_pcie_*) that are exported by the vendor pcie-mediatek-gen3
 * driver.  Mainline's pcie-mediatek-gen3 driver does not provide them yet, so
 * provide inert fallbacks that let the driver load and probe.  Replace this
 * file once those helpers are ported.
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/pci.h>

int mtk_pcie_probe_port(int port)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_probe_port);

int mtk_pcie_remove_port(int port)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_remove_port);

int mtk_pcie_soft_off(struct pci_bus *bus)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_soft_off);

int mtk_pcie_soft_on(struct pci_bus *bus)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_soft_on);

u32 mtk_pcie_dump_link_info(int port)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_dump_link_info);

int mtk_pcie_disable_data_trans(int port)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_disable_data_trans);

int mtk_pcie_hw_control_vote(int port, bool hw_mode_en, u8 who)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_hw_control_vote);

int mtk_pcie_pinmux_select(int port_num, int state)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_pinmux_select);

int mtk_pcie_enable_cfg_dump(int port)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_enable_cfg_dump);

int mtk_pcie_disable_cfg_dump(int port)
{
	return 0;
}
EXPORT_SYMBOL(mtk_pcie_disable_cfg_dump);
