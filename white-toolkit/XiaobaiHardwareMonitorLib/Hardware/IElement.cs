// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware;

public interface IElement
{
    
    void Accept(IVisitor visitor);

    void Traverse(IVisitor visitor);
}