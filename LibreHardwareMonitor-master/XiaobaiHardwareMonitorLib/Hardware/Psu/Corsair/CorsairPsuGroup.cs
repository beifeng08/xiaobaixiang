// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Collections.Generic;
using System.Linq;
using System.Text;
using HidSharp;

namespace XiaobaiHardwareMonitor.Hardware.Psu.Corsair;

public class CorsairPsuGroup : IGroup
{
    private static readonly int[] _productIds =
    {
        0x1c03, 
        0x1c04, 
        0x1c05, 
        0x1c06, 
        0x1c07, 
        0x1c1e, 
        0x1c08, 
        0x1c1f, 

        0x1c09, 
        0x1c0a, 
        0x1c0b, 
        0x1c0c, 
        0x1c0d, 

    };

    private static readonly ushort _vendorId = 0x1b1c;
    private readonly List<IHardware> _hardware;
    private readonly StringBuilder _report;

    public CorsairPsuGroup(ISettings settings)
    {
        _report = new StringBuilder();
        _report.AppendLine("Corsair HXi/RMi series PSU Hardware");
        _report.AppendLine();

        _hardware = new List<IHardware>();
        foreach (HidDevice dev in DeviceList.Local.GetHidDevices(_vendorId))
        {
            if (_productIds.Contains(dev.ProductID))
            {
                var device = new CorsairPsu(dev, settings, _hardware.Count);
                _hardware.Add(device);
                _report.AppendLine($"Device name: {device.Name}");
                _report.AppendLine();
            }
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
