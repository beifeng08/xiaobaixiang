// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware.Motherboard.Lpc.EC;

public class EmbeddedControllerSource(string name, SensorType type, ushort register, byte size = 1, float factor = 1.0f, float offset = 0.0f, int blank = int.MaxValue, bool isLittleEndian = false)
{
    public int Blank { get; } = blank;

    public float Factor { get; } = factor;

    public bool IsLittleEndian { get; } = isLittleEndian;

    public string Name { get; } = name;

    public float Offset { get; } = offset;

    public ushort Register { get; } = register;

    public byte Size { get; } = size;

    public SensorType Type { get; } = type;
}
