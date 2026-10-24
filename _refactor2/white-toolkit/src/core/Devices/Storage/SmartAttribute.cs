// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using DiskInfoToolkit;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Devices;

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
