// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Collections.Generic;

namespace LibreHardwareMonitor.Hardware;

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