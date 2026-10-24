// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using HidSharp;

namespace SysCore.Hardware.Controller.AquaComputer;

internal sealed class AquastreamUltimate : Hardware
{
    private readonly byte[] _rawData = new byte[104];
    private readonly HidStream _stream;

    private readonly Sensor[] _rpmSensors = new Sensor[2];
    private readonly Sensor[] _temperatures = new Sensor[2];
    private readonly Sensor[] _voltages = new Sensor[2];
    private readonly Sensor[] _currents = new Sensor[2];
    private readonly Sensor[] _powers = new Sensor[2];
    private readonly Sensor[] _flows = new Sensor[2];

    public AquastreamUltimate(HidDevice dev, ISettings settings) : base("AquastreamUltimate", new Identifier(dev), settings)
    {
        if (dev.TryOpen(out _stream))
        {
            
            _stream.Read(_rawData);

            FirmwareVersion = GetConvertedValue(0xD).GetValueOrDefault(0);

            Name = "Aquastream ULTIMATE";

            _temperatures[0] = new Sensor("Coolant", 0, SensorType.Temperature, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_temperatures[0]);

            _temperatures[1] = new Sensor("External Sensor", 1, SensorType.Temperature, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_temperatures[1]);

            _rpmSensors[0] = new Sensor("Pump", 0, SensorType.Fan, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_rpmSensors[0]);

            _voltages[0] = new Sensor("Pump", 0, SensorType.Voltage, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_voltages[0]);

            _currents[0] = new Sensor("Pump", 0, SensorType.Current, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_currents[0]);

            _powers[0] = new Sensor("Pump", 0, SensorType.Power, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_powers[0]);

            _flows[0] = new Sensor("Pump", 0, SensorType.Flow, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_flows[0]);

            _flows[1] = new Sensor("Pressure (mBar)", 1, SensorType.Factor, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_flows[1]);

            _rpmSensors[1] = new Sensor("Fan", 1, SensorType.Fan, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_rpmSensors[1]);

            _voltages[1] = new Sensor("Fan", 1, SensorType.Voltage, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_voltages[1]);

            _currents[1] = new Sensor("Fan", 1, SensorType.Current, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_currents[1]);

            _powers[1] = new Sensor("Fan", 1, SensorType.Power, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_powers[1]);
        }
    }

    public ushort FirmwareVersion { get; }

    public override HardwareType HardwareType
    {
        get { return HardwareType.Cooler; }
    }

    public override void Close()
    {
        _stream.Close();
        base.Close();
    }

    public override void Update()
    {
        
        _stream.Read(_rawData);

        _rpmSensors[0].Value = GetConvertedValue(0x51); 
        _rpmSensors[1].Value = GetConvertedValue(0x41 + 0x06); 

        _temperatures[0].Value = GetConvertedValue(0x2D) / 100f; 
        _temperatures[1].Value = GetConvertedValue(0x2F) / 100f; 

        _voltages[0].Value = GetConvertedValue(0x3D) / 100f; 
        _voltages[1].Value = GetConvertedValue(0x41 + 0x02) / 100f; 

        _currents[0].Value = GetConvertedValue(0x53) / 1000f; 
        _currents[1].Value = GetConvertedValue(0x41 + 0x00) / 1000f; 

        _powers[0].Value = GetConvertedValue(0x55) / 100f; 
        _powers[1].Value = GetConvertedValue(0x41 + 0x04) / 100f; 

        _flows[0].Value = GetConvertedValue(0x37); 
        _flows[1].Value = GetConvertedValue(0x57) / 1000f; 
    }

    private ushort? GetConvertedValue(int index)
    {
        if (_rawData[index] == sbyte.MaxValue)
            return null;

        return Convert.ToUInt16(_rawData[index + 1] | (_rawData[index] << 8));
    }
}
