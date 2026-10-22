// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.IO;
using System.Runtime.InteropServices;
using Windows.Win32;

namespace LibreHardwareMonitor.Interop;

internal static class NvidiaML
{
    private const string LinuxDllName = "nvidia-ml";
    private const string WindowsDllName = "nvml.dll";

    private static readonly object _syncRoot = new();

    private static FreeLibrarySafeHandle _windowsDll;

    private static WindowsNvmlGetHandleDelegate _windowsNvmlDeviceGetHandleByIndex;
    private static WindowsNvmlGetHandleByPciBusIdDelegate _windowsNvmlDeviceGetHandleByPciBusId;
    private static WindowsNvmlDeviceGetPcieThroughputDelegate _windowsNvmlDeviceGetPcieThroughputDelegate;
    private static WindowsNvmlDeviceGetPciInfo _windowsNvmlDeviceGetPciInfo;
    private static WindowsNvmlGetPowerUsageDelegate _windowsNvmlDeviceGetPowerUsage;
    private static WindowsNvmlDelegate _windowsNvmlInit;
    private static WindowsNvmlDelegate _windowsNvmlShutdown;

    public enum NvmlPcieUtilCounter
    {
        TxBytes = 0,
        RxBytes = 1
    }

    public enum NvmlReturn
    {
        
        Success = 0,

        Uninitialized = 1,

        InvalidArgument = 2,

        NotSupported = 3,

        NoPermission = 4,

        NotFound = 6,

        InsufficientSize = 7,

        InsufficientPower = 8,

        DriverNotLoaded = 9,

        TimeOut = 10,

        IRQIssue = 11,

        LibraryNotFound = 12,

        FunctionNotFound = 13,

        CorruptedInfoRom = 14,

        GpuIsLost = 15,

        ResetRequired = 16,

        OperatingSystem = 17,

        LibRmVersionMismatch = 18,

        InUse = 19,

        Unknown = 999
    }

    public static bool IsAvailable { get; private set; }

