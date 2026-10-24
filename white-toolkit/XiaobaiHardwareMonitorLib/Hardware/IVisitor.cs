// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware;

public interface IVisitor
{
    
    void VisitComputer(IComputer computer);

    void VisitHardware(IHardware hardware);

    void VisitSensor(ISensor sensor);

    void VisitParameter(IParameter parameter);
}