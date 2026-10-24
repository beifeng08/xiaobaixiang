// Copyright (c) 2026 OmniSensor

using System.Diagnostics;
using System.IO;
using System.Management;
using System.Runtime.InteropServices;
using System.Windows;

namespace YanJi.Plugin.StressTest;

public static class StressTestService
{
    
    public enum StressMode { Cpu, Gpu, Both }

    public enum FftPreset { Blend, SmallHot, LargeMem }

    public record FurMarkSettings(int Width, int Height, bool Fullscreen, int Msaa, bool ShowWindow = true);

    public record SysCorePrimeSettings(int Threads, FftPreset Fft, bool ShowWindow = true);

    static readonly object _gate = new();
    static Process? _xiaobaiprime;
    static Process? _furmark;
    static DateTime? _xiaobaiprimeStarted, _furmarkStarted; 
    static bool _stopping;      
    static IntPtr _job; 
    static bool _exitHooked;

    public static DateTime? StartedAt { get; private set; }

    public static bool IsRunning
    {
        get { lock (_gate) return Alive(_xiaobaiprime) || Alive(_furmark); }
    }

    public static event Action? StateChanged;

    public static event Action<string>? Logged;

    static void Log(string msg)
    {
        try { Logged?.Invoke($"{DateTime.Now:HH:mm:ss}  {msg}"); } catch {  }
    }

    public static string SysCorePrimeExe => Path.Combine(AppContext.BaseDirectory, "Tools", "xiaobaiprime", "xiaobaiprime.exe");
    public static string FurMarkExe => Path.Combine(AppContext.BaseDirectory, "Tools", "furmark", "FurMark.exe");
    public static bool HasSysCorePrime => File.Exists(SysCorePrimeExe);
    public static bool HasFurMark => File.Exists(FurMarkExe);

    static (int Physical, int Logical)? _cores;

    public static (int Physical, int Logical) CoreCounts
    {
        get
        {
            if (_cores == null)
            {
                int phys = 0, logical = 0;
                try
                {
                    using var searcher = new ManagementObjectSearcher(@"root\cimv2",
                        "SELECT NumberOfCores, NumberOfLogicalProcessors FROM Win32_Processor");
                    foreach (ManagementObject mo in searcher.Get())
                    {
                        phys += Convert.ToInt32(mo["NumberOfCores"]);
                        logical += Convert.ToInt32(mo["NumberOfLogicalProcessors"]);
                    }
                }
                catch {  }
                int fallback = Environment.ProcessorCount;
                _cores = (phys > 0 ? phys : fallback, logical > 0 ? logical : fallback);
            }
            return _cores.Value;
        }
    }

    public static long TotalMemoryMB
    {
        get
        {
            try
            {
                using var searcher = new ManagementObjectSearcher(@"root\cimv2",
                    "SELECT TotalPhysicalMemory FROM Win32_ComputerSystem");
                foreach (ManagementObject mo in searcher.Get())
                    return Convert.ToInt64(mo["TotalPhysicalMemory"]) / 1048576;
            }
            catch {  }
            return 8192;
        }
    }

    public static void Start(StressMode mode, SysCorePrimeSettings p95, FurMarkSettings fm)
    {
        if (mode is StressMode.Cpu or StressMode.Both) EnsureTool(SysCorePrimeExe);
        if (mode is StressMode.Gpu or StressMode.Both) EnsureTool(FurMarkExe);

        lock (_gate)
        {
            StopLocked();
            HookExitLocked();
            EnsureJobLocked();

            Log($"开始烤机：{ModeName(mode)}");
            if (mode is StressMode.Cpu or StressMode.Both)
            {
                _xiaobaiprime = WatchExit(StartSysCorePrime(p95), "SysCorePrime");
                _xiaobaiprimeStarted = DateTime.Now;
                Log($"Prime95 已启动（{p95.Threads} 线程，{FftName(p95.Fft)}，界面{(p95.ShowWindow ? "显示" : "隐藏")}）");
            }
            if (mode is StressMode.Gpu or StressMode.Both)
            {
                _furmark = WatchExit(StartFurMark(fm), "FurMark");
                _furmarkStarted = DateTime.Now;
                Log($"FurMark 已启动（{fm.Width}×{fm.Height}，{MsaaName(fm.Msaa)}，界面{(fm.ShowWindow ? "显示" : "隐藏")}）");
            }
            StartedAt = DateTime.Now;
        }
        StateChanged?.Invoke();
    }

