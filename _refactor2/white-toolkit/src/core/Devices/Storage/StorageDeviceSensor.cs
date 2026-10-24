// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using StorageDeviceDIT = DiskInfoToolkit.StorageDevice;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Devices;

internal delegate float GetStorageDeviceSensorValue(StorageDeviceDIT storage);

internal class StorageDeviceSensor : Sensor
{
    private readonly GetStorageDeviceSensorValue _getValue;

    public StorageDeviceSensor(string name, int index, bool defaultHidden, SensorType sensorType, Hardware hardware, ISettings settings, GetStorageDeviceSensorValue getValue)
        : base(name, index, defaultHidden, sensorType, hardware, null, settings)
    {
        _getValue = getValue;
    }

    public void Update(StorageDeviceDIT storage)
    {
        var value = _getValue(storage);

        Value = value;
    }
}
