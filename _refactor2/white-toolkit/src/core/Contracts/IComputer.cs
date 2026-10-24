// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Contracts;

public delegate void HardwareEventHandler(IHardware hardware);

public interface IComputer : IElement
{
    
    event HardwareEventHandler HardwareAdded;

    event HardwareEventHandler HardwareRemoved;

    IList<IHardware> Hardware { get; }

    bool IsBatteryEnabled { get; }

    bool IsControllerEnabled { get; }

    bool IsCpuEnabled { get; }

    bool IsGpuEnabled { get; }

    bool IsPowerMonitorEnabled { get; }

    bool IsMemoryEnabled { get; }

    bool IsMotherboardEnabled { get; }

    bool IsNetworkEnabled { get; }

    bool IsPsuEnabled { get; }

    bool IsStorageEnabled { get; }

    string GetReport();
}
