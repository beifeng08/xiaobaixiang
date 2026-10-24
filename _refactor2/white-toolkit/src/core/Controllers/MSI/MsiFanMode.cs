// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Controllers;

public enum MsiFanMode : byte
{
    Silent = 0,
    Bios = 1,
    Game = 2,
    Custom = 3,
    Unknown = 4,
    Smart = 5,
}
