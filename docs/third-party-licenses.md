# 第三方组件许可

本仓库**自有代码**按根目录 [`LICENSE`](../LICENSE)（MIT）授权。第三方组件不在此 MIT 范围内，各自沿用原许可。

本文件是第三方许可的**唯一声明处**：README 与 `docs/provenance.md` 只指向这里，不再重列许可表；
每个组件在仓库内只保留**一份**许可原文，不在别处重复抄录。

## 组件清单

| 组件（本项目内名称） | 仓库位置 | 许可 | 许可原文（仓库内唯一一份） | 安装包内位置 |
|---|---|---|---|---|
| LibreHardwareMonitor（XiaobaiHardwareMonitor） | `white-toolkit/` | MPL-2.0 | `white-toolkit/LICENSE` | `licenses/MPL-2.0-LibreHardwareMonitor.txt` |
| GIMPS prime95（XiaobaiPrime） | `xiaobaiprime/`、`验机软件/Tools/xiaobaiprime/` | GIMPS 最终用户许可 | `验机软件/Tools/xiaobaiprime/license.txt` | `licenses/GIMPS-Prime95-EULA.txt` |
| FurMark 负载（C#/OpenTK 重实现，含贴图） | `FurMark/` | MIT（Copyright (c) 2022 StanislavPetrovV） | `FurMark/Assets/LICENSE.FurMark.txt` | `licenses/MIT-FurMark.txt` |
| PawnIO 内核模块二进制（内嵌于 XiaobaiHardwareMonitorLib） | `white-toolkit/XiaobaiHardwareMonitorLib/Resources/PawnIo/` | LGPL-2.1 | `white-toolkit/XiaobaiHardwareMonitorLib/Resources/PawnIo/COPYING` | `licenses/LGPL-2.1-PawnIO.txt` |
| PawnIO 驱动安装器（官方签名 `PawnIO_setup.exe`） | `验机软件/Tools/pawnio/` | GPL-2.0（含设备 I/O 组合例外） | `验机软件/Tools/pawnio/COPYING.GPLv2` | `licenses/PawnIO-GPL-2.0.txt` |
| Aga.Controls | `white-toolkit/Aga.Controls/` | BSD（Copyright (c) 2009 Andrey Gliznetsov） | `white-toolkit/Aga.Controls/license.txt` | 不随包（仅源码子项目） |
| CrystalDiskMark 引擎 | `Plugins/DiskMark/CdmHost/` | 见上游项目许可（源码头声明 MIT） | 未随仓库（仅成品二进制） | 随包二进制，原文待补 |
| diskspd | `Plugins/DiskMark/CdmHost/CdmResource/diskspd/` | MIT | 未随仓库（仅成品二进制） | 随包二进制，原文待补 |
| NAudio | 影音会议插件 | MIT | 未随仓库（NuGet 包内 `license.txt`） | 未随包 |
| OpenTK | 散热测试插件 | MIT | 未随仓库（NuGet 包内 `license.txt`） | 未随包 |
| DiskInfoToolkit | 经 LibreHardwareMonitor 引用 | 见上游包声明 | 未随仓库（NuGet 包内） | 未随包 |

PawnIO 是两套许可，故占两行：安装器 `PawnIO_setup.exe` 按上游 GPL-2.0 分发，内嵌内核模块为 LGPL-2.1——上游 GPL-2.0 附带的设备 I/O 组合例外正是允许此类 LGPL 模块加载的依据，两者不可合并成一条。

上游地址与**改动范围**（改名映射、逐文件 diff）见 [`provenance.md`](provenance.md)，此处不重复。

## 打包规则

安装包由 `installer/build-release.ps1` 生成，许可文件的落位规则：

1. 安装目录根下只放本项目 `LICENSE.txt`（MIT 主许可证）。
2. 上表「安装包内位置」列的第三方许可原文，统一复制到安装目录的 `licenses/` 子目录，**不平铺到根目录**，也不在 `Tools/`、`plugins/` 等业务目录里重复保留副本。
3. 每个组件在 `licenses/` 内恰好一个文件；任一原件缺失或命名不符合本表时，打包脚本直接失败。
4. 上述规则不改变仓库内上游原文的位置（`white-toolkit/`、`FurMark/Assets/`、`验机软件/Tools/` 内的原文照旧保留——删它们等于抹掉上游署名，属反向合规风险）。

脚本里的 `$thirdPartyLicenses` 表与上表第 4 列一一对应，改文件名必须两边同步：

```
MPL-2.0-LibreHardwareMonitor.txt  <- white-toolkit/LICENSE
GIMPS-Prime95-EULA.txt            <- 验机软件/Tools/xiaobaiprime/license.txt
MIT-FurMark.txt                   <- FurMark/Assets/LICENSE.FurMark.txt
LGPL-2.1-PawnIO.txt               <- white-toolkit/XiaobaiHardwareMonitorLib/Resources/PawnIo/COPYING
PawnIO-GPL-2.0.txt                <- 验机软件/Tools/pawnio/COPYING.GPLv2
```

## 已知缺口

- CrystalDiskMark、diskspd 随包分发（`CdmHost.exe`、`diskspd64.exe`）但仓库内无许可原文，发布前应补入 `licenses/`；
  diskspd 为 Microsoft 的 MIT 原文，可直接取上游 `LICENSE`；CrystalDiskMark 上游不发独立 LICENSE 文件，只在每个源码头写 `License : MIT License`。
- NAudio、OpenTK、DiskInfoToolkit 的 MIT 原文可从 NuGet 包内取得，当前未附。
