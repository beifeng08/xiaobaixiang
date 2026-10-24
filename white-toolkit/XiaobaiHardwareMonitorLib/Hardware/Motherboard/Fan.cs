// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware.Motherboard;

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