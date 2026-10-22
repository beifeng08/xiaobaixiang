// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace YanJi.Plugin.StressTest;

public partial class StressTestPage : UserControl, YanJi.PluginSdk.IModulePage, YanJi.PluginSdk.IReportSection
{
    readonly DispatcherTimer _tick = new() { Interval = TimeSpan.FromMilliseconds(500) };
    TimeSpan _lastRun = TimeSpan.Zero; 
    DateTime? _runStart;               

    const int SampleIntervalSeconds = 2;
    const int MaxRecordedSamples = 6 * 3600 / SampleIntervalSeconds;
    readonly List<(double T, SensorMonitorService.Reading R)> _record = [];
    DateTime? _recordStart;

    public event Action<bool>? PassChanged;

    public bool Passed { get; private set; }

    public StressTestPage()
    {
        InitializeComponent();

        var (phys, logical) = StressTestService.CoreCounts;
        for (int i = 1; i <= logical; i++)
            ThreadsBox.Items.Add(new ComboBoxItem { Content = $"{i} 线程", Tag = i });
        ThreadsBox.SelectedIndex = logical - 1;
        CoresHint.Text = $"本机 {phys} 物理核 / {logical} 逻辑线程；超过物理核数的部分以超线程跑满";

        _tick.Tick += (_, _) => RefreshRunningState();
        StressTestService.StateChanged += OnServiceStateChanged;
        StressTestService.Logged += OnStressLogged;
        SensorMonitorService.SessionStarted += () => QueueSensorUpdate(ResetSensorState, allowCanceled: true);
        SensorMonitorService.Sampled += OnSensorSampled;
        SensorMonitorService.SensorsResolved += OnSensorsResolved;
        SensorMonitorService.GpuListReady += OnGpuListReady;
        SensorMonitorService.GpuChanging += OnGpuChanging;
        SensorMonitorService.Failed += message => QueueSensorUpdate(() =>
        {
            _sensorFailure = message;
            RefreshSensorHint();
        }, allowCanceled: true);
        
        Unloaded += (_, _) =>
        {
            _tick.Stop();
            try { StressTestService.Stop(); }
            finally { SensorMonitorService.Stop(); }
        };
        
        Loaded += (_, _) =>
        {
            ResetSensorState();
            SensorMonitorService.Start();
        };
        SensorHintText.Text = "正在初始化传感器……";
        RefreshRunningState();
    }

    void StartButton_Click(object sender, RoutedEventArgs e)
    {
        SetPassed(false);
        _record.Clear();
        _recordStart = null;
        var mode = ParseTag<StressTestService.StressMode>(ModeBox);
        var fft = ParseTag<StressTestService.FftPreset>(FftBox);
        int threads = ParseTag<int>(ThreadsBox);
        int msaa = ParseTag<int>(MsaaBox);
        var (w, h) = ParseResolution();
        try
        {
            StressTestService.Start(mode,
                new StressTestService.Prime95Settings(threads, fft, P95ShowUiCheck.IsChecked == true),
                new StressTestService.FurMarkSettings(w, h, FullscreenCheck.IsChecked == true, msaa,
                    FurShowUiCheck.IsChecked == true));
        }
        catch (Exception ex)
        {
            RunStateText.Text = "启动失败：" + ex.Message;
        }
        RefreshRunningState();
    }

    void OnStressLogged(string line) =>
        Dispatcher.BeginInvoke(() =>
        {
            LogBox.AppendText(line + Environment.NewLine);
            if (LogBox.LineCount > 500)
            {
                int i = LogBox.Text.IndexOf('\n');
                if (i >= 0) LogBox.Text = LogBox.Text[(i + 1)..];
            }
            LogBox.ScrollToEnd();
        });

    void StopButton_Click(object sender, RoutedEventArgs e) => StressTestService.Stop();

    void PassButton_Click(object sender, RoutedEventArgs e)
    {
        
        StressTestService.Stop();
        SetPassed(true);
    }

    void OnServiceStateChanged() =>
        Dispatcher.BeginInvoke(() =>
        {
            if (StressTestService.IsRunning) _tick.Start();
            else _tick.Stop();
            RefreshRunningState();
        });

    bool _cpuTempDown, _cpuPowerDown, _gpuTempDown, _gpuPowerDown;
    SensorMonitorService.SensorMap? _sensorMap;
    string? _sensorFailure;

