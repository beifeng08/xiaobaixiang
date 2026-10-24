// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Devices;

internal class MotherboardGroup : IGroup
{
    private readonly Motherboard[] _motherboards;

    public MotherboardGroup(SMBios smbios, ISettings settings)
    {
        _motherboards = new Motherboard[1];
        _motherboards[0] = new Motherboard(smbios, settings);
    }

    public IReadOnlyList<IHardware> Hardware => _motherboards;

    public void Close()
    {
        foreach (Motherboard mainboard in _motherboards)
            mainboard.Close();
    }

    public string GetReport()
    {
        return null;
    }
}