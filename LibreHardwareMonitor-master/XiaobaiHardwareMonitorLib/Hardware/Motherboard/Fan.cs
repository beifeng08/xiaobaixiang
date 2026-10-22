// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware.Motherboard;

internal class Fan
{
    public readonly int Index;
    public readonly string Name;

    public Fan(string name, int index)
    {
        Name = name;
        Index = index;
    }
}