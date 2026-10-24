// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using Windows.Wdk;
using Windows.Wdk.System.SystemInformation;
using Windows.Win32.Foundation;
using Windows.Win32.System.WindowsProgramming;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Devices;

internal class CpuLoad
{
    private readonly double[] _threadLoads;
    private long[] _idleTimes;
    private double _totalLoad;
    private long[] _totalTimes;

    public CpuLoad(CpuId[][] cpuid)
    {
        _threadLoads = new double[cpuid.Sum(x => x.Length)];
        _totalLoad = 0;

        try
        {
            GetTimes(out _idleTimes, out _totalTimes);
        }
        catch (Exception)
        {
            _idleTimes = null;
            _totalTimes = null;
        }

        if (_idleTimes != null)
            IsAvailable = true;
    }

    public bool IsAvailable { get; }

    private static bool GetTimes(out long[] idle, out long[] total)
    {
        return !SysCore.Platform.OperatingSystem.IsUnix ? GetWindowsTimes(out idle, out total) : GetUnixTimes(out idle, out total);
    }

    private static unsafe bool GetWindowsTimes(out long[] idle, out long[] total)
    {
        idle = null;
        total = null;

        int size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * 64;
        uint returnSize = 0;
        IntPtr handle = Marshal.AllocHGlobal(size);
        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[] perfInformation;

        while (true)
        {
            
            NTSTATUS status = PInvoke.NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS.SystemProcessorPerformanceInformation, (void*)handle, (uint)size, &returnSize);
            if (status == NTSTATUS.STATUS_INFO_LENGTH_MISMATCH)
            {
                size = (int)returnSize;
                handle = Marshal.ReAllocHGlobal(handle, new IntPtr(size));
            }
            else
            {
                if (status != NTSTATUS.STATUS_SUCCESS)
                {
                    Marshal.FreeHGlobal(handle);
                    return false;
                }

                SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* perfInformationPtr = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*)handle;

                int perfItemsCount = (int)(returnSize / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
                perfInformation = new SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[perfItemsCount];
                for (int i = 0; i < perfItemsCount; i++)
                {
                    perfInformation[i] = perfInformationPtr[i];
                }

                Marshal.FreeHGlobal(handle);
                break;
            }
        }

        idle = new long[perfInformation.Length];
        total = new long[perfInformation.Length];
        for (int i = 0; i < total.Length; i++)
        {
            idle[i] = perfInformation[i].IdleTime;
            total[i] = perfInformation[i].KernelTime + perfInformation[i].UserTime;
        }

        return true;
    }

    private static bool GetUnixTimes(out long[] idle, out long[] total)
    {
        idle = null;
        total = null;

        List<long> idleList = [];
        List<long> totalList = [];

        if (!File.Exists("/proc/stat"))
            return false;

        string[] cpuInfos = File.ReadAllLines("/proc/stat");

        foreach (string cpuInfo in cpuInfos.Where(s => s.StartsWith("cpu") && s.Length > 3 && s[3] != ' '))
        {
            string[] overall = cpuInfo.Split([' '], StringSplitOptions.RemoveEmptyEntries);

            try
            {
                
                idleList.Add(long.Parse(overall[4]));
            }
            catch
            {
                
            }

            long currentTotal = 0;
            foreach (string item in overall.Skip(1))
            {
                try
                {
                    currentTotal += long.Parse(item);
                }
                catch
                {
                    
                }
            }

            totalList.Add(currentTotal);
        }

        idle = idleList.ToArray();
        total = totalList.ToArray();
        return true;
    }

    public double GetTotalLoad()
    {
        return _totalLoad;
    }

    public double GetThreadLoad(int thread)
    {
        return _threadLoads[thread];
    }

    public void Update()
    {
        if (_idleTimes == null || !GetTimes(out long[] newIdleTimes, out long[] newTotalTimes))
            return;

        int minDiff = SysCore.Platform.OperatingSystem.IsUnix ? 100 : 100000;
        for (int i = 0; i < Math.Min(newTotalTimes.Length, _totalTimes.Length); i++)
        {
            if (newTotalTimes[i] - _totalTimes[i] < minDiff)
                return;
        }

        if (newIdleTimes == null)
            return;

        double total = 0;
        int count = 0;
        for (int i = 0; i < _threadLoads.Length && i < _idleTimes.Length && i < newIdleTimes.Length; i++)
        {
            double idle = (newIdleTimes[i] - _idleTimes[i]) / (double)(newTotalTimes[i] - _totalTimes[i]);
            idle = idle < 0.0 ? 0.0 : idle;
            idle = idle > 1.0 ? 1.0 : idle;

            double load = 100.0 * (1.0 - Math.Min(idle, 1.0));
            _threadLoads[i] = Math.Round(load, 2);
            total += idle;
            count++;
        }

        if (count > 0)
        {
            total = 1.0 - (total / count);
            total = total < 0.0 ? 0.0 : total;
            total = total > 1.0 ? 1.0 : total;
        }
        else
        {
            total = 0;
        }

        _totalLoad = Math.Round(total * 100.0, 2);
        _totalTimes = newTotalTimes;
        _idleTimes = newIdleTimes;
    }
}
