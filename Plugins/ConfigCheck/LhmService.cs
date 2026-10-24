// Copyright (c) 2026 OmniSensor

using SysCore.Hardware;
using YanJi.Hardware;

namespace YanJi.Plugin.ConfigCheck;

public static class SysCoreService
{
    
    public record SysCoreGpu(string Name, double? VramMB);

    public record SysCoreDisk(string Name, ulong? SizeBytes, int? HealthPercent, long? PowerOnHours, uint? DeviceNumber, long? MediaErrors);

    public record SysCoreDimm(string Name, double? CapacityGB);

    public record SysCoreBattery(string Name, double? DesignedMWh, double? FullChargedMWh);

    public class Snapshot
    {
        public List<string> Cpus { get; } = [];
        public List<SysCoreGpu> Gpus { get; } = [];
        public List<SysCoreDisk> Disks { get; } = [];
        public List<SysCoreDimm> Dimms { get; } = [];
        public List<SysCoreBattery> Batteries { get; } = [];
        
        public double? MemoryTotalGB { get; set; }
        
        public SMBios? Smbios { get; set; }
        public List<string> Errors { get; } = [];
        
        public string? Error { get; set; }
    }

    public static Snapshot Collect(CancellationToken cancellationToken = default)
    {
        var snap = new Snapshot();
        var computer = new Computer
        {
            IsCpuEnabled = true,
            IsGpuEnabled = true,
            IsMemoryEnabled = true,
            IsStorageEnabled = true,
            IsBatteryEnabled = true,
        };
        try
        {
            SysCoreComputerAccess.Run(computer, opened =>
            {
                snap.Smbios = opened.SMBios;
                foreach (IHardware hw in opened.Hardware)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    try { CollectHardware(hw, snap, cancellationToken); }
                    catch (Exception ex) when (ex is not OperationCanceledException)
                    { snap.Errors.Add($"{hw.Name}：{ex.Message}"); }
                }
            }, cancellationToken);
        }
        catch (Exception ex) when (ex is not OperationCanceledException)
        {
            snap.Error = ex.Message;
        }
        return snap;
    }

    static void CollectHardware(IHardware hw, Snapshot snap, CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        
        try { hw.Update(); }
        catch (Exception ex) when (ex is not OperationCanceledException)
        { snap.Errors.Add($"{hw.Name} 更新失败，使用已取得数据：{ex.Message}"); }
        token.ThrowIfCancellationRequested();

        switch (hw.HardwareType)
        {
            case HardwareType.Cpu:
                if (!string.IsNullOrWhiteSpace(hw.Name))
                    snap.Cpus.Add(hw.Name.Trim());
                break;

            case HardwareType.GpuNvidia or HardwareType.GpuAmd or HardwareType.GpuIntel:
                snap.Gpus.Add(new SysCoreGpu(hw.Name.Trim(), FindSensorMB(hw, "GPU Memory Total")));
                break;

            case HardwareType.Memory:
                CollectMemory(hw, snap);
                break;

            case HardwareType.Storage:
                CollectStorage(hw, snap);
                break;

            case HardwareType.Battery:
                
                snap.Batteries.Add(new SysCoreBattery(hw.Name.Trim(),
                    FindSensorMWh(hw, "Designed Capacity"), FindSensorMWh(hw, "Fully-Charged Capacity")));
                break;
        }

        foreach (IHardware sub in hw.SubHardware)
        {
            token.ThrowIfCancellationRequested();
            try { CollectHardware(sub, snap, token); }
            catch (Exception ex) when (ex is not OperationCanceledException)
            { snap.Errors.Add($"{sub.Name}：{ex.Message}"); }
        }
    }

    static void CollectMemory(IHardware hw, Snapshot snap)
    {
        
        string id = hw.Identifier.ToString();
        if (id.Equals("/ram", StringComparison.OrdinalIgnoreCase))
        {
            
            double? used = FindSensorGB(hw, "Memory Used");
            double? avail = FindSensorGB(hw, "Memory Available");
            if (used is > 0 && avail is >= 0)
                snap.MemoryTotalGB = used.Value + avail.Value;
        }
        else if (id.Contains("/dimm/", StringComparison.OrdinalIgnoreCase))
        {
            
            snap.Dimms.Add(new SysCoreDimm(hw.Name.Trim(), FindSensorGB(hw, "Capacity")));
        }
    }

    static void CollectStorage(IHardware hw, Snapshot snap)
    {
        
        if (hw is SysCore.Hardware.Storage.StorageDevice dev)
        {
            var s = dev.Storage;
            snap.Disks.Add(new SysCoreDisk(
                (s.ProductName ?? hw.Name).Trim(),
                s.DiskSizeBytes,
                ComputeHealth(s),
                ComputePowerOnHours(s),
                s.StorageDeviceNumber,
                ComputeMediaErrors(s)));
        }
        else
        {
            snap.Disks.Add(new SysCoreDisk(hw.Name.Trim(), null, null, null, null, null));
        }
    }

    static int? ComputeHealth(DiskInfoToolkit.StorageDevice s)
    {
        try
        {
            if (s.TransportKind == DiskInfoToolkit.StorageTransportKind.Nvme)
            {
                var used = s.SmartAttributes.FirstOrDefault(a => a.ID == 0xE4);
                if (used != null) return (int)Math.Clamp(100 - (long)used.RawValue, 0, 100);
            }
            foreach (byte id in new byte[] { 0xE7, 0xAD })
            {
                var attr = s.SmartAttributes.FirstOrDefault(a => a.ID == id && a.CurrentValue > 0);
                if (attr != null) return attr.CurrentValue;
            }
        }
        catch {  }
        return null;
    }

    static long? ComputePowerOnHours(DiskInfoToolkit.StorageDevice s)
    {
        try
        {
            if (s.PowerOnHours.HasValue) return (long)s.PowerOnHours.Value;
            var attr = s.SmartAttributes.FirstOrDefault(a => a.ID == 0x09 && a.RawValue > 0);
            if (attr != null) return (long)attr.RawValue;
        }
        catch {  }
        return null;
    }

    static long? ComputeMediaErrors(DiskInfoToolkit.StorageDevice s)
    {
        try
        {
            if (s.TransportKind != DiskInfoToolkit.StorageTransportKind.Nvme) return null;
            var attr = s.SmartAttributes.FirstOrDefault(a => a.ID == 0xED);
            if (attr != null) return (long)attr.RawValue;
        }
        catch {  }
        return null;
    }

    static double? FindSensorMB(IHardware hw, string name) =>
        hw.Sensors.FirstOrDefault(s => s.SensorType == SensorType.SmallData && s.Name == name)
            ?.Value is float v && v > 0 ? v : null;

    static double? FindSensorGB(IHardware hw, string name) =>
        hw.Sensors.FirstOrDefault(s => s.SensorType == SensorType.Data && s.Name == name)
            ?.Value is float v && v >= 0 ? v : null;

    static double? FindSensorMWh(IHardware hw, string name) =>
        hw.Sensors.FirstOrDefault(s => s.SensorType == SensorType.Energy && s.Name == name)
            ?.Value is float v && v > 0 ? v : null;
}
