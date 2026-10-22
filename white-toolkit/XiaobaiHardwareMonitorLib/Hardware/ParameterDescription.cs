// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware;

public struct ParameterDescription
{
    
    public ParameterDescription(string name, string description, float defaultValue)
    {
        Name = name;
        Description = description;
        DefaultValue = defaultValue;
    }

    public string Name { get; }

    public string Description { get; }

    public float DefaultValue { get; }
}