# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_MTK_UARTHUB) += uarthub_drv.o

uarthub_drv-y := common/uarthub_drv_core.o
uarthub_drv-y += common/uarthub_drv_debug.o
uarthub_drv-y += common/uarthub_drv_export.o
uarthub_drv-y += common/uarthub_drv_ut_test.o
uarthub_drv-y += common/platform/6991/mt6991.o
uarthub_drv-y += common/platform/6991/mt6991_debug.o

ccflags-y += -I$(src)/common
ccflags-y += -I$(src)/common/platform/6991
ccflags-y += -I$(srctree)/drivers/clk/mediatek
ccflags-y += -I$(srctree)/drivers/pinctrl/mediatek
ccflags-y += -I$(srctree)/drivers/misc/mediatek/connectivity/compat/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
