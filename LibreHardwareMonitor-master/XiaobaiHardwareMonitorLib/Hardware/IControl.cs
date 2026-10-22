// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware;

public enum ControlMode
{
    Undefined,
    Software,
    Default
}

public interface IControl
{
    ControlMode ControlMode { get; }

    Identifier Identifier { get; }

    float MaxSoftwareValue { get; }

    float MinSoftwareValue { get; }

    ISensor Sensor { get; }

    float SoftwareValue { get; }

    void SetDefault();

    void SetSoftware(float value);
}