    static string ModeName(StressMode mode) => mode switch
    {
        StressMode.Cpu => "CPU 单烤",
        StressMode.Gpu => "GPU 单烤",
        _ => "双烤",
    };

    static string FftName(FftPreset fft) => fft switch
    {
        FftPreset.SmallHot => "小 FFT（高热）",
        FftPreset.LargeMem => "大 FFT（压内存）",
        _ => "均衡 Blend",
    };

    static string MsaaName(int msaa) => msaa > 0 ? $"{msaa}x MSAA" : "无 MSAA";

    public static void Stop()
    {
        lock (_gate)
        {
            StopLocked();
            StartedAt = null;
        }
        StateChanged?.Invoke();
    }

    static void StopLocked()
    {
        _stopping = true; 
        try
        {
            KillTree(ref _xiaobaiprime, "SysCorePrime", _xiaobaiprimeStarted);
            KillTree(ref _furmark, "FurMark", _furmarkStarted);
            _xiaobaiprimeStarted = _furmarkStarted = null;
        }
        finally { _stopping = false; }
    }

    static void KillTree(ref Process? proc, string name, DateTime? started)
    {
        var p = proc;
        proc = null;
        if (p == null) return;
        bool wasAlive = false;
        try { wasAlive = !p.HasExited; } catch {  }
        try { if (wasAlive) p.Kill(entireProcessTree: true); } catch {  }
        try { p.WaitForExit(5000); } catch {  }
        p.Dispose();
        if (wasAlive && started is DateTime t0)
            Log($"{name} 已停止（运行 {FormatDuration(DateTime.Now - t0)}）");
    }

    static Process WatchExit(Process p, string displayName)
    {
        p.EnableRaisingEvents = true;
        p.Exited += (_, _) => OnProcessExited(p, displayName);
        return p;
    }

    static void OnProcessExited(Process p, string name)
    {
        int code;
        try { code = p.ExitCode; } catch { code = -1; }
        bool expected;
        lock (_gate)
        {
            expected = _stopping;
            if (ReferenceEquals(_xiaobaiprime, p)) { _xiaobaiprime = null; _xiaobaiprimeStarted = null; }
            else if (ReferenceEquals(_furmark, p)) { _furmark = null; _furmarkStarted = null; }
            if (!expected && !Alive(_xiaobaiprime) && !Alive(_furmark)) StartedAt = null;
        }
        if (!expected)
        {
            Log($"{name} 意外退出（代码 {code}）");
            StateChanged?.Invoke();
        }
    }

    static string FormatDuration(TimeSpan s) =>
        s.TotalHours >= 1 ? $"{(int)s.TotalHours} 小时 {s.Minutes} 分"
        : s.TotalMinutes >= 1 ? $"{(int)s.TotalMinutes} 分 {s.Seconds} 秒"
        : $"{s.Seconds} 秒";

    static bool Alive(Process? p)
    {
        if (p == null) return false;
        try { return !p.HasExited; } catch { return false; }
    }

    static void EnsureTool(string path)
    {
        if (!File.Exists(path))
            throw new FileNotFoundException("烤机工具缺失：" + path, path);
    }

    static Process StartSysCorePrime(SysCorePrimeSettings settings)
    {
        string dir = Path.GetDirectoryName(SysCorePrimeExe)!;
        WritePrimeTxt(Path.Combine(dir, "prime.txt"), settings);
        var p = Process.Start(new ProcessStartInfo
        {
            FileName = SysCorePrimeExe,
            Arguments = "-t",
            WorkingDirectory = dir,
            UseShellExecute = false,
            
            WindowStyle = settings.ShowWindow ? ProcessWindowStyle.Normal : ProcessWindowStyle.Hidden,
        })!;
        AssignToJob(p);
        return p;
    }

