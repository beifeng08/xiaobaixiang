// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Contracts;

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