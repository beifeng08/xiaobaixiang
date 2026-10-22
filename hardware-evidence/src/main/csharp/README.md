# 独立 C# 调用方：demo-args-v1

> 历史说明：本文保留 2026-10-13 初次交付时的接口与验证状态，不代表当前 SDK 或构建情况。
> 当前构建、原接口兼容调用、结构化结果接口与验证入口见 [hardware-evidence README](../../../README.md) 和 [result-demo-v1](../../../contracts/result-demo-v1.md)。

`demo-args-v1` 的接口定义见本目录 `CONTRACT.md`。调用方使用 .NET 10 标准库，无第三方 NuGet 依赖，不包含 Java 实现。

## 历史验证记录

2026-10-13 的 `dotnet --list-sdks` 在 Git Bash 中返回 127（`dotnet: command not found`），仅说明当时的命令环境找不到 dotnet，不能据此判断整机未安装 SDK。

该次记录只有源码静态审阅，没有编译、运行、子进程替身测试或真实 Java 联调证据。详细命令、文件清单及 C1–C6 覆盖情况见 `validation.json`；后续验证以目录上层 README 为入口。

## 文件分工

| 文件 | 功能、背景与方法 |
|---|---|
| Caller.csproj | net10.0 控制台项目，启用可空检查与警告作为错误，不声明 PackageReference。 |
| Program.cs | 接收三个位置参数，调用流程，输出中文分类说明及内存结果 JSON，以 Caller 自身约定退出。 |
| CallerConfiguration.cs | 校验 Windows 完整绝对路径、启动器和入口文件类型；创建不复用的任务目录。 |
| CallerResult.cs | 保存身份、目录、分类、Java 原退出码及两路日志，区分 Caller 自身退出码。 |
| JavaCaller.cs | 逐项启动，双管道 UTF-8 异步读取，实际退出码分类，十秒自然等待及三秒收尾。 |

## 构建与运行命令

以下命令保留初次交付时的独立目录示例，未在该次记录中执行。需要 Windows 和可用的 .NET 10 SDK；Java 启动器、class 与输出目录必须替换为实际配置。示例路径和 Java 21 兼容性不属于该次已验证内容。

`dotnet run` 先构建再运行；显式指定 `--project`，避免依赖当前工作目录：

```powershell
dotnet run --project "C:/Users/Administrator/WorkBuddy/biba-csharp-contract-demo-20261013/Caller.csproj" --configuration Release --no-launch-profile -- "C:/Users/Administrator/WorkBuddy/biba-csharp-contract-demo-20261013/Java 21/bin/java.exe" "C:/Users/Administrator/WorkBuddy/biba-csharp-contract-demo-20261013/编译产物 class" "C:/Users/Administrator/WorkBuddy/biba-csharp-contract-demo-20261013/演示任务 runs"
```

已成功构建后，只运行、不再次构建：

```powershell
dotnet run --project "C:/Users/Administrator/WorkBuddy/biba-csharp-contract-demo-20261013/Caller.csproj" --configuration Release --no-build --no-launch-profile -- "C:/Users/Administrator/WorkBuddy/biba-csharp-contract-demo-20261013/Java 21/bin/java.exe" "C:/Users/Administrator/WorkBuddy/biba-csharp-contract-demo-20261013/编译产物 class" "C:/Users/Administrator/WorkBuddy/biba-csharp-contract-demo-20261013/演示任务 runs"
```

在 PowerShell 中，命令后可读取 `$LASTEXITCODE`。Release 产物位于项目目录下的 `bin/Release/net10.0/Caller.exe`；初次交付时尚未生成该产物。

Java 配置要求：启动器是已存在的 `java.exe`，能加载 Java 21 编译产物；class 根目录包含默认包入口 `DemoCommand.class`。调用方仅确认入口文件存在且不是目录，不能静态证明字节码、JVM版本、其他依赖可用。JVM启动和依赖错误由启动器退出码及日志表达。

## 调用与输出语义

调用方按以下顺序把八项文本直接加入 `ProcessStartInfo.ArgumentList`：

```text
-cp
<classDirectory>
DemoCommand
demo
--execution-id
<executionId>
--out
<taskDirectory>
```

路径项不额外包引号，不经 shell 拼接。Java 启动器处理前三项后，Java main 只会得到后面的五项。两路输出分别指定 UTF-8，并在等待进程前开始异步读取。结果中的 stdout/stderr 是各自管道的文本，JSON 中的换行转义是正常表示，不承诺跨管道的交错顺序。

每次先校验配置，再生成 `Guid.NewGuid().ToString("N")` 身份，创建 `runsRoot/run-<executionId>` 空目录，最后启动进程。已有同名文件或目录则拒绝，不复用、覆盖或清理。启动失败保留已经创建的身份与目录；创建前失败可以为 null。可创建缺失的 runsRoot，其已有父目录不清理。受控串行检查不具备跨进程原子独占或抗恶意目录置换能力。

