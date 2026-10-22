// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Diagnostics;
using System.IO;
using LibreHardwareMonitor.Hardware;
using YanJi.Hardware;

namespace YanJi.Plugin.StressTest;

public static class SensorMonitorService
{
    
    public record Reading(double? CpuTemp, double? CpuPower, double? GpuTemp, double? GpuPower);

    public record SensorMap(string? CpuName, string? CpuTemp, string? CpuPower, string? GpuTemp, string? GpuPower, string? GpuName);

    public record GpuOption(int Index, string Name);

    public static event Action? SessionStarted;

    public static event Action<Reading>? Sampled;

    public static event Action<SensorMap>? SensorsResolved;

    public static event Action<IReadOnlyList<GpuOption>, int>? GpuListReady;

    public static event Action? GpuChanging;

    static readonly object _gate = new();
    static CancellationTokenSource? _cts;
    static CancellationToken _sessionToken = new(true);
    static Task? _loop;
    static bool _requested;
    public static event Action<string>? Failed;
    
    static IHardware? _cpu, _gpu;
    static ISensor? _cpuTemp, _cpuPower, _gpuTemp, _gpuPower;
    static IReadOnlyList<GpuOption> _gpuOptions = [];
    static int _gpuIndex = -1;        
    static int _pendingGpuIndex = -1; 

    public static bool IsRunning { get { lock (_gate) return _loop != null; } }
    
    public static CancellationToken SessionToken { get { lock (_gate) return _sessionToken; } }

    public static void Start()
    {
        lock (_gate)
        {
            _requested = true;
            if (IsRunning) return;
            StartLoop();
        }
    }

    static void StartLoop()
    {
        var source = new CancellationTokenSource();
        _cts = source;
        var token = _sessionToken = source.Token;
        var loop = Task.Run(() => Loop(token));
        _loop = loop;
        _ = loop.ContinueWith(_ =>
        {
            lock (_gate)
            {
                
                bool restart = _requested && source.IsCancellationRequested;
                try { source.Cancel(); }
                catch (Exception ex) { Trace.WriteLine("传感器会话取消回调失败：" + ex); }
                finally
                {
                    _loop = null;
                    _cts = null;
                    source.Dispose();
                }
                if (restart) StartLoop();
            }
        }, TaskScheduler.Default);
    }

    public static void Stop()
    {
        lock (_gate)
        {
            _requested = false;
            _cts?.Cancel();
        }
    }

    public static void SetGpu(int index)
    {
        lock (_gate)
        {
            if (_gpuOptions.Count == 0 || index < 0 || index >= _gpuOptions.Count || index == _gpuIndex)
                return;
            _pendingGpuIndex = index;
            _gpuIndex = index;
        }
        Notify(GpuChanging, handler => handler(), nameof(GpuChanging));
    }

    static void Notify<T>(T? handlers, Action<T> invoke, string eventName) where T : Delegate
    {
        if (handlers == null) return;
        foreach (T handler in handlers.GetInvocationList())
        {
            try { invoke(handler); }
            catch (Exception ex) { Trace.WriteLine($"传感器监控 {eventName} 订阅回调失败：{ex}"); }
        }
    }

    static void Loop(CancellationToken ct)
    {
        var computer = new Computer { IsCpuEnabled = true, IsGpuEnabled = true };
        try
        {
            ct.ThrowIfCancellationRequested();
            Notify(SessionStarted, handler => handler(), nameof(SessionStarted));
            EnsurePawnIoService(); 
            ct.ThrowIfCancellationRequested();
            LhmComputerAccess.Run(computer, _ => Monitor(computer, ct), ct);
        }
        catch (OperationCanceledException) when (ct.IsCancellationRequested) { }
        catch (Exception ex)
        {
            Notify(Failed, handler => handler("传感器监控失败：" + ex.Message), nameof(Failed));
        }
        finally
        {
            lock (_gate)
            {
                _cpu = _gpu = null;
                _cpuTemp = _cpuPower = _gpuTemp = _gpuPower = null;
                _gpuOptions = [];
                _gpuIndex = _pendingGpuIndex = -1;
            }
        }
    }

    static void Monitor(Computer computer, CancellationToken ct)
    {
            
            var gpus = computer.Hardware.Where(h =>
                h.HardwareType is HardwareType.GpuAmd or HardwareType.GpuNvidia or HardwareType.GpuIntel &&
                h.Sensors.Any(s => s.SensorType is SensorType.Temperature or SensorType.Power)).ToList();
            IReadOnlyList<GpuOption> options;
            int defaultIndex;
            lock (_gate)
            {
                _cpu = computer.Hardware.FirstOrDefault(h => h.HardwareType == HardwareType.Cpu);
                _cpuTemp = Find(_cpu, SensorType.Temperature, "Package");
                _cpuPower = Find(_cpu, SensorType.Power, "Package");
                _gpuOptions = options = gpus.Select((h, i) => new GpuOption(i, h.Name)).ToList();
                _gpuIndex = defaultIndex = SelectDefaultGpu(
                    gpus.Select(h => (h.HardwareType, h.Name)).ToList());
            }
            ApplyGpu(gpus.Count > 0 ? gpus[defaultIndex] : null);
            Notify(GpuListReady, handler => handler(options, defaultIndex), nameof(GpuListReady));

            while (!ct.IsCancellationRequested)
            {
                
                int pending;
                lock (_gate) { pending = _pendingGpuIndex; _pendingGpuIndex = -1; }
                if (pending >= 0 && pending < gpus.Count)
                    ApplyGpu(gpus[pending]);

                IHardware? cpu, gpu;
                ISensor? ct2, cp, gt, gp;
                lock (_gate) { cpu = _cpu; gpu = _gpu; ct2 = _cpuTemp; cp = _cpuPower; gt = _gpuTemp; gp = _gpuPower; }
                bool cpuUpdated = Update(cpu);
                bool gpuUpdated = Update(gpu);
                ct.ThrowIfCancellationRequested();
                
                var reading = new Reading(
                    cpuUpdated ? Valid(ct2) : null, cpuUpdated ? Valid(cp) : null,
                    gpuUpdated ? Valid(gt) : null, gpuUpdated ? Valid(gp) : null);
                Notify(Sampled, handler => handler(reading), nameof(Sampled));
                ct.WaitHandle.WaitOne(2000); 
            }
    }

