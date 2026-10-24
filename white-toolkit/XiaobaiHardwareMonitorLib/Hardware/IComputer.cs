// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) LibreHardwareMonitor and Contributors.
// Partial Copyright (C) Michael Möller <mmoeller@openhardwaremonitor.org> and Contributors.
// All Rights Reserved.

using System.Collections.Generic;

namespace SysCore.Hardware;

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
