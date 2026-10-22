// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.NetworkInformation;
using System.Text;

namespace XiaobaiHardwareMonitor.Hardware.Network;

internal class NetworkGroup : IGroup, IHardwareChanged
{
    public event HardwareEventHandler HardwareAdded;
    public event HardwareEventHandler HardwareRemoved;

    private readonly object _updateLock = new();
    private readonly ISettings _settings;
    private List<Network> _hardware = [];

    public NetworkGroup(ISettings settings)
    {
        _settings = settings;
        UpdateNetworkInterfaces(settings);

        NetworkChange.NetworkAddressChanged += NetworkChange_NetworkAddressChanged;
        NetworkChange.NetworkAvailabilityChanged += NetworkChange_NetworkAddressChanged;
    }

    public IReadOnlyList<IHardware> Hardware => _hardware;

    public string GetReport()
    {
        var report = new StringBuilder();

        foreach (Network network in _hardware)
        {
            report.AppendLine(network.NetworkInterface.Description);
            report.AppendLine(network.NetworkInterface.OperationalStatus.ToString());
            report.AppendLine();

            foreach (ISensor sensor in network.Sensors)
            {
                report.AppendLine(sensor.Name);
                report.AppendLine(sensor.Value.ToString());
                report.AppendLine();
            }
        }

        return report.ToString();
    }

    public void Close()
    {
        NetworkChange.NetworkAddressChanged -= NetworkChange_NetworkAddressChanged;
        NetworkChange.NetworkAvailabilityChanged -= NetworkChange_NetworkAddressChanged;

        foreach (Network network in _hardware)
            network.Close();
    }

    private void UpdateNetworkInterfaces(ISettings settings)
    {
        
        lock (_updateLock)
        {
            List<NetworkInterface> networkInterfaces = GetNetworkInterfaces();
            if (networkInterfaces == null)
                return;

            List<Network> removables = [];
            List<Network> additions = [];

            List<Network> hardware = [.. _hardware];

            for (int i = 0; i < hardware.Count; i++)
            {
                Network network = hardware[i];
                if (networkInterfaces.Any(x => x.Id == network.NetworkInterface.Id))
                    continue;

                hardware.RemoveAt(i--);
                removables.Add(network);
            }

            foreach (NetworkInterface networkInterface in networkInterfaces)
            {
                if (hardware.All(x => x.NetworkInterface.Id != networkInterface.Id))
                {
                    try
                    {
                        Network network = new(networkInterface, settings);
                        hardware.Add(network);
                        additions.Add(network);
                    }
                    catch (NetworkInformationException)
                    {
                        
                    }
                }
            }

            _hardware = hardware;

            foreach (Network removable in removables)
                HardwareRemoved?.Invoke(removable);

            foreach (Network addition in additions)
                HardwareAdded?.Invoke(addition);
        }
    }

    private static List<NetworkInterface> GetNetworkInterfaces()
    {
        int retry = 0;

        while (retry++ < 5)
        {
            try
            {
                return NetworkInterface.GetAllNetworkInterfaces()
                                       .Where(IsDesiredNetworkType)
                                       .OrderBy(static x => x.Name)
                                       .ToList();
            }
            catch (NetworkInformationException)
            {
                
            }
        }

        return null;
    }

    private void NetworkChange_NetworkAddressChanged(object sender, EventArgs e)
    {
        UpdateNetworkInterfaces(_settings);
    }

    private static bool IsDesiredNetworkType(NetworkInterface nic)
    {
        switch (nic.NetworkInterfaceType)
        {
            case NetworkInterfaceType.Loopback:
            case NetworkInterfaceType.Tunnel:
            case NetworkInterfaceType.Unknown:
                return false;
            default:
                return true;
        }
    }
}