控制台先打印分类说明，再打印以下字段组成的 JSON：

| 字段 | 语义 |
|---|---|
| executionId | 已创建任务的 32 位小写十六进制身份；创建前失败可为 null。 |
| taskDirectory | 实际创建的完整绝对目录；创建前失败可为 null。 |
| status | validated、request_invalid、java_failure、caller_failure 或 timeout。 |
| javaExitCode | 已自然观察到的 Java 原退出码；启动失败、超时为 null。 |
| stdout / stderr | 两路读取结果。读完的一路会保留；未完成或读取失败的一路返回空文本，并在 error 标注不完整。 |
| error | 调用方失败或超时收尾事实；正常结果通常为 null。 |

`demo-args-v1` 的 JSON 仅打印到控制台，不写 `result.json` 或 `result.tmp`，不定义后续业务 JSON 协议。初次交付未运行，因此不提供成功输出样例。

| 自然退出或调用状态 | status | javaExitCode | Caller 自身退出码 |
|---|---|---|---|
| Java 自然退出 0，日志读取成功 | validated | 0 | 0 |
| Java 自然退出 2，日志读取成功 | request_invalid | 2 | 2 |
| Java 自然退出其他非零值，日志读取成功 | java_failure | 原值 | 1 |
| 配置、创建、启动、等待、日志读取失败 | caller_failure | 仅已自然观察到的退出码可保留，否则 null | 1 |
| 十秒内没有自然结束 | timeout | null | 1 |

分类依赖退出码，不解析 `REQUEST_VALIDATED` 或中文解释行作为成败依据。即使替身打印成功标记但退出 2，仍应是 request_invalid。Java 退出 0 但日志收尾失败时归 caller_failure，已观察到的原退出码 0 仍保留。

成功只显示“本次参数校验通过”。没有硬件检测、WPF、真实宿主或网络通信，不生成业务报告。

## 超时与资源收尾

从进程启动后开始十秒自然等待。超时后只对本次持有的 Process 调用 `Kill(entireProcessTree: true)`，不按名称扫描其他进程。停止后最多等待三秒确认本次进程退出，两路日志共同最多再等待三秒；自然结束同样有三秒日志读取上限。

停止请求异常、确认等待超时、读取异常或日志不完整都会写入 error。超时分类保持 timeout，不借用停止后退出码，也不声称一定停止成功。已退出的本次进程无需再 Kill。即使确认本次进程已退出，也不能据此证明全部后代已退出。

读取任务及合并任务都有故障观察；超限时取消读取并关闭读端，避免无限等待，随后释放 Process 与取消源。不自动重试、不删除任务目录。十秒、三秒、三秒限定的是异步等待与日志收尾期限，不能据此声称同步文件系统操作、Process.Start、Kill 和 Dispose 一定让全流程精确在十六秒内返回。

## 历史验收清单（2026-10-13，全部未实跑）

| 契约 | 验收断言 | 当时的证据 |
|---|---|---|
| C1 | 连续两次身份/目录不同；子进程启动瞬间目录已存在、为空；目录持续保留。 | 静态审阅创建顺序及 Guid；运行未验证。 |
| C2 | 启动器、class、runsRoot 含中文空格，替身收到恰好八项且路径文本完整。 | 静态审阅 ArgumentList；运行未验证。 |
| C3 | 并发双管道输出超过管道缓冲仍不死锁；自然退出 0、2、其他非零分别分类；日志内容不左右分类。 | 静态审阅并行读取及映射；运行未验证。 |
| C4 | 缺失启动器/入口、相对路径、文件冒充 runsRoot 为 caller_failure；存在但不可执行的 java.exe 启动失败仍保留任务；与退出 2 区分。 | 静态审阅配置/启动异常路径；运行未验证。 |
| C5 | 替身超过十秒，保留身份/目录、javaExitCode=null；自身进程收尾有界，其他无关进程不被停止，无重试/删除。 | 静态审阅 Kill 范围及期限；运行未验证。 |
| C6 | 将子进程替身的输出、退出码及实跑时间记录为“替身测试”；真实 Java 联调另列证据。 | 已明确证据边界；未建立或运行替身，真实 Java 联调未验证。 |

替身测试须使用明确标注的独立 C# 子进程；配置预检查的入口占位文件不属于 Java 字节码，不能计为真实 Java 联调。初次交付时尚无这些测试文件。

等待与日志错误的结果对象由 `CONTRACT.md` 定义，控制台 JSON 只作本地展示。并发目录竞争、链接置换、无限日志及进程树权限限制均未在该次记录中验证；本文不改变契约行为。
