// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using RAMSPDToolkit.SPD.Interfaces;

namespace SysCore.Hardware.Memory.Sensors;

internal class SpdThermalSensor(string name, int index, SensorType sensorType, Hardware hardware, ISettings settings, IThermalSensor thermalSensor)
    : Sensor(name, index, sensorType, hardware, settings)
{
    public bool UpdateSensor()
    {
        if (!thermalSensor.UpdateTemperature())
            return false;

        Value = thermalSensor.Temperature;

        return true;
    }
}
