param(
    [Parameter(Mandatory = $true)][string]$IsccPath,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$PackageVersion = "0.0.7-preview.1",
    [string]$PackageFileVersion = "0.0.7.0",
    [switch]$LocalAcceptanceOnly,
    [switch]$ValidateOnly
)
$ErrorActionPreference = "Stop"
$repo = Split-Path $PSScriptRoot -Parent
$output = [IO.Path]::GetFullPath($OutputDirectory)
$publish = Join-Path $repo "publish"
# 唯一来源地址：manifest 与 START-HERE 都引用它，换仓只需改这一处
$repoUrl = "https://github.com/beifeng08/xiaobaixiang"
$hostDirectory = Get-ChildItem $repo -Directory | Where-Object { Test-Path (Join-Path $_.FullName "app.manifest") } | Select-Object -First 1
$hostName = $hostDirectory.Name
# 第三方硬件库：目录已由 LibreHardwareMonitor-master 收拢改名为 white-toolkit（见 docs/provenance.md）
$libraryDirectory = Join-Path $repo "white-toolkit"
$dotnet = (Get-Command dotnet -ErrorAction Stop).Source
if (!(Test-Path $IsccPath -PathType Leaf)) { throw "ISCC.exe not found: $IsccPath" }
if (Test-Path $output) { throw "Output directory must be new to avoid stale files: $output" }
if ($output.StartsWith($publish.TrimEnd("\") + "\", [StringComparison]::OrdinalIgnoreCase) -or $output -eq $publish) { throw "Output must be outside publish" }
if ($PackageVersion -notmatch "^\d+\.\d+\.\d+-preview\.\d+$" -or $PackageFileVersion -notmatch "^\d+\.\d+\.\d+\.\d+$") { throw "Expected preview SemVer and four-part file version" }
$inventoryPath = Join-Path $PSScriptRoot "legal/inventory.json"
$inventory = Get-Content $inventoryPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($inventory.schemaVersion -ne 1 -or $inventory.targetPlatform -ne "win-x64" -or @($inventory.components).Count -ne 73) { throw "Unexpected inventory schema, target, or component count; review inventory before updating the baseline" }
$componentIds = @($inventory.components | ForEach-Object { $_.id })
if (@($componentIds | Sort-Object -Unique).Count -ne $componentIds.Count) { throw "Duplicate component IDs" }
foreach ($component in $inventory.components) {
    if (!$component.id -or !$component.version -or $component.status -notin @("ready", "blocked") -or !($component.licenseFiles -is [array]) -or ($component.status -eq "ready" -and @($component.licenseFiles).Count -eq 0)) { throw "Invalid component record: $($component.id)" }
}
function Source-Path([string]$Relative) {
    if (!$Relative -or [IO.Path]::IsPathRooted($Relative)) { throw "Expected relative source path: $Relative" }
    $path = [IO.Path]::GetFullPath((Join-Path $repo $Relative))
    if (!$path.StartsWith($repo.TrimEnd("\") + "\", [StringComparison]::OrdinalIgnoreCase)) { throw "Path outside source tree: $Relative" }
    if (!(Test-Path $path -PathType Leaf)) { throw "Missing source file: $Relative" }
    return $path
}
$licensePaths = @($inventory.components | ForEach-Object { $_.licenseFiles } | Sort-Object -Unique)
$licenseHashes = [ordered]@{}
$evidence = @{}
foreach ($entry in $inventory.licenseFileEvidence) {
    if (!$entry.path -or $entry.sha256 -notmatch "^[a-fA-F0-9]{64}$" -or $evidence.ContainsKey($entry.path)) { throw "Invalid or duplicate license evidence: $($entry.path)" }
    $evidence[$entry.path] = $entry.sha256
}
if (@(Compare-Object ($licensePaths | Sort-Object) ($evidence.Keys | Sort-Object)).Count -ne 0) { throw "License references and evidence must match exactly" }
foreach ($relative in $licensePaths) {
    $hash = (Get-FileHash (Source-Path $relative) -Algorithm SHA256).Hash
    if ($hash -ne $evidence[$relative]) { throw "Pinned license hash mismatch: $relative" }
    $licenseHashes[$relative] = $hash
}
$inventoryHash = (Get-FileHash $inventoryPath -Algorithm SHA256).Hash
$noticePath = Source-Path "docs/third-party-licenses.md"
$noticeHash = (Get-FileHash $noticePath -Algorithm SHA256).Hash
$blocked = @($inventory.components | Where-Object { $_.status -eq "blocked" })
if ($blocked.Count -gt 0 -and !$LocalAcceptanceOnly) { throw "Public distribution blocked by $($blocked.Count) unresolved components. LocalAcceptanceOnly is for this machine, not a redistribution permission." }
if ($ValidateOnly) {
    Write-Output "Validated $($inventory.components.Count) components, $($licensePaths.Count) pinned license files; $($blocked.Count) blocked components remain."
    return
}
$revision = (& git -C $repo rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw "Cannot resolve source revision" }
$dirty = @(& git -C $repo status --porcelain)
if ($dirty.Count -gt 0) { throw "Commit source changes before producing a traceable release" }
New-Item -ItemType Directory -Path $output | Out-Null
function Run-Dotnet([string[]]$Arguments) {
    & $dotnet @Arguments
    if ($LASTEXITCODE -ne 0) { throw "dotnet failed with exit code $LASTEXITCODE" }
}
# Existing upstream order is significant: first publish self-contained FurMark, then the host.
$payload = Join-Path $output "BibaInspection-$PackageVersion-win-x64"
Run-Dotnet @("restore", (Join-Path $repo "$hostName.slnx"), "--nologo")
Run-Dotnet @("publish", (Join-Path $repo "FurMark/FurMark.csproj"), "-p:PublishProfile=FolderProfile", "-p:GeneratePackageOnBuild=false", "--nologo")
Run-Dotnet @("publish", (Join-Path $hostDirectory.FullName "$hostName.csproj"), "-p:PublishProfile=FolderProfile", "-p:GeneratePackageOnBuild=false", "-p:Version=$PackageVersion", "-p:PublishDir=$payload/", "--nologo")
# Preserve upstream notices in their original payload locations as well as this indexed copy.
Copy-Item (Join-Path $repo "LICENSE") (Join-Path $payload "LICENSE.txt")
$licenses = Join-Path $payload "licenses"
New-Item -ItemType Directory $licenses | Out-Null
foreach ($relative in $licensePaths) {
    $source = Source-Path $relative
    $destination = Join-Path $licenses $relative
    New-Item -ItemType Directory (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item $source $destination
    $hash = (Get-FileHash $source -Algorithm SHA256).Hash
    if ($hash -ne $evidence[$relative] -or (Get-FileHash $destination -Algorithm SHA256).Hash -ne $evidence[$relative]) { throw "Pinned license copy mismatch: $relative" }
    $licenseHashes[$relative] = $hash
}
Copy-Item $inventoryPath (Join-Path $licenses "inventory.json")
Copy-Item $noticePath (Join-Path $licenses "READ-ME.md")
if ((Get-FileHash (Join-Path $licenses "inventory.json")).Hash -ne $inventoryHash -or (Get-FileHash (Join-Path $licenses "READ-ME.md")).Hash -ne $noticeHash) { throw "Inventory or notice copy mismatch" }
$licenseHashes["inventory.json"] = $inventoryHash
$licenseHashes["READ-ME.md"] = $noticeHash
$licenseHashes | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $licenses "SHA256.json") -Encoding UTF8
$licenseEntries = @($licenseHashes.Keys) + @("SHA256.json") | ForEach-Object {
    'Source: "{#PublishSource}\licenses\' + $_.Replace("/", "\") + '"; DestDir: "{app}\licenses\' + $(if ($_.Contains("/")) { $_.Substring(0, $_.LastIndexOf("/")).Replace("/", "\") } else { "" }) + '"; Components: main; Flags: ignoreversion'
}
$licenseEntries | Set-Content (Join-Path $licenses "license-files.iss") -Encoding UTF8
$required = @("$hostName.exe", "coreclr.dll", "hostfxr.dll", "YanJi.PluginSdk.dll", "Tools/furmark/FurMark.exe", "Tools/furmark/coreclr.dll", "Tools/xiaobaiprime/xiaobaiprime.exe", "Tools/pawnio/PawnIO_setup.exe", "plugins/disk/CdmHost/CdmHost.exe", "plugins/disk/CdmHost/CdmResource/diskspd/diskspd64.exe")
foreach ($relative in $required) { if (!(Test-Path (Join-Path $payload $relative) -PathType Leaf)) { throw "Missing package payload: $relative" } }
$pluginIds = @("config", "keyboard", "screen", "av", "stress", "disk")
$actualPluginIds = @(Get-ChildItem (Join-Path $payload "plugins") -Directory | Select-Object -ExpandProperty Name)
if (@(Compare-Object ($pluginIds | Sort-Object) ($actualPluginIds | Sort-Object)).Count -gt 0) { throw "Unexpected or missing plugin directory" }
foreach ($id in $pluginIds) {
    $plugin = Get-Content (Join-Path $payload "plugins/$id/plugin.json") -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($plugin.id -ne $id -or !(Test-Path (Join-Path $payload "plugins/$id/$($plugin.entryAssembly)"))) { throw "Invalid plugin: $id" }
}
foreach ($relative in @("$hostName.runtimeconfig.json", "Tools/furmark/FurMark.runtimeconfig.json")) {
    $runtime = (Get-Content (Join-Path $payload $relative) -Raw -Encoding UTF8 | ConvertFrom-Json).runtimeOptions
    if (!$runtime.includedFrameworks -or $runtime.framework -or $runtime.frameworks) { throw "Payload is not self-contained: $relative" }
    foreach ($framework in $runtime.includedFrameworks) {
        $componentId = "$($framework.name).Runtime.win-x64"
        $record = @($inventory.components | Where-Object { $_.id -eq $componentId })
        if ($record.Count -ne 1 -or $record[0].version -ne $framework.version) { throw "Runtime inventory mismatch: $componentId $($framework.version) in $relative" }
    }
}
$artifactChecks = @()
foreach ($component in $inventory.components) {
    foreach ($artifact in $component.artifacts) {
        if ($artifact.sha256 -notmatch "^[a-fA-F0-9]{64}$") { throw "Missing artifact hash: $($component.id)" }
        $source = Source-Path $artifact.path
        if ($artifact.path.StartsWith("$hostName/Tools/")) { $relative = $artifact.path.Substring($hostName.Length + 1) }
        elseif ($artifact.path.StartsWith("Plugins/DiskMark/CdmHost/")) { $relative = "plugins/disk/" + $artifact.path.Substring("Plugins/DiskMark/".Length) }
        else { throw "Unmapped fixed artifact: $($artifact.path)" }
        $destination = Join-Path $payload $relative
        if (!(Test-Path $destination -PathType Leaf) -or (Get-FileHash $source).Hash -ne $artifact.sha256 -or (Get-FileHash $destination).Hash -ne $artifact.sha256) { throw "Pinned artifact mismatch: $relative" }
        $artifactChecks += [ordered]@{ component = $component.id; path = $relative; sha256 = $artifact.sha256; status = $component.status }
    }
}
$artifactChecks | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $payload "fixed-artifact-evidence.json") -Encoding UTF8
[IO.File]::WriteAllText((Join-Path $payload "preview-mode"), "")
$payloadFiles = @(Get-ChildItem $payload -File -Recurse | Sort-Object FullName | ForEach-Object {
    [ordered]@{ path = $_.FullName.Substring($payload.Length + 1).Replace("\", "/"); bytes = $_.Length; sha256 = (Get-FileHash $_.FullName -Algorithm SHA256).Hash }
})
[ordered]@{ schemaVersion = 1; scope = "published build files and indexed notices; excludes this index, manifest and START-HERE generated afterwards"; files = $payloadFiles } | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $payload "payload-files.json") -Encoding UTF8
$payloadIndexHash = (Get-FileHash (Join-Path $payload "payload-files.json")).Hash
$manifest = [ordered]@{ version = $PackageVersion; createdAt = [DateTimeOffset]::Now.ToString("o"); source = $repoUrl; sourceRevision = $revision; sourceDirty = ($dirty.Count -gt 0); plugins = $pluginIds; platform = "win-x64"; runtime = "self-contained .NET 10"; batteryAcceptance = "user-owned"; distribution = $(if ($LocalAcceptanceOnly) { "local acceptance on owner machine only; public distribution blocked" } else { "preview" }); unresolvedComponents = @($blocked.id); bundledMusic = "none; synthesized tone and user-selected local audio"; sourceArchive = "BibaInspection-$PackageVersion-source.zip"; sourceArchiveScope = "committed build-input snapshot; export-ignore excludes historical backups, experimental copies, caches and old verification results; includes required third-party binary inputs and is not pure source or cleared for public redistribution"; inventorySha256 = $inventoryHash; noticeSha256 = $noticeHash; fixedArtifactCount = $artifactChecks.Count; payloadFileCount = $payloadFiles.Count; payloadIndexSha256 = $payloadIndexHash }
$manifest | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $payload "release-manifest.json") -Encoding UTF8
@("笔吧验机 · 验机辅助与离线记录 $PackageVersion / win-x64", "本机验收预览：完整解压后以管理员运行 验机软件.exe，不要只移动 EXE。", "基于 Bibapcs / BibaYanji 二次开发；采集与负载沿用各自上游，不是从零自研。", "操作顺序：已知信息登记 → 逐项检查/人工确认 → 导出 JSON + 离线网页。", "缺失值/未测试不是通过，报告不是自动认证或防篡改凭证。分享前检查设备标识与备注。", "可播放合成音或选择本地音乐；未捆绑 Out of Love 的任何录音。", "许可库存仍有 $($blocked.Count) 项来源/源码义务未闭环。仅在本机验收，不得将本包作为可公开发行版本。", "安装器驱动选项默认不勾选。卸载保留数据；预览设置独立，硬盘成绩仍共享。", "当前预览的有效音频播放未完成最终复验；音质与设备兼容性仍需人工检查。", "安装/卸载生命周期、满载烤机、磁盘写入负载、全新系统、驱动重装与电池边界不属于自动验收结论。", "Source: $repoUrl (see release-manifest.json and licenses/inventory.json)") | Set-Content (Join-Path $payload "START-HERE.txt") -Encoding UTF8
$sourceArchive = Join-Path $output "BibaInspection-$PackageVersion-source.zip"
& git -C $repo -c core.autocrlf=false -c core.eol=lf archive --format=zip "--output=$sourceArchive" HEAD
if ($LASTEXITCODE -ne 0) { throw "Source archive creation failed" }
& $IsccPath "/Qp" "/DBlockThreads=4" "/DPreview=1" "/DPackageVersion=$PackageVersion" "/DPackageFileVersion=$PackageFileVersion" "/DPublishSource=$payload" "/O$output" (Join-Path $PSScriptRoot "setup.iss")
if ($LASTEXITCODE -ne 0) { throw "Inno Setup compilation failed: $LASTEXITCODE" }
$zip = Join-Path $output "BibaInspection-$PackageVersion-win-x64-portable.zip"
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($payload, $zip, [IO.Compression.CompressionLevel]::Optimal, $true)
Get-ChildItem $output -File | Where-Object { $_.Extension -in @(".zip", ".exe") } | Get-FileHash -Algorithm SHA256 | ForEach-Object { "$($_.Hash)  $([IO.Path]::GetFileName($_.Path))" } | Set-Content (Join-Path $output "SHA256SUMS.txt") -Encoding UTF8
Get-ChildItem $output -File | Select-Object Name,Length
