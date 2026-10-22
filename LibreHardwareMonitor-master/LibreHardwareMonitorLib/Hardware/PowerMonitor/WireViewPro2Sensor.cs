// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using LibreHardwareMonitor.Interop.PowerMonitor;

namespace LibreHardwareMonitor.Hardware.PowerMonitor;

internal delegate float GetWireViewPro2SensorValue(DeviceData wvp);

internal class WireViewPro2Sensor : Sensor
{
    readonly GetWireViewPro2SensorValue _getValue;

    public WireViewPro2Sensor(string name, int index, SensorType sensorType, Hardware hardware, ISettings settings, GetWireViewPro2SensorValue getValue)
        : base(name, index, sensorType, hardware, settings)
    {
        _getValue = getValue;
    }

    internal void Update(DeviceData wvp)
    {
        float value = _getValue(wvp);

        Value = value;
    }
}
