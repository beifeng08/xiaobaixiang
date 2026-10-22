// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware;

public interface IElement
{
    
    void Accept(IVisitor visitor);

    void Traverse(IVisitor visitor);
}