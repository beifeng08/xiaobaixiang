# white-toolkit 目录结构（按功能域重组）

重组时间：2026-10-23
状态：**隔离副本已完成，构建通过（0 错误）**，主工作树未改动。
副本路径：`F:/toolkit/_refactor2/white-toolkit`（`_refactor/` 为已作废的中间版本，可删）
重组前完整备份：`F:/toolkit/_backup/white-toolkit-pre-refactor-20261023.tar.gz`

---

## 1. 顶层：从"上游项目名"改为"系统角色"

| 重组前（上游仓库布局） | 重组后（功能角色） | 说明 |
|---|---|---|
| `XiaobaiHardwareMonitorLib/` | `src/core/` | 采集内核，多目标框架类库 |
| `XiaobaiHardwareMonitor.Windows.Forms/` | `src/app/` | WinForms 表现层，可执行程序 |
| `Aga.Controls/` | `src/third-party/treeview-adv/` | 第三方树控件，字节原样封存 |

程序集名（AssemblyName）**保持不变**，因为下游 `Plugins/ConfigCheck`、`Plugins/StressTest`、`验机软件.slnx` 按程序集名引用；改目录名不影响它们，改程序集名会。

## 2. 核心层：按功能域归类

```
src/core/
├── Contracts/      11  扁平接口层：IComputer / IHardware / ISensor / IVisitor / IControl /
│                       IParameter / IGroup / IElement / ISettings / ISensorLimits / IHardwareChanged
├── Model/          10  领域模型：Hardware / Sensor / Parameter / Control / CompositeSensor /
│                       Identifier / HardwareType / CoreType / SensorVisitor
├── Platform/        8  平台与系统设施：FirmwareTable / SMBios / OpCode / Mutexes /
│                       GroupAffinity / ThreadAffinity / D3DDisplayDevice / OperatingSystem
├── Devices/        62  设备驱动域：Cpu(10) Gpu(8) Memory(6) Storage(5) Network(2) Battery(3)
│   │                   Motherboard(6，不含传感描述符)
│   ├── SuperIo/            14  SuperIO 芯片驱动：Nct677X / IT87XX / F718XX / W836XX / LpcIO / Ipmi …
│   └── EmbeddedControllers/ 8  EC 通道：ChromeOS / Windows 两套实现
├── Sensors/         5  传感量纲域（**从 Motherboard 里拆出**）
│   ├── Temperature/     Temperature.cs、SpdThermalSensor.cs
│   ├── Voltage/         Voltage.cs（含 Ri/Rf/Vf 分压换算）
│   ├── Fan/             Fan.cs
│   └── Control/         ControlChannel.cs（原 Motherboard/Control.cs，改名避免与 Model.Control 冲突）
├── Controllers/    32  外置控制器：8 个厂商子目录，**命名空间合并为 1 个**
├── Power/          18  电源与功耗：Corsair / Msi / Monitors(含 WireView 协议)
└── Interop/        20  原生互操作：ADL / NVAPI / NVML / IGCL / FTD2xx / IPMI / PawnIo(12)
```

## 3. 表现层

```
src/app/
├── Program.cs          入口（命名空间 SysCore.Windows.Forms）
├── UI/
│   ├── Forms/          5 个窗体 + Designer + resx
│   ├── Tree/           树节点与树模型
│   ├── Gadgets/        桌面小工具、托盘
│   ├── Plot/           曲线绘制
│   ├── Themes/         4 套主题 + 自绘滚动条
│   └── (根)            管理器、单位、访问者等
├── Infrastructure/     原 Utilities：资源、HTTP、日志、持久化设置
├── Resources/          icon.ico、app.manifest、Web 报告页
└── TestScripts/
```

## 4. 命名空间收敛

| 指标 | 重组前 | 重组后 |
|---|---|---|
| 核心层命名空间 | 28 | **12** |
| 命名空间最大深度 | 5（`SysCore.Hardware.Motherboard.Lpc.EC`） | 3（`SysCore.Devices.SuperIo`） |
| 厂商级命名空间 | 10（8 控制器 + 2 电源） | 2（`SysCore.Controllers`、`SysCore.Power`） |

