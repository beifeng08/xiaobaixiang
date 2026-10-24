// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware.Memory;

internal sealed class TotalMemory : Hardware
{
    public TotalMemory(ISettings settings)
        : base("Total Memory", new Identifier("ram"), settings)
    {
        PhysicalMemoryUsed = new Sensor("Memory Used", 0, SensorType.Data, this, settings);
        ActivateSensor(PhysicalMemoryUsed);

        PhysicalMemoryAvailable = new Sensor("Memory Available", 1, SensorType.Data, this, settings);
        ActivateSensor(PhysicalMemoryAvailable);

        PhysicalMemoryLoad = new Sensor("Memory", 0, SensorType.Load, this, settings);
        ActivateSensor(PhysicalMemoryLoad);
    }

    public override HardwareType HardwareType => HardwareType.Memory;

    internal Sensor PhysicalMemoryAvailable { get; }

    internal Sensor PhysicalMemoryLoad { get; }

    internal Sensor PhysicalMemoryUsed { get; }

    public override void Update()
    {
        if (Software.OperatingSystem.IsUnix)
        {
            MemoryLinux.Update(this);
        }
        else
        {
            MemoryWindows.Update(this);
        }
    }
}
