# 来源与许可边界

本仓库包含两份上游源码的改名衍生副本。两者都**只改标识符与目录归属，不改算法与逻辑**；
原文凭据一律保留在各自目录内，供核对与合规审计。

## XiaobaiHardwareMonitor（源自 LibreHardwareMonitor）

| 项 | 值 |
|---|---|
| 上游 | https://github.com/LibreHardwareMonitor/LibreHardwareMonitor |
| 许可 | MPL-2.0，原文见 `white-toolkit/LICENSE` |
| 上游说明 | `white-toolkit/README.upstream.md`（原样保留，未改一字） |
| 上游 CI 与社区文件 | `white-toolkit/.github/`（原样保留；GitHub 只读仓库根 `.github/`，此副本不触发任何流程） |
| 改名映射 | `docs/review/refactor-mapping.md` |
| 全量 diff | `docs/review/refactor-full.diff` |

改动范围：命名空间 / 程序集名 / 项目名 `LibreHardwareMonitor*` → `XiaobaiHardwareMonitor*`，
WMI 命名空间 `root\LibreHardwareMonitor` → `root\XiaobaiHardwareMonitor`。

**未改动**（MPL-2.0 与来源透明要求）：`AboutBox` 中的原始版权声明与上游链接、
csproj 的 `PackageLicense` 指向、上游 CI 的 `github.repository` 发布门禁。

## XiaobaiPrime（源自 GIMPS Prime95）

| 项 | 值 |
|---|---|
| 上游 | GIMPS / Mersenne Research, Inc.（George Woltman） |
| 许可 | GIMPS 最终用户许可，原文见 `prime95/license.txt` |
| 上游说明 | `prime95/README.upstream.md`（原样保留） |
| 叠加构建工程 | `prime95-build/xiaobaiprime.yanji.vcxproj`（依赖指向 `prime95-build/deps/`） |

改动范围：MFC 工程名、类名（`CPrime95App` / `CPrime95Doc` / `CPrime95View`）、
可执行文件名与资源引用 `prime95` → `xiaobaiprime`。

**未改动**：计算内核、通信协议与全部算法实现；Mersenne Research, Inc. 的版权与 GIMPS 署名。
程序内仍以「XiaobaiPrime（基于 GIMPS Prime95, George Woltman）」明示出处。

## 两者的构建边界

- `white-toolkit/XiaobaiHardwareMonitor.sln` 可在本目录独立构建（.NET SDK）。
- `prime95/xiaobaiprime.sln` 需 MSVC + MFC；本仓库当前环境未验证其编译，只做了静态一致性核对。
- 根 `验机软件.slnx` 只引用 `white-toolkit/XiaobaiHardwareMonitorLib`，不引入上述两个独立解决方案。
