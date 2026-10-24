// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
using SysCore.Sensors;
namespace SysCore.Contracts;

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
