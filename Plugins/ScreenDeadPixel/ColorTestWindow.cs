// Copyright (c) 2026 OmniSensor

using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace YanJi.Plugin.ScreenDeadPixel;

public class ColorTestWindow : Window
{
    static readonly Color[] Cycle = [Colors.White, Colors.Red, Colors.Lime, Colors.Blue, Colors.Black];
    int _index;
    readonly Border _hint;
    readonly DispatcherTimer _hintTimer;

    public ColorTestWindow(int x, int y, int width, int height)
    {
        WindowStyle = WindowStyle.None;
        ResizeMode = ResizeMode.NoResize;
        Topmost = true;
        ShowInTaskbar = false;
        
        Left = x; Top = y; Width = width; Height = height;
        Background = new SolidColorBrush(Cycle[0]);

        _hint = new Border
        {
            Background = new SolidColorBrush(Color.FromArgb(0x99, 0, 0, 0)),
            CornerRadius = new CornerRadius(8),
            Padding = new Thickness(24, 12, 24, 12),
            HorizontalAlignment = HorizontalAlignment.Center,
            VerticalAlignment = VerticalAlignment.Center,
            Child = new TextBlock
            {
                Text = "按任意键或单击鼠标切换颜色，Esc 退出",
                Foreground = Brushes.White,
                FontSize = 20,
            },
        };
        Content = new Grid { Children = { _hint } };

        _hintTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(3) };
        _hintTimer.Tick += (_, _) => DismissHint();

        KeyDown += (_, e) =>
        {
            if (e.Key == Key.Escape) Close();
            else Next();
        };
        MouseLeftButtonDown += (_, _) => Next();
        Closed += (_, _) => _hintTimer.Stop();

        Loaded += (_, _) =>
        {
            var hwnd = new System.Windows.Interop.WindowInteropHelper(this).Handle;
            SetWindowPos(hwnd, HwndTopmost, x, y, width, height, SwpShowwindow);
            _hintTimer.Start();
        };
    }

    public void Next()
    {
        DismissHint();
        _index = (_index + 1) % Cycle.Length;
        Background = new SolidColorBrush(Cycle[_index]);
    }

    void DismissHint()
    {
        _hint.Visibility = Visibility.Collapsed;
        _hintTimer.Stop();
    }

    [DllImport("user32.dll", SetLastError = true)]
    static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    static readonly IntPtr HwndTopmost = new(-1);
    const uint SwpShowwindow = 0x0040;
}
