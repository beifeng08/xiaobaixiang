// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Collections.Generic;

namespace LibreHardwareMonitor.Hardware.Motherboard.Lpc.EC;

public class WindowsEmbeddedController : EmbeddedController
{
    public WindowsEmbeddedController(IEnumerable<EmbeddedControllerSource> sources, ISettings settings) : base(sources, settings)
    { }

    protected override IEmbeddedControllerIO AcquireIOInterface()
    {
        return new WindowsEmbeddedControllerIO();
    }
}