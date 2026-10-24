// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware.Gpu;

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
