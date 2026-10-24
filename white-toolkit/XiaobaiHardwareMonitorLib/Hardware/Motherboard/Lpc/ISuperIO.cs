// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware.Motherboard.Lpc;

internal interface ISuperIO
{
    Chip Chip { get; }

    float?[] Controls { get; }

    float?[] Fans { get; }

    float?[] Temperatures { get; }

    float?[] Voltages { get; }

    void SetControl(int index, byte? value);

    byte? ReadGpio(int index);

    void WriteGpio(int index, byte value);

    string GetReport();

    void Update();

    void Close();
}
