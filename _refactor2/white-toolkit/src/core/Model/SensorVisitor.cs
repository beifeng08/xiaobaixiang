// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Model;

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