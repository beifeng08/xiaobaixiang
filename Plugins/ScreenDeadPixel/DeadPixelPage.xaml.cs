// Copyright (c) 2026 OmniSensor

using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using YanJi.PluginSdk.Services;

namespace YanJi.Plugin.ScreenDeadPixel;

public partial class DeadPixelPage : UserControl, YanJi.PluginSdk.IModulePage
{
    List<DisplayInfo> _monitors = [];
    ColorTestWindow? _testWindow;

    public event Action<bool>? PassChanged;

    public bool Passed { get; private set; }

    public DeadPixelPage()
    {
        InitializeComponent();
        Loaded += (_, _) => RefreshMonitors();
        Unloaded += (_, _) => _testWindow?.Close();
    }

    void RefreshMonitors()
    {
        try { _monitors = DisplayInfoService.GetDisplays(); }
        catch { _monitors = []; }

        MonitorBox.Items.Clear();
        int primary = 0;
        for (int i = 0; i < _monitors.Count; i++)
        {
            var d = _monitors[i];
            string name = d.Edid is { PanelName.Length: > 0 } e ? e.PanelName : d.GdiName;
            MonitorBox.Items.Add($"#{i + 1} {name} ({d.Width}×{d.Height})");
            if (d is { X: 0, Y: 0 }) primary = i;
        }
        if (MonitorBox.Items.Count > 0) MonitorBox.SelectedIndex = primary;
        StartButton.IsEnabled = MonitorBox.Items.Count > 0;
    }

    void StartButton_Click(object sender, RoutedEventArgs e)
    {
        if (_testWindow != null || MonitorBox.SelectedIndex < 0 || MonitorBox.SelectedIndex >= _monitors.Count)
            return;
        
        SetPassed(false);
        PassButton.IsEnabled = true;

        var d = _monitors[MonitorBox.SelectedIndex];
        _testWindow = new ColorTestWindow(d.X, d.Y, d.Width, d.Height)
        {
            Owner = Window.GetWindow(this),
        };
        _testWindow.Closed += (_, _) =>
        {
            _testWindow = null;
            StartButton.IsEnabled = true;
            TestingHint.Text = "";
        };
        StartButton.IsEnabled = false;
        TestingHint.Text = "测试进行中：按任意键或单击鼠标切换颜色，Esc 退出";
        _testWindow.Show();
    }

    void PassButton_Click(object sender, RoutedEventArgs e)
    {
        SetPassed(true);
        PassButton.IsEnabled = false;
    }

    void SetPassed(bool passed)
    {
        Passed = passed;
        if (passed)
        {
            StatusText.Text = "✓ 已确认通过";
            StatusText.Foreground = Brushes.ForestGreen;
        }
        else
        {
            StatusText.Text = "待测试";
            StatusText.SetResourceReference(TextBlock.ForegroundProperty, "TextSecondaryBrush");
        }
        PassChanged?.Invoke(passed);
    }
}
