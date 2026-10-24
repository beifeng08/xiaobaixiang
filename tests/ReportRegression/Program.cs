// Copyright (c) 2026 OmniSensor

using System.Collections;
using System.IO;
using System.Reflection;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Windows.Controls;
using YanJi.PluginSdk;
using YanJi.Services;

internal static class Program
{
    static readonly List<string> Observations = [];
    static int _passed;

    [STAThread]
    static void Main(string[] args)
    {
        if (args is ["--compare", var baselineDirectory, var currentDirectory])
        {
            foreach (string file in new[] { "behavior.json", "public-api.txt" })
            {
                Check(File.ReadAllBytes(Path.Combine(baselineDirectory, file)).SequenceEqual(File.ReadAllBytes(Path.Combine(currentDirectory, file))));
                Console.WriteLine($"IDENTICAL {file}");
            }
            return;
        }

        Run("automatic export sequences", TestAutoReport);
        Run("sections, module flags and callback order", TestBuild);
        Run("empty, null and exceptional input", TestBuildEdges);
        Run("save with complete and missing web assets", TestSave);
        Directory.CreateDirectory(args[0]);
        File.WriteAllText(Path.Combine(args[0], "behavior.json"), JsonSerializer.Serialize(Observations));
        var api = typeof(ReportService).Assembly.GetExportedTypes().OrderBy(type => type.FullName)
            .SelectMany(type => new[] { type.FullName! }.Concat(type.GetMembers(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly)
                .Select(member => member + (member is MethodBase method ? " (" + string.Join(",", method.GetParameters().Select(parameter => parameter.Name)) + ")" : "")).Order()));
        File.WriteAllLines(Path.Combine(args[0], "public-api.txt"), api);
        Console.WriteLine($"PASS {_passed}/4 groups; behavior observations={Observations.Count}");
    }

    static void Run(string name, Action action) { action(); _passed++; Console.WriteLine($"PASS {name}"); }
    static void Check(bool condition) { if (!condition) throw new Exception("Regression assertion failed"); }
    static void Throws<T>(Action action) where T : Exception
    {
        try { action(); } catch (T) { Observations.Add(typeof(T).Name); return; }
        throw new Exception($"Expected {typeof(T).Name}");
    }

    static void TestAutoReport()
    {
        var scenario = new AutoReportState();
        Check(scenario.Request(false) is null);
        int first = scenario.Request(true)!.Value;
        Check(scenario.Request(true) is null);
        Check(scenario.Request(false) is null && !scenario.IsPending(first));
        int second = scenario.Request(true)!.Value;
        Check(second > first);
        scenario.Finish(first, true);
        Check(scenario.IsPending(second));
        scenario.Finish(second, false);
        int retry = scenario.Request(true)!.Value;
        scenario.Finish(second, false);
        Check(scenario.IsPending(retry));
        scenario.Finish(retry, true);
        Check(scenario.Request(true) is null);
        scenario.Finish(first, false);
        Check(scenario.Request(true) is null);
        Observations.Add($"state-scenario:{first}:{second}:{retry}");

        var actual = new AutoReportState();
        bool exported = false;
        int generation = 0;
        int? pending = null;
        var random = new Random(20261022);
        int? Request(bool allDone)
        {
            if (!allDone) { exported = false; pending = null; return null; }
            if (exported || pending.HasValue) return null;
            pending = ++generation;
            return pending;
        }
        for (int index = 0; index < 10000; index++)
        {
            int operation = random.Next(4);
            int candidate = random.Next(Math.Max(1, generation + 2));
            if (operation < 2) Check(actual.Request(operation == 1) == Request(operation == 1));
            else if (operation == 2)
            {
                bool succeeded = random.Next(2) == 1;
                actual.Finish(candidate, succeeded);
                if (pending == candidate) { pending = null; exported = succeeded; }
            }
            else Check(actual.IsPending(candidate) == (pending == candidate));
            Check(actual.IsPending(generation) == (pending == generation));
            Observations.Add($"state-step:{index}:{operation}:{candidate}:{pending}:{exported}:{actual.IsPending(candidate)}");
        }
        Observations.Add($"state:10000:{generation}:{pending}:{exported}");
    }

    static void TestBuild()
    {
        var pages = new TracedPages();
        pages.ValuesById.Add("config", new SectionPage(() => { pages.Trace.Add("section:config"); return new { Cpu = "CPU", Missing = (string?)null }; }));
        pages.ValuesById.Add("keyboard", new SectionPage(() => { pages.Trace.Add("section:keyboard"); throw new InvalidOperationException("section failed"); }));
        pages.ValuesById.Add("plain", new UserControl());
        pages.ValuesById.Add("terminal", new SectionPage(() => null));
        PluginInfo[] modules = [new("config", "配置", "1", 10, false), new("keyboard", "键盘", "1", 20, false),
            new("plain", "普通", "1", 30, false), new("terminal", "终点", "1", 40, true), new("absent", "缺失", "1", 50, false), new("config", "重复", "1", 60, false)];
        string json = ReportService.Build(modules, id => { pages.Trace.Add("done:" + id); return id == "config"; }, pages,
            new DateTime(2026, 10, 21, 12, 0, 0), [new("cpu", "处理器", " CPU "), new("notes", "备注", "保留原文")], "  设备 A  ");
        var root = JsonNode.Parse(json)!;
        Check(root["device"]!["label"]!.GetValue<string>() == "设备 A");
        Check(root["remarks"]!.GetValue<string>() == "保留原文");
        Check(root["keyboard"] is null && root["sectionErrors"]!["keyboard"]!.GetValue<string>() == "section failed");
        Check(root["config"]!["missing"] is null && root["modules"]!.AsArray().Count == 5);
        Check(root["modules"]![0]!["done"]!.GetValue<bool>() && !root["modules"]![2]!["passable"]!.GetValue<bool>());
        Check(pages.Trace.Count(entry => entry == "get:config") == 4);
        Check(pages.Trace.Count(entry => entry == "done:config") == 2);
        Check(pages.Trace.IndexOf("section:config") < pages.Trace.IndexOf("done:config"));
        root["exportedAt"] = "<clock>";
        Observations.Add(root.ToJsonString());
        Observations.Add(string.Join("|", pages.Trace));
    }

