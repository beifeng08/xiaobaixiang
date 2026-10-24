// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Contracts;

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