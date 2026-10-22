// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace YanJi.Plugin.KeyboardTest;

public partial class KeyboardTestPage : UserControl, YanJi.PluginSdk.IModulePage, YanJi.PluginSdk.IKeyHandlerPage, YanJi.PluginSdk.IReportSection
{
    const double U = 48;  
    const double Gap = 6; 

    class KeyVisual
    {
        public required string Label { get; init; }
        public required Border Bd { get; init; }
        public required TextBlock Tb { get; init; }
        public bool Tested { get; set; }
    }

    readonly List<KeyVisual> _all = [];
    readonly Dictionary<Key, List<KeyVisual>> _keyMap = [];
    readonly HashSet<Key> _testedKeys = [];
    Window? _hostWindow;
    int _testedCount;

    enum Layout { Full104, Tkl87 }
    Layout _layout = Layout.Full104;

    public event Action<bool>? PassChanged;

    public bool Passed { get; private set; }

    public KeyboardTestPage()
    {
        InitializeComponent();
        BuildKeyboard();
        Loaded += (_, _) =>
        {
            _hostWindow = Window.GetWindow(this);
            if (_hostWindow != null) _hostWindow.Deactivated += HostWindow_Deactivated;
        };
        Unloaded += (_, _) =>
        {
            if (_hostWindow != null) _hostWindow.Deactivated -= HostWindow_Deactivated;
            _hostWindow = null;
            ClearPressedVisuals();
        };
        IsKeyboardFocusWithinChanged += (_, _) =>
        {
            if (!IsKeyboardFocusWithin) ClearPressedVisuals();
        };
    }

    void HostWindow_Deactivated(object? sender, EventArgs e) => ClearPressedVisuals();

    void ClearPressedVisuals()
    {
        foreach (var v in _all)
        {
            if (v.Tested) SetTested(v);
            else SetNormal(v);
        }
    }

    void BuildKeyboard()
    {
        KeyboardCanvas.Children.Clear();
        _all.Clear();
        _keyMap.Clear();
        _testedCount = 0;
        bool full = _layout == Layout.Full104;
        
        KeyboardCanvas.Width = (full ? 23 : 18.5) * U;
        KeyboardCanvas.Height = 6.5 * U; 
        foreach (var cap in BuildLayout(full))
        {
            var tb = new TextBlock
            {
                Text = cap.Label,
                FontSize = cap.Label.Length > 3 ? 10 : 12,
                HorizontalAlignment = HorizontalAlignment.Center,
            };
            tb.SetResourceReference(TextBlock.ForegroundProperty, "TextBrush");
            var bd = new Border
            {
                CornerRadius = new CornerRadius(4),
                BorderThickness = new Thickness(1),
                Width = cap.W * U - Gap,
                Height = cap.H * U - Gap,
                Child = tb,
            };
            bd.SetResourceReference(Border.BackgroundProperty, "SurfaceBgBrush");
            bd.SetResourceReference(Border.BorderBrushProperty, "BorderStrongBrush");
            Canvas.SetLeft(bd, cap.X * U + Gap / 2);
            Canvas.SetTop(bd, cap.Y * U + Gap / 2);
            KeyboardCanvas.Children.Add(bd);

            var v = new KeyVisual { Label = cap.Label, Bd = bd, Tb = tb, Tested = _testedKeys.Contains(cap.Key) };
            _all.Add(v);
            if (v.Tested)
            {
                _testedCount++;
                SetTested(v);
            }
            if (!_keyMap.TryGetValue(cap.Key, out var list))
                _keyMap[cap.Key] = list = [];
            list.Add(v);
        }
        KeyProgress.Maximum = _all.Count;
        UpdateProgress();
    }

    void Layout104_Checked(object sender, RoutedEventArgs e)
    {
        if (KeyboardCanvas == null) return; 
        if (_layout == Layout.Full104) return;
        _layout = Layout.Full104;
        BuildKeyboard();
        SetPassed(false);
        PassButton.IsEnabled = true;
    }

