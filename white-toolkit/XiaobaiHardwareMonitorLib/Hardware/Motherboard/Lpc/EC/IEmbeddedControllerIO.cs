// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;

namespace SysCore.Hardware.Motherboard.Lpc.EC;

public interface IEmbeddedControllerIO : IDisposable
{
    void Read(ushort[] registers, byte[] data);
}