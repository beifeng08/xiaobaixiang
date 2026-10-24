// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) LibreHardwareMonitor and Contributors.
// Partial Copyright (C) Michael Möller <mmoeller@openhardwaremonitor.org> and Contributors.
// All Rights Reserved.

using System;
using System.Collections.Generic;

namespace SysCore.Hardware;

public enum SensorType
{
    Voltage, 
    Current, 
    Power, 
    Clock, 
    Temperature, 
    Load, 
    Frequency, 
    Fan, 
    Flow, 
    Control, 
    Level, 
    Factor, 
    Data, 
    SmallData, 
    Throughput, 
    TimeSpan, 
    Timing, 
    Energy, 
    Noise, 
    Conductivity, 
    Humidity 
}

public struct SensorValue
{
    
    public SensorValue(float value, DateTime time)
    {
        Value = value;
        Time = time;
    }

    public float Value { get; }

    public DateTime Time { get; }
}

public interface ISensor : IElement
{
    IControl Control { get; }

    IHardware Hardware { get; }

    Identifier Identifier { get; }

    int Index { get; }

    bool IsDefaultHidden { get; }

    float? Max { get; }

    float? Min { get; }

    string Name { get; set; }

    IReadOnlyList<IParameter> Parameters { get; }

    SensorType SensorType { get; }

    float? Value { get; }

    IEnumerable<SensorValue> Values { get; }

    TimeSpan ValuesTimeWindow { get; set; }

    void ResetMin();

    void ResetMax();

    void ClearValues();
}
