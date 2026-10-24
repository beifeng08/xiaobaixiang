// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Controllers;

internal delegate float GetMsiSensorValue(MsiFanControl msi);

internal class MsiSensor : Sensor
{
    private readonly GetMsiSensorValue _getValue;

    public MsiSensor(string name, int index, SensorType sensorType, Hardware hardware, ISettings settings, GetMsiSensorValue getValue)
        : base(name, index, sensorType, hardware, settings)
    {
        _getValue = getValue;
    }

    internal void Update(MsiFanControl msi)
    {
        float value = _getValue(msi);

        Value = value;
    }
}
