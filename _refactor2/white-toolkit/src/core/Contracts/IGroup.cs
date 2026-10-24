// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Contracts;

internal interface IGroup
{
    
    IReadOnlyList<IHardware> Hardware { get; }

    string GetReport();

    void Close();
}