    static void ApplyGpu(IHardware? gpu)
    {
        var gt = Find(gpu, SensorType.Temperature, "GPU Core", "GPU");
        var gp = Find(gpu, SensorType.Power, "Package", "Board", "GPU Core", "GPU Power", "GPU");
        string? cpuName;
        ISensor? ct, cp;
        lock (_gate)
        {
            _gpu = gpu;
            _gpuTemp = gt;
            _gpuPower = gp;
            cpuName = _cpu?.Name;
            ct = _cpuTemp;
            cp = _cpuPower;
        }
        var map = new SensorMap(cpuName, ct?.Name, cp?.Name, gt?.Name, gp?.Name, gpu?.Name);
        Notify(SensorsResolved, handler => handler(map), nameof(SensorsResolved));
    }

    internal static bool IsDiscreteGpu(HardwareType type, string name) => type switch
    {
        HardwareType.GpuNvidia => true,
        HardwareType.GpuAmd =>
            name.Contains("RX", StringComparison.OrdinalIgnoreCase) ||
            !name.TrimEnd().EndsWith("Graphics", StringComparison.OrdinalIgnoreCase),
        HardwareType.GpuIntel => name.Contains("Arc", StringComparison.OrdinalIgnoreCase),
        _ => false,
    };

    internal static int SelectDefaultGpu(IReadOnlyList<(HardwareType Type, string Name)> gpus)
    {
        for (int i = 0; i < gpus.Count; i++)
            if (IsDiscreteGpu(gpus[i].Type, gpus[i].Name)) return i;
        return 0;
    }

    public static string InstallerExe =>
        Path.Combine(AppContext.BaseDirectory, "Tools", "pawnio", "PawnIO_setup.exe");

    public static bool HasInstaller => File.Exists(InstallerExe);

    public static bool AllowAutomaticDriverStart { get; set; } = true;

    public static bool IsPawnIoInstalled()
    {
        try
        {
            using var key = Microsoft.Win32.Registry.LocalMachine
                .OpenSubKey(@SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\PawnIO");
            return key?.GetValue("DisplayVersion") != null;
        }
        catch { return false; }
    }

    public static bool InstallDriver()
    {
        try
        {
            using var p = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
            {
                FileName = InstallerExe,
                Arguments = "-install -silent",
                WorkingDirectory = Path.GetDirectoryName(InstallerExe)!,
                UseShellExecute = false,
                CreateNoWindow = true,
            })!;
            if (!p.WaitForExit(120_000)) { try { p.Kill(); } catch { } return false; }
            return p.ExitCode == 0 && IsPawnIoInstalled();
        }
        catch { return false; }
    }

    public static void Restart()
    {
        Stop();
        Start();
    }

    static void EnsurePawnIoService()
    {
        if (!IsPawnIoInstalled()) return;
        try
        {
            
            using var searcher = new System.Management.ManagementObjectSearcher(
                "SELECT State FROM Win32_SystemDriver WHERE Name='PawnIO'");
            using var drivers = searcher.Get();
            foreach (System.Management.ManagementObject driver in drivers)
            {
                using (driver)
                    if (string.Equals(driver["State"] as string, "Running", StringComparison.OrdinalIgnoreCase))
                        return;
            }

            if (!AllowAutomaticDriverStart)
            {
                Notify(Failed, handler => handler("PawnIO 未运行，当前验收模式禁止启动驱动服务。"), nameof(Failed));
                return;
            }
            using var p = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
            {
                FileName = "sc.exe",
                Arguments = "start PawnIO",
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
            })!;
            p.WaitForExit(15_000);
        }
        catch
        {
            
            Notify(Failed, handler => handler("无法确认或启动 PawnIO 服务；传感器将按实际可用状态显示。"), nameof(Failed));
        }
    }

    static bool Update(IHardware? hw)
    {
        if (hw == null) return false;
        try { hw.Update(); return true; }
        catch (Exception ex)
        {
            Trace.WriteLine("传感器硬件更新失败：" + ex);
            return false;
        }
    }

    static double? Valid(ISensor? s) =>
        s?.Value is float v && float.IsFinite(v) && v > 0 ? v : null;

    static ISensor? Find(IHardware? hw, SensorType type, params string[] keywords)
    {
        if (hw == null) return null;
        var all = hw.Sensors.Where(s => s.SensorType == type).ToList();
        foreach (string k in keywords)
        {
            var hit = all.FirstOrDefault(s => s.Name.Contains(k, StringComparison.OrdinalIgnoreCase));
            if (hit != null) return hit;
        }
        return all.FirstOrDefault();
    }
}
