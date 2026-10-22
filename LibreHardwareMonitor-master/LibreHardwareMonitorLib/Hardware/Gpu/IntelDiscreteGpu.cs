// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Runtime.InteropServices;
using LibreHardwareMonitor.Interop;
using Windows.Win32;
using Windows.Win32.Devices.DeviceAndDriverInstallation;
using Windows.Win32.Devices.Properties;
using Windows.Win32.Foundation;

namespace LibreHardwareMonitor.Hardware.Gpu;

internal sealed class IntelDiscreteGpu : GenericGpu
{
    
    private const double MemoryFrequencyDivisor = 8.0; 

    private readonly Sensor _clockCore;
    private readonly Sensor _clockMemory;

    private readonly Sensor[] _fans;

    private readonly Sensor _loadGlobalActivity;
    private readonly Sensor _loadMedia;
    private readonly Sensor _loadRenderCompute;

    private readonly Sensor _powerGpu;
    private readonly Sensor _powerTotal;

    private readonly Sensor _temperatureGpuCore;
    private readonly Sensor _temperatureMemory;

    private readonly Sensor _voltageCore;
    private readonly Sensor _voltageMemory;

    private readonly Sensor _memoryFree;
    private readonly Sensor _memoryTotal;
    private readonly Sensor _memoryUsed;
    private readonly Sensor _memoryLoad;

    private readonly Sensor _memoryBandwidthRead;
    private readonly Sensor _memoryBandwidthWrite;

    private double _currentTimestamp = double.NaN;
    private string _deviceId;
    private string _d3dDeviceId;

    private readonly IntelGcl.ctl_device_adapter_handle_t _handle;

    private double _lastEnergyReading = double.NaN;

    private double _lastGlobalActivityCounter = double.NaN;
    private double _lastMediaActivityCounter = double.NaN;
    private double _lastRenderComputeActivityCounter = double.NaN;
    private double _lastTimestamp = double.NaN;
    private double _lastTotalCardEnergyReading = double.NaN;
    private double _lastVramReadBandwidthCounter = double.NaN;
    private double _lastVramWriteBandwidthCounter = double.NaN;
    private IntelGcl.ctl_device_adapter_properties_t _properties;

    private IntelGcl.ctl_power_telemetry_t _telemetry;

    public IntelDiscreteGpu(IntelGcl.ctl_device_adapter_handle_t handle, ISettings settings)
        : base(GetDeviceName(handle), new Identifier("gpu-intel", GetDeviceId(handle)), settings)
    {
        _handle = handle;
        IsValid = false;

        if (!InitializeDevice())
            return;

        _d3dDeviceId = GetD3DDeviceId();

        _temperatureGpuCore = new Sensor("GPU Core", 0, SensorType.Temperature, this, settings);
        _temperatureMemory = new Sensor("GPU Memory", 1, SensorType.Temperature, this, settings);

        _clockCore = new Sensor("GPU Core", 0, SensorType.Clock, this, settings);
        _clockMemory = new Sensor("GPU Memory", 1, SensorType.Clock, this, settings);

        _voltageCore = new Sensor("GPU Core", 0, SensorType.Voltage, this, settings);
        _voltageMemory = new Sensor("GPU Memory", 1, SensorType.Voltage, this, settings);

        _powerGpu = new Sensor("GPU Package", 0, SensorType.Power, this, settings);
        _powerTotal = new Sensor("GPU Total", 1, SensorType.Power, this, settings);

        _loadGlobalActivity = new Sensor("GPU Core", 0, SensorType.Load, this, settings);
        _loadRenderCompute = new Sensor("GPU Render/Compute", 1, SensorType.Load, this, settings);
        _loadMedia = new Sensor("GPU Media", 2, SensorType.Load, this, settings);

        _memoryFree = new Sensor("GPU Memory Free", 0, SensorType.SmallData, this, settings);
        _memoryUsed = new Sensor("GPU Memory Used", 1, SensorType.SmallData, this, settings);
        _memoryTotal = new Sensor("GPU Memory Total", 2, SensorType.SmallData, this, settings);
        _memoryLoad = new Sensor("GPU Memory", 3, SensorType.Load, this, settings);

        _memoryBandwidthRead = new Sensor("GPU Memory Read", 0, SensorType.Throughput, this, settings);
        _memoryBandwidthWrite = new Sensor("GPU Memory Write", 1, SensorType.Throughput, this, settings);

        int fanCount = (int)GetFanCount();
        _fans = new Sensor[fanCount];

        for (int i = 0; i < fanCount; i++)
        {
            string fanName = fanCount == 1 ? "GPU Fan" : $"GPU Fan {i + 1}";
            _fans[i] = new Sensor(fanName, i, SensorType.Fan, this, settings);
        }

        Update();
    }

