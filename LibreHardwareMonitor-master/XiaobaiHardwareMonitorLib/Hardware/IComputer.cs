// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Collections.Generic;

namespace XiaobaiHardwareMonitor.Hardware;

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
