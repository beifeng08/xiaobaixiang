// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using LibreHardwareMonitor.Hardware;

namespace LibreHardwareMonitor.Windows.Forms.UI;

public class UpdateVisitor : IVisitor
{
    public void VisitComputer(IComputer computer)
    {
        computer.Traverse(this);
    }

    public void VisitHardware(IHardware hardware)
    {
        hardware.Update();
        foreach (IHardware subHardware in hardware.SubHardware)
            subHardware.Accept(this);
    }

    public void VisitSensor(ISensor sensor) { }

    public void VisitParameter(IParameter parameter) { }
}