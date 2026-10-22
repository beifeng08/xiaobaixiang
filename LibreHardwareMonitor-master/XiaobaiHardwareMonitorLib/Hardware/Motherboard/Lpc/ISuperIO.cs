// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware.Motherboard.Lpc;

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
