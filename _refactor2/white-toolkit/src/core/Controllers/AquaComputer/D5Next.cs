// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using HidSharp;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
using SysCore.Sensors;
namespace SysCore.Controllers;

internal sealed class D5Next : Hardware
{
    
    private readonly byte[] _rawData = new byte[1025];
    private readonly Sensor[] _rpmSensors = new Sensor[2];
    private readonly HidStream _stream;
    private readonly Sensor[] _temperatures = new Sensor[1];
    private readonly Sensor[] _voltages = new Sensor[4];
    private readonly Sensor[] _powers = new Sensor[2];
    private readonly Sensor[] _flows = new Sensor[1];
    private readonly Sensor[] _fanControl = new Sensor[2];

    public D5Next(HidDevice dev, ISettings settings) : base("D5Next", new Identifier(dev), settings)
    {
        if (dev.TryOpen(out _stream))
        {
            
            _stream.Read(_rawData);

            Name = "D5Next";
            FirmwareVersion = Convert.ToUInt16(_rawData[14] | (_rawData[13] << 8));
            _temperatures[0] = new Sensor("Water Temperature", 0, SensorType.Temperature, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_temperatures[0]);

            _rpmSensors[0] = new Sensor("Pump", 0, SensorType.Fan, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_rpmSensors[0]);

            _rpmSensors[1] = new Sensor("Fan", 1, SensorType.Fan, this, settings);
            ActivateSensor(_rpmSensors[1]);

            _voltages[0] = new Sensor("Fan Voltage", 0, SensorType.Voltage, this, settings);
            ActivateSensor(_voltages[0]);

            _voltages[1] = new Sensor("Pump Voltage", 1, SensorType.Voltage, this, settings);
            ActivateSensor(_voltages[1]);

            _voltages[2] = new Sensor("+5V Voltage", 2, SensorType.Voltage, this, settings);
            ActivateSensor(_voltages[2]);

            _voltages[3] = new Sensor("+12V Voltage", 3, SensorType.Voltage, this, settings);
            ActivateSensor(_voltages[3]);
           
            _powers[0] = new Sensor("Fan Power", 0, SensorType.Power, this, settings);
            ActivateSensor(_powers[0]);

            _powers[1] = new Sensor("Pump Power", 1, SensorType.Power, this, settings);
            ActivateSensor(_powers[1]);

            _flows[0] = new Sensor("Viritual Flow", 0, SensorType.Flow, this, settings);
            ActivateSensor(_flows[0]);

            _fanControl[0] = new Sensor("Fan Control", 0, SensorType.Control, this, settings);
            ActivateSensor(_fanControl[0]);

            _fanControl[1] = new Sensor("Pump Control", 1, SensorType.Control, this, settings);
            ActivateSensor(_fanControl[1]);
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
        _temperatures[0].Value = (_rawData[88] | (_rawData[87] << 8)) / 100f; 
        _rpmSensors[0].Value = _rawData[117] | (_rawData[116] << 8); 
        _rpmSensors[1].Value = ReadUInt16BE(_rawData, 103); 
        _voltages[0].Value = ReadUInt16BE(_rawData, 97) / 100f; 
        _voltages[1].Value = ReadUInt16BE(_rawData, 110) / 100f; 
        _voltages[2].Value = ReadUInt16BE(_rawData, 57) / 100f; 
        _voltages[3].Value = ReadUInt16BE(_rawData, 55) / 100f; 
        _powers[0].Value = ReadUInt16BE(_rawData, 101) / 100f; 
        _powers[1].Value = ReadUInt16BE(_rawData, 114) / 100f; 
        _flows[0].Value = ReadUInt16BE(_rawData, 89) / 10f; 
        _fanControl[0].Value = ReadUInt16BE(_rawData, 95) / 100f; 
        _fanControl[1].Value = ReadUInt16BE(_rawData, 108) / 100f; 
    }

    private ushort ReadUInt16BE(byte[] value, int startIndex)
    {
        return (ushort)(value[startIndex + 1] | (value[startIndex] << 8));
    }
}
