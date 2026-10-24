// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Model;

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