// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;

namespace SysCore.Hardware.Motherboard.Lpc;

internal interface IGigabyteController: IDisposable
{
    bool Enable(bool enabled);

    void Restore();
}