    void ResetSensorState()
    {
        CpuTempChart.Clear();
        CpuPowerChart.Clear();
        GpuTempChart.Clear();
        GpuPowerChart.Clear();
        _cpuTempDown = _cpuPowerDown = _gpuTempDown = _gpuPowerDown = false;
        CpuTempOverlay.Visibility = CpuPowerOverlay.Visibility = Visibility.Collapsed;
        _sensorMap = null;
        _sensorFailure = null;
        _gpuBoxSyncing = true;
        try
        {
            GpuBox.Items.Clear();
            GpuBox.Visibility = Visibility.Collapsed;
        }
        finally { _gpuBoxSyncing = false; }
        SensorHintText.Text = "正在初始化传感器……";
    }

    void QueueSensorUpdate(Action update, bool allowCanceled = false)
    {
        var session = SensorMonitorService.SessionToken;
        Dispatcher.BeginInvoke(() =>
        {
            if (IsLoaded && session == SensorMonitorService.SessionToken &&
                (allowCanceled || !session.IsCancellationRequested)) update();
        });
    }

    void OnSensorSampled(SensorMonitorService.Reading r) =>
        QueueSensorUpdate(() =>
        {
            UpdateChart(CpuTempChart, CpuTempOverlay, r.CpuTemp, ref _cpuTempDown);
            UpdateChart(CpuPowerChart, CpuPowerOverlay, r.CpuPower, ref _cpuPowerDown);
            UpdateChart(GpuTempChart, null, r.GpuTemp, ref _gpuTempDown);
            UpdateChart(GpuPowerChart, null, r.GpuPower, ref _gpuPowerDown);
            if (StressTestService.IsRunning && StressTestService.StartedAt is DateTime t0)
            {
                if (_recordStart != t0)
                {
                    _record.Clear();
                    _recordStart = t0;
                }
                if (_record.Count < MaxRecordedSamples)
                    _record.Add(((DateTime.Now - t0).TotalSeconds, r));
            }
        });

    void UpdateChart(LineChartControl chart, UIElement? overlay, double? value, ref bool unavailable)
    {
        if (value is double v)
        {
            if (unavailable)
            {
                unavailable = false;
                if (overlay != null) overlay.Visibility = Visibility.Collapsed;
            }
            chart.PushSample(v);
        }
        else if (!unavailable)
        {
            unavailable = true;
            if (overlay != null)
            {
                chart.SetUnavailable(""); 
                overlay.Visibility = Visibility.Visible;
            }
            else
            {
                chart.SetUnavailable("传感器不可用");
            }
        }
        RefreshSensorHint();
    }

    void RefreshSensorHint()
    {
        var m = _sensorMap;
        if (m == null)
        {
            SensorHintText.Text = _sensorFailure ?? "正在初始化传感器……";
            return;
        }
        string hint =
            $"CPU 温度  ←  {Line(m.CpuName, m.CpuTemp, _cpuTempDown, needsDriver: true)}\n" +
            $"CPU 功耗  ←  {Line(m.CpuName, m.CpuPower, _cpuPowerDown, needsDriver: true)}\n" +
            $"GPU 温度  ←  {Line(m.GpuName, m.GpuTemp, _gpuTempDown, needsDriver: false)}\n" +
            $"GPU 功耗  ←  {Line(m.GpuName, m.GpuPower, _gpuPowerDown, needsDriver: false)}";
        SensorHintText.Text = _sensorFailure == null ? hint : _sensorFailure + "\n" + hint;

        static string Line(string? hw, string? sensor, bool down, bool needsDriver)
        {
            if (sensor == null) return "未找到传感器";
            string s = $"{hw} / {sensor}";
            if (down) s += needsDriver ? "（读数无效，需 PawnIO 驱动）" : "（当前不可用）";
            return s;
        }
    }

    void OnSensorsResolved(SensorMonitorService.SensorMap map) =>
        QueueSensorUpdate(() =>
        {
            _sensorMap = map;
            RefreshSensorHint();
        });

    bool _gpuBoxSyncing;

    void OnGpuListReady(IReadOnlyList<SensorMonitorService.GpuOption> options, int selected) =>
        QueueSensorUpdate(() =>
        {
            _gpuBoxSyncing = true;
            GpuBox.Items.Clear();
            foreach (var g in options)
                GpuBox.Items.Add(new ComboBoxItem { Content = g.Name, Tag = g.Index });
            GpuBox.SelectedIndex = selected;
            GpuBox.Visibility = options.Count > 0 ? Visibility.Visible : Visibility.Collapsed;
            _gpuBoxSyncing = false;
        });