    public override string DeviceId => _deviceId ?? GetDeviceId(_handle);

    public uint DriverVersion { get; private set; }

    public override HardwareType HardwareType => HardwareType.GpuIntel;

    public bool IsValid { get; private set; }

    public uint RevisionId { get; private set; }

    public uint VendorId { get; private set; }

    private static bool TryGetDeviceProperties(IntelGcl.ctl_device_adapter_handle_t handle, out IntelGcl.ctl_device_adapter_properties_t properties)
    {
        properties = new IntelGcl.ctl_device_adapter_properties_t
        {
            Size = (uint)Marshal.SizeOf(typeof(IntelGcl.ctl_device_adapter_properties_t)),
            Version = 2
        };

        int result = IntelGcl.ctlGetDeviceProperties(handle, ref properties);
        return result == (int)IntelGcl.ctl_result_t.CTL_RESULT_SUCCESS &&
               properties.device_type == IntelGcl.ctl_device_type_t.CTL_DEVICE_TYPE_GRAPHICS;
    }

    private static string GetDeviceName(IntelGcl.ctl_device_adapter_handle_t handle)
    {
        if (TryGetDeviceProperties(handle, out IntelGcl.ctl_device_adapter_properties_t properties))
        {
            return properties.name;
        }

        return "Intel GPU";
    }

    private static string GetDeviceId(IntelGcl.ctl_device_adapter_handle_t handle)
    {
        if (TryGetDeviceProperties(handle, out IntelGcl.ctl_device_adapter_properties_t properties))
        {
            return $"0x{properties.pci_device_id:X4}";
        }

        return "0x0000";
    }

    private bool InitializeDevice()
    {
        if (TryGetDeviceProperties(_handle, out _properties))
        {
            _deviceId = $"0x{_properties.pci_device_id:X4}";
            VendorId = _properties.pci_vendor_id;
            RevisionId = _properties.rev_id;
            DriverVersion = (uint)_properties.driver_version;
            IsValid = true;
            return true;
        }

        return false;
    }

    private string GetD3DDeviceId()
    {
        
        string[] deviceIdentifiers = D3DDisplayDevice.GetDeviceIdentifiers();
        if (deviceIdentifiers == null || deviceIdentifiers.Length == 0)
            return null;

        string vendorPattern = $"VEN_{_properties.pci_vendor_id:X}";
        string devicePattern = $"DEV_{_properties.pci_device_id:X}";

        foreach (string deviceIdentifier in deviceIdentifiers)
        {
            
            if (deviceIdentifier.IndexOf(vendorPattern, StringComparison.OrdinalIgnoreCase) != -1 &&
                deviceIdentifier.IndexOf(devicePattern, StringComparison.OrdinalIgnoreCase) != -1)
            {
                
                if (GetBdfFromDeviceIdentifier(deviceIdentifier, out int bus, out int device, out int function))
                {
                    if (bus == _properties.adapter_bdf.bus && device == _properties.adapter_bdf.device && function == _properties.adapter_bdf.function)
                    {
                        
                        if (D3DDisplayDevice.GetDeviceInfoByIdentifier(deviceIdentifier, out D3DDisplayDevice.D3DDeviceInfo deviceInfo))
                        {
                            return deviceIdentifier;
                        }
                    }
                }
                else
                {
                    
                    if (D3DDisplayDevice.GetDeviceInfoByIdentifier(deviceIdentifier, out D3DDisplayDevice.D3DDeviceInfo deviceInfo))
                    {
                        return deviceIdentifier;
                    }
                }
            }
        }

        return null;
    }

