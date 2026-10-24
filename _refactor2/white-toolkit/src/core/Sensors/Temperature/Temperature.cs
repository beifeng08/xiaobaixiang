// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Sensors;

internal class Temperature
{
    public readonly int Index;
    public readonly string Name;

    public Temperature(string name, int index)
    {
        Name = name;
        Index = index;
    }
}