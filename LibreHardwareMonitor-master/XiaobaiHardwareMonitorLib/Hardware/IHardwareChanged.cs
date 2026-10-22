// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware;

internal interface IHardwareChanged
{
    event HardwareEventHandler HardwareAdded;
    event HardwareEventHandler HardwareRemoved;
}