# 第三方许可、通知与发行状态

根 [LICENSE](../LICENSE) 保留 Bibapcs 应用基础署名和本仓库新增/修改部分声明；**不覆盖第三方代码与素材**。本页是导航，不是唯一可保留声明的地方：源码头、上游附属通知、原件及二进制随附文件都应保留，不能为了“每组件只一份”而删除必要通知。

## 可机器读取的库存

`installer/legal/inventory.json` 列出组件版本、许可、已核实来源、许可文件、状态及限制。`ready` 仅表示所述材料已收集，不等于整包获准发行；`blocked` 表示来源、对应源码或完整通知仍有缺口。子组件 ready 不解除父二进制的 blocked。

本轮原文收集共 73 项，其中 20 项保留 blocked。最终随包状态应以其实际复制的库存为准，不能把预览改名当作授权闭环。

## 主要组件

| 组件 | 原件/说明 | 许可与边界 |
|---|---|---|
| LibreHardwareMonitor 衍生库 | `white-toolkit/LICENSE`、各源码通知 | MPL-2.0；个别文件保留自身 GNU 通知及作者，不概括成全树统一授权 |
| DiskInfoToolkit / BlackSharp.Core / RAMSPDToolkit-NDD | `installer/legal/*NOTICE.txt`、共用 MPL 原文 | MPL-2.0，不是 MIT；库存记录 NuGet 源提交和来源 |
| HidSharp | `installer/legal/HidSharp-2.6.4.LICENSE.txt` | Apache-2.0 |
| NAudio 系列 | `installer/legal/NAudio-2.2.1.LICENSE.txt` | MIT |
| OpenTK 及内含 OpenEXR Half | OpenTK LICENSE + THIRD_PARTIES 两份原文 | 主库 MIT，内含通知不省略 |
| GLFW | `OpenTK.redist.glfw-3.4.0.44.COPYING.txt` | 保留 GLFW 自身许可及版权 |
| StbImageSharp | LICENSE + NOTICE | 上游提供 MIT/公有领域选择；不将所有素材统称无版权 |
| FurMark-Python 衍生负载 | `FurMark/Assets/LICENSE.FurMark.txt` | StanislavPetrovV MIT，非官方同名工具整体授权 |
| PawnIO 安装器 / 嵌入模块 | 既有 COPYING、GPLv2 与新增 NOTICE | 两者独立核对；精确模块版本、源码与替换条件未闭环 |
| GIMPS Prime95 | `验机软件/Tools/xiaobaiprime/license.txt`、逐文件通知 | GIMPS EULA；不覆盖其所有依赖 |
| curl / GMP / hwloc / Boost / cJSON | `installer/legal/` 的对应原文 | 不能以 GIMPS 或 curl 主许可覆盖内含依赖；GMP 头与 DLL 版本不一致，保留阻断 |
| CrystalDiskMark / CdmHost | 官方源码头参考材料 | 现有成品无完整对应源/构建证据，参考通知不冒充完整授权 |
| diskspd | 官方 v2.0.20a 原文 | MIT；现有二进制精确发行对应仍待核验 |
| .NET / WindowsDesktop / System.* | 对应 LICENSE 与第三方通知 | 自包含运行时也属于分发内容，不能只列直接 NuGet 引用 |
| Windows SDK 投影 / Mono.Posix | 原链接所指向的许可参考材料 | 精确再分发依据、helper 版本/通知仍待核验 |
| Aga.Controls | `white-toolkit/Aga.Controls/license.txt`、GifDecoder 头 | BSD 与特定来源通知；当前宿主未分发其 GUI DLL |

## 打包与公开发行阻断

1. 打包不删除业务目录原始通知；另外把库存引用的原文复制到 `licenses/`，库存同时随包。
2. 每个引用必须真实存在，复制后哈希与原件一致；不能仅比较许可证固定数量。
3. 不捆绑未确认录音版本及再分发许可的《Out of Love》。支持用户选择本地音频，不上传、不复制到安装器。
4. 默认禁止用尚有 blocked 的库存生成**公开可分发**包。已有历史负载仅允许明确标注的本机验收构建，未上传 Release 或其他分发站；此模式不是法律授权或豁免。
5. 公开发行前需补齐：PawnIO 模块/安装器对应源码及替换条件，Prime95 与原生 DLL 完整构建来源/通知，CdmHost 精确源码与许可，Mono helper 及 SDK 投影分发依据。没有闭环前不将库存状态改为 ready。
6. MPL 修改源码应按实际发行版本供应，LGPL/GPL 组件还需满足其对应源码、替换/重链接等适用义务。仓库链接和单份许可文本不能自动替代这些义务。

构建记录、源码提交和 SHA256 只证明包的身份与完整性，**不证明所有授权已完成**。最终交付必须同时说明实际测试覆盖和未验证项。
