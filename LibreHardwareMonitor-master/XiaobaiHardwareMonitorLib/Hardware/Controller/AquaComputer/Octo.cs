// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using HidSharp;

namespace XiaobaiHardwareMonitor.Hardware.Controller.AquaComputer;

internal sealed class Octo : Hardware
{
    private readonly byte[] _rawData = new byte[1025];
    private readonly Sensor[] _rpmSensors = new Sensor[8];
    private readonly HidStream _stream;
    private readonly Sensor[] _temperatures = new Sensor[4];
    private readonly Sensor[] _voltages = new Sensor[9];
    private readonly Sensor[] _currents = new Sensor[8];
    private readonly Sensor[] _powers = new Sensor[8];

    public Octo(HidDevice dev, ISettings settings) : base("Octo", new Identifier(dev), settings)
    {
        if (dev.TryOpen(out _stream))
        {
            
            _stream.Read(_rawData);

            Name = "OCTO";
            FirmwareVersion = GetConvertedValue(OctoDataIndexes.FIRMWARE_VERSION).GetValueOrDefault(0);

            for (int i = 0; i < 4; i++)
            {
                _temperatures[i] = new Sensor($"Temperature #{i + 1}", i, SensorType.Temperature, this, Array.Empty<ParameterDescription>(), settings);
                ActivateSensor(_temperatures[i]);
            }

            for (int i = 0; i < 8; i++)
            {
                _rpmSensors[i] = new Sensor($"Fan #{i + 1}", i, SensorType.Fan, this, Array.Empty<ParameterDescription>(), settings);
                ActivateSensor(_rpmSensors[i]);
            }

            _voltages[0] = new Sensor("Input", 0, SensorType.Voltage, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_voltages[0]);

            for (int i = 1; i < 9; i++)
            {
                _voltages[i] = new Sensor($"Fan #{i}", i, SensorType.Voltage, this, Array.Empty<ParameterDescription>(), settings);
                ActivateSensor(_voltages[i]);
            }

            for (int i = 0; i < 8; i++)
            {
                _currents[i] = new Sensor($"Fan #{i + 1}", i, SensorType.Current, this, Array.Empty<ParameterDescription>(), settings);
                ActivateSensor(_currents[i]);
            }

            for (int i = 0; i < 8; i++)
            {
                _powers[i] = new Sensor($"Fan #{i + 1}", i, SensorType.Power, this, Array.Empty<ParameterDescription>(), settings);
                ActivateSensor(_powers[i]);
            }
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

        _temperatures[0].Value = GetConvertedValue(OctoDataIndexes.TEMP_1) / 100f; 
        _temperatures[1].Value = GetConvertedValue(OctoDataIndexes.TEMP_2) / 100f; 
        _temperatures[2].Value = GetConvertedValue(OctoDataIndexes.TEMP_3) / 100f; 
        _temperatures[3].Value = GetConvertedValue(OctoDataIndexes.TEMP_4) / 100f; 

        _rpmSensors[0].Value = GetConvertedValue(OctoDataIndexes.FAN_SPEED_1); 
        _rpmSensors[1].Value = GetConvertedValue(OctoDataIndexes.FAN_SPEED_2); 
        _rpmSensors[2].Value = GetConvertedValue(OctoDataIndexes.FAN_SPEED_3); 
        _rpmSensors[3].Value = GetConvertedValue(OctoDataIndexes.FAN_SPEED_4); 
        _rpmSensors[4].Value = GetConvertedValue(OctoDataIndexes.FAN_SPEED_5); 
        _rpmSensors[5].Value = GetConvertedValue(OctoDataIndexes.FAN_SPEED_6); 
        _rpmSensors[6].Value = GetConvertedValue(OctoDataIndexes.FAN_SPEED_7); 
        _rpmSensors[7].Value = GetConvertedValue(OctoDataIndexes.FAN_SPEED_8); 

        _voltages[0].Value = GetConvertedValue(OctoDataIndexes.VOLTAGE) / 100f; 
        _voltages[1].Value = GetConvertedValue(OctoDataIndexes.FAN_VOLTAGE_1) / 100f; 
        _voltages[2].Value = GetConvertedValue(OctoDataIndexes.FAN_VOLTAGE_2) / 100f; 
        _voltages[3].Value = GetConvertedValue(OctoDataIndexes.FAN_VOLTAGE_3) / 100f; 
        _voltages[4].Value = GetConvertedValue(OctoDataIndexes.FAN_VOLTAGE_4) / 100f; 
        _voltages[5].Value = GetConvertedValue(OctoDataIndexes.FAN_VOLTAGE_5) / 100f; 
        _voltages[6].Value = GetConvertedValue(OctoDataIndexes.FAN_VOLTAGE_6) / 100f; 
        _voltages[7].Value = GetConvertedValue(OctoDataIndexes.FAN_VOLTAGE_7) / 100f; 
        _voltages[8].Value = GetConvertedValue(OctoDataIndexes.FAN_VOLTAGE_8) / 100f; 

        _currents[0].Value = GetConvertedValue(OctoDataIndexes.FAN_CURRENT_1) / 1000f; 
        _currents[1].Value = GetConvertedValue(OctoDataIndexes.FAN_CURRENT_2) / 1000f; 
        _currents[2].Value = GetConvertedValue(OctoDataIndexes.FAN_CURRENT_3) / 1000f; 
        _currents[3].Value = GetConvertedValue(OctoDataIndexes.FAN_CURRENT_4) / 1000f; 
        _currents[4].Value = GetConvertedValue(OctoDataIndexes.FAN_CURRENT_5) / 1000f; 
        _currents[5].Value = GetConvertedValue(OctoDataIndexes.FAN_CURRENT_6) / 1000f; 
        _currents[6].Value = GetConvertedValue(OctoDataIndexes.FAN_CURRENT_7) / 1000f; 
        _currents[7].Value = GetConvertedValue(OctoDataIndexes.FAN_CURRENT_8) / 1000f; 

        _powers[0].Value = GetConvertedValue(OctoDataIndexes.FAN_POWER_1) / 100f; 
        _powers[1].Value = GetConvertedValue(OctoDataIndexes.FAN_POWER_2) / 100f; 
        _powers[2].Value = GetConvertedValue(OctoDataIndexes.FAN_POWER_3) / 100f; 
        _powers[3].Value = GetConvertedValue(OctoDataIndexes.FAN_POWER_4) / 100f; 
        _powers[4].Value = GetConvertedValue(OctoDataIndexes.FAN_POWER_5) / 100f; 
        _powers[5].Value = GetConvertedValue(OctoDataIndexes.FAN_POWER_6) / 100f; 
        _powers[6].Value = GetConvertedValue(OctoDataIndexes.FAN_POWER_7) / 100f; 
        _powers[7].Value = GetConvertedValue(OctoDataIndexes.FAN_POWER_8) / 100f; 
    }

    private sealed class OctoDataIndexes
    {
        public const int FIRMWARE_VERSION = 13;

        public const int TEMP_1 = 61;
        public const int TEMP_2 = 63;
        public const int TEMP_3 = 65;
        public const int TEMP_4 = 67;

        public const int FAN_SPEED_1 = 133;
        public const int FAN_SPEED_2 = 146;
        public const int FAN_SPEED_3 = 159;
        public const int FAN_SPEED_4 = 172;
        public const int FAN_SPEED_5 = 185;
        public const int FAN_SPEED_6 = 198;
        public const int FAN_SPEED_7 = 211;
        public const int FAN_SPEED_8 = 224;

        public const int FAN_POWER_1 = 131;
        public const int FAN_POWER_2 = 144;
        public const int FAN_POWER_3 = 157;
        public const int FAN_POWER_4 = 170;
        public const int FAN_POWER_5 = 183;
        public const int FAN_POWER_6 = 196;
        public const int FAN_POWER_7 = 209;
        public const int FAN_POWER_8 = 222;

        public const int VOLTAGE = 117;
        public const int FAN_VOLTAGE_1 = 127;
        public const int FAN_VOLTAGE_2 = 140;
        public const int FAN_VOLTAGE_3 = 153;
        public const int FAN_VOLTAGE_4 = 166;
        public const int FAN_VOLTAGE_5 = 179;
        public const int FAN_VOLTAGE_6 = 192;
        public const int FAN_VOLTAGE_7 = 205;
        public const int FAN_VOLTAGE_8 = 218;

        public const int FAN_CURRENT_1 = 129;
        public const int FAN_CURRENT_2 = 142;
        public const int FAN_CURRENT_3 = 155;
        public const int FAN_CURRENT_4 = 168;
        public const int FAN_CURRENT_5 = 181;
        public const int FAN_CURRENT_6 = 194;
        public const int FAN_CURRENT_7 = 207;
        public const int FAN_CURRENT_8 = 220;
    }

    private ushort? GetConvertedValue(int index)
    {
        if (_rawData[index] == sbyte.MaxValue)
            return null;

        return Convert.ToUInt16(_rawData[index + 1] | (_rawData[index] << 8));
    }
}