    public override void Update()
    {
        if (!IsValid)
            return;

        try
        {
            
            if (!UpdateTelemetry())
                return;

            UpdatePowerFromEnergyCounter(_telemetry.gpuEnergyCounter, ref _lastEnergyReading, _powerGpu);
            UpdatePowerFromEnergyCounter(_telemetry.totalCardEnergyCounter, ref _lastTotalCardEnergyReading, _powerTotal);

            UpdateSensorFromTelemetry(_telemetry.gpuCurrentTemperature, _temperatureGpuCore);
            UpdateSensorFromTelemetry(_telemetry.vramCurrentTemperature, _temperatureMemory);

            UpdateSensorFromTelemetry(_telemetry.gpuCurrentClockFrequency, _clockCore);
            UpdateMemoryFrequency(_clockMemory);

            UpdateSensorFromTelemetry(_telemetry.gpuVoltage, _voltageCore);
            UpdateSensorFromTelemetry(_telemetry.vramVoltage, _voltageMemory);

            UpdateUtilizationFromActivityCounter(_telemetry.globalActivityCounter, ref _lastGlobalActivityCounter, _loadGlobalActivity);
            UpdateUtilizationFromActivityCounter(_telemetry.renderComputeActivityCounter, ref _lastRenderComputeActivityCounter, _loadRenderCompute);
            UpdateUtilizationFromActivityCounter(_telemetry.mediaActivityCounter, ref _lastMediaActivityCounter, _loadMedia);

            if (string.IsNullOrEmpty(_d3dDeviceId))
            {
                _d3dDeviceId = GetD3DDeviceId();
            }

            UpdateMemorySensors();

            UpdateBandwidthFromCounter(_telemetry.vramReadBandwidth, ref _lastVramReadBandwidthCounter, _memoryBandwidthRead);
            UpdateBandwidthFromCounter(_telemetry.vramWriteBandwidth, ref _lastVramWriteBandwidthCounter, _memoryBandwidthWrite);

            UpdateFanSpeeds(_fans);
        }
        catch (Exception ex)
        {
            
            System.Diagnostics.Debug.WriteLine($"Error updating Intel GPU sensors: {ex.Message}");
        }
    }

    private bool UpdateTelemetry()
    {
        if (!IsValid)
            return false;

        var telemetry = new IntelGcl.ctl_power_telemetry_t
        {
            Size = (uint)Marshal.SizeOf(typeof(IntelGcl.ctl_power_telemetry_t)),
            Version = 1,
            psu = new IntelGcl.ctl_psu_info_t[IntelGcl.CTL_PSU_COUNT],
            fanSpeed = new IntelGcl.ctl_oc_telemetry_item_t[IntelGcl.CTL_FAN_COUNT]
        };

        if (IntelGcl.ctlPowerTelemetryGet(_handle, ref telemetry) == (int)IntelGcl.ctl_result_t.CTL_RESULT_SUCCESS)
        {
            _telemetry = telemetry;
            _lastTimestamp = _currentTimestamp;
            _currentTimestamp = _telemetry.timeStamp.bSupported ? GetTelemetryValue(_telemetry.timeStamp) : DateTimeOffset.UtcNow.Ticks;
            return true;
        }

        return false;
    }

