// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Globalization;
using System.Windows;
using System.Windows.Media;

namespace YanJi.Plugin.StressTest;

public sealed class LineChartControl : FrameworkElement
{
    readonly List<float> _samples = [];
    double? _current;
    string? _unavailableText;

    public static readonly DependencyProperty TitleProperty = DependencyProperty.Register(
        nameof(Title), typeof(string), typeof(LineChartControl),
        new FrameworkPropertyMetadata("", FrameworkPropertyMetadataOptions.AffectsRender));

    public static readonly DependencyProperty UnitProperty = DependencyProperty.Register(
        nameof(Unit), typeof(string), typeof(LineChartControl),
        new FrameworkPropertyMetadata("", FrameworkPropertyMetadataOptions.AffectsRender));

    public static readonly DependencyProperty YMaxFloorProperty = DependencyProperty.Register(
        nameof(YMaxFloor), typeof(double), typeof(LineChartControl),
        new FrameworkPropertyMetadata(50.0, FrameworkPropertyMetadataOptions.AffectsRender));

    public static readonly DependencyProperty WindowSecondsProperty = DependencyProperty.Register(
        nameof(WindowSeconds), typeof(int), typeof(LineChartControl),
        new FrameworkPropertyMetadata(120, FrameworkPropertyMetadataOptions.AffectsRender));

    public static readonly DependencyProperty SampleIntervalSecondsProperty = DependencyProperty.Register(
        nameof(SampleIntervalSeconds), typeof(int), typeof(LineChartControl),
        new FrameworkPropertyMetadata(1, FrameworkPropertyMetadataOptions.AffectsRender));

    public string Title { get => (string)GetValue(TitleProperty); set => SetValue(TitleProperty, value); }
    public string Unit { get => (string)GetValue(UnitProperty); set => SetValue(UnitProperty, value); }
    public double YMaxFloor { get => (double)GetValue(YMaxFloorProperty); set => SetValue(YMaxFloorProperty, value); }
    public int WindowSeconds { get => (int)GetValue(WindowSecondsProperty); set => SetValue(WindowSecondsProperty, value); }
    public int SampleIntervalSeconds { get => (int)GetValue(SampleIntervalSecondsProperty); set => SetValue(SampleIntervalSecondsProperty, value); }

    int Capacity => Math.Max(2, WindowSeconds / Math.Max(1, SampleIntervalSeconds));

    public LineChartControl()
    {
        Height = 150;
        SnapsToDevicePixels = false;
    }

    public void PushSample(double value)
    {
        _unavailableText = null;
        _current = value;
        _samples.Add((float)value);
        while (_samples.Count > Capacity) _samples.RemoveAt(0);
        InvalidateVisual();
    }

    public void SetUnavailable(string text)
    {
        _unavailableText = text;
        _current = null;
        _samples.Clear();
        InvalidateVisual();
    }

    public void Clear()
    {
        _unavailableText = null;
        _current = null;
        _samples.Clear();
        InvalidateVisual();
    }

    Color ResColor(string key, Color fallback) =>
        TryFindResource(key) is SolidColorBrush b ? b.Color : fallback;

    protected override void OnRender(DrawingContext dc)
    {
        double dpi = VisualTreeHelper.GetDpi(this).PixelsPerDip;
        var gray = ResColor("ChartTextBrush", Color.FromRgb(0x80, 0x80, 0x80));
        var grid = ResColor("ChartGridBrush", Color.FromRgb(0xE6, 0xE6, 0xE6));
        var noData = ResColor("ChartNoDataBrush", Color.FromRgb(0xD0, 0xD0, 0xD0));
        var accent = ResColor("AccentBrush", Colors.SteelBlue);
        var grayBrush = new SolidColorBrush(gray);
        var gridPen = new Pen(new SolidColorBrush(grid), 1);
        double W = ActualWidth, H = ActualHeight;

        FormattedText MkText(string s, Color color, bool semiBold = false) => new(s,
            CultureInfo.CurrentCulture, FlowDirection.LeftToRight,
            new Typeface(new FontFamily("Microsoft YaHei UI"), FontStyles.Normal,
                semiBold ? FontWeights.SemiBold : FontWeights.Normal, FontStretches.Normal),
            12, new SolidColorBrush(color), dpi);

        var titleFt = MkText(Title, gray);
        dc.DrawText(titleFt, new Point(4, 2));
        if (_current is double cur)
        {
            var curFt = MkText($"{cur:0.0} {Unit}", accent, semiBold: true);
            curFt.TextAlignment = TextAlignment.Right;
            curFt.MaxTextWidth = Math.Max(1, W - titleFt.Width - 20);
            
            dc.DrawText(curFt, new Point(W - 4 - curFt.MaxTextWidth, 2));
        }
        double top = titleFt.Height + 10, bottom = H - 20;

        if (_unavailableText != null || _samples.Count == 0)
        {
            string msg = _unavailableText ?? "等待数据…";
            var ft = MkText(msg, noData);
            dc.DrawText(ft, new Point((W - ft.Width) / 2, (top + bottom - ft.Height) / 2));
            
        }

        double yMax = Math.Max(NiceCeil(_samples.Count > 0 ? _samples.Max() : 0), YMaxFloor);
        double left = Math.Max(44, MkText(FmtTick(yMax), gray).Width + 10);
        double right = W - 10;
        
        for (int i = 0; i <= 4; i++)
        {
            double y = bottom - (bottom - top) * i / 4;
            dc.DrawLine(gridPen, new Point(left, y), new Point(right, y));
            var tick = MkText(FmtTick(yMax * i / 4), gray);
            tick.TextAlignment = TextAlignment.Right;
            tick.MaxTextWidth = Math.Max(1, left - 6);
            dc.DrawText(tick, new Point(0, y - tick.Height / 2));
        }
        
        dc.DrawText(MkText($"-{WindowSeconds} s", gray), new Point(left, bottom + 4));
        var nowFt = MkText("0", gray);
        nowFt.TextAlignment = TextAlignment.Right;
        nowFt.MaxTextWidth = 40;
        dc.DrawText(nowFt, new Point(right - 40, bottom + 4)); 

        if (_samples.Count > 0)
        {
            var pen = new Pen(new SolidColorBrush(accent), 2) { LineJoin = PenLineJoin.Round };
            double step = (right - left) / (Capacity - 1);
            double Y(float v) => bottom - v / yMax * (bottom - top);
            int n = _samples.Count;
            if (n == 1)
            {
                dc.DrawEllipse(new SolidColorBrush(accent), null, new Point(right, Y(_samples[0])), 2.5, 2.5);
            }
            else
            {
                var fig = new PathFigure { StartPoint = new Point(right - (n - 1) * step, Y(_samples[0])) };
                for (int i = 1; i < n; i++)
                    fig.Segments.Add(new LineSegment(new Point(right - (n - 1 - i) * step, Y(_samples[i])), true));
                var geo = new PathGeometry();
                geo.Figures.Add(fig);
                dc.DrawGeometry(null, pen, geo);
            }
        }
    }

    static double NiceCeil(double max)
    {
        if (max <= 0) return 1;
        double mag = Math.Pow(10, Math.Floor(Math.Log10(max)));
        foreach (var m in new[] { 1.0, 2.0, 2.5, 5.0, 10.0 })
            if (max <= m * mag) return m * mag;
        return 10 * mag;
    }

    static string FmtTick(double v) =>
        v >= 1000 ? ((int)v).ToString() : v.ToString("0.##");
}
