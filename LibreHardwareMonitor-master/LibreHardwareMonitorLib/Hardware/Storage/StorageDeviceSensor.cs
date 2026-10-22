// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using StorageDeviceDIT = DiskInfoToolkit.StorageDevice;

namespace LibreHardwareMonitor.Hardware.Storage;

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
