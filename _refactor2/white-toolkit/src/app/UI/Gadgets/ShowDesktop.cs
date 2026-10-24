// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace SysCore.Windows.Forms.UI;

public class ShowDesktop : IDisposable
{
    private readonly NativeWindow _referenceWindow;
    private readonly string _referenceWindowCaption = "SysCoreShowDesktopReferenceWindow";
    private readonly System.Threading.Timer _timer;
    private bool _showDesktop;

    private ShowDesktop()
    {
        
        _referenceWindow = new NativeWindow();

        CreateParams cp = new CreateParams { ExStyle = GadgetWindow.WS_EX_TOOLWINDOW, Caption = _referenceWindowCaption };
        _referenceWindow.CreateHandle(cp);
        NativeMethods.SetWindowPos(_referenceWindow.Handle,
                                   GadgetWindow.HWND_BOTTOM,
                                   0,
                                   0,
                                   0,
                                   0,
                                   GadgetWindow.SWP_NOMOVE |
                                   GadgetWindow.SWP_NOSIZE |
                                   GadgetWindow.SWP_NOACTIVATE |
                                   GadgetWindow.SWP_NOSENDCHANGING);

        _timer = new System.Threading.Timer(OnTimer, null, System.Threading.Timeout.Infinite, System.Threading.Timeout.Infinite);
    }

    public delegate void ShowDesktopChangedEventHandler(bool showDesktop);

    private event ShowDesktopChangedEventHandler ShowDesktopChangedEvent;

    public event ShowDesktopChangedEventHandler ShowDesktopChanged
    {
        add
        {
            
            if (ShowDesktopChangedEvent == null)
                StartTimer();

            ShowDesktopChangedEvent += value;
        }
        remove
        {
            ShowDesktopChangedEvent -= value;
            
            if (ShowDesktopChangedEvent == null)
                StopTimer();
        }
    }

    public static ShowDesktop Instance { get; } = new ShowDesktop();

    public void Dispose()
    {
        _timer?.Dispose();
        _referenceWindow.ReleaseHandle();
    }

    private void StartTimer()
    {
        _timer.Change(0, 200);
    }

    private void StopTimer()
    {
        _timer.Change(System.Threading.Timeout.Infinite, System.Threading.Timeout.Infinite);
    }

    private IntPtr GetDesktopWorkerWindow()
    {
        IntPtr shellWindow = NativeMethods.GetShellWindow();
        if (shellWindow == IntPtr.Zero)
            return IntPtr.Zero;

        NativeMethods.GetWindowThreadProcessId(shellWindow, out int shellId);

        IntPtr workerWindow = IntPtr.Zero;
        while ((workerWindow = NativeMethods.FindWindowEx(IntPtr.Zero, workerWindow, "WorkerW", null)) != IntPtr.Zero)
        {
            NativeMethods.GetWindowThreadProcessId(workerWindow, out int workerId);
            if (workerId == shellId)
            {
                IntPtr window = NativeMethods.FindWindowEx(workerWindow, IntPtr.Zero, "SHELLDLL_DefView", null);
                if (window != IntPtr.Zero)
                {
                    IntPtr desktopWindow = NativeMethods.FindWindowEx(window, IntPtr.Zero, "SysListView32", null);
                    if (desktopWindow != IntPtr.Zero)
                        return workerWindow;
                }
            }
        }

        return IntPtr.Zero;
    }

    private void OnTimer(object state)
    {
        bool showDesktopDetected;

        IntPtr workerWindow = GetDesktopWorkerWindow();
        if (workerWindow != IntPtr.Zero)
        {
            
            IntPtr reference = NativeMethods.FindWindowEx(IntPtr.Zero, workerWindow, null, _referenceWindowCaption);
            showDesktopDetected = reference != IntPtr.Zero;
        }
        else
        {
            
            showDesktopDetected = false;
        }

        if (_showDesktop != showDesktopDetected)
        {
            _showDesktop = showDesktopDetected;
            ShowDesktopChangedEvent?.Invoke(_showDesktop);
        }
    }

    private static class NativeMethods
    {
        private const string USER = "user32.dll";

        [DllImport(USER, CallingConvention = CallingConvention.Winapi)]
        public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        [DllImport(USER, CallingConvention = CallingConvention.Winapi)]
        public static extern IntPtr FindWindowEx(IntPtr hwndParent, IntPtr hwndChildAfter, string lpszClass, string lpszWindow);

        [DllImport(USER, CallingConvention = CallingConvention.Winapi)]
        public static extern IntPtr GetShellWindow();

        [DllImport(USER, CallingConvention = CallingConvention.Winapi)]
        public static extern int GetWindowThreadProcessId(IntPtr hWnd, out int processId);
    }
}