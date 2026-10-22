# CPUID 采集与解码探针

日期：2026-10-14。独立 exe，不依赖 `src/main/csharp` 的 Caller，也不修改或扩展 `demo-args-v1` 契约。

## 功能与边界

- 采集原始寄存器，写入 `cpuid.json`。临时文件关闭后提交最终名；最终名已存在则拒绝覆盖。输出包含原始寄存器、厂商/品牌解码结果和 `decodeRuleId`。
- 厂商/品牌解码由 C# 完成，采用已确定的候选 B；解码规则见下文。原始寄存器始终保留。
- CPU 名称规范化与规则核对属于 Java 侧职责；探针的 `decoded.value` 原样保留，不做规范化。
- 探针没有真实 Java 命令参数，`executionId/captureId` 为探针自建的 Guid N 占位身份；接入正式契约时须改为命令参数传入并精确一致。
- JSON 结构仍是候选方案，字段、校验与失败语义尚未冻结，以后续契约版本为准。

## 解码规则 cpuid-decode-b-v1（明确规则全文）

1. **厂商**：叶 `00000000`，按 EBX、EDX、ECX 顺序，各寄存器低字节先出，拼 12 字节。
2. **品牌**：叶 `80000002`、`80000003`、`80000004` 顺序，每叶按 EAX、EBX、ECX、EDX，各寄存器低字节先出，拼 48 字节；品牌叶未采集（扩展最大叶不足 `80000004`，无符号比较）时 brand=notProvided，来源仍可 captured、规则证据不足。
3. **收尾**：删末尾连续 `00`；剩余含 `00` 或字节 > `7F`（非 ASCII）→ invalid（记 warning 诊断，原寄存器保留）；空串或全 ASCII 空白（仅 U+0009–U+000D、U+0020）→ notProvided；否则 present，value 为原样字符串（不 trim、不规范化）。
4. **来源失败**（notSupported/failed）：vendor/brand 均 unavailable、值为 null。
5. 改规则必须升 id（v2…），不静默改语义。

## 用法

```powershell
dotnet build CpuidProbe.csproj -c Release
& "bin/Release/net10.0/CpuidProbe.exe" "<输出目录绝对路径>"
```

- 输出目录不存在时由探针创建；输出写入 `<目录>/cpuid.json`。
- 退出码：0=已采集并提交；1=采集或落盘失败（含最终名已占用）；2=参数/目录不合法。

## 采集内容

- 固定叶：`00000000`、`80000000`（subleaf 均 `00000000`）；扩展最大叶 ≥ `80000004`（无符号比较）时加采 `80000002/3/4`。
- 每条寄存器六键 leaf/subleaf/eax/ebx/ecx/edx，8 位小写 hex、不带 0x，保 32 位位模式。
- 平台不支持或采集异常 → status=notSupported/failed，registers 为空且至少一条诊断，不伪造全零寄存器。

## 证据分级

本机探针采集和候选 B 解码已实跑，JSON 结构尚未冻结。此结果不代表正式契约验收或真实 Java 联调通过。
