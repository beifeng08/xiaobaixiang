// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace LibreHardwareMonitor.Hardware;

public interface IParameter : IElement
{
    
    float DefaultValue { get; }

    string Description { get; }

    Identifier Identifier { get; }

    bool IsDefault { get; set; }

    string Name { get; }

    ISensor Sensor { get; }

    float Value { get; set; }
}