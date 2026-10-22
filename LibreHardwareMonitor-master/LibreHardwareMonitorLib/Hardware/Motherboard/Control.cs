// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace LibreHardwareMonitor.Hardware.Motherboard;

internal class Control
{
    public readonly int Index;
    public readonly string Name;

    public Control(string name, int index)
    {
        Name = name;
        Index = index;
    }
}