    private void UpdateMemoryFrequency(Sensor sensor)
    {
        double frequency = double.NaN;

        uint freqCount = 0;
        int result = IntelGcl.ctlEnumFrequencyDomains(_handle, ref freqCount, null);

        if (result == (int)IntelGcl.ctl_result_t.CTL_RESULT_SUCCESS && freqCount > 0)
        {
            var freqHandles = new IntelGcl.ctl_freq_handle_t[freqCount];
            result = IntelGcl.ctlEnumFrequencyDomains(_handle, ref freqCount, freqHandles);

            if (result == (int)IntelGcl.ctl_result_t.CTL_RESULT_SUCCESS)
            {
                for (int i = 0; i < freqCount; i++)
                {
                    var properties = new IntelGcl.ctl_freq_properties_t
                    {
                        Size = (uint)Marshal.SizeOf(typeof(IntelGcl.ctl_freq_properties_t)),
                        Version = 0
                    };

                    result = IntelGcl.ctlFrequencyGetProperties(freqHandles[i], ref properties);

                    if (result == (int)IntelGcl.ctl_result_t.CTL_RESULT_SUCCESS &&
                        properties.type == IntelGcl.ctl_freq_domain_t.CTL_FREQ_DOMAIN_MEMORY)
                    {
                        var state = new IntelGcl.ctl_freq_state_t
                        {
                            Size = (uint)Marshal.SizeOf(typeof(IntelGcl.ctl_freq_state_t)),
                            Version = 0
                        };

                        result = IntelGcl.ctlFrequencyGetState(freqHandles[i], ref state);

                        if (result == (int)IntelGcl.ctl_result_t.CTL_RESULT_SUCCESS && state.actual >= 0)
                        {
                            frequency = state.actual / MemoryFrequencyDivisor;
                            break;
                        }
                    }
                }
            }
        }

        if (double.IsNaN(frequency) && _telemetry.vramCurrentClockFrequency.bSupported)
        {
            frequency = GetTelemetryValue(_telemetry.vramCurrentClockFrequency);
        }

        if (!double.IsNaN(frequency))
        {
            sensor.Value = (float)frequency;
            ActivateSensor(sensor);
        }
        else
        {
            sensor.Value = null;
        }
    }

    private uint GetFanCount()
    {
        uint fanCount = 0;
        int result = IntelGcl.ctlEnumFans(_handle, ref fanCount, null);

        if (result == (int)IntelGcl.ctl_result_t.CTL_RESULT_SUCCESS)
        {
            return fanCount;
        }

        return 0;
    }

    private void UpdateFanSpeeds(Sensor[] fanSensors)
    {
        uint fanCount = (uint)Math.Min(Math.Max(0, GetFanCount()), fanSensors.Length);
        if (fanCount == 0)
            return;

        var fanHandles = new IntelGcl.ctl_fan_handle_t[fanCount];
        int result = IntelGcl.ctlEnumFans(_handle, ref fanCount, fanHandles);

        if (result == (int)IntelGcl.ctl_result_t.CTL_RESULT_SUCCESS)
        {
            for (int i = 0; i < fanCount; i++)
            {
                int fanSpeed = -1;
                result = IntelGcl.ctlFanGetState(fanHandles[i], IntelGcl.ctl_fan_speed_units_t.CTL_FAN_SPEED_UNITS_RPM, ref fanSpeed);

                if (result == (int)IntelGcl.ctl_result_t.CTL_RESULT_SUCCESS && fanSpeed >= 0)
                {
                    fanSensors[i].Value = fanSpeed;
                    ActivateSensor(fanSensors[i]);
                }
                else
                {
                    fanSensors[i].Value = null;
                }
            }

            for (int i = (int)fanCount; i < fanSensors.Length; i++)
            {
                fanSensors[i].Value = null;
            }
        }
    }

    private void UpdateSensorFromTelemetry(IntelGcl.ctl_oc_telemetry_item_t telemetryItem, Sensor sensor)
    {
        if (telemetryItem.bSupported)
        {
            sensor.Value = (float)GetTelemetryValue(telemetryItem);
            ActivateSensor(sensor);
        }
        else
        {
            sensor.Value = null;
        }
    }

