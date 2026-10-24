// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) LibreHardwareMonitor and Contributors.
// Partial Copyright (C) Michael Möller <mmoeller@openhardwaremonitor.org> and Contributors.
// All Rights Reserved.

using DiskInfoToolkit;

namespace SysCore.Hardware.Storage;

public sealed class SmartAttribute
{
    
    public SmartAttribute(SmartAttributeEntry smartAttribute, SensorType? sensorType, int sensorChannel, string sensorName, bool defaultHiddenSensor = false)
    {
        Attribute = smartAttribute;
        SensorType = sensorType;
        SensorChannel = sensorChannel;
        SensorName = sensorName ?? Name;
        IsHiddenByDefault = defaultHiddenSensor;
    }

    public SmartAttributeEntry Attribute { get; internal set; }

    public byte Id => Attribute.ID;
    public string Name => Attribute.Name;

    public SensorType? SensorType { get; }
    public int SensorChannel { get; }
    public string SensorName { get; }
    public bool IsHiddenByDefault { get; }

    public float Value => Attribute.RawValue;
    public byte Threshold => Attribute.ThresholdValue;
}
