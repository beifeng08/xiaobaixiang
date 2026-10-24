// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;
using System.Linq;
using System.Text;
using HidSharp;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Power;

public class MsiPsuGroup : IGroup
{
    private static readonly int[] _productIds =
    {
        0x56d4, 
    };

    private static readonly ushort _vendorId = 0x0db0;
    private readonly List<IHardware> _hardware;
    private readonly StringBuilder _report;

    public MsiPsuGroup(ISettings settings)
    {
        _report = new StringBuilder();
        _report.AppendLine("MSI Ai series PSU Hardware");
        _report.AppendLine();

        _hardware = new List<IHardware>();
        foreach (HidDevice dev in DeviceList.Local.GetHidDevices(_vendorId))
        {
            if (_productIds.Contains(dev.ProductID))
            {
                var device = new MsiPsu(dev, settings, _hardware.Count);
                _hardware.Add(device);
                _report.AppendLine($"Device name: {device.Name}");
                _report.AppendLine();
            }
        }

        if (_hardware.Count == 0)
        {
            _report.AppendLine("No MSI PSU Hardware found.");
            _report.AppendLine();
        }
    }

    public IReadOnlyList<IHardware> Hardware => _hardware;

    public void Close()
    {
        foreach (IHardware iHardware in _hardware)
        {
            if (iHardware is Hardware hardware)
                hardware.Close();
        }
    }

    public string GetReport()
    {
        return _report.ToString();
    }
}
