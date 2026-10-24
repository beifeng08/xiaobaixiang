// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;

namespace SysCore.Hardware.Motherboard.Lpc.EC;

public class WindowsEmbeddedController : EmbeddedController
{
    public WindowsEmbeddedController(IEnumerable<EmbeddedControllerSource> sources, ISettings settings) : base(sources, settings)
    { }

    protected override IEmbeddedControllerIO AcquireIOInterface()
    {
        return new WindowsEmbeddedControllerIO();
    }
}