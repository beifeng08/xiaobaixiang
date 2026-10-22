// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace LibreHardwareMonitor.Hardware.Gpu;

public abstract class GenericGpu : Hardware
{
    
    protected GenericGpu(string name, Identifier identifier, ISettings settings) : base(name, identifier, settings)
    {
    }

    public abstract string DeviceId { get; }

    public override void Close()
    {
        base.Close();
    }
}