    void GpuBox_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (_gpuBoxSyncing) return;
        if ((GpuBox.SelectedItem as ComboBoxItem)?.Tag is int index)
            SensorMonitorService.SetGpu(index);
    }

    void OnGpuChanging() =>
        QueueSensorUpdate(() =>
        {
            
            GpuTempChart.Clear();
            GpuPowerChart.Clear();
            _gpuTempDown = _gpuPowerDown = false;
        });

    async void InstallPawnIo_Click(object sender, RoutedEventArgs e)
    {
        if (sender is not Button btn) return;
        btn.IsEnabled = false;
        btn.Content = "正在安装……";
        bool ok = await Task.Run(SensorMonitorService.InstallDriver);
        if (ok)
        {
            if (IsLoaded) SensorMonitorService.Restart();
            btn.Content = "驱动已安装";
        }
        else
        {
            btn.Content = "安装失败，点这里重试";
            btn.IsEnabled = true;
        }
    }

    void RefreshRunningState()
    {
        bool running = StressTestService.IsRunning;
        StartButton.IsEnabled = !running;
        StopButton.IsEnabled = running;

        if (running && StressTestService.StartedAt is DateTime t0)
        {
            _runStart = t0;
            var span = DateTime.Now - t0;
            ElapsedText.Text = $"已烤 {FormatSpan(span)}";
            RunStateText.Text = "烤机进行中…… 请留意温度与风扇状态；切走本页会自动停止烤机";
        }
        else
        {
            
            if (_runStart is DateTime rs)
            {
                _lastRun = DateTime.Now - rs;
                _runStart = null;
            }
            ElapsedText.Text = "未在烤机";
            if (_lastRun > TimeSpan.Zero)
                RunStateText.Text = $"本次已烤 {FormatSpan(_lastRun)}；可再次开始或直接判断散热是否合格";
            else if (!RunStateText.Text.StartsWith("启动失败"))
                RunStateText.Text = "尚未烤过机；烤机会让硬件满载发热，请留意温度与风扇状态";
        }

        if (!Passed)
            PassHintText.Text = running ? "正在烤机" :
                _lastRun > TimeSpan.Zero ? $"本次已烤 {FormatSpan(_lastRun)}" : "尚未烤过机";
    }

    void SetPassed(bool passed)
    {
        Passed = passed;
        if (passed)
        {
            StatusText.Text = "✓ 已核对通过";
            StatusText.Foreground = Brushes.ForestGreen;
            PassHintText.Text = "";
        }
        else
        {
            StatusText.Text = "待核对";
            StatusText.SetResourceReference(TextBlock.ForegroundProperty, "TextSecondaryBrush");
        }
        PassChanged?.Invoke(passed);
    }

    (int W, int H) ParseResolution()
    {
        string tag = (ResolutionBox.SelectedItem as ComboBoxItem)?.Tag as string ?? "1920x1080";
        string[] parts = tag.Split('x');
        return (int.Parse(parts[0]), int.Parse(parts[1]));
    }

    static T ParseTag<T>(ComboBox box) where T : notnull
    {
        object tag = ((ComboBoxItem)box.SelectedItem).Tag;
        
        if (typeof(T).IsEnum) return (T)Enum.Parse(typeof(T), (string)tag);
        return (T)Convert.ChangeType(tag, typeof(T));
    }

    static string FormatSpan(TimeSpan s) =>
        s.TotalHours >= 1 ? $"{(int)s.TotalHours}:{s.Minutes:00}:{s.Seconds:00}" : $"{s.Minutes:00}:{s.Seconds:00}";

    public object? GetReportSection()
    {
        if (_recordStart is not DateTime start || _record.Count == 0) return null;
        var m = _sensorMap;
        static double? R1(double? v) => v is double d ? Math.Round(d, 1) : null;
        return new
        {
            startedAt = start.ToString("yyyy-MM-ddTHH:mm:sszzz"),
            sampleIntervalSeconds = SampleIntervalSeconds,
            sensors = new
            {
                cpuName = m?.CpuName, cpuTemp = m?.CpuTemp, cpuPower = m?.CpuPower,
                gpuName = m?.GpuName, gpuTemp = m?.GpuTemp, gpuPower = m?.GpuPower,
            },
            samples = _record.Select(s => new
            {
                t = Math.Round(s.T, 1),
                cpuTemp = R1(s.R.CpuTemp), cpuPower = R1(s.R.CpuPower),
                gpuTemp = R1(s.R.GpuTemp), gpuPower = R1(s.R.GpuPower),
            }),
        };
    }
}
