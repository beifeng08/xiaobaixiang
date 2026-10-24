// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware;

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