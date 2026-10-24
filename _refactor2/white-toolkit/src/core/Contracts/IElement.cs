// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Contracts;

public interface IElement
{
    
    void Accept(IVisitor visitor);

    void Traverse(IVisitor visitor);
}