// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;
using System.Globalization;
using HidSharp;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
using SysCore.Sensors;
namespace SysCore.Power;

internal sealed class MsiPsu : Hardware
{
    private readonly HidDevice _device;
    private readonly List<PsuSensor> _sensors = [];

    public MsiPsu(HidDevice device, ISettings settings, int index)
        : base("MSI PSU", new Identifier("psu", "msi", index.ToString()), settings)
    {
        _device = device;
        using HidStream stream = device.Open();
        MsiUsbApi.MsiFirmwareInfo fwInfo = MsiUsbApi.FwInfo(stream);
        Name = $"{fwInfo.Vendor} {fwInfo.Product}";

        AddSensors(settings);
    }

    public override HardwareType HardwareType => HardwareType.Psu;

    public override void Update()
    {
        using HidStream stream = _device.Open();
        float[] info = MsiUsbApi.InfoList(stream);
        _sensors.ForEach(s => s.Update(info));
    }

    private void AddSensors(ISettings settings)
    {
        int sensorIndex = 0;
        
        _sensors.Add(new PsuSensor("Case", sensorIndex++, SensorType.Fan, this, settings, MsiUsbApi.IndexInfo.FAN_RPM));
        _sensors.Add(new PsuSensor("Case", sensorIndex++, SensorType.Temperature, this, settings, MsiUsbApi.IndexInfo.TEMP));

        _sensors.Add(new PsuSensor("+12V", sensorIndex++, SensorType.Voltage, this, settings, MsiUsbApi.IndexInfo.VOLTS_12));
        _sensors.Add(new PsuSensor("+12V", sensorIndex++, SensorType.Current, this, settings, MsiUsbApi.IndexInfo.AMPS_12));

        _sensors.Add(new PsuSensor("+5V", sensorIndex++, SensorType.Voltage, this, settings, MsiUsbApi.IndexInfo.VOLTS_5));
        _sensors.Add(new PsuSensor("+5V", sensorIndex++, SensorType.Current, this, settings, MsiUsbApi.IndexInfo.AMPS_5));

        _sensors.Add(new PsuSensor("+3.3V", sensorIndex++, SensorType.Voltage, this, settings, MsiUsbApi.IndexInfo.VOLTS_3V3));
        _sensors.Add(new PsuSensor("+3.3V", sensorIndex++, SensorType.Current, this, settings, MsiUsbApi.IndexInfo.AMPS_3V3));

        _sensors.Add(new PsuSensor("PSU Efficiency", sensorIndex++, SensorType.Level, this, settings, MsiUsbApi.IndexInfo.EFFICIENCY));
        _sensors.Add(new PsuSensor("PSU Out", sensorIndex++, SensorType.Power, this, settings, MsiUsbApi.IndexInfo.PSU_OUT));
        _sensors.Add(new PsuSensor("Total Runtime", sensorIndex++, SensorType.TimeSpan, this, settings, MsiUsbApi.IndexInfo.RUNTIME, true));
    }

    private class PsuSensor : Sensor
    {
        private readonly MsiUsbApi.IndexInfo _indexInfo;

        public PsuSensor(string name, int index, SensorType type, MsiPsu hardware, ISettings settings, MsiUsbApi.IndexInfo indexInfo, bool noHistory = false)
            : base(name, index, false, type, hardware, null, settings, noHistory)
        {
            _indexInfo = indexInfo;
            hardware.ActivateSensor(this);
        }

        public void Update(float[] info)
        {
            Value = info[(int)_indexInfo];
        }
    }
}
