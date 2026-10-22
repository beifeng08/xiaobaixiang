// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware;

public interface ISensorLimits
{
    
    float? HighLimit { get; }

    float? LowLimit { get; }
}

public interface ICriticalSensorLimits
{
    
    float? CriticalHighLimit { get; }

    float? CriticalLowLimit { get; }
}