    static void WritePrimeTxt(string path, SysCorePrimeSettings settings)
    {
        var (phys, logical) = CoreCounts;
        int threads = Math.Clamp(settings.Threads, 1, logical);
        
        int cores = Math.Min(threads, phys);
        bool ht = threads > phys;

        long mem70 = Math.Min((long)(TotalMemoryMB * 0.7), (long)(TotalMemoryMB * 0.9));
        var (minFft, maxFft, memMB) = settings.Fft switch
        {
            FftPreset.SmallHot => (4, 64, 8),
            FftPreset.LargeMem => (1024, 4096, (int)Math.Max(1024, mem70)),
            _ => (4, 4096, (int)Math.Max(1024, mem70)),
        };

        File.WriteAllLines(path,
        [
            "; 由「笔吧验机」散热测试模块生成（xiaobaiprime 启动时读取，勿手工编辑）",
            "StressTester=1",
            "UsePrimenet=0",
            $"TortureCores={cores}",
            $"TortureHyperthreading={(ht ? 1 : 0)}",
            $"MinTortureFFT={minFft}",
            $"MaxTortureFFT={maxFft}",
            $"TortureMem={memMB}",
            "TortureTime=3",
            
            "[Windows]",
            $"HideIcon={(settings.ShowWindow ? 0 : 1)}",
            "TrayIcon=0",
        ]);
    }

    static Process StartFurMark(FurMarkSettings settings)
    {
        string dir = Path.GetDirectoryName(FurMarkExe)!;
        string args = $"--width {settings.Width} --height {settings.Height} --msaa {settings.Msaa}"
                    + (settings.Fullscreen ? " --fullscreen" : "")
                    + (settings.ShowWindow ? "" : " --hidden"); 
        var p = Process.Start(new ProcessStartInfo
        {
            FileName = FurMarkExe,
            Arguments = args,
            WorkingDirectory = dir,
            UseShellExecute = false,
        })!;
        AssignToJob(p);
        return p;
    }

    static void KillStaleByPath(string exeName, string expectPath)
    {
        foreach (var p in Process.GetProcessesByName(exeName))
        {
            try
            {
                if (string.Equals(p.MainModule?.FileName, expectPath, StringComparison.OrdinalIgnoreCase))
                {
                    p.Kill(entireProcessTree: true);
                    p.WaitForExit(3000);
                }
            }
            catch {  }
            finally { p.Dispose(); }
        }
    }

    static void HookExitLocked()
    {
        if (_exitHooked) return;
        _exitHooked = true;
        
        AppDomain.CurrentDomain.ProcessExit += (_, _) => StopLocked();
        if (Application.Current != null)
            Application.Current.Exit += (_, _) => StopLocked();
    }

    static void EnsureJobLocked()
    {
        if (_job == IntPtr.Zero)
        {
            _job = CreateJobObject(IntPtr.Zero, null);
            if (_job == IntPtr.Zero) return; 
            var info = new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
            info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(_job, JobObjectInfoType.ExtendedLimitInformation,
                ref info, (uint)Marshal.SizeOf<JOBOBJECT_EXTENDED_LIMIT_INFORMATION>());
        }
        
        KillStaleByPath("xiaobaiprime", SysCorePrimeExe);
        KillStaleByPath("FurMark", FurMarkExe);
    }

    static void AssignToJob(Process p)
    {
        if (_job == IntPtr.Zero) return;
        try { AssignProcessToJobObject(_job, p.Handle); } catch {  }
    }

    const int JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x2000;

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern IntPtr CreateJobObject(IntPtr lpJobAttributes, string? lpName);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool SetInformationJobObject(IntPtr hJob, int infoClass,
        ref JOBOBJECT_EXTENDED_LIMIT_INFORMATION lpJobObjectInfo, uint cbJobObjectInfoLength);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool AssignProcessToJobObject(IntPtr hJob, IntPtr hProcess);

    static class JobObjectInfoType { public const int ExtendedLimitInformation = 9; }

    [StructLayout(LayoutKind.Sequential)]
    struct IO_COUNTERS
    {
        public ulong ReadOperationCount, WriteOperationCount, OtherOperationCount;
        public ulong ReadTransferCount, WriteTransferCount, OtherTransferCount;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct JOBOBJECT_BASIC_LIMIT_INFORMATION
    {
        public long PerProcessUserTimeLimit, PerJobUserTimeLimit;
        public int LimitFlags;
        public UIntPtr MinimumWorkingSetSize, MaximumWorkingSetSize;
        public int ActiveProcessLimit;
        public UIntPtr Affinity;
        public int PriorityClass, SchedulingClass;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct JOBOBJECT_EXTENDED_LIMIT_INFORMATION
    {
        public JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
        public IO_COUNTERS IoInfo;
        public UIntPtr ProcessMemoryLimit, JobMemoryLimit, PeakProcessMemoryUsed, PeakJobMemoryUsed;
    }
}
