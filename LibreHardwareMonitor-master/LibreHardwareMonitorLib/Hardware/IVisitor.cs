// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace LibreHardwareMonitor.Hardware;

public interface IVisitor
{
    
    void VisitComputer(IComputer computer);

    void VisitHardware(IHardware hardware);

    void VisitSensor(ISensor sensor);

    void VisitParameter(IParameter parameter);
}