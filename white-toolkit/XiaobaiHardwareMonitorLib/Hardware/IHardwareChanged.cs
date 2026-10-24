// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware;

internal interface IHardwareChanged
{
    event HardwareEventHandler HardwareAdded;
    event HardwareEventHandler HardwareRemoved;
}