    public static bool Initialize()
    {
        lock (_syncRoot)
        {
            if (IsAvailable)
            {
                return true;
            }

            if (Software.OperatingSystem.IsUnix)
            {
                try
                {
                    IsAvailable = nvmlInit() == NvmlReturn.Success;
                }
                catch (DllNotFoundException)
                { }
                catch (EntryPointNotFoundException)
                {
                    try
                    {
                        IsAvailable = nvmlInitLegacy() == NvmlReturn.Success;
                    }
                    catch (EntryPointNotFoundException)
                    { }
                }
            }
            else if (IsNvmlCompatibleWindowsVersion())
            {
                
                _windowsDll = PInvoke.LoadLibrary(WindowsDllName);

                if (_windowsDll.IsInvalid)
                {
                    string programFilesDirectory = Environment.ExpandEnvironmentVariables("%ProgramW6432%");
                    string dllPath = Path.Combine(programFilesDirectory, @NVIDIA Corporation\NVSMI", WindowsDllName);

                    _windowsDll = PInvoke.LoadLibrary(dllPath);
                }

                IsAvailable = !_windowsDll.IsInvalid && InitialiseDelegates() && (_windowsNvmlInit() == NvmlReturn.Success);
            }

            return IsAvailable;
        }
    }

    private static bool IsNvmlCompatibleWindowsVersion()
    {
        return Software.OperatingSystem.Is64Bit && ((Environment.OSVersion.Version.Major > 6) || (Environment.OSVersion.Version.Major == 6 && Environment.OSVersion.Version.Minor >= 1));
    }

    private static bool InitialiseDelegates()
    {
        IntPtr nvmlInit = PInvoke.GetProcAddress(_windowsDll, "nvmlInit_v2");

        if (nvmlInit != IntPtr.Zero)
        {
            _windowsNvmlInit = Marshal.GetDelegateForFunctionPointer<WindowsNvmlDelegate>(nvmlInit);
        }
        else
        {
            nvmlInit = PInvoke.GetProcAddress(_windowsDll, "nvmlInit");
            if (nvmlInit != IntPtr.Zero)
                _windowsNvmlInit = Marshal.GetDelegateForFunctionPointer<WindowsNvmlDelegate>(nvmlInit);
            else
                return false;
        }

        IntPtr nvmlShutdown = PInvoke.GetProcAddress(_windowsDll, "nvmlShutdown");
        if (nvmlShutdown != IntPtr.Zero)
            _windowsNvmlShutdown = Marshal.GetDelegateForFunctionPointer<WindowsNvmlDelegate>(nvmlShutdown);
        else
            return false;

        IntPtr nvmlGetHandle = PInvoke.GetProcAddress(_windowsDll, "nvmlDeviceGetHandleByIndex_v2");
        if (nvmlGetHandle != IntPtr.Zero)
            _windowsNvmlDeviceGetHandleByIndex = Marshal.GetDelegateForFunctionPointer<WindowsNvmlGetHandleDelegate>(nvmlGetHandle);
        else
        {
            nvmlGetHandle = PInvoke.GetProcAddress(_windowsDll, "nvmlDeviceGetHandleByIndex");
            if (nvmlGetHandle != IntPtr.Zero)
                _windowsNvmlDeviceGetHandleByIndex = Marshal.GetDelegateForFunctionPointer<WindowsNvmlGetHandleDelegate>(nvmlGetHandle);
            else
                return false;
        }

        IntPtr nvmlGetPowerUsage = PInvoke.GetProcAddress(_windowsDll, "nvmlDeviceGetPowerUsage");
        if (nvmlGetPowerUsage != IntPtr.Zero)
            _windowsNvmlDeviceGetPowerUsage = Marshal.GetDelegateForFunctionPointer<WindowsNvmlGetPowerUsageDelegate>(nvmlGetPowerUsage);
        else
            return false;

        IntPtr nvmlGetPcieThroughput = PInvoke.GetProcAddress(_windowsDll, "nvmlDeviceGetPcieThroughput");
        if (nvmlGetPcieThroughput != IntPtr.Zero)
            _windowsNvmlDeviceGetPcieThroughputDelegate = Marshal.GetDelegateForFunctionPointer<WindowsNvmlDeviceGetPcieThroughputDelegate>(nvmlGetPcieThroughput);
        else
            return false;

        IntPtr nvmlGetHandlePciBus = PInvoke.GetProcAddress(_windowsDll, "nvmlDeviceGetHandleByPciBusId_v2");
        if (nvmlGetHandlePciBus != IntPtr.Zero)
            _windowsNvmlDeviceGetHandleByPciBusId = Marshal.GetDelegateForFunctionPointer<WindowsNvmlGetHandleByPciBusIdDelegate>(nvmlGetHandlePciBus);
        else
            return false;

        IntPtr nvmlDeviceGetPciInfo = PInvoke.GetProcAddress(_windowsDll, "nvmlDeviceGetPciInfo_v2");
        if (nvmlDeviceGetPciInfo != IntPtr.Zero)
            _windowsNvmlDeviceGetPciInfo = Marshal.GetDelegateForFunctionPointer<WindowsNvmlDeviceGetPciInfo>(nvmlDeviceGetPciInfo);
        else
            return false;

        return true;
    }

    public static void Close()
    {
        lock (_syncRoot)
        {
            if (IsAvailable)
            {
                if (Software.OperatingSystem.IsUnix)
                {
                    nvmlShutdown();
                }
                else if (!_windowsDll.IsInvalid)
                {
                    _windowsNvmlShutdown();
                    _windowsDll.Dispose();
                }

                IsAvailable = false;
            }
        }
    }

    public static NvmlDevice? NvmlDeviceGetHandleByIndex(int index)
    {
        if (IsAvailable)
        {
            NvmlDevice nvmlDevice;
            if (Software.OperatingSystem.IsUnix)
            {
                try
                {
                    if (nvmlDeviceGetHandleByIndex(index, out nvmlDevice) == NvmlReturn.Success)
                        return nvmlDevice;
                }
                catch (EntryPointNotFoundException)
                {
                    if (nvmlDeviceGetHandleByIndexLegacy(index, out nvmlDevice) == NvmlReturn.Success)
                        return nvmlDevice;
                }
            }
            else
            {
                try
                {
                    if (_windowsNvmlDeviceGetHandleByIndex(index, out nvmlDevice) == NvmlReturn.Success)
                        return nvmlDevice;
                }
                catch { }
            }
        }

        return null;
    }

    public static NvmlDevice? NvmlDeviceGetHandleByPciBusId(string pciBusId)
    {
        if (IsAvailable)
        {
            NvmlDevice nvmlDevice;
            if (Software.OperatingSystem.IsUnix)
            {
                if (nvmlDeviceGetHandleByPciBusId(pciBusId, out nvmlDevice) == NvmlReturn.Success)
                    return nvmlDevice;
            }
            else
            {
                try
                {
                    if (_windowsNvmlDeviceGetHandleByPciBusId(pciBusId, out nvmlDevice) == NvmlReturn.Success)
                        return nvmlDevice;
                }
                catch { }
            }
        }

        return null;
    }

    public static int? NvmlDeviceGetPowerUsage(NvmlDevice nvmlDevice)
    {
        if (IsAvailable)
        {
            int powerUsage;
            if (Software.OperatingSystem.IsUnix)
            {
                if (nvmlDeviceGetPowerUsage(nvmlDevice, out powerUsage) == NvmlReturn.Success)
                    return powerUsage;
            }
            else
            {
                try
                {
                    if (_windowsNvmlDeviceGetPowerUsage(nvmlDevice, out powerUsage) == NvmlReturn.Success)
                        return powerUsage;
                }
                catch { }
            }
        }

        return null;
    }

    public static uint? NvmlDeviceGetPcieThroughput(NvmlDevice nvmlDevice, NvmlPcieUtilCounter counter)
    {
        if (IsAvailable)
        {
            uint pcieThroughput;
            if (Software.OperatingSystem.IsUnix)
            {
                if (nvmlDeviceGetPcieThroughput(nvmlDevice, counter, out pcieThroughput) == NvmlReturn.Success)
                    return pcieThroughput;
            }
            else
            {
                try
                {
                    if (_windowsNvmlDeviceGetPcieThroughputDelegate(nvmlDevice, counter, out pcieThroughput) == NvmlReturn.Success)
                        return pcieThroughput;
                }
                catch { }
            }
        }

        return null;
    }

    public static NvmlPciInfo? NvmlDeviceGetPciInfo(NvmlDevice nvmlDevice)
    {
        if (IsAvailable)
        {
            var pci = new NvmlPciInfo();

            if (Software.OperatingSystem.IsUnix)
            {
                if (nvmlDeviceGetPciInfo(nvmlDevice, ref pci) == NvmlReturn.Success)
                    return pci;
            }
            else
            {
                try
                {
                    if (_windowsNvmlDeviceGetPciInfo(nvmlDevice, ref pci) == NvmlReturn.Success)
                        return pci;
                }
                catch { }

            }
        }

        return null;
    }

    [DllImport(LinuxDllName, EntryPoint = "nvmlInit_v2", ExactSpelling = true)]
    private static extern NvmlReturn nvmlInit();

    [DllImport(LinuxDllName, EntryPoint = "nvmlInit", ExactSpelling = true)]
    private static extern NvmlReturn nvmlInitLegacy();

    [DllImport(LinuxDllName, EntryPoint = "nvmlShutdown", ExactSpelling = true)]
    private static extern NvmlReturn nvmlShutdown();

    [DllImport(LinuxDllName, EntryPoint = "nvmlDeviceGetHandleByIndex_v2", ExactSpelling = true)]
    private static extern NvmlReturn nvmlDeviceGetHandleByIndex(int index, out NvmlDevice device);

    [DllImport(LinuxDllName, EntryPoint = "nvmlDeviceGetHandleByPciBusId_v2", ExactSpelling = true)]
    private static extern NvmlReturn nvmlDeviceGetHandleByPciBusId([MarshalAs(UnmanagedType.LPStr)] string pciBusId, out NvmlDevice device);

    [DllImport(LinuxDllName, EntryPoint = "nvmlDeviceGetHandleByIndex", ExactSpelling = true)]
    private static extern NvmlReturn nvmlDeviceGetHandleByIndexLegacy(int index, out NvmlDevice device);

    [DllImport(LinuxDllName, EntryPoint = "nvmlDeviceGetPowerUsage", ExactSpelling = true)]
    private static extern NvmlReturn nvmlDeviceGetPowerUsage(NvmlDevice device, out int power);

    [DllImport(LinuxDllName, EntryPoint = "nvmlDeviceGetPcieThroughput", ExactSpelling = true)]
    private static extern NvmlReturn nvmlDeviceGetPcieThroughput(NvmlDevice device, NvmlPcieUtilCounter counter, out uint value);

    [DllImport(LinuxDllName, EntryPoint = "nvmlDeviceGetPciInfo_v2")]
    private static extern NvmlReturn nvmlDeviceGetPciInfo(NvmlDevice device, ref NvmlPciInfo pci);

    private delegate NvmlReturn WindowsNvmlDelegate();

    private delegate NvmlReturn WindowsNvmlGetHandleDelegate(int index, out NvmlDevice device);

    private delegate NvmlReturn WindowsNvmlGetHandleByPciBusIdDelegate([MarshalAs(UnmanagedType.LPStr)] string pciBusId, out NvmlDevice device);

    private delegate NvmlReturn WindowsNvmlGetPowerUsageDelegate(NvmlDevice device, out int power);

    private delegate NvmlReturn WindowsNvmlDeviceGetPcieThroughputDelegate(NvmlDevice device, NvmlPcieUtilCounter counter, out uint value);

    private delegate NvmlReturn WindowsNvmlDeviceGetPciInfo(NvmlDevice device, ref NvmlPciInfo pci);

    [StructLayout(LayoutKind.Sequential)]
    public struct NvmlDevice
    {
        public IntPtr Handle;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NvmlPciInfo
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string busId;

        public uint domain;

        public uint bus;

        public uint device;

        public ushort pciVendorId;

        public ushort pciDeviceId;

        public uint pciSubSystemId;

        public uint reserved0;
        public uint reserved1;
        public uint reserved2;
        public uint reserved3;
    }
}
