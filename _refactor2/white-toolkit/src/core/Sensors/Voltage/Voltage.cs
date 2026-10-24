// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Sensors;

internal class Voltage
{
    public readonly bool Hidden;
    public readonly int Index;
    public readonly string Name;
    public readonly float Rf;
    public readonly float Ri;
    public readonly float Vf;

    public Voltage(string name, int index, bool hidden = false) : this(name, index, 0, 1, 0, hidden)
    { }

    public Voltage(string name, int index, float ri, float rf, float vf = 0, bool hidden = false)
    {
        Name = name;
        Index = index;
        Ri = ri;
        Rf = rf;
        Vf = vf;
        Hidden = hidden;
    }
}