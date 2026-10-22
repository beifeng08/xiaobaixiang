// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;

namespace XiaobaiHardwareMonitor.Hardware;

public class SensorVisitor : IVisitor
{
    private readonly SensorEventHandler _handler;

    public SensorVisitor(SensorEventHandler handler)
    {
        _handler = handler ?? throw new ArgumentNullException(nameof(handler));
    }

    public void VisitComputer(IComputer computer)
    {
        if (computer == null)
            throw new ArgumentNullException(nameof(computer));

        computer.Traverse(this);
    }

    public void VisitHardware(IHardware hardware)
    {
        if (hardware == null)
            throw new ArgumentNullException(nameof(hardware));

        hardware.Traverse(this);
    }

    public void VisitSensor(ISensor sensor)
    {
        _handler(sensor);
    }

    public void VisitParameter(IParameter parameter)
    { }
}