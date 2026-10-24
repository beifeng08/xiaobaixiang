using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Management;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Automation.Peers;
using System.Windows.Automation.Provider;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using YanJi;
using YanJi.Services;
using YanJi.Views;
using YanJi.PluginSdk;
using YanJi.Plugin.StressTest;

// Supplementary current-entry acceptance. The legacy VerifyRelease assertions stay unchanged.
// Automation peers and routed button events drive real controls; reflection observes state except
// the labelled disappeared-file/stale-callback checks. Never starts stress/disk workloads.
internal static class VerifyDelivery
{
    static readonly List<object> Checks = [];
    static MainWindow Host = null!;
    static string Output = "";
    static int ExitCode;
    static string? Failure;
    static readonly Stopwatch Clock = Stopwatch.StartNew();
    static readonly string Label = "delivery-" + Guid.NewGuid().ToString("N")[..12];
    static readonly BindingFlags Flags = BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public;
    static int OutputDeviceIndex;
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);

    static object? Field(object value, string name) => value.GetType().GetField(name, Flags)!.GetValue(value);
    static object? Call(object value, string name, params object[] args) => value.GetType().GetMethod(name, Flags)!.Invoke(value, args);
    static T Control<T>(FrameworkElement owner, string name) where T : FrameworkElement => (T)owner.FindName(name);
    static Dictionary<string, UserControl> Pages => (Dictionary<string, UserControl>)Field(Host, "_pages")!;
    static IEnumerable<DependencyObject> Tree(DependencyObject root)
    {
        yield return root;
        for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
            foreach (var child in Tree(VisualTreeHelper.GetChild(root, i))) yield return child;
    }
    static void Check(string name, bool pass, object? detail = null)
    {
        Checks.Add(new { name, passed = pass, detail, elapsedSeconds = Clock.Elapsed.TotalSeconds });
        Console.WriteLine($"{(pass ? "PASS" : "FAIL")} {name}: {JsonSerializer.Serialize(detail)}");
        if (!pass) throw new InvalidOperationException(name);
    }
    static async Task Until(Func<bool> condition, int seconds = 120)
    {
        await Dispatcher.Yield(DispatcherPriority.ContextIdle);
        var watch = Stopwatch.StartNew();
        while (!condition())
        {
            if (watch.Elapsed.TotalSeconds > seconds) throw new TimeoutException("Acceptance condition timed out");
            await Task.Delay(100);
        }
    }
    static async Task Click(Button button)
    {
        Check("real enabled button: " + (button.Name.Length > 0 ? button.Name : button.Content?.ToString()), button.IsEnabled && button.IsVisible);
        var peer = UIElementAutomationPeer.CreatePeerForElement(button) ?? new ButtonAutomationPeer(button);
        ((IInvokeProvider)peer.GetPattern(PatternInterface.Invoke)!).Invoke();
        await Dispatcher.Yield(DispatcherPriority.ContextIdle);
    }
    static Button NamedButton(FrameworkElement owner, string name) => Tree(owner).OfType<Button>().Single(b =>
        AutomationProperties.GetName(b) == name || b.Content as string == name);
    static async Task Nav(string id)
    {
        var list = Control<ListBox>(Host, "NavList");
        var item = list.Items.Cast<ListBoxItem>().Single(x => Equals(x.Tag, id));
        new ListBoxItemAutomationPeer(item, new ListBoxAutomationPeer(list)).GetPattern(PatternInterface.SelectionItem).As<ISelectionItemProvider>().Select();
        await Until(() => Pages[id].IsLoaded);
        Check("navigation real content: " + id, ReferenceEquals(Control<ContentControl>(Host, "ContentHost").Content, Pages[id]));
    }
    static T As<T>(this object value) => (T)value;
    static IntakeWindow? Intake => Application.Current.Windows.OfType<IntakeWindow>().FirstOrDefault(w => w.IsVisible);
    static async Task FillIntake(bool save)
    {
        await Until(() => Intake != null, 20);
        var dialog = Intake!;
        dialog.UpdateLayout();
        var edits = Tree(dialog).OfType<TextBox>().ToDictionary(AutomationProperties.GetName);
        Check("nine actual intake text boxes", edits.Count == 9, edits.Count);
        edits["设备编号"].SetCurrentValue(TextBox.TextProperty, save ? "  " + Label + "  " : "cancelled-device");
        edits["处理器"].SetCurrentValue(TextBox.TextProperty, "预期CPU（验收输入，非采集结果）");
        edits["其它备注"].SetCurrentValue(TextBox.TextProperty, "交付验收；未判定硬件通过");
        await Click(NamedButton(dialog, save ? "开始验机" : "跳过"));
        await Until(() => Intake == null, 10);
    }
    static async Task FileDialog(string? path)
    {
        int pid = Environment.ProcessId;
        await Task.Run(async () =>
        {
            AutomationElement? dialog = null;
            var timer = Stopwatch.StartNew();
            while (timer.Elapsed.TotalSeconds < 15)
            {
                var windows = AutomationElement.RootElement.FindAll(TreeScope.Children,
                    new PropertyCondition(AutomationElement.ProcessIdProperty, pid));
                const string title = "选择本地试音文件（不会上传或复制）";
                dialog = windows.Cast<AutomationElement>().FirstOrDefault(w => w.Current.Name == title);
                if (dialog == null)
                    foreach (var owner in windows.Cast<AutomationElement>())
                    {
                        dialog = owner.FindFirst(TreeScope.Descendants, new AndCondition(
                            new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.Window),
                            new PropertyCondition(AutomationElement.NameProperty, title)));
                        if (dialog != null) break;
                    }
                if (dialog != null) break;
                await Task.Delay(100);
            }
            if (dialog == null)
            {
                var visible = AutomationElement.RootElement.FindAll(TreeScope.Children, System.Windows.Automation.Condition.TrueCondition)
                    .Cast<AutomationElement>().Select(w => new { w.Current.Name, w.Current.ProcessId, w.Current.ClassName }).ToArray();
                File.WriteAllText(Path.Combine(Output, "picker-diagnostic.json"), JsonSerializer.Serialize(visible));
                throw new TimeoutException("Real local file dialog not found; see picker-diagnostic.json");
            }
            if (path == null)
            {
                if (!PostMessage(new IntPtr(dialog.Current.NativeWindowHandle), 0x0010, IntPtr.Zero, IntPtr.Zero))
                    throw new InvalidOperationException("Cannot cancel own file dialog");
                return;
            }
            var nameBox = dialog.FindFirst(TreeScope.Descendants,
                new AndCondition(new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.Edit),
                    new PropertyCondition(AutomationElement.AutomationIdProperty, "1148")));
            nameBox ??= dialog.FindAll(TreeScope.Descendants, new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.Edit))
                .Cast<AutomationElement>().FirstOrDefault(e => e.Current.Name.StartsWith("文件名") || e.Current.Name.StartsWith("File name"));
            if (nameBox == null) throw new InvalidOperationException("Real file-name edit not found");
            ((ValuePattern)nameBox.GetCurrentPattern(ValuePattern.Pattern)).SetValue(path);
            var open = dialog.FindFirst(TreeScope.Descendants,
                new AndCondition(new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.Button),
                    new PropertyCondition(AutomationElement.AutomationIdProperty, "1")));
            if (open == null) throw new InvalidOperationException("Real file-open button not found");
            ((InvokePattern)open.GetCurrentPattern(InvokePattern.Pattern)).Invoke();
        });
    }
    static async Task Choose(UserControl av, string? path)
    {
        var button = Control<Button>(av, "PlayLocalButton");
        button.BringIntoView();
        Host.UpdateLayout();
        await Dispatcher.Yield(DispatcherPriority.ApplicationIdle);
        var handling = FileDialog(path);
        Check("real local picker button enabled", button.IsEnabled && button.IsVisible);
        button.RaiseEvent(new RoutedEventArgs(Button.ClickEvent, button));
        await handling;
        await Dispatcher.Yield(DispatcherPriority.ContextIdle);
    }
    static bool AudioStopped(UserControl av)
    {
        object audio = Field(av, "_audio")!;
        return Field(audio, "_output") == null && Field(audio, "_reader") == null && Field(audio, "_outputDevice") == null;
    }
    static void Unlocked(string path)
    {
        using var stream = new FileStream(path, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
        Check("local audio file handle released", stream.Length > 0);
    }
    static string Wave(string name, int seconds)
    {
        string path = Path.Combine(Output, name);
        using var writer = new BinaryWriter(File.Create(path));
        int bytes = 44100 * seconds * 2;
        writer.Write("RIFF"u8); writer.Write(36 + bytes); writer.Write("WAVEfmt "u8);
        writer.Write(16); writer.Write((short)1); writer.Write((short)1); writer.Write(44100);
        writer.Write(88200); writer.Write((short)2); writer.Write((short)16); writer.Write("data"u8); writer.Write(bytes);
        for (int i = 0; i < bytes / 2; i++) writer.Write((short)(Math.Sin(i * 2 * Math.PI * 440 / 44100) * 250));
        return path;
    }
    static async Task AudioChecks()
    {
        await Nav("av");
        var av = Pages["av"];
        await Until(() => !(bool)Field(av, "_initializing")! && Control<ComboBox>(av, "SpkBox").IsEnabled, 30);
        var button = Control<Button>(av, "PlayLocalButton");
        Check("local picker enabled with real output device", button.IsEnabled);
        var status = Control<TextBlock>(av, "SpeakerStatus");
        string initial = status.Text;
        await Choose(av, null);
        Check("cancel real picker leaves playback unchanged", AudioStopped(av) && status.Text == initial);
        string invalid = Path.Combine(Output, "invalid.wav");
        File.WriteAllText(invalid, "not an audio stream");
        await Choose(av, invalid);
        Check("decoder failure displayed, no leaked output", status.Text.StartsWith("本地音频播放失败：") && AudioStopped(av), status.Text);
        Call(av, "PlayLocalFile", Path.Combine(Output, "disappeared.wav"));
        Check("direct vanished-file branch gives diagnostic", status.Text.Contains("所选文件已不存在") && AudioStopped(av));
        string longFile = Wave("quiet-long.wav", 8);
        string shortFile = Wave("quiet-short.wav", 1);
        var speakers = Control<ComboBox>(av, "SpkBox");
        Check("requested output device exists", OutputDeviceIndex < speakers.Items.Count,
            new { requestedIndex = OutputDeviceIndex, available = speakers.Items.Cast<object>().Select(x => x.ToString()).ToArray() });
        speakers.SetCurrentValue(System.Windows.Controls.Primitives.Selector.SelectedIndexProperty, OutputDeviceIndex);
        await Dispatcher.Yield(DispatcherPriority.ContextIdle);
        Check("requested real output device selected", speakers.SelectedIndex == OutputDeviceIndex,
            new { index = speakers.SelectedIndex, device = speakers.SelectedItem?.ToString() });
        await Choose(av, longFile);
        await Until(() => !AudioStopped(av), 5);
        Check("selected local file enters playback", button.Content as string == "停止音乐" && status.Text == "播放中…");
        int oldSession = (int)Field(av, "_session")!, oldGeneration = (int)Field(av, "_playbackGeneration")!;
        await Click(button);
        Check("stop button releases output and resets label", AudioStopped(av) && button.Content as string == "选择本地音乐…");
        Unlocked(longFile);
        await Choose(av, shortFile);
        await Until(() => AudioStopped(av) && button.Content as string == "选择本地音乐…", 10);
        Check("natural completion resets button", status.Text != "播放中…");
        Unlocked(shortFile);
        await Choose(av, longFile);
        Call(av, "OnPlaybackStopped", oldSession, oldGeneration);
        await Dispatcher.Yield(DispatcherPriority.ContextIdle);
        Check("direct stale callback cannot reset new playback", button.Content as string == "停止音乐" && !AudioStopped(av));
        string avJson = JsonSerializer.Serialize(((IReportSection)av).GetReportSection());
        Check("report fragment omits local audio path", !avJson.Contains(longFile) && !avJson.Contains("quiet-long"));
        await Nav("keyboard");
        await Until(() => AudioStopped(av) && Field(Field(av, "_audio")!, "_capture") == null, 15);
        Check("leave page stops timers and clears session", Field(av, "_sessionCts") == null
            && !((DispatcherTimer)Field(av, "_levelTimer")!).IsEnabled
            && !((DispatcherTimer)Field(av, "_recordLimitTimer")!).IsEnabled && !button.IsEnabled);
        Unlocked(longFile);
    }
    static async Task Run()
    {
        await FillIntake(true);
        Check("saved intake reaches real host", Field(Host, "_deviceLabel") as string == Label);
        var handling = FillIntake(false);
        await Click(NamedButton(Host, "已知信息"));
        await handling;
        Check("cancel intake preserves saved identity", Field(Host, "_deviceLabel") as string == Label);
        await Nav("config");
        var config = Pages["config"];
        await Until(() => !(bool)Field(config, "_running")! && ((IReportSection)config).GetReportSection() != null, 180);
        using var observed = JsonDocument.Parse(JsonSerializer.Serialize(((IReportSection)config).GetReportSection()));
        Check("actual hardware collection has groups", observed.RootElement.GetProperty("groups").GetArrayLength() > 0);
        await Nav("keyboard");
        var keyboard = Pages["keyboard"];
        await Click(Control<Button>(keyboard, "PassButton"));
        Check("manual state event reaches host (UI only, not hardware quality)", ((HashSet<string>)Field(Host, "_doneTags")!).Contains("keyboard"));
        await Nav("keyboard");
        await Click(Control<Button>(keyboard, "ResetButton"));
        Check("reset clears host done state", !((HashSet<string>)Field(Host, "_doneTags")!).Contains("keyboard"));
        try { await AudioChecks(); }
        catch (Exception ex)
        {
            Failure = ex.ToString(); ExitCode = 1;
            Console.Error.WriteLine("AUDIO_ACCEPTANCE_FAILED: " + ex);
            await Nav("keyboard");
        }
        var hint = Control<TextBlock>(Host, "ModuleHintText");
        DateTime before = DateTime.Now;
        await Click(NamedButton(Host, "导出报告"));
        await Until(() => hint.Text.StartsWith("报告已导出：") || hint.Text.StartsWith("报告已保存：") || hint.Text.StartsWith("报告导出失败："), 30);
        var exported = Directory.GetFiles(ReportService.ReportsDirectory, "*.json")
            .Where(p => File.GetLastWriteTime(p) >= before.AddSeconds(-1)).OrderByDescending(File.GetLastWriteTime).First();
        using var report = JsonDocument.Parse(File.ReadAllText(exported));
        var data = report.RootElement;
        Check("export identity matches actual intake", data.GetProperty("device").GetProperty("label").GetString() == Label);
        Check("export contains real config and six modules", data.GetProperty("config").ValueKind == JsonValueKind.Object && data.GetProperty("modules").GetArrayLength() == 6);
        Check("untested and reset modules not automatically passed", data.GetProperty("modules").EnumerateArray().All(m => !m.GetProperty("done").GetBoolean()));
        Check("expected input preserved separately", data.GetProperty("expected").GetProperty("fields").EnumerateArray().Any(f => f.GetProperty("key").GetString() == "cpu" && f.GetProperty("value").GetString()!.StartsWith("预期CPU")));
        string web = Path.Combine(Path.GetDirectoryName(exported)!, Path.GetFileNameWithoutExtension(exported));
        Check("offline web contains this report, not sample", File.Exists(Path.Combine(web, "index.html")) && File.ReadAllText(Path.Combine(web, "report-data.js")).Contains(Label));
        Check("current version in exported report", data.GetProperty("source").GetProperty("appVersion").GetString()!.StartsWith("0.0.7-preview.1"));
        string exportStatus = Control<TextBlock>(Host, "ModuleHintText").Text;
        File.WriteAllText(Path.Combine(Output, "export-locations.json"), JsonSerializer.Serialize(new { json = exported, html = Path.Combine(web, "index.html"), identity = Label, status = exportStatus }));
        Check("export status visible", exportStatus.Contains(exported), new { exportStatus, exported });
        Host.UpdateLayout();
        var bitmap = new RenderTargetBitmap((int)Host.ActualWidth, (int)Host.ActualHeight, 96, 96, PixelFormats.Pbgra32);
        bitmap.Render(Host);
        var png = new PngBitmapEncoder(); png.Frames.Add(BitmapFrame.Create(bitmap));
        using (var image = File.Create(Path.Combine(Output, "current-window.png"))) png.Save(image);
        Check("no stress workload launched", !StressTestService.IsRunning);
    }
    [STAThread]
    static int Main(string[] args)
    {
        if (args.Length != 3 || !Directory.Exists(args[0]) || args[1] != "--output-device-index"
            || !int.TryParse(args[2], out OutputDeviceIndex) || OutputDeviceIndex < 0)
        {
            Console.Error.WriteLine("Usage: VerifyDelivery <existing-evidence-directory> --output-device-index <nonnegative-index>");
            return 2;
        }
        Output = Path.GetFullPath(args[0]);
        Console.OutputEncoding = System.Text.Encoding.UTF8;
        using (var query = new ManagementObjectSearcher("SELECT State FROM Win32_SystemDriver WHERE Name='PawnIO'"))
        using (var rows = query.Get())
        {
            if (!rows.Cast<ManagementObject>().Any(r => r["State"] as string == "Running")) return 3;
        }
        SensorMonitorService.AllowAutomaticDriverStart = false;
        var app = new Application { ShutdownMode = ShutdownMode.OnExplicitShutdown };
        app.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("/验机软件;component/Themes/Light.xaml", UriKind.Relative) });
        app.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("/验机软件;component/Themes/Controls.xaml", UriKind.Relative) });
        app.DispatcherUnhandledException += (_, e) => { Failure = e.Exception.ToString(); Console.Error.WriteLine(e.Exception); ExitCode = 1; e.Handled = true; };
        Host = new MainWindow();
        bool started = false;
        Host.Loaded += (_, _) =>
        {
            if (started) return;
            started = true;
            app.Dispatcher.BeginInvoke(async () =>
            {
                try { await Run(); }
                catch (Exception ex) { Failure = ex.ToString(); ExitCode = 1; Console.Error.WriteLine(ex); }
                finally
                {
                    foreach (Window w in app.Windows.Cast<Window>().Where(w => w != Host).ToArray()) w.Close();
                    Host.Close();
                    try
                    {
                        await Until(() => !SensorMonitorService.IsRunning && AudioStopped(Pages["av"]), 30);
                        Check("normal close releases monitor and playback", !Host.IsVisible && !StressTestService.IsRunning);
                    }
                    catch (Exception ex) { Failure = (Failure ?? "") + ex; ExitCode = 1; }
                    File.WriteAllText(Path.Combine(Output, "delivery-acceptance.json"), JsonSerializer.Serialize(new
                    {
                        mode = "current-production-WPF-controls-real-hardware-no-stress-load", exitCode = ExitCode,
                        checks = Checks, failure = Failure, elapsedSeconds = Clock.Elapsed.TotalSeconds,
                        limitations = new[] { "Automation peers and routed button events are not physical mouse/keyboard practice; the output-device index must be explicitly supplied for each machine.", "Audio quality and real hardware pass/fail are human-owned; fixtures are quiet synthesized WAVs.", "No driver install/start, stress load, disk benchmark, clean-system, battery, or installer lifecycle acceptance.", "Missing-file/stale-callback checks explicitly use reflection; other actions drive real controls and native file picker.", "Legacy VerifyRelease remains unchanged and incompatible with current audio control names." }
                    }, new JsonSerializerOptions { WriteIndented = true }));
                    app.Shutdown(ExitCode);
                }
            }, DispatcherPriority.Background);
        };
        app.Run(Host);
        return ExitCode;
    }
}
