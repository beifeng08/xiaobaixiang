// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Devices.EmbeddedControllers;

public class WindowsEmbeddedController : EmbeddedController
{
    public WindowsEmbeddedController(IEnumerable<EmbeddedControllerSource> sources, ISettings settings) : base(sources, settings)
    { }

    protected override IEmbeddedControllerIO AcquireIOInterface()
    {
        return new WindowsEmbeddedControllerIO();
    }
}