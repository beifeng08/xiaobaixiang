// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware;

public enum HardwareType
{
    Motherboard,
    SuperIO,
    Cpu,
    Memory,
    GpuNvidia,
    GpuAmd,
    GpuIntel,
    Storage,
    Network,
    Cooler,
    EmbeddedController,
    Psu,
    Battery,
    PowerMonitor,
}
