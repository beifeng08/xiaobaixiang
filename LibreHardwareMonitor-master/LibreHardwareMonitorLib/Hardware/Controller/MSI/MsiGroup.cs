// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using HidSharp;

namespace LibreHardwareMonitor.Hardware.Controller.MSI;

internal class MsiGroup : IGroup
{
    private readonly List<IHardware> _hardware = new();
    private readonly StringBuilder _report = new();

    public MsiGroup(ISettings settings)
    {
        _report.AppendLine("MSI Hardware:");
        _report.AppendLine();

        var hidDevices = DeviceList.Local.GetHidDevices();

        foreach (var hidDevice in hidDevices)
        {
            
            var found = MsiConstants.SupportedDevices.FirstOrDefault(md => md.VendorId == hidDevice.VendorID && md.ProductIdController == hidDevice.ProductID);

            if (found == null)
            {
                continue;
            }

            try
            {
                var coreLiquidController = new MsiCoreLiquidController(found, hidDevice, settings);

                _hardware.Add(coreLiquidController);
                _report.AppendLine($"MSI Controller for '{hidDevice.GetProductName()}' ({hidDevice.VendorID:X4}:{hidDevice.ProductID:X4}) initialized successfully");
            }
            catch (Exception e)
            {
                _report.AppendLine($"Msi Controller Plugin initialization failed: {e.Message}");
            }
        }
    }

    public IReadOnlyList<IHardware> Hardware => _hardware;

    public void Close()
    {
        foreach (Hardware hw in _hardware.OfType<Hardware>())
        {
            hw.Close();
        }
    }

    public string GetReport()
    {
        var sb = new StringBuilder();
        sb.AppendLine(_report.ToString());

        _hardware.ForEach(hw => sb.AppendLine(hw.GetReport()));

        return sb.ToString();
    }
}
