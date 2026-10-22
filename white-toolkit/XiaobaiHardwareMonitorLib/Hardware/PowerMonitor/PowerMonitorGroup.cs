// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XiaobaiHardwareMonitor.Hardware.PowerMonitor;

internal class PowerMonitorGroup : IGroup
{
    private readonly List<IHardware> _hardware = new();
    private readonly StringBuilder _report = new();

    public PowerMonitorGroup(ISettings settings)
    {
        _report.AppendLine("Power Monitors:");
        _report.AppendLine();

        var devices = WireViewPro2.TryFindDevices(settings);

        devices.ForEach(wvp2 =>
        {
            if (wvp2.IsConnected)
            {
                _report.AppendLine($"Power Monitor for '{wvp2.Name}' initialized successfully");

                _hardware.Add(wvp2);
            }
        });
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
