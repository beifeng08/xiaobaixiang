# 笔吧验机

**免费开源的 Windows 硬件验机工具**——二手/新机到手，一键采集硬件信息、逐项人工核对、确认通过。

本项目的能力分两层：应用层（插件框架、核对流程、报告导出、安装器）是本项目的主要工作；底层采集与烤机能力在成熟开源方案上集成——硬件采集基于 LibreHardwareMonitor（MPL-2.0）源码定制，CPU 烤机基于 GIMPS 公开源码构建 prime95，硬盘跑分集成 CrystalDiskMark 引擎，GPU 烤机为 FurMark 负载的 C#/OpenTK 重实现。各组件的上游地址、许可与改动范围见 [`docs/third-party-licenses.md`](docs/third-party-licenses.md) 与 [`docs/provenance.md`](docs/provenance.md)。

## 核心亮点：多源信息交叉验证

每个硬件字段都由**多个独立来源交叉验证**，单一来源拿不到或读不准时自动降级到下一精度层级，而不是简单显示"未知"：

- **显存三级精度**：厂商驱动接口（NVAPI / ADL）直读 → 注册表 `qwMemorySize` → WMI `AdapterRAM`（后者 >4GB 溢出，仅兜底）——规避了 WMI 在大显存显卡上读数溢出的经典问题
- **CPU 型号**：CPUID 直读优先，WMI 兜底，两源互证
- **电池**：电池类驱动 IOCTL 优先，`root\WMI` 电池类补充，与 `powercfg /batteryreport` 同源
- **硬盘**：SMART 直读（NVMe / SATA 分路），无管理员权限时自动降级到系统接口

这套**"开源采集层 + Windows 自带接口"的多源降级策略**，把通常被认为"必须闭源库"（HWiNFO 等）才能达到的采集深度做成了可复现的开源实现——这是本项目与主流验机工具的差异所在。

## 功能一览

| 模块 | 能力 |
|---|---|
| **配置核对** | CPU/显卡/内存/硬盘/网卡/屏幕/电池 自动采集，与订单逐项核对；含 SMART 健康度、EDID 色域容积比估算、电池损耗与循环次数 |
| **键盘测试** | 104 键可视化，按住高亮、松开变绿，进度实时显示 |
| **屏幕坏点** | 全屏纯色切换（白/红/绿/蓝/黑），多屏逐台检测 |
| **影音会议** | 摄像头预览、麦克风电平+录制回放、扬声器试音 |
| **散热测试** | CPU/GPU/双烤三模式，四路温度功耗实时折线；烤机进程无窗口后台运行，多层兜底不留残留 |
| **硬盘跑分** | 集成 CrystalDiskMark 引擎，SEQ1M + RND4K 四项，成绩 JSON/CSV 落盘 |

## 技术关键

- **插件化架构**：每个验机模块是独立 class library 插件，安装向导可按模块勾选，程序内支持 zip 插件包导入/删除（导入即加载免重启，删除自动重启完成），致谢信息随插件走
- **采集深度**：NVMe/SATA SMART 直读（健康度/通电时间/介质错误）；EDID 色度坐标估算 sRGB/DCI-P3 容积比；电池数据与 `powercfg /batteryreport` 同源；显存直读规避 WMI 4GB 溢出
- **烤机负载**：CPU 烤机基于 GIMPS 公开源码构建；GPU 烤机建立在 FurMark 负载的 C#/OpenTK 重实现之上；传感器监控经 PawnIO 内核驱动（官方签名安装器，页面一键静默安装）
- **工程化**：自包含发布（目标机无需装 .NET）、Inno Setup 中文安装向导、深/浅主题、全局异常落盘

## 技术栈

.NET 10 · WPF · LibreHardwareMonitorLib(MPL-2.0) · NAudio · CrystalDiskMark 引擎 · diskspd · OpenTK · prime95(GIMPS) · PawnIO · Inno Setup

## 运行与构建

- **使用**：到 [Releases](https://github.com/beifeng08/xiaobaixiang/releases) 下载安装包（约 116MB），Windows 10/11 x64，需管理员权限（SMART 与传感器读取需要）
- **构建**：需 .NET SDK 10，`dotnet build 验机软件.slnx`；开发细节见 `AGENTS.md`

## 依赖与致谢

采集、烤机与跑分能力建立在成熟开源方案之上：硬件采集基于 LibreHardwareMonitor（MPL-2.0）源码定制，CPU 烤机基于 GIMPS prime95 公开源码构建，硬盘跑分集成 CrystalDiskMark 引擎与 diskspd，GPU 烤机为 FurMark 负载的 C#/OpenTK 重实现，传感器访问经 PawnIO，影音模块用 NAudio。

**组件清单、上游地址、许可条款与随包落位只在 [`docs/third-party-licenses.md`](docs/third-party-licenses.md) 维护一份**，本文不再重列表格。改名映射与逐文件 diff 见 `docs/provenance.md`；程序内「插件管理 → 致谢」按插件列出声明的组件（删除插件即删对应条目）。

## 开源协议

本项目自身代码按 **MIT** 开源，全文见根目录 [`LICENSE`](LICENSE)。

第三方组件不在此授权范围内，各自沿用原许可，清单见 [`docs/third-party-licenses.md`](docs/third-party-licenses.md)；
安装包里每份第三方原文只出现在 `licenses\` 子目录，根目录只有本项目的 `LICENSE.txt`。

---

_开发手册见 `AGENTS.md`（工程细节）与 `AGENTS-AI.md`（AI 协作约定）。_
