// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Contracts;

public delegate void SensorEventHandler(ISensor sensor);

public interface IHardware : IElement
{
    
    HardwareType HardwareType { get; }

    Identifier Identifier { get; }

    string Name { get; set; }

    IHardware Parent { get; }

    ISensor[] Sensors { get; }

    IHardware[] SubHardware { get; }

    string GetReport();

    void Update();

    event SensorEventHandler SensorAdded;

    event SensorEventHandler SensorRemoved;

    IDictionary<string, string> Properties { get; }
}