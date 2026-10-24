// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware;

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