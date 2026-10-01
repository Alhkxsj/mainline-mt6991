# SPDX-License-Identifier: GPL-2.0
#
# MT6991 connectivity stack (downstream connv3 / MT6653 fullmac port).
# Built-in only for the realme GT7 (RMX6688) mainline bring-up.

obj-$(CONFIG_MTK_COMBO) += connadp/
obj-$(CONFIG_MTK_COMBO) += conninfra/
obj-$(CONFIG_MTK_COMBO) += connfem/
obj-$(CONFIG_MTK_COMBO) += wlan/adaptor/build/connac3x_6989_6653/
obj-$(CONFIG_MTK_COMBO) += wlan/core/gen4m/
# Vendor bt/linux_v2 (Android/bluedroid) disabled: replaced by the mainline
# BlueZ path (drivers/bluetooth/btmtkuart) for MT6653.  Re-enabling it would
# collide on btmtk_* symbols.
# obj-$(CONFIG_MTK_COMBO) += bt/linux_v2/

# Select the exact MT6653 2g2a build via SEGMENT=SP + MODULE_NAME, which makes
# gen4m/Kbuild include Kbuild.6991_6653_2g2a.
export SEGMENT := SP
export MODULE_NAME := wlan_drv_gen4m_6991_6653_2g2a
export CONFIG_WLAN_DRV_BUILD_IN := y
export CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH := y
export TARGET_BUILD_VARIANT := user
# Marker consumed by the downstream wlan/bt Makefiles to select built-in
# behaviour (e.g. do not redefine cfg80211 internals such as is_world_regdom).
export KBUILD_SUBDIR_CCFLAGS += MTK_WCN_REMOVE_KERNEL_MODULE
