# RMX6688 (realme GT7 / Dimensity 9400+ / MT6991) mainline bring-up

主仓库 = linux-7.2.3 内核树本身(git 根 = 本目录)。构建、打包全部在本机(手机
aarch64)完成,无交叉工具链。历史/约束见文末。

## 功能现状

### 已实现 / 正常工作 ✅

| 子系统 | 说明 |
|---|---|
| CPU 频率 | `cpufreq-hw`(performance-domain):A720 339–2400 / X4 622–3300 / X925 798–3730 MHz |
| UFS 存储 | 挂载 Ubuntu rootfs 正常启动 |
| DRM + 面板 | EXDMA3+BLENDER+OUTPROC 新架构;1280×2800;**60/90/120/144Hz**;DSC 1.2;DPMS off/on;`mediatekdrmfb` fbdev 控制台 |
| 显示颜色 | 强制 OVL 只 advertise 8-bit,修 24.04 mutter 选 10-bit 导致的颜色错乱 |
| GPU | panthor / Immortalis-G925:probe、渲染、devfreq/DVFS(338–1612MHz);修了空闲 runtime-PM 循环崩溃 |
| 触摸屏 | Synaptics S3910 (TCM v1, SPI6):单点/多点、量程 12799×27999→1280×2800、边缘校准;固件不自动刷写 |
| USB-C Gadget | CDC-ACM (g_serial);修 `VBUS_FRC_EN`、xsphy vendor 序列、NXP repeater 驱动 |
| WiFi | MT6653 (connv3, PCIe HIF):扫描/认证/关联/WPA2 四次握手/联网全部正常;conninfra+connadp+connfem+gen4m+PCIe 全量内建 |
| Connectivity 电源栈 | connv3/conninfra、connfem(FEM)、NVRAM 加载、自动上电 |
| 电池电量计 | TI bq27541 @ i2c0 0x55;`/sys/class/power_supply/` 暴露容量/电压/电流/温度,GNOME 显示电量。**只读** |
| 热管理 (LVTS) | 31 传感器(25 有效);LVTS 硬件热复位保护,不做软件降频 |
| 桌面系统 | Ubuntu 24.04 + GNOME/mutter 稳定运行 |
| 日志通道 | pstore/ramoops 可保存 panic 前日志 |

### 未完成 / 阻塞 ❌

| 子系统 | 状态 / 卡点 |
|---|---|
| 蓝牙 (MT6653) | 正在从厂商 `bt/linux_v2` 改走**主线 BlueZ(`btmtk`/`btmtkuart`, serdev+H4+STP)**:驱动绑定、`hci0` 注册、connv3 电源、MT6653 握手代码均已实现;但 BT UART 经 **UARTHUB**,AP 第一帧始终无回应(`query uart failed -110`)。引脚复用/复位/uarthub 放行序列厂商 pre_on 顺序均已试过,疑点转向 uarthub 数据面与 connv3 上电竞争 |
| 传感器 | 缺厂商总线/地址/上电/初始化固件信息(BMI2xy/AK09918/TCS3720),厂商架构依赖 SCP sensorhub |
| 系统 suspend/resume | 电源域下电会丢 bootloader 状态(MIPI TX PHY、blender 链、SMMU、唤醒源均未自包含) |
| 充电控制 (MT6379) | 电量计可读;充电需补 charger 驱动 |
| GPS / FM | 需编译 `gps/` + 补 DTS/EMI 节点 |
| 冷启动无 LK 显示 | 依赖 bootloader 显示的中间级(PHY/mutex/blender)尚未由主线完整接管 |
| 10-bit / HDR scanout | 当前只有 8-bit 管线 |
| MT6373/MT6363 PMIC 驱动 | 主线上电走 raw SPMI/寄存器 hack(触摸 AVDD、USB repeater 等由 init 处理) |

## 布局

| 路径 | 内容 |
|---|---|
| `arch/arm64/boot/dts/mediatek/mt6991*.dts*` | 板级/SoC DTS + 保留内存(提交进内核树) |
| `arch/arm64/kernel/embedded-dtb.S` 等 | CONFIG_ARM64_EMBEDDED_DTB:把 DTB 用 .incbin 链进 Image |
| `initramfs/` | **独立嵌套 git 仓库**:initramfs 源码(init.c 调试 init / init-rootfs.c rootfs 切换 init)、busybox、触摸 + mali(GPUEB CSF)固件。不在主仓库版本控制内 |
| `out/` | kbuild O=out 全部产物(.config/Image/System.map/dtb,已 gitignore) |
| `bt/` | pack_boot.sh 工作区:magiskboot + 原厂 boot.img + 成品(已 gitignore) |
| `build.sh` | 一次构建(5 步,见下) |
| `pack_boot.sh` | 把 out 的 Image 打包成可刷 boot.img(magiskboot 往返校验) |
| `rmx6688.fragment` | defconfig 之上合并的 Kconfig 增量 |
| `gen_resmem.py` / `verify_dtb.py` / `fdt` | resmem dtsi 的生成/核对工具 + 原厂 FDT |

## 一次构建

```sh
# 1) init 必须前台编译过(后台链会吞错误)。默认编 rootfs 切换 init:
gcc -static -O2 -Wall -Wextra -o initramfs/init initramfs/init-rootfs.c
#    调试 init(gcc 换成 init.c),或直接 INIT_SRC=init.c ./build.sh
# 2) 后台构建:
nohup ./build.sh > out/build.log 2>&1 &   # 产物:out/arch/arm64/boot/Image
```

build.sh 步骤:[1/5] 静态 init(INIT_SRC,默认 init-rootfs.c)→ [2/5]
cpio_list(含 busybox + mali CSF 固件)→ [3/5] merge_config + 17 项
CONFIG 校验 → [4/5] 只编本板 DTB → [5/5] Image + **内嵌 DTB 字节级比对**
(System.map 符号定位 dd 出来与产物 dtb cmp)。

## 打包刷机

```sh
./pack_boot.sh        # 自动取下一轮号(N=max+1);或 ./pack_boot.sh 39
```

magiskboot 流程:unpack 原厂 boot.img → 换 kernel → repack → 再 unpack 成品
与 Image 逐字节 cmp。成品 `bt/new-boot.img.N`。

## resmem 工具(只在改保留内存时用)

- `./gen_resmem.py` — 从 `fdt`(原厂 FDT 提取件)重新生成
  `.../mt6991-realme-rmx6688-resmem.dtsi`(期望零 diff;pstore 节点手写在
  板级 dts,SKIP 表跳过)。fdt.dts 可由 `scripts/dtc/dtc -I dtb -O dts fdt` 还原,不入库。
- `./verify_dtb.py` — 构建后核对产物 DTB(out 下)的保留内存与原厂一致。

## Git 历史与纪律

- 历史:import 原版 7.2.3(提交 1)→ 内核 bring-up delta(提交 2)→ 本构建设施
  (提交 3)。今后每次改动按提交记录。
- `initramfs/` 独立仓库:在其目录内单独 commit,不并入内核提交。
- 约束(不得违反):
  - 打包只用 `bt/magiskboot`;别处任何 bootimg.py 都是废件,禁用。
  - 永不修改内核 ext4 相关源码(用户态 init 解决)。
  - init 任何改动先前台 gcc 编译过再后台跑 build.sh。
  - 日志采集 = 硬重启 ring capture(pstore_report + 40s panic → 重启后
    Android 侧读 console-ramoops-0);sdc5 pstore-stash 方案已废弃,不复用。
  - 内核 ext4/initramfs 的 CONFIG 基线见 rmx6688.fragment,build.sh 里 17 项
    校验防回归。