- `SysCore.Hardware.Controller.{8 个厂商}` → `SysCore.Controllers`
- `SysCore.Hardware.Psu.{Corsair,Msi}` + `Hardware.PowerMonitor` + `Interop.PowerMonitor` → `SysCore.Power`
- `SysCore.Hardware.Motherboard.Lpc` → `SysCore.Devices.SuperIo`
- `SysCore.Hardware.Motherboard.Lpc.EC` → `SysCore.Devices.EmbeddedControllers`（复数，避开与类型 `EmbeddedController` 同名）
- `SysCore.Hardware.{Cpu,Gpu,Memory,Storage,Network,Battery,Motherboard}` → `SysCore.Devices`
- `SysCore.Hardware`（30 个平铺文件）→ 拆为 `Contracts` / `Model` / `Platform`
- `SysCore.PawnIo` → `SysCore.Interop.PawnIo`；`SysCore.Software` → `SysCore.Platform`
- `SysCore.Windows.Forms.UI.Themes` → `...UI`；`...Utilities` → `...Infrastructure`

## 5. 为消除歧义而做的重命名

| 原 | 新 | 原因 |
|---|---|---|
| `Motherboard/Control.cs` → `Control` | `ControlChannel` | 与 `Model.Control`（传感器控制）同名 |
| `Psu/Corsair/UsbApi` | `CorsairUsbApi` | 与 MSI 的合并到同一命名空间后冲突 |
| `Psu/Msi/UsbApi` | `MsiUsbApi` | 同上 |
| `Psu/*/FirmwareInfo` | `CorsairFirmwareInfo` / `MsiFirmwareInfo` | 同上 |

## 6. 构建验证

```
dotnet build XiaobaiHardwareMonitor.sln -c Release -p:Platform=x64
→ 0 错误
→ 警告仅剩重组前既有的 NU1702 / MSB3245 / MSB3243 / WFO0003，无 CS0105 重复 using
```

格式核对：以 `git HEAD` 的 293 个源文件为基线，逐文件比对行数，**0 个文件偏差超过 12 行**；
`HttpServer.cs` 941→944、`SuperIOHardware.cs` 6142→6146，增量即新增的 `using` 行。
第三方 `treeview-adv` 94 个文件与工作树逐字节一致（仅 `Aga.Controls.csproj` 的 OutputPath 因目录变深而调整）。

## 7. 关于重组前那次批量改写造成的行尾问题

事实核对（以 `git HEAD`、17:29 备份、当前工作树三方比对）：

| 状态 | 1 个 CR | 2 个 CR（`\r\r\n`） | 3 个 CR（`\r\r\r\n`） |
|---|---|---|---|
| `git HEAD`（已提交） | 170 | 123 | 0 |
| 17:32 批量改写后 | 5 | 164 | 124 |

- **`\r\r\n` 不是那次改写首创**：`HEAD` 里已有 123 个文件是 2 个 CR，属于更早一次改名留下的、已提交的历史问题。
- **那次改写做的是每行多加 1 个 CR**：123 个文件从 2 CR 变 3 CR，另有 41 个从 1 CR 变 2 CR。
  典型成因是「读入的字符串里已含 `\r\n`，写回时又走了文本模式的换行翻译」，于是每个 `\n` 前面再补一个 `\r`。
- 字节证据：`SuperIOHardware.cs` 从 322913 字节涨到 328976，**+6063 字节 ≈ 6142 行**，即每行 1 字节。
  所以**行数没有变化**（6142 = 6142），不存在空行翻倍。
- 影响：编译不受影响，但 git diff 会逐行飘红，review 与按行比对全部失效。

**更正**：本文件早前版本称「131 个文件每行后多插一个空行、6142→11828 行」，该结论错误。
空行翻倍是我自己在归一 `\r\r\r\n` 时用两次 `str.replace` 造成的（把 `\r\r\r\n` 拆成了三个换行），
已通过对 `\r+\n` 一次性折叠修正，并以重建方式从头生成当前副本。

## 8. 合并回工作树前还需处理

1. 下游引用路径更新（本次未改，属合并动作）：
   - `Plugins/ConfigCheck/YanJi.Plugin.ConfigCheck.csproj:22`
   - `Plugins/StressTest/YanJi.Plugin.StressTest.csproj:21`
   - `验机软件.slnx:16`
   - `installer/build-release.ps1:41`（PawnIo 许可文件路径）
2. 合并会覆盖另一会话正在改的同一批文件，需先确认其停止或提交。
3. 是否顺手把 2 CR / 3 CR 的历史行尾统一为 `\r\n`，需单独决定（本次只在副本内归一，未改主树）。
