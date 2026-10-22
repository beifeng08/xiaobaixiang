// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using YanJi.PluginSdk.Services;

namespace YanJi.Plugin.ConfigCheck;

public partial class ConfigCheckPage : UserControl, YanJi.PluginSdk.IModulePage, YanJi.PluginSdk.IReportSection
{
    bool _running;
    bool _autoStarted;
    bool _active, _resume, _shutdown;
    CancellationTokenSource? _collection;
    HardwareReport? _report;

    public event Action<bool>? PassChanged;

    public bool Passed { get; private set; }

    public ConfigCheckPage()
    {
        InitializeComponent();
        
        Loaded += (_, _) =>
        {
            _active = true;
            if (_shutdown || (_autoStarted && !_resume)) return;
            _autoStarted = true;
            StartDetect();
        };
        Unloaded += (_, _) =>
        {
            _active = false;
            if (_running) _resume = true;
            _collection?.Cancel();
        };
    }

    void RunButton_Click(object sender, RoutedEventArgs e) => StartDetect();

    async void StartDetect()
    {
        if (_running || !_active || _shutdown) return;
        _resume = false;
        _running = true;
        using var source = new CancellationTokenSource();
        _collection = source;
        SetPassed(false);
        RunButton.IsEnabled = false;
        PassButton.IsEnabled = false;
        DetectProgress.Value = 0;
        ResultsHost.ItemsSource = null;
        _report = null;
        try
        {
            
            var progress = new Progress<HardwareInfoService.CollectProgress>(p =>
            {
                if (!_active || source.IsCancellationRequested || _collection != source) return;
                DetectProgress.Value = 100.0 * p.Step / p.Total;
                PhaseText.Text = $"正在采集：{p.Stage}（{p.Step + 1}/{p.Total}）";
            });
            var report = await Task.Run(() => HardwareInfoService.Collect(progress, source.Token), source.Token);
            if (!_active || source.IsCancellationRequested || _shutdown) return;
            _report = report;
            ResultsHost.ItemsSource = report.Groups;
            DetectProgress.Value = 100;
            PhaseText.Text = $"检测完成（{report.CollectedAt:HH:mm:ss}），请逐项核对下方配置信息";
            PassButton.IsEnabled = true;
        }
        catch (OperationCanceledException) when (source.IsCancellationRequested)
        {
            if (_active) PhaseText.Text = "已停止采集，正在准备重新检测";
        }
        catch (Exception ex)
        {
            if (_active && !_shutdown) PhaseText.Text = "检测出错：" + ex.Message;
        }
        finally
        {
            _collection = null;
            _running = false;
            RunButton.IsEnabled = true;
            if (_active && _resume && !_shutdown)
                _ = Dispatcher.BeginInvoke(new Action(StartDetect));
        }
    }

    public void StopCollection()
    {
        _shutdown = true;
        _resume = false;
        _collection?.Cancel();
    }

    void PassButton_Click(object sender, RoutedEventArgs e)
    {
        SetPassed(true);
        PassButton.IsEnabled = false;
        PhaseText.Text = "已确认通过；如更换硬件或需复查，可点「重新检测」";
    }

    void SetPassed(bool passed)
    {
        Passed = passed;
        if (passed)
        {
            StatusText.Text = "✓ 已核对通过";
            StatusText.Foreground = Brushes.ForestGreen;
        }
        else
        {
            StatusText.Text = "待核对";
            StatusText.SetResourceReference(TextBlock.ForegroundProperty, "TextSecondaryBrush");
        }
        PassChanged?.Invoke(passed);
    }

    public object? GetReportSection() => _report is not { } r ? null : new
    {
        collectedAt = r.CollectedAt.ToString("yyyy-MM-ddTHH:mm:sszzz"),
        groups = r.Groups.Select(g => new
        {
            title = g.Title,
            entries = g.Entries.Select(e => new { name = e.Name, value = e.Value }),
            details = g.Details.Select(e => new { name = e.Name, value = e.Value }),
        }),
    };
}
