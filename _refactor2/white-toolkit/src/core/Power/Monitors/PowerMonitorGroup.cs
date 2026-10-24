// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;
using System.Linq;
using System.Text;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Power;

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
