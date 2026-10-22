# LibreHardwareMonitor -> XiaobaiHardwareMonitor 映射表

## 一、文件与目录重命名

| 序号 | 旧路径 | 新路径 | 类型 |
|---|---|---|---|
| 1 | `LibreHardwareMonitor.sln` | `XiaobaiHardwareMonitor.sln` | 解决方案 |
| 2 | `LibreHardwareMonitorLib/` | `XiaobaiHardwareMonitorLib/` | 项目目录 |
| 3 | `LibreHardwareMonitor.Windows.Forms/` | `XiaobaiHardwareMonitor.Windows.Forms/` | 项目目录 |
| 4 | `LibreHardwareMonitorLib/LibreHardwareMonitorLib.csproj` | `XiaobaiHardwareMonitorLib/XiaobaiHardwareMonitorLib.csproj` | 项目文件 |
| 5 | `LibreHardwareMonitor.Windows.Forms/LibreHardwareMonitor.Windows.Forms.csproj` | `XiaobaiHardwareMonitor.Windows.Forms/XiaobaiHardwareMonitor.Windows.Forms.csproj` | 项目文件 |

## 二、标识符替换

| 序号 | 旧值 | 新值 | 影响范围 |
|---|---|---|---|
| 1 | `LibreHardwareMonitor.Windows.Forms` | `XiaobaiHardwareMonitor.Windows.Forms` | 命名空间、程序集名、StartupObject、CI 路径 |
| 2 | `LibreHardwareMonitorLib` | `XiaobaiHardwareMonitorLib` | 命名空间、程序集名、NuGet 包 ID、DLL 文件名 |
| 3 | `LibreHardwareMonitor` | `XiaobaiHardwareMonitor` | 根命名空间、程序集名、CI 名称 |
| 4 | `Libre Hardware Monitor` | `Xiaobai Hardware Monitor` | AssemblyTitle、README |
| 5 | `root\LibreHardwareMonitor` | `root\XiaobaiHardwareMonitor` | WMI 命名空间（basicwmi.py） |
| 6 | `libre open hardware monitor` | `xiaobai open hardware monitor` | NuGet PackageTags |

## 三、刻意保留项（合规 / 来源透明 / 逻辑不变）

| 序号 | 保留内容 | 位置 | 保留理由 |
|---|---|---|---|
| 1 | `PackageLicense` 指向原仓库 Licenses/LICENSE | `XiaobaiHardwareMonitorLib.csproj` | MPL-2.0 许可证合规，必须可追溯 |
| 2 | `Copyright © LibreHardwareMonitor and Contributors` | `UI/AboutBox.Designer.cs:67` | 原始版权声明，MPL-2.0 要求保留 |
| 3 | `https://github.com/LibreHardwareMonitor/LibreHardwareMonitor` | `UI/AboutBox.cs:21`、README | 来源透明与上游出处 |
| 4 | `github.repository == 'LibreHardwareMonitor/LibreHardwareMonitor'` | `.github/workflows/master.yml:103` | NuGet 发布条件，逻辑不变 |
| 5 | `librehardwaremonitor.com` 钓鱼站警告 | `README.md` | 事实性安全提示，避免用户误访 |
| 6 | README 徽章、贡献者图、nightly 链接 | `README.md` | 上游来源与构建状态引用 |
| 7 | `LICENSE` 文件 | 根目录 | 许可证原文，禁止改写 |

## 四、未改动项

| 项 | 说明 |
|---|---|
| `Aga.Controls/` | 第三方控件库，源码中无 libre 引用 |
| `bin/`、`obj/` | 构建产物，已被 `.gitignore` 忽略 |
| 所有方法体、分支、循环、异常处理 | 仅做字符串替换，逻辑零改动 |
| 公共 API 成员签名 | 类型名、方法名、参数名均未变，仅命名空间前缀变化 |
