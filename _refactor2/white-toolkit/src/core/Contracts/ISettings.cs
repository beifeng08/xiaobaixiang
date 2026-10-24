// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Contracts;

public interface ISettings
{
    
    bool Contains(string name);

    void SetValue(string name, string value);

    string GetValue(string name, string value);

    void Remove(string name);
}