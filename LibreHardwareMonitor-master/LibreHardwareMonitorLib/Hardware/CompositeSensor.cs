// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Linq;

namespace LibreHardwareMonitor.Hardware;

internal class CompositeSensor : Sensor
{
    private readonly ISensor[] _components;
    private readonly Func<float, ISensor, float> _reducer;
    private readonly float _seedValue;

    public CompositeSensor
    (
        string name,
        int index,
        SensorType sensorType,
        Hardware hardware,
        ISettings settings,
        ISensor[] components,
        Func<float, ISensor, float> reducer,
        float seedValue = .0f)
        : base(name, index, sensorType, hardware, settings)
    {
        _components = components;
        _reducer = reducer;
        _seedValue = seedValue;
    }

    public override float? Value
    {
        get { return _components.Aggregate(_seedValue, _reducer); }
        set => throw new NotImplementedException();
    }
}