    void Layout87_Checked(object sender, RoutedEventArgs e)
    {
        if (KeyboardCanvas == null) return;
        if (_layout == Layout.Tkl87) return;
        _layout = Layout.Tkl87;
        BuildKeyboard();
        SetPassed(false);
        PassButton.IsEnabled = true;
    }

    public object? GetReportSection() => new
    {
        layout = _layout == Layout.Full104 ? "104" : "87",
        tested = _testedCount,
        total = _all.Count,
        untested = _all.Where(v => !v.Tested).Select(v => v.Label).Distinct(),
    };

    static List<KeyCap> BuildLayout(bool full)
    {
        var caps = new List<KeyCap>(104);

        caps.Add(new KeyCap("Esc", Key.Escape, 0, 0));
        double fx = 2;
        for (int i = 1; i <= 12; i++)
        {
            caps.Add(new KeyCap($"F{i}", (Key)((int)Key.F1 + i - 1), fx, 0));
            fx += 1;
            if (i is 4 or 8) fx += 0.5; 
        }
        caps.Add(new KeyCap("PrtSc", Key.PrintScreen, 15.5, 0));
        caps.Add(new KeyCap("ScrLk", Key.Scroll, 16.5, 0));
        caps.Add(new KeyCap("Pause", Key.Pause, 17.5, 0));

        Row(caps, 1.5, 0, ("`~", Key.OemTilde, 1), ("1!", Key.D1, 1), ("2@", Key.D2, 1), ("3#", Key.D3, 1),
            ("4$", Key.D4, 1), ("5%", Key.D5, 1), ("6^", Key.D6, 1), ("7&", Key.D7, 1), ("8*", Key.D8, 1),
            ("9(", Key.D9, 1), ("0)", Key.D0, 1), ("-_", Key.OemMinus, 1), ("=+", Key.OemPlus, 1),
            ("Backspace", Key.Back, 2));
        Row(caps, 1.5, 15.5, ("Ins", Key.Insert, 1), ("Home", Key.Home, 1), ("PgUp", Key.PageUp, 1));
        Row(caps, 1.5, 19, ("Num", Key.NumLock, 1), ("/", Key.Divide, 1), ("*", Key.Multiply, 1), ("-", Key.Subtract, 1));

        Row(caps, 2.5, 0, ("Tab", Key.Tab, 1.5), ("Q", Key.Q, 1), ("W", Key.W, 1), ("E", Key.E, 1), ("R", Key.R, 1),
            ("T", Key.T, 1), ("Y", Key.Y, 1), ("U", Key.U, 1), ("I", Key.I, 1), ("O", Key.O, 1), ("P", Key.P, 1),
            ("[{", Key.OemOpenBrackets, 1), ("]}", Key.OemCloseBrackets, 1), (@\|", Key.OemPipe, 1.5));
        Row(caps, 2.5, 15.5, ("Del", Key.Delete, 1), ("End", Key.End, 1), ("PgDn", Key.PageDown, 1));
        Row(caps, 2.5, 19, ("7", Key.NumPad7, 1), ("8", Key.NumPad8, 1), ("9", Key.NumPad9, 1));
        caps.Add(new KeyCap("+", Key.Add, 22, 2.5, 1, 2));

        Row(caps, 3.5, 0, ("Caps", Key.Capital, 1.75), ("A", Key.A, 1), ("S", Key.S, 1), ("D", Key.D, 1),
            ("F", Key.F, 1), ("G", Key.G, 1), ("H", Key.H, 1), ("J", Key.J, 1), ("K", Key.K, 1), ("L", Key.L, 1),
            (";:", Key.OemSemicolon, 1), ("'\"", Key.OemQuotes, 1), ("Enter", Key.Return, 2.25));
        Row(caps, 3.5, 19, ("4", Key.NumPad4, 1), ("5", Key.NumPad5, 1), ("6", Key.NumPad6, 1));

        Row(caps, 4.5, 0, ("Shift", Key.LeftShift, 2.25), ("Z", Key.Z, 1), ("X", Key.X, 1), ("C", Key.C, 1),
            ("V", Key.V, 1), ("B", Key.B, 1), ("N", Key.N, 1), ("M", Key.M, 1), (",<", Key.OemComma, 1),
            (".>", Key.OemPeriod, 1), ("/?", Key.OemQuestion, 1), ("Shift", Key.RightShift, 2.75));
        caps.Add(new KeyCap("↑", Key.Up, 16.5, 4.5));
        Row(caps, 4.5, 19, ("1", Key.NumPad1, 1), ("2", Key.NumPad2, 1), ("3", Key.NumPad3, 1));
        caps.Add(new KeyCap("Enter", Key.Return, 22, 4.5, 1, 2));

        Row(caps, 5.5, 0, ("Ctrl", Key.LeftCtrl, 1.25), ("Win", Key.LWin, 1.25), ("Alt", Key.LeftAlt, 1.25),
            ("Space", Key.Space, 6.25), ("Alt", Key.RightAlt, 1.25), ("Win", Key.RWin, 1.25),
            ("Menu", Key.Apps, 1.25), ("Ctrl", Key.RightCtrl, 1.25));
        Row(caps, 5.5, 15.5, ("←", Key.Left, 1), ("↓", Key.Down, 1), ("→", Key.Right, 1));
        Row(caps, 5.5, 19, ("0", Key.NumPad0, 2), (".", Key.Decimal, 1));

        if (!full) caps.RemoveAll(c => c.X >= 19);
        return caps;
    }

    static void Row(List<KeyCap> caps, double y, double x, params (string Label, Key Key, double W)[] keys)
    {
        foreach (var (label, key, w) in keys)
        {
            caps.Add(new KeyCap(label, key, x, y, w));
            x += w;
        }
    }

    public bool HandleKey(Key key, bool down)
    {
        if (!_keyMap.TryGetValue(key, out var visuals)) return false;
        Log(visuals[0].Label, down);
        foreach (var v in visuals)
        {
            if (down)
            {
                SetPressed(v);
            }
            else
            {
                _testedKeys.Add(key);
                if (!v.Tested) { v.Tested = true; _testedCount++; }
                SetTested(v);
            }
        }
        if (!down) UpdateProgress();
        return true;
    }

    const int MaxLogLines = 500;

    void Log(string label, bool down)
    {
        LogBox.AppendText($"{DateTime.Now:HH:mm:ss.fff}  {label}  {(down ? "按下" : "松开")}\r\n");
        if (LogBox.LineCount > MaxLogLines)
        {
            int cut = LogBox.Text.IndexOf('\n');
            if (cut >= 0) LogBox.Text = LogBox.Text[(cut + 1)..];
        }
        LogBox.ScrollToEnd();
    }

    void SetPressed(KeyVisual v)
    {
        v.Bd.Background = (Brush)FindResource("AccentBrush");
        v.Bd.BorderBrush = (Brush)FindResource("AccentBrush");
        v.Tb.Foreground = Brushes.White;
    }

    void SetTested(KeyVisual v)
    {
        v.Bd.Background = Brushes.ForestGreen;
        v.Bd.BorderBrush = Brushes.ForestGreen;
        v.Tb.Foreground = Brushes.White;
    }

    void SetNormal(KeyVisual v)
    {
        v.Bd.SetResourceReference(Border.BackgroundProperty, "SurfaceBgBrush");
        v.Bd.SetResourceReference(Border.BorderBrushProperty, "BorderStrongBrush");
        v.Tb.SetResourceReference(TextBlock.ForegroundProperty, "TextBrush");
    }

    void UpdateProgress()
    {
        KeyProgress.Value = _testedCount;
        ProgressText.Text = $"已测试 {_testedCount} / {_all.Count}";
    }

    void ResetButton_Click(object sender, RoutedEventArgs e)
    {
        foreach (var v in _all)
        {
            v.Tested = false;
            SetNormal(v);
        }
        _testedKeys.Clear();
        _testedCount = 0;
        UpdateProgress();
        LogBox.Clear();
        SetPassed(false);
        PassButton.IsEnabled = true;
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