    private double GetTelemetryValue(IntelGcl.ctl_oc_telemetry_item_t item)
    {
        return item.type switch
        {
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_FLOAT => item.value.datafloat,
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_DOUBLE => item.value.datadouble,
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_UINT32 => item.value.datau32,
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_INT32 => item.value.data32,
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_UINT64 => item.value.datau64,
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_INT64 => item.value.data64,
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_UINT16 => item.value.datau16,
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_INT16 => item.value.data16,
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_UINT8 => item.value.datau8,
            IntelGcl.ctl_data_type_t.CTL_DATA_TYPE_INT8 => item.value.data8,
            _ => double.NaN
        };
    }

    private void UpdatePowerFromEnergyCounter(IntelGcl.ctl_oc_telemetry_item_t energyCounter, ref double lastEnergyReading, Sensor powerSensor)
    {
        if (!IsValid || powerSensor == null)
            return;

        double currentEnergy = energyCounter.bSupported ? GetTelemetryValue(energyCounter) : double.NaN;
        double deltaTime = _currentTimestamp - _lastTimestamp;

        if (deltaTime > 0.0 && !double.IsNaN(currentEnergy) && !double.IsNaN(lastEnergyReading))
        {
            double deltaEnergy = currentEnergy - lastEnergyReading;
            double power = deltaEnergy / deltaTime;
            power = power < 0 ? 0 : power;

            powerSensor.Value = (float)power;
            ActivateSensor(powerSensor);
        }
        else
        {
            powerSensor.Value = null;
        }

        lastEnergyReading = currentEnergy;
    }

    private void UpdateUtilizationFromActivityCounter(IntelGcl.ctl_oc_telemetry_item_t activityCounter, ref double lastActivityReading, Sensor activitySensor)
    {
        if (!IsValid || activitySensor == null)
            return;

        double currentActivity = activityCounter.bSupported ? GetTelemetryValue(activityCounter) : double.NaN;
        double deltaTime = _currentTimestamp - _lastTimestamp;

        if (deltaTime > 0 && !double.IsNaN(currentActivity) && !double.IsNaN(lastActivityReading))
        {
            double activeDiff = currentActivity - lastActivityReading;
            if (activeDiff >= 0)
            {
                double activity = (activeDiff / deltaTime) * 100.0;
                activity = Math.Min(Math.Max(activity, 0.0), 100.0);

                activitySensor.Value = (float)activity;
                ActivateSensor(activitySensor);
            }
            else
            {
                activitySensor.Value = null;
            }
        }
        else
        {
            activitySensor.Value = null;
        }

        lastActivityReading = currentActivity;
    }

    private void UpdateMemorySensors()
    {
        if (string.IsNullOrEmpty(_d3dDeviceId))
        {
            
            string[] deviceIdentifiers = D3DDisplayDevice.GetDeviceIdentifiers();
            if (deviceIdentifiers != null)
            {
                foreach (string deviceId in deviceIdentifiers)
                {
                    if (deviceId.IndexOf("VEN_8086", StringComparison.OrdinalIgnoreCase) != -1)
                    {
                        if (D3DDisplayDevice.GetDeviceInfoByIdentifier(deviceId, out D3DDisplayDevice.D3DDeviceInfo testInfo))
                        {
                            if (testInfo.GpuDedicatedLimit > 0 && !testInfo.Integrated)
                            {
                                _d3dDeviceId = deviceId;
                                break;
                            }
                        }
                    }
                }
            }

            if (string.IsNullOrEmpty(_d3dDeviceId))
                return;
        }

        if (D3DDisplayDevice.GetDeviceInfoByIdentifier(_d3dDeviceId, out D3DDisplayDevice.D3DDeviceInfo deviceInfo))
        {
            
            ulong totalBytes = deviceInfo.GpuVideoMemoryLimit;
            ulong usedBytes = deviceInfo.GpuDedicatedUsed;
            ulong freeBytes = totalBytes > usedBytes ? totalBytes - usedBytes : 0;

            if (totalBytes > 0)
            {
                
                _memoryTotal.Value = totalBytes / (1024.0f * 1024.0f);
                ActivateSensor(_memoryTotal);

                _memoryUsed.Value = usedBytes / (1024.0f * 1024.0f);
                ActivateSensor(_memoryUsed);

                _memoryFree.Value = freeBytes / (1024.0f * 1024.0f);
                ActivateSensor(_memoryFree);

                _memoryLoad.Value = (float)((double)usedBytes / totalBytes * 100.0);
                ActivateSensor(_memoryLoad);
            }
        }
    }

