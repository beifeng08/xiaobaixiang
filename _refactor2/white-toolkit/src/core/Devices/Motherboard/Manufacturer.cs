// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Diagnostics.CodeAnalysis;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Devices;

[SuppressMessage("ReSharper", "IdentifierTypo")]
[SuppressMessage("ReSharper", "CommentTypo")]
[SuppressMessage("ReSharper", "InconsistentNaming")]
public enum Manufacturer
{
    Abit,
    Acer,
    Alienware,
    AMD,
    AOpen,
    Apple,
    ASRock,
    ASUS,
    Biostar,
    Clevo,
    Dell,
    DFI,
    ECS,
    EPoX,
    EVGA,
    FIC,
    Foxconn,
    Framework,
    Fujitsu,
    Gateway,
    Gigabyte,
    HP,
    IBM,
    Intel,
    Jetway,
    LattePanda,
    Lenovo,
    Medion,
    Microsoft,
    MSI,
    NEC,
    Pegatron,
    Samsung,
    Sapphire,
    Shuttle,
    Sony,
    Supermicro,
    Toshiba,
    XFX,
    Zotac,
    Unknown
}