    static void TestBuildEdges()
    {
        var pages = new Dictionary<string, UserControl> { ["config"] = new SectionPage(() => null) };
        var root = JsonNode.Parse(ReportService.Build([], _ => throw new Exception("unreachable"), pages, null, [], " \t"))!;
        Check(root["expected"] is null && root["remarks"] is null && root["device"]!["label"] is null);
        Check(root["config"] is null && root["sectionErrors"]!.AsObject().Count == 0);
        root["exportedAt"] = "<clock>";
        Observations.Add(root.ToJsonString());
        PluginInfo[] modules = [new("config", "配置", "1", 0, false)];
        Throws<InvalidOperationException>(() => ReportService.Build(modules, _ => throw new InvalidOperationException(), pages, null, [], null));
        Throws<ArgumentNullException>(() => ReportService.Build(null!, _ => true, pages, null, [], null));
        Throws<NullReferenceException>(() => ReportService.Build([], _ => true, pages, null, null!, null));
        pages["config"] = new SectionPage(() => new Action(() => { }));
        root = JsonNode.Parse(ReportService.Build([], _ => true, pages, null, [], null))!;
        Check(root["config"] is null && root["sectionErrors"]!.AsObject().ContainsKey("config"));
        Observations.Add("serialization failure recorded");
    }

    static void TestSave()
    {
        string source = Path.Combine(AppContext.BaseDirectory, "ReportWeb");
        string output = Path.Combine(AppContext.BaseDirectory, "save-fixtures");
        Directory.CreateDirectory(source);
        Directory.CreateDirectory(output);
        string[] assets = ["index.html", "styles.css", "app.js", "sample-data.js", "inspection-summary.schema.json"];
        foreach (string asset in assets) File.WriteAllText(Path.Combine(source, asset), "fixture:" + asset);
        const string json = "{\"schema\":\"biba-inspection-summary/1\",\"note\":\"中文\"}";
        var saved = ReportService.Save(json, output);
        Check(File.ReadAllText(saved.JsonPath) == json && saved.HtmlPath is not null);
        Check(!File.ReadAllBytes(saved.JsonPath).Take(3).SequenceEqual(new byte[] { 0xEF, 0xBB, 0xBF }));
        string directory = Path.GetDirectoryName(saved.HtmlPath!)!;
        foreach (string asset in assets) Check(File.ReadAllText(Path.Combine(directory, asset)) == "fixture:" + asset);
        Check(File.ReadAllText(Path.Combine(directory, "report-data.js")) ==
            $"window.BIBA_REPORT_LABEL = {JsonSerializer.Serialize(Path.GetFileName(saved.JsonPath))};\nwindow.BIBA_REPORT = {json};\n");
        var second = ReportService.Save(json, output);
        Check(second.JsonPath != saved.JsonPath && second.HtmlPath != saved.HtmlPath);
        string missing = Path.Combine(source, assets[0]);
        File.Move(missing, missing + ".held");
        try { var fallback = ReportService.Save(json, output); Check(fallback.HtmlPath is null && File.ReadAllText(fallback.JsonPath) == json); }
        finally { File.Move(missing + ".held", missing); }
        Throws<IOException>(() => ReportService.Save(json, saved.JsonPath));
        Observations.Add("save:UTF8-no-BOM:5-assets:script:unique-paths:missing-fallback");
    }

    sealed class SectionPage(Func<object?> section) : UserControl, IModulePage, IReportSection
    {
        event Action<bool>? IModulePage.PassChanged { add { } remove { } }
        public object? GetReportSection() => section();
    }

    sealed class TracedPages : IReadOnlyDictionary<string, UserControl>
    {
        public Dictionary<string, UserControl> ValuesById { get; } = [];
        public List<string> Trace { get; } = [];
        public UserControl this[string key] { get { Trace.Add("get:" + key); return ValuesById[key]; } }
        public IEnumerable<string> Keys => ValuesById.Keys;
        public IEnumerable<UserControl> Values => ValuesById.Values;
        public int Count => ValuesById.Count;
        public bool ContainsKey(string key) { Trace.Add("contains:" + key); return ValuesById.ContainsKey(key); }
        public bool TryGetValue(string key, out UserControl value) { Trace.Add("try:" + key); return ValuesById.TryGetValue(key, out value!); }
        public IEnumerator<KeyValuePair<string, UserControl>> GetEnumerator() => ValuesById.GetEnumerator();
        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
    }
}
