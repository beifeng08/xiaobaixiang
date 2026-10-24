// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;

namespace SysCore.Hardware.Storage;

public interface ISmart
{
    
    IReadOnlyList<SmartAttribute> Attributes { get; }
}
