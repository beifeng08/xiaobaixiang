// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Diagnostics;
using System.Net.NetworkInformation;

namespace XiaobaiHardwareMonitor.Hardware.Network;

internal sealed class Network : Hardware
{
    private readonly Sensor _dataDownloaded;
    private readonly Sensor _dataUploaded;
    private readonly Sensor _downloadSpeed;
    private readonly Sensor _networkUtilization;
    private readonly Sensor _uploadSpeed;
    private long _bytesDownloaded;
    private long _bytesUploaded;
    private long _lastTick;

    public Network(NetworkInterface networkInterface, ISettings settings)
        : base(networkInterface.Name, new Identifier("nic", networkInterface.Id), settings)
    {
        NetworkInterface = networkInterface;
        _dataUploaded = new Sensor("Data Uploaded", 2, SensorType.Data, this, settings);
        ActivateSensor(_dataUploaded);
        _dataDownloaded = new Sensor("Data Downloaded", 3, SensorType.Data, this, settings);
        ActivateSensor(_dataDownloaded);
        _uploadSpeed = new Sensor("Upload Speed", 7, SensorType.Throughput, this, settings);
        ActivateSensor(_uploadSpeed);
        _downloadSpeed = new Sensor("Download Speed", 8, SensorType.Throughput, this, settings);
        ActivateSensor(_downloadSpeed);
        _networkUtilization = new Sensor("Network Utilization", 1, SensorType.Load, this, settings);
        ActivateSensor(_networkUtilization);
        _bytesUploaded = NetworkInterface.GetIPStatistics().BytesSent;
        _bytesDownloaded = NetworkInterface.GetIPStatistics().BytesReceived;
        _lastTick = Stopwatch.GetTimestamp();
    }

    public override HardwareType HardwareType
    {
        get { return HardwareType.Network; }
    }

    internal NetworkInterface NetworkInterface { get; private set; }

    public override void Update()
    {
        try
        {
            if (NetworkInterface == null)
                return;

            long newTick = Stopwatch.GetTimestamp();
            double dt = new TimeSpan(newTick - _lastTick).TotalSeconds;

            IPv4InterfaceStatistics interfaceStats = NetworkInterface.GetIPv4Statistics();

            _dataUploaded.Value = (float)(interfaceStats.BytesSent / (double)0x40000000);
            _dataDownloaded.Value = (float)(interfaceStats.BytesReceived / (double)0x40000000);

            if (interfaceStats.BytesSent < _bytesUploaded || interfaceStats.BytesReceived < _bytesDownloaded)
            {
                _bytesUploaded = 0;
                _bytesDownloaded = 0;
            }

            long dBytesUploaded = interfaceStats.BytesSent - _bytesUploaded;
            long dBytesDownloaded = interfaceStats.BytesReceived - _bytesDownloaded;

            _uploadSpeed.Value = (float)(dBytesUploaded / dt);
            _downloadSpeed.Value = (float)(dBytesDownloaded / dt);

            long dbits = (dBytesUploaded + dBytesDownloaded) * 8;

            double load = (dbits / dt / NetworkInterface.Speed) * 100;

            _networkUtilization.Value = (float)Math.Min(Math.Max(load, 0), 100);

            _bytesUploaded = interfaceStats.BytesSent;
            _bytesDownloaded = interfaceStats.BytesReceived;
            _lastTick = newTick;
        }
        catch (NetworkInformationException networkInformationException) when (unchecked(networkInformationException.HResult == (int)0x80004005))
        {
            foreach (NetworkInterface networkInterface in NetworkInterface.GetAllNetworkInterfaces())
            {
                if (networkInterface.Id.Equals(NetworkInterface?.Id))
                {
                    NetworkInterface = networkInterface;
                    break;
                }
            }
        }
    }
}