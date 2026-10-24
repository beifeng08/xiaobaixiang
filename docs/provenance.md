# 来源、继承关系与贡献边界

## 项目定位

本仓库是基于 Bibapcs / BibaYanji 的 Windows 验机辅助与离线报告二次开发。复用不等于原创贡献；改名、整理目录、编译既有源码不据此变成自研算法。根 MIT 许可恢复应用基础作者署名，第三方文件保留各自原通知。

## 应用层继承与本仓库增量

本地 BibaYanji 对照基线 `1d48216dfb42941e3c3845bf8ea4cd4e6fe02d13` 已有应用外壳、插件 SDK、加载/管理、多个检测页面与配置报告模型。它们不属于本次从零创作。`Plugins/ConfigCheck/HardwareReport.cs` 也不是本次新建的整体原创模型。

相对该继承基础，可核查的报告增强集中在：

| 类型 | 代码 | 实际作用 |
|---|---|---|
| 报告收集与存储 | `验机软件/Services/ReportService.cs` | 登记字段、人工模块状态、五类模块片段、片段异常，保存 JSON 与离线网页资源 |
| 自动导出状态控制 | `验机软件/Services/AutoReportState.cs` | 防重复导出、过期回调失效、回退后重新导出 |
| 登记录入 | `验机软件/Views/IntakeWindow.xaml(.cs)` | 设备备注、预期配置，由操作者输入 |
| 离线报告 | `验机软件/ReportWeb/` | 可离线加载的报告页、schema 与醒目标注的示例数据 |
| 接入修改 | `PluginSdk/Contracts.cs`、宿主窗口及五个模块页面 | `IReportSection.GetReportSection()` 与人工状态/导出接入，不把所在旧文件全算新增 |
| 验证 | `tests/ReportRegression/` 与网页回归 | 状态序列、缺失/异常片段、保存与网页边界的回归资产 |

以上是仓库层面的变更范围，不单凭文件存在认定某人的全部独立贡献；需要个人贡献认定时应结合提交与代码审查。屏幕坏点页面等继承能力不混入报告增量。

## LibreHardwareMonitor 改名副本

上游：https://github.com/LibreHardwareMonitor/LibreHardwareMonitor 。当前 `white-toolkit/` 中程序集/目录采用 `XiaobaiHardwareMonitor*`，部分命名空间与 UI 使用 `SysCore` / `OmniSensor`。它们仍是上游衍生副本，不改变来源和许可。

历史提交 `608de9e` 等曾删改文件头通知、将作者换为产品名，且 About 链接被清空。本文不再声称这些声明“从未改动”。本次从本仓库历史 `5aa2d7bdb6a3202175b00a28738f0111578ce24b` 对应路径逐文件恢复前导通知，保留当前业务正文、命名空间和目录；不是把整个文件回退到历史实现。

- `Identifier.cs`、`Computer.cs` 等恢复原作者和 MPL 通知。
- PSU 个别文件历史为 GNU Public License v2.0，按各自通知保留，不统一替换成 MPL。
- MSI/PowerMonitor 等多作者文件保留其特定作者；Aga.Controls 保留 BSD 与 GifDecoder 的来源说明。
- About 恢复 LibreHardwareMonitor 项目链接、MPL 链接和原贡献者展示，同时允许产品标题继续沿用当前名称。

旧 `docs/review/` 是当时的改名留痕，不是当前版本“仅改名、完全未改逻辑”的证明。未完成行为等价审查前，不对整库或所有历史变更作该保证。

## Prime95 与其他负载

`xiaobaiprime/` 来自 GIMPS / Mersenne Research, Inc.（George Woltman）；`prime95-build/` 为叠加构建工程。当前源码与历史二进制是否精确对应仍待闭环，不以资源版本号或目录名替代构建证明。恢复 Mersenne Research、MT19937 等逐文件原始通知；GIMPS EULA 不覆盖 curl、GMP、hwloc、Boost 等全部依赖。

`FurMark/` 已声明源自 StanislavPetrovV 的 FurMark-Python，是 C#/OpenTK 重实现，不宣称与官方同名工具运行行为完全等同。CrystalDiskMark/CdmHost、diskspd 与 PawnIO 的历史负载继续单列来源及未闭环事项。

## 构建和发行边界

根解决方案构建宿主、SDK、六插件及采集库；WinForms 上游 GUI 和 Prime95 独立工程不在同一构建成功结论内。恢复源码通知不等于已完成所有原生负载的重建、再分发或对应源码供应。

具体版本、许可原文、通知与公开发行阻断见 [第三方许可](third-party-licenses.md) 及 `installer/legal/inventory.json`。本机验收不代表公开发行获批，不使用免责声明替代授权条件。
