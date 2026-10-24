// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Devices.SuperIo;

internal interface IGigabyteController: IDisposable
{
    bool Enable(bool enabled);

    void Restore();
}