    private void UpdateBandwidthFromCounter(IntelGcl.ctl_oc_telemetry_item_t bandwidthItem, ref double lastBandwidthReading, Sensor bandwidthSensor)
    {
        if (!IsValid || bandwidthSensor == null)
            return;

        if (bandwidthItem.bSupported)
        {
            double bandwidthValue = GetTelemetryValue(bandwidthItem);

            if (!double.IsNaN(bandwidthValue) && bandwidthValue >= 0)
            {
                
                if (bandwidthItem.units == IntelGcl.ctl_units_t.CTL_UNITS_BANDWIDTH_MBPS)
                {
                    
                    bandwidthValue = bandwidthValue * 1024.0 * 1024.0;
                }
                else if (bandwidthItem.units == IntelGcl.ctl_units_t.CTL_UNITS_MEM_SPEED_GBPS)
                {
                    
                    bandwidthValue = bandwidthValue * 1024.0 * 1024.0 * 1024.0;
                }

                bandwidthSensor.Value = (float)bandwidthValue;
                ActivateSensor(bandwidthSensor);
            }
            else
            {
                bandwidthSensor.Value = null;
            }
        }
        else
        {
            bandwidthSensor.Value = null;
        }
    }

    private unsafe bool GetBdfFromDeviceIdentifier(string deviceIdentifier, out int bus, out int device, out int function)
    {
        bus = -1;
        device = -1;
        function = -1;

        if (string.IsNullOrEmpty(deviceIdentifier))
            return false;

        string instanceId = deviceIdentifier;
        if (instanceId.StartsWith(@\\?\"))
            instanceId = instanceId.Substring(4);

        int lastHash = instanceId.LastIndexOf('#');
        if (lastHash == -1)
            return false;

        instanceId = instanceId.Substring(0, lastHash);
        
        instanceId = instanceId.Replace('#', '\\');

        uint devInst;
        fixed (char* pInstanceId = instanceId)
        {
            if (PInvoke.CM_Locate_DevNode(out devInst, (PWSTR)pInstanceId, CM_LOCATE_DEVNODE_FLAGS.CM_LOCATE_DEVNODE_NORMAL) != CONFIGRET.CR_SUCCESS)
            {
                return false;
            }
        }

        DEVPROPTYPE propertyType;
        uint bufferSize = sizeof(uint);
        uint busNum = 0;
        uint address = 0;

        fixed (DEVPROPKEY* pBusKey = &PInvoke.DEVPKEY_Device_BusNumber)
        {
            if (PInvoke.CM_Get_DevNode_Property(devInst, pBusKey, &propertyType, (byte*)&busNum, &bufferSize, 0) == CONFIGRET.CR_SUCCESS && propertyType == DEVPROPTYPE.DEVPROP_TYPE_UINT32)
            {
                bus = (int)busNum;
            }
            else
            {
                return false;
            }
        }

        bufferSize = sizeof(uint);
        propertyType = default;
        fixed (DEVPROPKEY* pAddrKey = &PInvoke.DEVPKEY_Device_Address)
        {
            if (PInvoke.CM_Get_DevNode_Property(devInst, pAddrKey, &propertyType, (byte*)&address, &bufferSize, 0) == CONFIGRET.CR_SUCCESS &&
                propertyType == DEVPROPTYPE.DEVPROP_TYPE_UINT32)
            {
                
                device = (int)(address >> 16) & 0xFFFF;
                function = (int)address & 0xFFFF;
                return true;
            }
        }

        return false;
    }
}
