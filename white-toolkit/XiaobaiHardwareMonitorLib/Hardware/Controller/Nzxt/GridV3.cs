// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) LibreHardwareMonitor and Contributors.
// All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Threading;
using HidSharp;

namespace SysCore.Hardware.Controller.Nzxt;

internal sealed class GridV3 : Hardware
{
    private const int FANS_COUNT = 6;

    private static readonly byte[] _initialize1 = { 0x01, 0x5c };
    private static readonly byte[] _initialize2 = { 0x01, 0x5d };
    private static readonly byte[] _initialize3 = { 0x01, 0x59 };

    private readonly Sensor[] _currents = new Sensor[FANS_COUNT];
    private readonly Control[] _fanControls = new Control[FANS_COUNT];
    private readonly Sensor _noise;
    private readonly Sensor[] _powers = new Sensor[FANS_COUNT];
    private readonly Sensor[] _pwmControls = new Sensor[FANS_COUNT];
    private readonly Dictionary<int, byte[]> _rawData = new();
    private readonly Sensor[] _rpmSensors = new Sensor[FANS_COUNT];
    private readonly byte[] _setFanSpeedMsg;
    private readonly HidStream _stream;
    private readonly Sensor[] _voltages = new Sensor[FANS_COUNT];

    public GridV3(HidDevice dev, ISettings settings) : base("NZXT GRID+ V3", new Identifier(dev), settings)
    {
        if (dev.TryOpen(out _stream))
        {
            for (int fanId = 0; fanId < FANS_COUNT; fanId++)
                _rawData[fanId] = new byte[21];

            _setFanSpeedMsg = new byte[65];
            _setFanSpeedMsg[0] = 0x02;
            _setFanSpeedMsg[1] = 0x4d;
            _setFanSpeedMsg[3] = 0x00;

            _stream.Write(_initialize1);
            _stream.Write(_initialize2);
            _stream.Write(_initialize3);

            do
            {
                _stream.Read(_rawData[0]);
                if (_rawData[0][0] == 0x04)
                {
                    FirmwareVersion = $"{_rawData[0][11]}.{_rawData[0][14]}";
                }
            }
            while (FirmwareVersion == null);

            Name = "NZXT GRID+ V3";

            for (int i = 0; i < FANS_COUNT; i++)
            {
                _rpmSensors[i] = new Sensor($"GRID Fan #{i + 1}", i, SensorType.Fan, this, Array.Empty<ParameterDescription>(), settings);
                _voltages[i] = new Sensor($"GRID Fan #{i + 1}", i, SensorType.Voltage, this, Array.Empty<ParameterDescription>(), settings);
                _currents[i] = new Sensor($"GRID Fan #{i + 1}", i, SensorType.Current, this, Array.Empty<ParameterDescription>(), settings);
                _powers[i] = new Sensor($"GRID Fan #{i + 1}", i, SensorType.Power, this, Array.Empty<ParameterDescription>(), settings);
                _pwmControls[i] = new Sensor($"GRID Fan #{i + 1}", i, SensorType.Control, this, Array.Empty<ParameterDescription>(), settings);

                _fanControls[i] = new Control(_pwmControls[i], settings, 0, 100);

                _pwmControls[i].Control = _fanControls[i];
                _fanControls[i].ControlModeChanged += SoftwareControlValueChanged;
                _fanControls[i].SoftwareControlValueChanged += SoftwareControlValueChanged;
                SoftwareControlValueChanged(_fanControls[i]);

                ActivateSensor(_rpmSensors[i]);
                ActivateSensor(_voltages[i]);
                ActivateSensor(_currents[i]);
                ActivateSensor(_powers[i]);
                ActivateSensor(_pwmControls[i]);

                _fanControls[i].SetDefault();
            }

            _noise = new Sensor("GRID Noise", 0, SensorType.Noise, this, Array.Empty<ParameterDescription>(), settings);
            ActivateSensor(_noise);

            Thread readGridReports = new(ContinuousRead) { IsBackground = true };
            readGridReports.Start(_rawData);

            IsValid = true;
        }
    }

    public string FirmwareVersion { get; }

    public override HardwareType HardwareType => HardwareType.Cooler;

    public bool IsValid { get; }

    private void SoftwareControlValueChanged(Control control)
    {
        switch (control.ControlMode)
        {
            case ControlMode.Software:
                float value = control.SoftwareValue;
                byte fanSpeed = (byte)(value > 100 ? 100 : value < 0 ? 0 : value); 

                _setFanSpeedMsg[2] = (byte)control.Sensor.Index;
                _setFanSpeedMsg[4] = fanSpeed;

                _stream.Write(_setFanSpeedMsg);

                _pwmControls[control.Sensor.Index].Value = value;
                break;
            case ControlMode.Default:
                
                _setFanSpeedMsg[2] = (byte)control.Sensor.Index;
                _setFanSpeedMsg[4] = 40;

                _stream.Write(_setFanSpeedMsg);

                _pwmControls[control.Sensor.Index].Value = 40;
                break;
        }
    }

    public override void Close()
    {
        _stream?.Close();
        base.Close();
    }

    private void ContinuousRead(object state)
    {
        byte[] buffer = new byte[_rawData[0].Length];
        while (_stream.CanRead)
        {
            try
            {
                _stream.Read(buffer); 
                if (buffer[0] == 0x04)
                {
                    lock (_rawData)
                    {
                        int fanId = (buffer[15] >> 4) & 0x0f;
                        Array.Copy(buffer, _rawData[fanId], buffer.Length);
                    }
                }
            }
            catch (TimeoutException)
            {
                
                Thread.Sleep(500);
            }
            catch (ObjectDisposedException)
            {
                
                return;
            }
        }
    }

    public override void Update()
    {
        
        lock (_rawData)
        {
            for (int fanId = 0; fanId < FANS_COUNT; fanId++)
            {
                _rpmSensors[fanId].Value = (_rawData[fanId][3] << 8) | _rawData[fanId][4];
                _voltages[fanId].Value = _rawData[fanId][7] + _rawData[fanId][8] / 100.0f;
                _currents[fanId].Value = _rawData[fanId][9] + _rawData[fanId][10] / 100.0f;
                _powers[fanId].Value = _currents[fanId].Value * _voltages[fanId].Value;
            }

            _noise.Value = _rawData[2][1];
        }
    }
}
