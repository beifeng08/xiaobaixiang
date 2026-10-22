// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;
using System.Text.Encodings.Web;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Windows.Controls;
using YanJi.PluginSdk;

namespace YanJi.Services;

/// <summary>验机前录入的已知信息（订单/卖家描述等）；Key 与报告网页的对照映射一致。</summary>
public record ExpectedField(string Key, string Label, string Value);

/// <summary>收集模块状态与 IReportSection 片段，生成 biba-inspection-summary/1 JSON。
/// 保存时为每份报告复制网页并附带 report-data.js，以支持 file:// 离线读取。</summary>
public static class ReportService
{
    public const string Schema = "biba-inspection-summary/1";

    static readonly JsonSerializerOptions Json = new()
    {
        WriteIndented = true,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        Encoder = JavaScriptEncoder.UnsafeRelaxedJsonEscaping,
        DefaultIgnoreCondition = JsonIgnoreCondition.Never,
    };

    public static string ReportsDirectory => Path.Combine(Settings.DataDirectory, "reports");
    static string WebSource => Path.Combine(AppContext.BaseDirectory, "ReportWeb");
    static readonly string[] WebFiles = ["index.html", "styles.css", "app.js", "sample-data.js", "inspection-summary.schema.json"];

    public static string Build(IReadOnlyList<PluginInfo> modules, Func<string, bool> isDone,
        IReadOnlyDictionary<string, UserControl> pages, DateTime? expectedAt,
        IReadOnlyList<ExpectedField> expected, string? deviceLabel)
    {
        var sectionErrors = new Dictionary<string, string>();
        object? GetSection(string id)
        {
            try
            {
                return pages.GetValueOrDefault(id) is IReportSection section
                    ? JsonSerializer.SerializeToElement(section.GetReportSection(), Json) : null;
            }
            catch (Exception exception)
            {
                sectionErrors[id] = exception.Message;
                return null;
            }
        }

        bool IsPassable(PluginInfo module) => !module.Terminal && pages[module.Id] is IModulePage;

        var payload = new
        {
            schema = Schema,
            exportedAt = DateTimeOffset.Now.ToString("yyyy-MM-ddTHH:mm:sszzz"),
            source = new { appVersion = AppVersion() },
            device = new { label = string.IsNullOrWhiteSpace(deviceLabel) ? null : deviceLabel.Trim() },
            modules = modules.Where(module => pages.ContainsKey(module.Id)).Select(module => new
            {
                id = module.Id, name = module.Name, version = module.Version, order = module.Order, terminal = module.Terminal,
                passable = IsPassable(module),
                done = IsPassable(module) && isDone(module.Id),
            }),
            config = GetSection("config"),
            keyboard = GetSection("keyboard"),
            av = GetSection("av"),
            stress = GetSection("stress"),
            disk = GetSection("disk"),
            sectionErrors,
            expected = expected.Count == 0 ? null : new
            {
                enteredAt = expectedAt?.ToString("yyyy-MM-ddTHH:mm:sszzz"),
                fields = expected.Select(field => new { key = field.Key, label = field.Label, value = field.Value }),
            },
            remarks = expected.FirstOrDefault(field => field.Key == "notes")?.Value is { Length: > 0 } notes ? notes : null,
        };
        return JsonSerializer.Serialize(payload, Json);
    }

    /// <summary>落盘 JSON 并刷新本地网页副本；返回 (json 路径, 网页入口路径或 null)。</summary>
    public static (string JsonPath, string? HtmlPath) Save(string json, string? reportsDirectory = null)
    {
        string directory = reportsDirectory ?? ReportsDirectory;
        Directory.CreateDirectory(directory);
        string reportName = $"inspection-{DateTime.Now:yyyyMMdd-HHmmss-fff}-{Guid.NewGuid():N}";
        string jsonPath = Path.Combine(directory, reportName + ".json");
        File.WriteAllText(jsonPath, json, new UTF8Encoding(false));

        if (WebFiles.Any(file => !File.Exists(Path.Combine(WebSource, file)))) return (jsonPath, null);
        string webTarget = Path.Combine(directory, reportName);
        Directory.CreateDirectory(webTarget);
        foreach (string file in WebFiles)
            File.Copy(Path.Combine(WebSource, file), Path.Combine(webTarget, file));
        string label = JsonSerializer.Serialize(Path.GetFileName(jsonPath));
        File.WriteAllText(Path.Combine(webTarget, "report-data.js"),
            $"window.BIBA_REPORT_LABEL = {label};\nwindow.BIBA_REPORT = {json};\n", new UTF8Encoding(false));
        return (jsonPath, Path.Combine(webTarget, "index.html"));
    }

    public static void Open(string path) =>
        Process.Start(new ProcessStartInfo { FileName = path, UseShellExecute = true });

    static string? AppVersion() =>
        typeof(ReportService).Assembly.GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion;
}
