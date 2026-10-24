// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Interop.PowerMonitor;

namespace SysCore.Hardware.PowerMonitor;

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
